#include "DirectoryTools.h"
#include "BufferTools.h"
#include "Common.h"
#include "Instructions.h"
#include "Threader.h"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <vector>


void DRT::CompressDirectory(const std::string & srcDirectory, char ** packBuffer, size_t & packSize, size_t & fileCount)
{
	// Variables
	Threader threader;
	const auto absolute_path_length = srcDirectory.size();
	const auto directoryArray = get_file_paths(srcDirectory);
	struct FileData {
		std::string fullpath, trunc_path;
		size_t size, unitSize;
	};
	std::vector<FileData> files;
	files.reserve(directoryArray.size());

	// Calculate final file size using all the files in this directory,
	// and make a list containing all relevant files and their attributes
	size_t archiveSize(0);
	for each (const auto & entry in directoryArray) {
		auto path = entry.path().string();
		path = path.substr(absolute_path_length, path.size() - absolute_path_length);
		const auto pathSize = path.size();

		const size_t unitSize =
			size_t(sizeof(size_t)) +							// size of path size variable in bytes
			pathSize +											// the actual path data
			size_t(sizeof(size_t)) +							// size of the file size variable in bytes
			entry.file_size();									// the actual file data
		archiveSize += unitSize;
		files.push_back({ entry.path().string(), path, entry.file_size(), unitSize });
	}
	fileCount = files.size();

	// Create buffer for final file data
	char * filebuffer = new char[archiveSize];

	// Modify buffer
	void * pointer = filebuffer;
	for each (const auto & file in files) {
		threader.addJob([file, pointer]() {
			// Write the total number of characters in the path string, into the archive
			void * ptr = pointer;
			auto pathSize = file.trunc_path.size();
			memcpy(ptr, reinterpret_cast<char*>(&pathSize), size_t(sizeof(size_t)));
			ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));

			// Write the file path string, into the archive
			memcpy(ptr, file.trunc_path.data(), pathSize);
			ptr = PTR_ADD(ptr, pathSize);

			// Write the file size in bytes, into the archive
			auto size = file.size;
			memcpy(ptr, reinterpret_cast<char*>(&size), size_t(sizeof(size_t)));
			ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));

			// Read the file
			std::ifstream fileOnDisk(file.fullpath, std::ios::binary | std::ios::beg);
			fileOnDisk.read(reinterpret_cast<char*>(ptr), (std::streamsize)size);
			fileOnDisk.close();
		});

		pointer = PTR_ADD(pointer, file.unitSize);
	}

	// Wait for threaded operations to complete
	while (!threader.isFinished())
		continue;
	threader.shutdown();

	// Compress the archive
	if (!BFT::CompressBuffer(filebuffer, archiveSize, packBuffer, packSize))
		exit_program("Critical failure: cannot compress package file, aborting...\n");

	// Clean up
	delete[] filebuffer;
}

void DRT::DecompressDirectory(const std::string & dstDirectory, char * packBuffer, const size_t & packSize, size_t & byteCount, size_t & fileCount)
{
	Threader threader;
	char * decompressedBuffer(nullptr);
	size_t decompressedSize(0ull);
	if (!BFT::DecompressBuffer(packBuffer, packSize, &decompressedBuffer, decompressedSize))
		exit_program("Critical failure: cannot decompress package file, aborting...\n");

	// Read the archive
	size_t bytesRead(0ull);
	std::atomic_size_t filesWritten(0ull), bytesWritten(0ull);
	void * readingPtr = decompressedBuffer;
	while (bytesRead < decompressedSize) {
		// Read the total number of characters from the path string, from the archive
		const auto pathSize = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = PTR_ADD(readingPtr, size_t(sizeof(size_t)));

		// Read the file path string, from the archive
		const char * path_array = reinterpret_cast<char*>(readingPtr);
		const auto path = dstDirectory + std::string(path_array, pathSize);
		readingPtr = PTR_ADD(readingPtr, pathSize);

		// Read the file size in bytes, from the archive 
		const auto fileSize = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = PTR_ADD(readingPtr, size_t(sizeof(size_t)));

		// Write file out to disk, from the archive
		void * ptrCopy = readingPtr; // needed for lambda, since readingPtr gets incremented
		threader.addJob([ptrCopy, path, fileSize, &filesWritten, &bytesWritten]() {	
			// Write-out the file if it doesn't exist yet, if the size is different, or if it's older
			std::filesystem::create_directories(std::filesystem::path(path).parent_path());
			std::ofstream file(path, std::ios::binary | std::ios::out);
			if (file.is_open())
				file.write(reinterpret_cast<char*>(ptrCopy), (std::streamsize)fileSize);
			file.close();
			bytesWritten += fileSize;
			filesWritten++;			
		});

		readingPtr = PTR_ADD(readingPtr, fileSize);
		bytesRead += size_t(sizeof(size_t)) + pathSize + size_t(sizeof(size_t)) + fileSize;
	}

	// Wait for threaded operations to complete
	while (!threader.isFinished())
		continue;
	threader.shutdown();

	// Success
	fileCount = filesWritten;
	byteCount = bytesWritten;
	delete[] decompressedBuffer;
}

void DRT::DiffDirectory(const std::string & oldDirectory, const std::string & newDirectory, char ** diffBuffer, size_t & diffSize, size_t & instructionCount)
{
	// Declarations that will only be used here
	typedef std::vector<std::filesystem::directory_entry> PathList;
	typedef std::vector<std::pair<std::filesystem::directory_entry, std::filesystem::directory_entry>> PathPairList;
	std::vector<char> vecBuffer;

	// Some helper lambda's that will only be used here
	static constexpr auto getRelativePath = [](const std::filesystem::directory_entry & file, const std::string & directory) -> std::string {
		const auto path = file.path().string();
		return path.substr(directory.length(), path.length() - directory.length());
	};
	static constexpr auto readFile = [](const std::filesystem::directory_entry & file, const std::string & directory, std::string & relativePath, char ** buffer, size_t & hash) -> bool {
		relativePath = getRelativePath(file, directory);
		std::ifstream f(file, std::ios::binary | std::ios::beg);
		if (f.is_open()) {
			*buffer = new char[file.file_size()];
			f.read(*buffer, std::streamsize(file.file_size()));
			f.close();
			hash = BFT::HashBuffer(*buffer, file.file_size());
			return true;
		}
		return false;
	};
	static constexpr auto getFileLists = [](const std::string & oldDirectory, const std::string & newDirectory, PathPairList & commonFiles, PathList & addFiles, PathList & delFiles) {
		auto oldFiles = get_file_paths(oldDirectory);
		const auto newFiles = get_file_paths(newDirectory);

		// Find all common and new files first
		for each (const auto & nFile in newFiles) {
			std::string newRelativePath = getRelativePath(nFile, newDirectory);
			bool found = false;
			size_t oIndex(0ull);
			for each (const auto & oFile in oldFiles) {
				auto oldRelativePath = getRelativePath(oFile, oldDirectory);
				if (oldRelativePath == newRelativePath) {
					// Common file found		
					commonFiles.push_back(std::make_pair(oFile, nFile));

					// Remove old file from list (so we can use all that remain)
					oldFiles.erase(oldFiles.begin() + oIndex);
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
		delFiles = oldFiles;
	};
	static constexpr auto writeInstructions = [](const std::string & path, const size_t & oldHash, const size_t & newHash, char * buffer, const size_t & bufferSize, const char & flag, std::vector<char> & vecBuffer) {
		auto pathLength = path.length();
		const size_t instructionSize = (sizeof(size_t) * 4) + (sizeof(char) * pathLength) + sizeof(char) + bufferSize;
		const size_t bufferOldSize = vecBuffer.size();
		vecBuffer.resize(bufferOldSize + instructionSize);
		void * ptr = PTR_ADD(vecBuffer.data(), bufferOldSize);

		// Write file path length
		std::memcpy(ptr, reinterpret_cast<char*>(&pathLength), size_t(sizeof(size_t)));
		ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Write file path
		std::memcpy(ptr, path.data(), pathLength);
		ptr = PTR_ADD(ptr, pathLength);

		// Write operation flag
		std::memcpy(ptr, &flag, size_t(sizeof(char)));
		ptr = PTR_ADD(ptr, size_t(sizeof(char)));

		// Write old hash
		std::memcpy(ptr, reinterpret_cast<char*>(const_cast<size_t*>(&oldHash)), size_t(sizeof(size_t)));
		ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Write new hash
		std::memcpy(ptr, reinterpret_cast<char*>(const_cast<size_t*>(&newHash)), size_t(sizeof(size_t)));
		ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Write buffer size
		std::memcpy(ptr, reinterpret_cast<char*>(const_cast<size_t*>(&bufferSize)), size_t(sizeof(size_t)));
		ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Write buffer
		std::memcpy(ptr, buffer, bufferSize);
		ptr = PTR_ADD(ptr, bufferSize);
	};

	// Retrieve all common, added, and removed files
	PathPairList commonFiles;
	PathList addedFiles, removedFiles;
	getFileLists(oldDirectory, newDirectory, commonFiles, addedFiles, removedFiles);

	// Generate Instructions from file lists
	for each (const auto & cFiles in commonFiles) {
		std::string  oldRelativePath(""), newRelativePath("");
		char *oldBuffer(nullptr), *newBuffer(nullptr);
		size_t oldHash(0ull), newHash(0ull);
		if (
			readFile(cFiles.first, oldDirectory, oldRelativePath, &oldBuffer, oldHash)
			&&
			readFile(cFiles.second, newDirectory, newRelativePath, &newBuffer, newHash)
			)
		{
			// Successfully opened both files
			if (oldHash != newHash) {
				// Files are different versions
				char * buffer(nullptr);
				size_t size(0ull);
				if (BFT::DiffBuffers(oldBuffer, cFiles.first.file_size(), newBuffer, cFiles.second.file_size(), &buffer, size, &instructionCount)) {
					std::cout << "diffing file \"" << oldRelativePath << "\"\n";
					writeInstructions(newRelativePath, oldHash, newHash, buffer, size, 'U', vecBuffer);
					delete[] buffer;
				}
			}
		}
		delete[] oldBuffer;
		delete[] newBuffer;
	}
	for each (const auto & nFile in addedFiles) {
		std::string newRelativePath("");
		char *newBuffer(nullptr);
		size_t newHash(0ull);
		if (readFile(nFile, newDirectory, newRelativePath, &newBuffer, newHash)) {
			// Successfully opened file
			// This file is new
			char * buffer(nullptr);
			size_t size(0ull);
			if (BFT::DiffBuffers(nullptr, 0ull, newBuffer, nFile.file_size(), &buffer, size, &instructionCount)) {
				std::cout << "adding file \"" << newRelativePath << "\"\n";
				writeInstructions(newRelativePath, 0ull, newHash, buffer, size, 'N', vecBuffer);
				delete[] buffer;
			}
		}
		delete[] newBuffer;
	}
	for each (const auto & oFile in removedFiles) {
		std::string oldRelativePath("");
		char *oldBuffer(nullptr);
		size_t oldHash(0ull);
		if (readFile(oFile, oldDirectory, oldRelativePath, &oldBuffer, oldHash)) {
			// Successfully opened file
			// This file is no longer used
			instructionCount++;
			std::cout << "removing file \"" << oldRelativePath << "\"\n";
			writeInstructions(oldRelativePath, oldHash, 0ull, nullptr, 0ull, 'D', vecBuffer);
		}
		delete[] oldBuffer;
	}

	// Compress final buffer
	if (!BFT::CompressBuffer(vecBuffer.data(), vecBuffer.size(), diffBuffer, diffSize))
		exit_program("Critical failure: cannot compress diff file, aborting...\n");
}

void DRT::PatchDirectory(const std::string & dstDirectory, char * diffBufferCompressed, const size_t & diffSizeCompressed, size_t & bytesWritten, size_t & instructionsUsed)
{
	char * diffBuffer(nullptr);
	size_t diffSize(0ull);
	if (!BFT::DecompressBuffer(diffBufferCompressed, diffSizeCompressed, &diffBuffer, diffSize))
		exit_program("Critical failure: cannot decompress diff file, aborting...\n");

	static constexpr auto readFile = [](const std::string & path, const size_t & size, char ** buffer, size_t & hash) -> bool {
		std::ifstream f(path, std::ios::binary | std::ios::beg);
		if (f.is_open()) {
			*buffer = new char[size];
			f.read(*buffer, std::streamsize(size));
			f.close();
			hash = BFT::HashBuffer(*buffer, size);
			return true;
		}
		return false;
	};
	
	// Start reading diff file
	void * ptr = diffBuffer;
	size_t bytesRead(0ull);
	while (bytesRead < diffSize) {
		// Read file path length
		std::string path;
		size_t pathLength(0ull);
		std::memcpy(reinterpret_cast<char*>(&pathLength), ptr, size_t(sizeof(size_t)));
		ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));
		path.resize(pathLength);	

		// Read file path
		std::memcpy(path.data(), ptr, pathLength);
		ptr = PTR_ADD(ptr, pathLength);
		const auto fullPath = dstDirectory + path;

		// Read operation flag
		char flag;
		std::memcpy(&flag, ptr, size_t(sizeof(char)));
		ptr = PTR_ADD(ptr, size_t(sizeof(char)));

		// Read old hash
		size_t diff_oldHash(0ull), diff_newHash(0ull);
		std::memcpy(reinterpret_cast<char*>(const_cast<size_t*>(&diff_oldHash)), ptr, size_t(sizeof(size_t)));
		ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Read new hash
		std::memcpy(reinterpret_cast<char*>(const_cast<size_t*>(&diff_newHash)), ptr, size_t(sizeof(size_t)));
		ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Read buffer size
		size_t instructionSize(0ull);
		std::memcpy(reinterpret_cast<char*>(const_cast<size_t*>(&instructionSize)), ptr, size_t(sizeof(size_t)));
		ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Read buffer
		char * instructionSet = new char[instructionSize];
		std::memcpy(instructionSet, ptr, instructionSize);
		ptr = PTR_ADD(ptr, instructionSize);

		// Update range of memory read
		bytesRead += (sizeof(size_t) * 4) + (sizeof(char) * pathLength) + sizeof(char) + instructionSize;

		// Process instruction set
		// Try to read the target file (may not exist)
		char *oldBuffer(nullptr);
		size_t oldSize(0ull), oldHash(0ull);
		if (std::filesystem::exists(fullPath))
			oldSize = std::filesystem::file_size(fullPath);
		const bool srcRead = readFile(fullPath, oldSize, &oldBuffer, oldHash);

		switch (flag) {
		case 'D': // Delete source file
			// Only remove source files if they match entirely
			if (srcRead && oldHash == diff_oldHash)
				if (!std::filesystem::remove(fullPath))
					exit_program("Critical failure: cannot delete file from disk, aborting...\n");
			std::cout << "removing file \"" << path << "\"\n";
			break; // Done deletion procedure

		case 'N': // Create source file
			std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());
			if (!srcRead)
				std::cout << "adding file \"" << path << "\"\n";
		case 'U': // Update source file
		default:
			if (flag == 'U') {
				if (!srcRead)
					exit_program("Cannot read source file from disk, aborting...\n");
				if (oldHash == diff_newHash) {
					std::cout << "The file \"" << path << "\" is already up to date, skipping...\n";
					break;
				}
				else
					std::cout << "diffing file \"" << path << "\"\n";
			}

			// Patch buffer
			char * newBuffer(nullptr);
			size_t newSize(0ull);
			BFT::PatchBuffer(oldBuffer, oldSize, &newBuffer, newSize, instructionSet, instructionSize, &instructionsUsed);
			const size_t newHash = BFT::HashBuffer(newBuffer, newSize);
			delete[] instructionSet;

			// Confirm new hashes match
			if (newHash != diff_newHash)
				exit_program("Critical failure: patched file is corrupted (hash mismatch), aborting...\n");

			// Write patched buffer to disk
			std::ofstream newFile(fullPath, std::ios::binary | std::ios::out);
			if (!newFile.is_open())
				exit_program("Cannot write updated file to disk, aborting...\n");
			newFile.write(newBuffer, std::streamsize(newSize));
			newFile.close();
			delete[] newBuffer;
			bytesWritten += newSize;

			// Cleanup and finish
			delete[] oldBuffer;
		}
	}
}