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