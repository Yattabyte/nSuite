#include "Directory.h"
#include "Log.h"
#include "Progress.h"
#include "Resource.h"
#include <direct.h>
#include <filesystem>
#include <fstream>
#include <shlobj.h>

using namespace NST;


// Private Static Methods

static auto check_exclusion(const std::string & path, const std::vector<std::string> & exclusions)
{
	const auto extension = std::filesystem::path(path).extension();
	for each (const auto & excl in exclusions) {
		if (excl.empty())
			continue;
		// Compare Paths && Extensions
		if (path == excl || extension == excl) {
			// Don't use path
			return false;
		}
	}
	// Safe to use path
	return true;
}

static auto get_file_paths(const std::string & directory, const std::vector<std::string> & exclusions)
{
	std::vector<std::filesystem::directory_entry> paths;
	if (std::filesystem::is_directory(directory))
		for (const auto & entry : std::filesystem::recursive_directory_iterator(directory))
			if (entry.is_regular_file()) {
				auto path = entry.path().string();
				path = path.substr(directory.size(), path.size() - directory.size());
				if (check_exclusion(path, exclusions))
					paths.emplace_back(entry);
			}
	return paths;
}


// Public (de)Constructors

void Directory::devirtualize()
{
	for (auto & file : m_files) {
		if (file.data != nullptr) {
			delete[] file.data;
			file.data = nullptr;
		}
	}
}

Directory::~Directory()
{
	devirtualize();
}

Directory::Directory(const std::string & path, const std::vector<std::string> & exclusions)
{
	m_directoryPath = path;
	m_exclusions = exclusions;

	// Treat path as a folder
	if (std::filesystem::is_directory(path))
		virtualize_folder(path);

	// Treat path as a resource
	else if (std::filesystem::path(path).extension() == ".exe") {
		auto libSuccess = LoadLibraryA(path.c_str());
		auto handle = GetModuleHandle(path.c_str());
		Resource fileResource(IDR_ARCHIVE, "ARCHIVE", handle);
		if (!libSuccess || handle == NULL || !fileResource.exists())
			Log::PushText("Error: cannot load the resource file specified!\r\n");
		else {
			virtualize_package(Buffer(fileResource.getPtr(), fileResource.getSize(), false));
			FreeLibrary(handle);
		}
	}

	// Treat path as a package
	else {
		// Last resort: try to treat the file as an nSuite package
		std::ifstream packFile(path, std::ios::binary | std::ios::beg);
		if (!packFile.is_open())
			Log::PushText("Error: cannot open the package file specified!\r\n");
		else {
			Buffer packBuffer = Buffer(std::filesystem::file_size(path));
			packFile.read(packBuffer.cArray(), std::streamsize(packBuffer.size()));
			packFile.close();
			virtualize_package(packBuffer);
		}
	}
}

Directory::Directory(const Buffer & package, const std::string & path, const std::vector<std::string> & exclusions)
{
	m_directoryPath = path;
	m_exclusions = exclusions;
	virtualize_package(package);
}

Directory::Directory(const Directory & other)
	: m_files(other.m_files), m_directoryPath(other.m_directoryPath), m_directoryName(other.m_directoryName), m_exclusions(other.m_exclusions)
{
	for (size_t x = 0, size = m_files.size(); x < size; ++x) {
		m_files[x].data = new std::byte[m_files[x].size];
		std::copy(other.m_files[x].data, &other.m_files[x].data[m_files[x].size], m_files[x].data);
	}
}

Directory::Directory(Directory && other)
{
	m_files = other.m_files;
	m_directoryPath = other.m_directoryPath;
	m_directoryName = other.m_directoryName;
	m_exclusions = other.m_exclusions;

	other.m_files.clear();
	other.m_files.shrink_to_fit();
	other.m_directoryName = "";
	other.m_directoryName = "";
	other.m_exclusions.clear();
	other.m_exclusions.shrink_to_fit();
}


// Public Operators

Directory & Directory::operator=(const Directory & other)
{
	if (this != &other) {
		devirtualize();
		m_files = other.m_files;
		m_directoryPath = other.m_directoryPath;
		m_directoryName = other.m_directoryName;
		m_exclusions = other.m_exclusions;

		for (size_t x = 0, size = m_files.size(); x < size; ++x) {
			m_files[x].data = new std::byte[m_files[x].size];
			std::copy(other.m_files[x].data, &other.m_files[x].data[m_files[x].size], m_files[x].data);
		}
	}
	return *this;
}

Directory & Directory::operator=(Directory && other)
{
	if (this != &other) {
		devirtualize();
		m_files = other.m_files;
		m_directoryPath = other.m_directoryPath;
		m_directoryName = other.m_directoryName;
		m_exclusions = other.m_exclusions;

		other.m_files.clear();
		other.m_files.shrink_to_fit();
		other.m_directoryName = "";
		other.m_directoryName = "";
		other.m_exclusions.clear();
		other.m_exclusions.shrink_to_fit();
	}
	return *this;
}


// Public Manipulation Methods

std::optional<Buffer> Directory::make_package() const
{
	// Ensure the source-directory has files
	if (!m_files.size())
		Log::PushText("Error: this directory with no (usable) files to package!\r\n");
	else {
		// Get a folder name, to name the package after
		auto srcPath = std::filesystem::path(m_directoryPath);
		auto folderName = srcPath.stem().string();
		while (folderName.empty()) {
			srcPath = srcPath.parent_path();
			folderName = srcPath.stem().string();
		};

		// Ensure we have a non-zero sized archive
		size_t archiveSize(byteCount());
		if (archiveSize <= 0ull)
			Log::PushText("Error: the archive has no data in it!\r\n");
		else {
			// Create file buffer to contain all the file data
			Buffer filebuffer(archiveSize);

			// Write file data into the buffer
			Progress::SetRange(archiveSize + 2);
			size_t byteIndex(0ull);
			for (auto & file : m_files) {
				// Write the total number of characters in the path string, into the archive
				const auto pathSize = file.relativePath.size();
				byteIndex = filebuffer.writeData(&pathSize, size_t(sizeof(size_t)), byteIndex);

				// Write the file path string, into the archive
				byteIndex = filebuffer.writeData(file.relativePath.data(), size_t(sizeof(char)) * pathSize, byteIndex);

				// Write the file size in bytes, into the archive
				byteIndex = filebuffer.writeData(&file.size, size_t(sizeof(size_t)), byteIndex);

				// Copy the file data
				if (file.size > 0ull && file.data != nullptr)
					std::copy(file.data, file.data + file.size, &filebuffer[byteIndex]);
				byteIndex += file.size;
				Progress::SetProgress(byteIndex);
			}

			// Compress the archive
			auto compressedBuffer = filebuffer.compress();
			filebuffer.release();
			Progress::IncrementProgress();
			if (!compressedBuffer)
				Log::PushText("Error: cannot compress the file-buffer for the chosen source directory!\r\n");
			else {
				// Prepend header information
				PackageHeader header(folderName.size(), folderName.c_str());
				compressedBuffer->writeHeader(&header);
				Progress::IncrementProgress();

				// Success
				return compressedBuffer;
			}
		}
	}

	// Failure
	return {};
}

bool Directory::apply_folder() const
{
	// Ensure the source-directory has files
	if (!m_files.size())
		Log::PushText("Error: this virtual-directory has no (usable) files to un-package!\r\n");
	else {
		Log::PushText("Dumping directory contents...\r\n");
		const auto finalDestionation = SanitizePath(m_directoryPath + "\\" + m_directoryName);
		Progress::SetRange(m_files.size());
		size_t fileCount(0ull);
		for (auto & file : m_files) {
			// Write-out the file
			const auto fullPath = finalDestionation + file.relativePath;
			std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());
			std::ofstream fileWriter(fullPath, std::ios::binary | std::ios::out);
			if (!fileWriter.is_open())
				Log::PushText("Error: cannot write file \"" + file.relativePath + "\" to disk.\r\n");
			else {
				Log::PushText("Writing file: \"" + file.relativePath + "\"\r\n");
				fileWriter.write(reinterpret_cast<char*>(file.data), (std::streamsize)file.size);
			}
			Progress::SetProgress(++fileCount);
			fileWriter.close();
		}

		// Success
		return true;
	}

	// Failure
	return false;
}

std::optional<Buffer> Directory::make_delta(const Directory & newDirectory) const
{
	// Declarations that will only be used here	
	typedef std::vector<DirFile> PathList;
	typedef std::vector<std::pair<DirFile, DirFile>> PathPairList;
	static constexpr auto getFileLists = [](const Directory & oldDirectory, const Directory & newDirectory, PathPairList & commonFiles, PathList & addFiles, PathList & delFiles) {
		// Find all common and new files first
		auto srcOld_Files = oldDirectory.m_files;
		auto srcNew_Files = newDirectory.m_files;
		for each (const auto & nFile in srcNew_Files) {
			bool found = false;
			size_t oIndex(0ull);
			for each (const auto & oFile in srcOld_Files) {
				if (nFile.relativePath == oFile.relativePath) {
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
	if (fileCount() <= 0 && newDirectory.fileCount() <= 0)
		Log::PushText("Error: input directories are empty!\r\n");
	else {
		// Retrieve all common, added, and removed files
		PathPairList commonFiles;
		PathList addedFiles, removedFiles;
		getFileLists(*this, newDirectory, commonFiles, addedFiles, removedFiles);
		// Generate Instructions from file lists, store them in this expanding buffer
		Buffer instructionBuffer;

		// These files are common, maybe some have changed
		size_t fileCount(0ull);
		for each (const auto & cFiles in commonFiles) {
			Buffer oldBuffer(cFiles.first.data, cFiles.first.size, false), newBuffer(cFiles.second.data, cFiles.second.size);
			size_t oldHash(oldBuffer.hash()), newHash(newBuffer.hash());
			if (oldHash != newHash) {
				// Files are different versions
				auto diffBuffer = oldBuffer.diff(newBuffer);
				if (diffBuffer) {
					Log::PushText("Diffing file \"" + cFiles.first.relativePath + "\"\r\n");
					writeInstructions(cFiles.first.relativePath, oldHash, newHash, *diffBuffer, 'U', instructionBuffer);
					fileCount++;
				}
			}
		}
		commonFiles.clear();
		commonFiles.shrink_to_fit();

		// These files are brand new
		for each (const auto & nFile in addedFiles) {
			Buffer newBuffer(nFile.data, nFile.size, false);
			size_t newHash(newBuffer.hash());
			auto diffBuffer = Buffer().diff(newBuffer);
			if (diffBuffer) {
				Log::PushText("Adding file \"" + nFile.relativePath + "\"\r\n");
				writeInstructions(nFile.relativePath, 0ull, newHash, *diffBuffer, 'N', instructionBuffer);
				fileCount++;
			}
		}
		addedFiles.clear();
		addedFiles.shrink_to_fit();

		// These files are deprecated
		for each (const auto & oFile in removedFiles) {
			Buffer oldBuffer(oFile.data, oFile.size, false);
			size_t oldHash(oldBuffer.hash());
			Log::PushText("Removing file \"" + oFile.relativePath + "\"\r\n");
			writeInstructions(oFile.relativePath, oldHash, 0ull, Buffer(), 'D', instructionBuffer);
			fileCount++;
		}
		removedFiles.clear();
		removedFiles.shrink_to_fit();

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

bool Directory::apply_delta(const Buffer & diffBuffer)
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
		if (!header.isValid())
			Log::PushText("Critical failure: supplied an invalid nSuite patch!\r\n");
		else {
			// Try to decompress the instruction buffer
			auto instructionBuffer = Buffer(dataPtr, dataSize, false).decompress();
			if (!instructionBuffer)
				Log::PushText("Error: cannot complete directory patching, as the instruction buffer cannot be decompressed!\r\n");
			else {
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
					FI.fullPath = m_directoryPath + FI.path;

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
				bool failed = false;
				for (FileInstruction & inst : diffFiles) {
					// Try to read the target file
					Buffer oldBuffer;
					size_t oldHash(0ull);
					DirFile * storedFile = nullptr;
					for (auto & file : m_files)
						if (file.relativePath == inst.path) {
							storedFile = &file;
							oldBuffer = Buffer(file.data, file.size, false);
							oldHash = oldBuffer.hash();
							break;
						}
					// Fail on missing Files
					if (!storedFile)
						Log::PushText("Critical failure: Cannot update \"" + inst.path + "\", the file is missing!\r\n");
					// Fail on empty files
					else if (!oldBuffer.hasData()) 
						Log::PushText("Critical failure: Cannot update \"" + inst.path + "\", the file is empty!\r\n");
					// Skip updated files
					else if (oldHash == inst.diff_newHash) {
						Log::PushText("The file \"" + inst.path + "\" is already up to date, skipping...\r\n");
						inst.instructionBuffer.release();
						continue;
					}
					// Fail on different version
					else if (oldHash != inst.diff_oldHash)
						Log::PushText("Critical failure: the file \"" + inst.path + "\" is of an unexpected version!\r\n");
					// Attempt Patching Process
					else {
						Log::PushText("Patching file \"" + inst.path + "\"\r\n");
						auto newBuffer = oldBuffer.patch(inst.instructionBuffer);
						if (!newBuffer)
							Log::PushText("Critical failure: patching failed!\r\n");
						else {
							// Confirm new hashes match
							const size_t newHash = newBuffer->hash();
							if (newHash != inst.diff_newHash)
								Log::PushText("Critical failure: patched file is corrupted (hash mismatch)!\r\n");
							else {
								// Update virtualized folder
								// Remove old data
								delete[] storedFile->data;

								// Replace with new data
								storedFile->data = new std::byte[newBuffer->size()];
								storedFile->size = newBuffer->size();
								std::copy(newBuffer->data(), &newBuffer->data()[newBuffer->size()], storedFile->data);

								// Reflect changes on disk
								std::ofstream newFile(inst.fullPath, std::ios::binary | std::ios::out);
								if (!newFile.is_open())
									Log::PushText("Critical failure: cannot write patched file to disk!\r\n");
								else 
									newFile.write(newBuffer->cArray(), std::streamsize(newBuffer->size()));
								newFile.close();
								inst.instructionBuffer.release();
								continue;
							}
						}
					}
					
					failed = true;
					inst.instructionBuffer.release();
					break;
				}
				diffFiles.clear();
				diffFiles.shrink_to_fit();

				if (!failed) {
					// By this point all files matched, safe to add new ones
					for (FileInstruction & inst : addedFiles) {					
						// Try to read the target file
						Buffer oldBuffer;
						size_t oldHash(0ull);
						DirFile * storedFile = nullptr;
						for (auto & file : m_files)
							if (file.relativePath == inst.path) {
								storedFile = &file;
								oldBuffer = Buffer(file.data, file.size, false);
								oldHash = oldBuffer.hash();
								break;
							}
						if (storedFile) {
							// Skip updated files
							if (oldHash == inst.diff_newHash) {
								Log::PushText("The file \"" + inst.path + "\" already exists, skipping...\r\n");
								inst.instructionBuffer.release();
								continue;
							}
						}
						else {
							Log::PushText("Adding file \"" + inst.path + "\"\r\n");
							std::filesystem::create_directories(std::filesystem::path(inst.fullPath).parent_path());

							// Write the 'insert' instructions
							// Remember that we use the diff/patch function to add new files too
							auto newBuffer = Buffer().patch(inst.instructionBuffer);
							if (!newBuffer)
								Log::PushText("Critical failure: cannot derive new file from patch instructions!\r\n");
							else {
								// Confirm new hashes match
								const size_t newHash = newBuffer->hash();
								if (newHash != inst.diff_newHash)
									Log::PushText("Critical failure: new file is corrupted (hash mismatch)!\r\n");
								else {
									// Update virtualized folder
									std::byte * fileData = new std::byte[newBuffer->size()];
									std::copy(newBuffer->data(), &newBuffer->data()[newBuffer->size()], fileData);
									m_files.push_back(DirFile{ inst.path, newBuffer->size(), fileData });
									
									// Reflect changes on disk
									std::ofstream newFile(inst.fullPath, std::ios::binary | std::ios::out);
									if (!newFile.is_open())
										Log::PushText("Critical failure: cannot write new file to disk!\r\n");
									else
										newFile.write(newBuffer->cArray(), std::streamsize(newBuffer->size()));
									newFile.close();
									inst.instructionBuffer.release();
									continue;
								}
							}
							failed = true;
							inst.instructionBuffer.release();
							break;
						}
					}
					addedFiles.clear();
					addedFiles.shrink_to_fit();

					if (!failed) {
						// If we made it this far, it should be safe to delete all files
						for (FileInstruction & inst : removedFiles) {
							// Try to read the target file (may not exist)
							size_t oldHash(0ull);

							// Try to find the source file (may not exist)
							size_t index(0ull);
							for each (const auto & file in m_files) {
								if (file.relativePath == inst.path) {
									oldHash = Buffer(file.data, file.size, false).hash();
									// Only remove source files if they match entirely
									if (oldHash == inst.diff_oldHash) {
										// Update virtualized folder
										m_files.erase(m_files.begin() + index);

										// Reflect changes on disk
										if (!std::filesystem::remove(inst.fullPath))
											Log::PushText("Error: cannot delete file \"" + inst.path + "\" from disk, delete this file manually if you can!\r\n");
										else
											Log::PushText("Removing file: \"" + inst.path + "\"\r\n");
									}
									break;
								}
								index++;
							}
							inst.instructionBuffer.release();
						}
						removedFiles.clear();
						removedFiles.shrink_to_fit();

						// Success
						return true;
					}
				}
			}
		}
	}

	return false;
}


// Public Accessor/Information Methods

size_t Directory::fileCount() const
{
	return m_files.size();
}

size_t Directory::byteCount() const
{
	size_t spaceUsed(0ull);
	for each (const auto & file in m_files) {
		const auto pathSize = file.relativePath.size();
		const size_t unitSize =
			size_t(sizeof(size_t)) +	// size of path size variable in bytes
			pathSize +					// the actual path data
			size_t(sizeof(size_t)) +	// size of the file size variable in bytes
			file.size;					// the actual file data
		spaceUsed += unitSize;
	}
	return spaceUsed;
}

std::string Directory::GetStartMenuPath()
{
	char cPath[FILENAME_MAX];
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, cPath)))
		return std::string(cPath);
	return std::string();
}

std::string Directory::GetDesktopPath()
{
	char cPath[FILENAME_MAX];
	if (SHGetSpecialFolderPathA(HWND_DESKTOP, cPath, CSIDL_DESKTOP, FALSE))
		return std::string(cPath);
	return std::string();
}

std::string Directory::GetRunningDirectory()
{
	char cCurrentPath[FILENAME_MAX];
	if (_getcwd(cCurrentPath, sizeof(cCurrentPath)))
		cCurrentPath[sizeof(cCurrentPath) - 1ull] = char('\0');
	return std::string(cCurrentPath);
}

std::string Directory::SanitizePath(const std::string & path)
{
	std::string cpy(path);
	while (cpy.front() == '"' || cpy.front() == '\'' || cpy.front() == '\"' || cpy.front() == '\\')
		cpy.erase(0ull, 1ull);
	while (cpy.back() == '"' || cpy.back() == '\'' || cpy.back() == '\"' || cpy.back() == '\\')
		cpy.erase(cpy.size() - 1ull);
	return cpy;
}


// Private Methods

void Directory::virtualize_folder(const std::string & input_path) 
{
	m_directoryName = std::filesystem::path(m_directoryPath).stem().string();
	for (const auto & entry : get_file_paths(input_path, m_exclusions)) {
		if (entry.is_regular_file()) {
			auto path = entry.path().string();
			path = path.substr(input_path.size(), path.size() - input_path.size());
			
			// Read the file data
			DirFile file{ path, entry.file_size(), nullptr };
			std::ifstream fileOnDisk(input_path + path, std::ios::binary | std::ios::beg | std::ios::in);
			if (fileOnDisk.is_open()) {
				file.data = new std::byte[file.size];
				fileOnDisk.read(reinterpret_cast<char*>(file.data), (std::streamsize)file.size);
				fileOnDisk.close();
			}
			m_files.push_back(file);
		}
	}
}

void Directory::virtualize_package(const Buffer & buffer)
{
	// Read in header		
	Directory::PackageHeader header;
	std::byte * dataPtr(nullptr);
	size_t dataSize(0ull);
	buffer.readHeader(&header, &dataPtr, dataSize);
	m_directoryName = header.m_folderName;

	// Ensure header title matches
	if (!header.isValid())
		Log::PushText("Critical failure: cannot parse package header!\r\n");
	else {
		// Try to decompress the buffer
		auto decompressedBuffer = Buffer(dataPtr, dataSize, false).decompress();
		if (!decompressedBuffer)
			Log::PushText("Critical failure: cannot decompress package file!\r\n");
		else {
			// Get lists of all files involved
			size_t byteIndex(0ull);
			Progress::SetRange(decompressedBuffer->size() + 100);
			while (byteIndex < decompressedBuffer->size()) {
				// Read path char count, path string, and file size
				size_t pathSize(0ull);
				byteIndex = decompressedBuffer->readData(&pathSize, size_t(sizeof(size_t)), byteIndex);
				char * pathArray = new char[pathSize];
				byteIndex = decompressedBuffer->readData(pathArray, pathSize, byteIndex);
				const std::string path(pathArray, pathSize);
				const auto extension = std::filesystem::path(path).extension();
				delete[] pathArray;
				size_t fileSize(0ull);
				byteIndex = decompressedBuffer->readData(&fileSize, size_t(sizeof(size_t)), byteIndex);

				// Save the file data
				if (check_exclusion(path, m_exclusions)) {
					std::byte * fileData = new std::byte[fileSize];
					std::copy(&((*decompressedBuffer)[byteIndex]), &((*decompressedBuffer)[byteIndex + fileSize]), fileData);
					m_files.push_back(DirFile{ path, fileSize, fileData });
				}
				byteIndex += fileSize;
			}
		}
	}
}