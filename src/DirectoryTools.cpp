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
	threader.prepareForShutdown();
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
	threader.prepareForShutdown();
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
	struct File {
		std::string path = "", relative = "";
		size_t size = 0ull;
		inline File(const std::string & p, const std::string & r, const size_t & s) : path(p), relative(r), size(s) {}
		inline virtual bool open(char ** buffer, size_t & hash) const {
			std::ifstream f(path, std::ios::binary | std::ios::beg);
			if (f.is_open()) {
				*buffer = new char[size];
				f.read(*buffer, std::streamsize(size));
				f.close();
				hash = BFT::HashBuffer(*buffer, size);
				return true;
			}
			return false;
		}
	};
	struct FileMem : File {
		void * ptr = nullptr;
		inline FileMem(const std::string & p, const std::string & r, const size_t & s, void * b) : File(p, r, s), ptr(b) {}
		inline virtual bool open(char ** buffer, size_t & hash) const override {
			*buffer = new char[size];
			std::memcpy(*buffer, ptr, size);
			hash = BFT::HashBuffer(*buffer, size);
			return true;
		}
	};
	typedef std::vector<File*> PathList;
	typedef std::vector<std::pair<File*, File*>> PathPairList;
	static constexpr auto getRelativePath = [](const std::string & filePath, const std::string & directory) -> std::string {
		return filePath.substr(directory.length(), filePath.length() - directory.length());
	};
	static constexpr auto getFiles = [](const std::string & directory, char ** snapshot, size_t & size) -> std::vector<File*> {
		std::vector<File*> files;
		if (std::filesystem::is_directory(directory)) {
			for each (const auto & srcFile in get_file_paths(directory)) {
				const std::string path = srcFile.path().string();
				size += srcFile.file_size();
				files.push_back(new File(path, getRelativePath(path, directory), srcFile.file_size()));
			}
		}
		else {
			// Treat as a snapshot file if it isn't a directory
			// Open diff file
			std::ifstream packFile(directory, std::ios::binary | std::ios::beg);
			const size_t packSize = std::filesystem::file_size(directory);
			if (!packFile.is_open())
				exit_program("Cannot read snapshot file, aborting...\n");
			char * compBuffer = new char[packSize];
			packFile.read(compBuffer, std::streamsize(packSize));
			packFile.close();

			// Decompress
			size_t snapSize(0ull);
			if (!BFT::DecompressBuffer(compBuffer, packSize, snapshot, snapSize))
				exit_program("Cannot decompress snapshot file, aborting...\n");
			delete[] compBuffer;

			// Get lists of all files involved
			size_t bytesRead(0ull);
			void * ptr = *snapshot;
			while (bytesRead < snapSize) {
				// Read the total number of characters from the path string, from the archive
				const auto pathSize = *reinterpret_cast<size_t*>(ptr);
				ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));

				// Read the file path string, from the archive
				const char * path_array = reinterpret_cast<char*>(ptr);
				const auto path = std::string(path_array, pathSize);
				ptr = PTR_ADD(ptr, pathSize);

				// Read the file size in bytes, from the archive 
				const auto fileSize = *reinterpret_cast<size_t*>(ptr);
				ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));

				files.push_back(new FileMem(path, path, fileSize, ptr));
				ptr = PTR_ADD(ptr, fileSize);
				bytesRead += size_t(sizeof(size_t)) + pathSize + size_t(sizeof(size_t)) + fileSize;
				size += fileSize;
			}
		}
		return files;
	};
	static constexpr auto getFileLists = [](const std::string & oldDirectory, char ** oldSnapshot, const std::string & newDirectory, char ** newSnapshot, size_t & reserveSize, PathPairList & commonFiles, PathList & addFiles, PathList & delFiles) {
		// Get files
		size_t size1(0ull), size2(0ull);
		auto srcOld_Files = getFiles(oldDirectory, oldSnapshot, size1);
		auto srcNew_Files = getFiles(newDirectory, newSnapshot, size2);
		reserveSize = std::max<size_t>(size1, size2);

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
	char * oldSnap(nullptr), * newSnap(nullptr);
	size_t reserveSize(0ull);
	getFileLists(oldDirectory, &oldSnap, newDirectory, &newSnap, reserveSize, commonFiles, addedFiles, removedFiles);

	// Generate Instructions from file lists, store them in this expanding buffer
	std::vector<char> vecBuffer;
	vecBuffer.reserve(reserveSize);

	// These files are common, maybe some have changed
	for each (const auto & cFiles in commonFiles) {
		char * oldBuffer(nullptr), * newBuffer(nullptr);
		size_t oldHash(0ull), newHash(0ull);
		if (cFiles.first->open(&oldBuffer, oldHash) && cFiles.second->open(&newBuffer, newHash)) {
			if (oldHash != newHash) {
				// Files are different versions
				char * buffer(nullptr);
				size_t size(0ull);
				if (BFT::DiffBuffers(oldBuffer, cFiles.first->size, newBuffer, cFiles.second->size, &buffer, size, &instructionCount)) {
					std::cout << "diffing file \"" << cFiles.first->relative << "\"\n";
					writeInstructions(cFiles.first->relative, oldHash, newHash, buffer, size, 'U', vecBuffer);
				}
				delete[] buffer;
			}
		}
		delete[] oldBuffer;
		delete[] newBuffer;
		delete cFiles.first;
		delete cFiles.second;
	}
	commonFiles.clear();

	// These files are brand new
	for each (const auto & nFile in addedFiles) {
		char * newBuffer(nullptr);
		size_t newHash(0ull);
		if (nFile->open(&newBuffer, newHash)) {
			char * buffer(nullptr);
			size_t size(0ull);
			if (BFT::DiffBuffers(nullptr, 0ull, newBuffer, nFile->size, &buffer, size, &instructionCount)) {
				std::cout << "adding file \"" << nFile->relative << "\"\n";
				writeInstructions(nFile->relative, 0ull, newHash, buffer, size, 'N', vecBuffer);
			}
			delete[] buffer;
		}
		delete[] newBuffer;
		delete nFile;
	}
	addedFiles.clear();
	delete[] newSnap;

	// These files are deprecated
	for each (const auto & oFile in removedFiles) {
		char * oldBuffer(nullptr);
		size_t oldHash(0ull);
		if (oFile->open(&oldBuffer, oldHash)) {
			instructionCount++;
			std::cout << "removing file \"" << oFile->relative << "\"\n";
			writeInstructions(oFile->relative, oldHash, 0ull, nullptr, 0ull, 'D', vecBuffer);
		}
		delete[] oldBuffer;
		delete oFile;
	}
	removedFiles.clear();
	delete[] oldSnap;

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