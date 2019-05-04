#include "nSuite.h"
#include "Resource.h"
#include "Log.h"
#include "Threader.h"
#include "Progress.h"
#include <atomic>
#include <direct.h>
#include <filesystem>
#include <fstream>
#include <shlobj.h>
#include <vector>

using namespace NST;


std::optional<Buffer> NST::DiffDirectories(const std::string & oldDirectory, const std::string & newDirectory)
{
	// Declarations that will only be used here	
	struct File {
		std::string path = "", relative = "";
		size_t size = 0ull;
		inline File(const std::string & p, const std::string & r, const size_t & s) : path(p), relative(r), size(s) {}
		inline virtual bool open(Buffer & buffer, size_t & hash) const {
			std::ifstream f(path, std::ios::binary | std::ios::beg);
			if (f.is_open()) {
				buffer = Buffer(size);
				f.read(buffer.cArray(), std::streamsize(size));
				f.close();
				hash = buffer.hash();
				return true;
			}
			return false;
		}
	};
	struct FileMem : File {
		void * ptr = nullptr;
		inline FileMem(const std::string & p, const std::string & r, const size_t & s, void * b) : File(p, r, s), ptr(b) {}
		inline virtual bool open(Buffer & buffer, size_t & hash) const override {
			buffer = Buffer(size);
			auto * ptrCast = reinterpret_cast<std::byte*>(ptr);
			std::copy(ptrCast, ptrCast + size, buffer.data());
			hash = buffer.hash();
			return true;
		}
	};
	typedef std::vector<File*> PathList;
	typedef std::vector<std::pair<File*, File*>> PathPairList;
	static constexpr auto getRelativePath = [](const std::string & filePath, const std::string & directory) -> std::string {
		return filePath.substr(directory.length(), filePath.length() - directory.length());
	};
	static constexpr auto getFiles = [](const std::string & directory, Buffer & snapshot, std::vector<File*> & files) -> bool {		
		// See if the input path is a directory
		if (std::filesystem::is_directory(directory)) {
			for each (const auto & srcFile in GetFilePaths(directory)) {
				const std::string path = srcFile.path().string();
				files.push_back(new File(path, getRelativePath(path, directory), srcFile.file_size()));
			}
			return true;
		}
		else {
			// See if the file is actually a program with an embedded archive
			NST::Buffer packBuffer;
			bool canLoad = std::filesystem::path(directory).extension() == ".exe" ? LoadLibraryA(directory.c_str()) : false;
			auto handle = GetModuleHandle(directory.c_str());
			Resource fileResource(IDR_ARCHIVE, "ARCHIVE", handle);
			if (canLoad && handle != NULL && fileResource.exists()) {
				packBuffer = NST::Buffer(reinterpret_cast<std::byte*>(fileResource.getPtr()), fileResource.getSize());
				FreeLibrary(handle);
			}
			else {
				// Last resort: try to treat the file as an nSuite package
				std::ifstream packFile(directory, std::ios::binary | std::ios::beg);
				if (!packFile.is_open()) {
					Log::PushText("Error: cannot open the package file specified!\r\n");
					return false;
				}
				packBuffer = NST::Buffer(std::filesystem::file_size(directory));
				packFile.read(packBuffer.cArray(), std::streamsize(packBuffer.size()));
				packFile.close();
			}

			// Read in header		
			Directory::PackageHeader header;
			std::byte * dataPtr(nullptr);
			size_t dataSize(0ull);
			packBuffer.readHeader(&header, &dataPtr, dataSize);

			// Ensure header title matches
			if (std::strcmp(header.m_title, Directory::PackageHeader::TITLE) != 0)
				NST::Log::PushText("Critical failure: cannot parse packaged content's header!\r\n");
			else {
				// Try to decompress the buffer
				auto decompressedBuffer = Buffer(dataPtr, dataSize).decompress();
				packBuffer.release();

				// Handle decompression failure now
				if (!decompressedBuffer)
					Log::PushText("Critical failure: cannot decompress package file!\r\n");
				else {
					// Get lists of all files involved
					snapshot = *decompressedBuffer;
					size_t byteIndex(0ull);
					while (byteIndex < snapshot.size()) {
						// Read path char count, path string, and file size
						size_t pathSize(0ull);
						byteIndex = decompressedBuffer->readData(&pathSize, size_t(sizeof(size_t)), byteIndex);
						char * pathArray = new char[pathSize];
						byteIndex = decompressedBuffer->readData(pathArray, pathSize, byteIndex);
						const std::string path(pathArray, pathSize);
						delete[] pathArray;
						size_t fileSize(0ull);
						byteIndex = decompressedBuffer->readData(&fileSize, size_t(sizeof(size_t)), byteIndex);

						// Save file path + size + pointer info
						files.push_back(new FileMem(path, path, fileSize, &((*decompressedBuffer)[byteIndex])));
						byteIndex += fileSize;
					}
					return true;
				}
			}
		}

		// Failure 
		return false;
	};
	static constexpr auto getFileLists = [](const std::string & oldDirectory, Buffer & oldSnapshot, const std::string & newDirectory, Buffer & newSnapshot, PathPairList & commonFiles, PathList & addFiles, PathList & delFiles) {
		// Get files
		std::vector<File*> srcOld_Files, srcNew_Files;
		if (!getFiles(oldDirectory, oldSnapshot, srcOld_Files) || !getFiles(newDirectory, newSnapshot, srcNew_Files))
			Log::PushText("Error: cannot retrieve both lists of files!\r\n");
		else {
			// Find all common and new files first
			for each (const auto & nFile in srcNew_Files) {
				bool found = false;
				size_t oIndex(0ull);
				for each (const auto & oFile in srcOld_Files) {
					if (nFile->relative == oFile->relative) {
						// Common file found		
						commonFiles.push_back(std::make_pair(oFile, nFile));

						// Remove old file from list (so we can use all that remain)
						srcOld_Files.erase(srcOld_Files.begin() + oIndex);
						found = true;
						break;
					}
					oIndex++;
				}
				// New file found, add it
				if (!found)
					addFiles.push_back(nFile);
			}

			// All 'old files' that remain didn't exist in the 'new file' set
			delFiles = srcOld_Files;

			// Success
			return true;
		}

		// Failure
		return false;
	};
	static constexpr auto writeInstructions = [](const std::string & path, const size_t & oldHash, const size_t & newHash, const Buffer & buffer, const char & flag, Buffer & instructionBuffer) {
		const auto bufferSize = buffer.size();
		auto pathLength = path.length();
		const size_t instructionSize = (sizeof(size_t) * 4) + (size_t(sizeof(char)) * pathLength) + size_t(sizeof(char)) + bufferSize;
		const size_t bufferOldSize = instructionBuffer.size();
		instructionBuffer.resize(bufferOldSize + instructionSize);

		// Write file path length
		size_t byteIndex = instructionBuffer.writeData(&pathLength, size_t(sizeof(size_t)), bufferOldSize);

		// Write file path
		byteIndex = instructionBuffer.writeData(path.data(), size_t(sizeof(char)) * pathLength, byteIndex);

		// Write operation flag
		byteIndex = instructionBuffer.writeData(&flag, size_t(sizeof(char)), byteIndex);

		// Write old hash
		byteIndex = instructionBuffer.writeData(&oldHash, size_t(sizeof(size_t)), byteIndex);

		// Write new hash
		byteIndex = instructionBuffer.writeData(&newHash, size_t(sizeof(size_t)), byteIndex);

		// Write buffer size
		byteIndex = instructionBuffer.writeData(&bufferSize, size_t(sizeof(size_t)), byteIndex);

		// Write buffer
		byteIndex = instructionBuffer.writeData(buffer.data(), (size_t(sizeof(std::byte)) * bufferSize), byteIndex);
	};

	// Retrieve all common, added, and removed files
	PathPairList commonFiles;
	PathList addedFiles, removedFiles;
	Buffer oldSnap, newSnap;
	if (!getFileLists(oldDirectory, oldSnap, newDirectory, newSnap, commonFiles, addedFiles, removedFiles))
		Log::PushText("Error: could not retrieve all requisite file lists for diffing!\r\n");
	else {
		// Generate Instructions from file lists, store them in this expanding buffer
		Buffer instructionBuffer;

		// These files are common, maybe some have changed
		size_t fileCount(0ull);
		for each (const auto & cFiles in commonFiles) {
			Buffer oldBuffer, newBuffer;
			size_t oldHash(0ull), newHash(0ull);
			if (cFiles.first->open(oldBuffer, oldHash) && cFiles.second->open(newBuffer, newHash)) {
				if (oldHash != newHash) {
					// Files are different versions
					auto buffer = oldBuffer.diff(newBuffer);
					if (buffer) {
						Log::PushText("Diffing file: \"" + cFiles.first->relative + "\"\r\n");
						writeInstructions(cFiles.first->relative, oldHash, newHash, *buffer, 'U', instructionBuffer);
						fileCount++;
					}
				}
			}
			delete cFiles.first;
			delete cFiles.second;
		}
		commonFiles.clear();
		commonFiles.shrink_to_fit();

		// These files are brand new
		for each (const auto & nFile in addedFiles) {
			Buffer newBuffer;
			size_t newHash(0ull);
			if (nFile->open(newBuffer, newHash)) {
				auto buffer = Buffer().diff(newBuffer);
				if (buffer) {
					Log::PushText("Adding file: \"" + nFile->relative + "\"\r\n");
					writeInstructions(nFile->relative, 0ull, newHash, *buffer, 'N', instructionBuffer);
					fileCount++;
				}
			}
			delete nFile;
		}
		addedFiles.clear();
		addedFiles.shrink_to_fit();
		newSnap.release();

		// These files are deprecated
		for each (const auto & oFile in removedFiles) {
			Buffer oldBuffer;
			size_t oldHash(0ull);
			if (oFile->open(oldBuffer, oldHash)) {
				Log::PushText("Removing file: \"" + oFile->relative + "\"\r\n");
				writeInstructions(oFile->relative, oldHash, 0ull, Buffer(), 'D', instructionBuffer);
				fileCount++;
			}
			delete oFile;
		}
		removedFiles.clear();
		removedFiles.shrink_to_fit();
		oldSnap.release();

		// Try to compress the instruction buffer
		auto compressedBuffer = instructionBuffer.compress();
		instructionBuffer.release();
		if (!compressedBuffer)
			Log::PushText("Critical failure: cannot compress diff instructions!\r\n");
		else {
			// Prepend header information
			Directory::PatchHeader header(fileCount);
			compressedBuffer->writeHeader(&header);

			// Success
			return compressedBuffer;
		}
	}

	// Failure
	return {};
}

bool NST::PatchDirectory(const std::string & dstDirectory, const Buffer & diffBuffer, size_t * bytesWritten)
{
	// Ensure buffer at least *exists*
	if (!diffBuffer.hasData())
		Log::PushText("Error: patch buffer doesn't exist or has no content!\r\n");
	else {
		// Read in header
		Directory::PatchHeader header;
		std::byte * dataPtr(nullptr);
		size_t dataSize(0ull);
		diffBuffer.readHeader(&header, &dataPtr, dataSize);

		// Ensure header title matches
		if (std::strcmp(header.m_title, Directory::PatchHeader::TITLE) != 0)
			Log::PushText("Critical failure: supplied an invalid nSuite patch!\r\n");
		else {
			// Try to decompress the instruction buffer
			auto instructionBuffer = Buffer(dataPtr, dataSize).decompress();
			if (!instructionBuffer)
				Log::PushText("Error: cannot complete directory patching, as the instruction buffer cannot be decompressed!\r\n");
			else {
				static constexpr auto readFile = [](const std::string & path, Buffer & buffer, size_t & hash) -> bool {
					std::ifstream f(path, std::ios::binary | std::ios::beg);
					if (f.is_open()) {
						buffer = Buffer(std::filesystem::file_size(path));
						f.read(buffer.cArray(), std::streamsize(buffer.size()));
						f.close();
						hash = buffer.hash();
						return true;
					}
					return false;
				};

				// Start reading diff file
				struct FileInstruction {
					std::string path = "", fullPath = "";
					Buffer instructionBuffer;
					size_t diff_oldHash = 0ull, diff_newHash = 0ull;
				};
				std::vector<FileInstruction> diffFiles, addedFiles, removedFiles;
				size_t files(0ull), byteIndex(0ull);
				while (files < header.m_fileCount) {
					FileInstruction FI;

					// Read file path length
					size_t pathLength(0ull);
					byteIndex = instructionBuffer->readData(&pathLength, size_t(sizeof(size_t)), byteIndex);
					FI.path.resize(pathLength);

					// Read file path
					byteIndex = instructionBuffer->readData(FI.path.data(), size_t(sizeof(char)) * pathLength, byteIndex);
					FI.fullPath = dstDirectory + FI.path;

					// Read operation flag
					char flag(0);
					byteIndex = instructionBuffer->readData(&flag, size_t(sizeof(char)), byteIndex);

					// Read old hash
					byteIndex = instructionBuffer->readData(&FI.diff_oldHash, size_t(sizeof(size_t)), byteIndex);

					// Read new hash
					byteIndex = instructionBuffer->readData(&FI.diff_newHash, size_t(sizeof(size_t)), byteIndex);

					// Read buffer size
					size_t instructionSize(0ull);
					byteIndex = instructionBuffer->readData(&instructionSize, size_t(sizeof(size_t)), byteIndex);

					// Copy buffer
					if (instructionSize > 0)
						FI.instructionBuffer = Buffer(&(*instructionBuffer)[byteIndex], instructionSize, true);
					byteIndex += instructionSize;

					// Sort instructions
					if (flag == 'U')
						diffFiles.push_back(std::move(FI));
					else if (flag == 'N')
						addedFiles.push_back(std::move(FI));
					else if (flag == 'D')
						removedFiles.push_back(std::move(FI));
					files++;
				}
				instructionBuffer->release();

				// Patch all files first
				size_t byteNum(0ull);
				bool success = false;
				for (FileInstruction & file : diffFiles) {
					// Try to read the target file
					Buffer oldBuffer;
					size_t oldHash(0ull);

					// Try to read source file
					if (!readFile(file.fullPath, oldBuffer, oldHash))
						Log::PushText("Critical failure: Cannot read source file from disk!\r\n");
					else {
						// Patch if this source file hasn't been patched yet
						if (oldHash == file.diff_newHash) {
							Log::PushText("The file \"" + file.path + "\" is already up to date, skipping...\r\n");
							file.instructionBuffer.release();
							success = true;
							continue;
						}
						else if (oldHash != file.diff_oldHash) 
							Log::PushText("Critical failure: the file \"" + file.path + "\" is of an unexpected version!\r\n");
						else {
							// Patch buffer
							Log::PushText("patching file \"" + file.path + "\"\r\n");
							auto newBuffer = oldBuffer.patch(file.instructionBuffer);
							if (!newBuffer)
								Log::PushText("Critical failure: patching failed!\r\n");
							else {
								// Confirm new hashes match
								const size_t newHash = newBuffer->hash();
								if (newHash != file.diff_newHash) 
									Log::PushText("Critical failure: patched file is corrupted (hash mismatch)!\r\n");
								else {
									// Write patched buffer to disk
									std::ofstream newFile(file.fullPath, std::ios::binary | std::ios::out);
									if (!newFile.is_open()) 
										Log::PushText("Critical failure: cannot write patched file to disk!\r\n");
									else {
										newFile.write(newBuffer->cArray(), std::streamsize(newBuffer->size()));
										newFile.close();
										byteNum += newBuffer->size();
										file.instructionBuffer.release();
										success = true;
										continue;
									}
								}
							}
						}
					}
					success = false;
					break;
				}
				diffFiles.clear();
				diffFiles.shrink_to_fit();

				if (success) {
					// By this point all files matched, safe to add new ones
					success = false;
					for (FileInstruction & file : addedFiles) {
						std::filesystem::create_directories(std::filesystem::path(file.fullPath).parent_path());
						Log::PushText("Writing file: \"" + file.path + "\"\r\n");

						// Write the 'insert' instructions
						// Remember that we use the diff/patch function to add new files too
						auto newBuffer = Buffer().patch(file.instructionBuffer);
						if (!newBuffer)
							Log::PushText("Critical failure: cannot derive new file from patch instructions!\r\n");
						else {
							// Confirm new hashes match
							const size_t newHash = newBuffer->hash();
							if (newHash != file.diff_newHash)
								Log::PushText("Critical failure: new file is corrupted (hash mismatch)!\r\n");
							else {
								// Write new file to disk
								std::ofstream newFile(file.fullPath, std::ios::binary | std::ios::out);
								if (!newFile.is_open())
									Log::PushText("Critical failure: cannot write new file to disk!\r\n");
								else {
									newFile.write(newBuffer->cArray(), std::streamsize(newBuffer->size()));
									newFile.close();
									byteNum += newBuffer->size();
									file.instructionBuffer.release();
									success = true;
									continue;
								}
							}
						}
						success = false;
						break;
					}
					addedFiles.clear();
					addedFiles.shrink_to_fit();

					if (success) {
						// If we made it this far, it should be safe to delete all files
						for (FileInstruction & file : removedFiles) {
							// Try to read the target file (may not exist)
							Buffer oldBuffer;
							size_t oldHash(0ull);

							// Try to read source file
							if (!readFile(file.fullPath, oldBuffer, oldHash))
								Log::PushText("The file \"" + file.path + "\" has already been removed, skipping...\r\n");
							else {
								// Only remove source files if they match entirely
								if (oldHash == file.diff_oldHash) {
									if (!std::filesystem::remove(file.fullPath))
										Log::PushText("Error: cannot delete file \"" + file.path + "\" from disk, delete this file manually if you can!\r\n");
									else
										Log::PushText("Removing file: \"" + file.path + "\"\r\n");
								}
							}
						}
						removedFiles.clear();
						removedFiles.shrink_to_fit();

						// Update optional parameters
						if (bytesWritten != nullptr)
							*bytesWritten = byteNum;

						// Success
						return true;
					}
				}
			}
		}
	}

	return false;
}

std::vector<std::filesystem::directory_entry> NST::GetFilePaths(const std::string & directory)
{
	std::vector<std::filesystem::directory_entry> paths;
	if (std::filesystem::is_directory(directory))
		for (const auto & entry : std::filesystem::recursive_directory_iterator(directory))
			if (entry.is_regular_file())
				paths.emplace_back(entry);
	return paths;
}

std::string NST::GetStartMenuPath()
{
	char cPath[FILENAME_MAX];
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, cPath)))
		return std::string(cPath);
	return std::string();
}

std::string NST::GetDesktopPath()
{
	char cPath[FILENAME_MAX];
	if (SHGetSpecialFolderPathA(HWND_DESKTOP, cPath, CSIDL_DESKTOP, FALSE))
		return std::string(cPath);
	return std::string();
}

std::string NST::GetRunningDirectory()
{
	char cCurrentPath[FILENAME_MAX];
	if (_getcwd(cCurrentPath, sizeof(cCurrentPath)))
		cCurrentPath[sizeof(cCurrentPath) - 1ull] = char('\0');
	return std::string(cCurrentPath);
}

std::string NST::SanitizePath(const std::string & path)
{
	std::string cpy(path);
	while (cpy.front() == '"' || cpy.front() == '\'' || cpy.front() == '\"' || cpy.front() == '\\')
		cpy.erase(0ull, 1ull);
	while (cpy.back() == '"' || cpy.back() == '\'' || cpy.back() == '\"' || cpy.back() == '\\')
		cpy.erase(cpy.size() - 1ull);
	return cpy;
}