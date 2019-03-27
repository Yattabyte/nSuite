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
				exit_program("Cannot read package file, aborting...\n");
			char * compBuffer = new char[packSize];
			packFile.read(compBuffer, std::streamsize(packSize));
			packFile.close();

			// Decompress
			size_t snapSize(0ull);
			if (!BFT::DecompressBuffer(compBuffer, packSize, snapshot, snapSize))
				exit_program("Cannot decompress package file, aborting...\n");
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

bool DRT::PatchDirectory(const std::string & dstDirectory, char * diffBufferCompressed, const size_t & diffSizeCompressed, size_t & bytesWritten, size_t & instructionsUsed)
{
	char * diffBuffer(nullptr);
	size_t diffSize(0ull);
	if (!BFT::DecompressBuffer(diffBufferCompressed, diffSizeCompressed, &diffBuffer, diffSize)) {
		std::cout << "Critical failure: cannot decompress diff file.\n";
		return false;
	}

	static constexpr auto readFile = [](const std::string & path, size_t & size, char ** buffer, size_t & hash) -> bool {
		std::ifstream f(path, std::ios::binary | std::ios::beg);
		if (f.is_open()) {
			size = std::filesystem::file_size(path);
			*buffer = new char[size];
			f.read(*buffer, std::streamsize(size));
			f.close();
			hash = BFT::HashBuffer(*buffer, size);
			return true;
		}
		return false;
	};
	
	// Start reading diff file
	struct FileInstruction {
		std::string path = "", fullPath = "";
		char * instructionSet = nullptr;
		size_t instructionSize = 0ull, diff_oldHash = 0ull, diff_newHash = 0ull;
	};
	std::vector<FileInstruction> diffFiles, addedFiles, removedFiles;
	void * ptr = diffBuffer;
	size_t bytesRead(0ull);
	while (bytesRead < diffSize) {
		FileInstruction FI;

		// Read file path length
		size_t pathLength(0ull);
		std::memcpy(reinterpret_cast<char*>(&pathLength), ptr, size_t(sizeof(size_t)));
		ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));
		FI.path.resize(pathLength);

		// Read file path
		std::memcpy(FI.path.data(), ptr, pathLength);
		ptr = PTR_ADD(ptr, pathLength);
		FI.fullPath = dstDirectory + FI.path;

		// Read operation flag
		char flag;
		std::memcpy(&flag, ptr, size_t(sizeof(char)));
		ptr = PTR_ADD(ptr, size_t(sizeof(char)));

		// Read old hash
		std::memcpy(reinterpret_cast<char*>(const_cast<size_t*>(&FI.diff_oldHash)), ptr, size_t(sizeof(size_t)));
		ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Read new hash
		std::memcpy(reinterpret_cast<char*>(const_cast<size_t*>(&FI.diff_newHash)), ptr, size_t(sizeof(size_t)));
		ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Read buffer size
		std::memcpy(reinterpret_cast<char*>(const_cast<size_t*>(&FI.instructionSize)), ptr, size_t(sizeof(size_t)));
		ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Read buffer
		FI.instructionSet = new char[FI.instructionSize];
		std::memcpy(FI.instructionSet, ptr, FI.instructionSize);
		ptr = PTR_ADD(ptr, FI.instructionSize);

		// Update range of memory read
		bytesRead += (sizeof(size_t) * 4) + (sizeof(char) * pathLength) + sizeof(char) + FI.instructionSize;

		// Sort instructions
		if (flag == 'U')
			diffFiles.push_back(FI);
		else if (flag == 'N')
			addedFiles.push_back(FI);
		else if (flag == 'D')
			removedFiles.push_back(FI);
	}

	// Patch all files first
	for each (const auto & file in diffFiles) {
		// Try to read the target file
		char *oldBuffer(nullptr), *newBuffer(nullptr);
		size_t oldSize(0ull), oldHash(0ull);
	
		// Try to read source file
		if (!readFile(file.fullPath, oldSize, &oldBuffer, oldHash)) {
			std::cout << "Cannot read source file from disk.\n";
			return false;
		}

		// Patch if this source file hasn't been patched yet
		if (oldHash != file.diff_newHash) {
			// Patch buffer
			std::cout << "patching file \"" << file.path << "\"\n";
			size_t newSize(0ull);
			BFT::PatchBuffer(oldBuffer, oldSize, &newBuffer, newSize, file.instructionSet, file.instructionSize, &instructionsUsed);
			const size_t newHash = BFT::HashBuffer(newBuffer, newSize);

			// Confirm new hashes match
			if (newHash != file.diff_newHash) {
				std::cout << "Critical failure: patched file is corrupted (hash mismatch).\n";
				return false;
			}

			// Write patched buffer to disk
			std::ofstream newFile(file.fullPath, std::ios::binary | std::ios::out);
			if (!newFile.is_open()) {
				std::cout << "Cannot write patched file to disk.\n";
				return false;
			}
			newFile.write(newBuffer, std::streamsize(newSize));
			newFile.close();
			bytesWritten += newSize;
		}
		else
			std::cout << "The file \"" << file.path << "\" is already up to date, skipping...\n";

		// Cleanup and finish
		delete[] file.instructionSet;
		delete[] newBuffer;
		delete[] oldBuffer;
	}

	// By this point all files matched, safe to add new ones
	for each (const auto & file in addedFiles) {
		std::filesystem::create_directories(std::filesystem::path(file.fullPath).parent_path());
		std::cout << "adding file \"" << file.path << "\"\n";
		
		// Write the 'insert' instructions
		// Remember that we use the diff/patch function to add new files too
		char * newBuffer(nullptr);
		size_t newSize(0ull);
		BFT::PatchBuffer(nullptr, 0ull, &newBuffer, newSize, file.instructionSet, file.instructionSize, &instructionsUsed);
		const size_t newHash = BFT::HashBuffer(newBuffer, newSize);

		// Confirm new hashes match
		if (newHash != file.diff_newHash) {
			std::cout << "Critical failure: new file is corrupted (hash mismatch).\n";
			return false;
		}

		// Write new file to disk
		std::ofstream newFile(file.fullPath, std::ios::binary | std::ios::out);
		if (!newFile.is_open()) {
			std::cout << "Cannot write new file to disk.\n";
			return false;
		}
		newFile.write(newBuffer, std::streamsize(newSize));
		newFile.close();
		bytesWritten += newSize;

		// Cleanup and finish
		delete[] file.instructionSet;
		delete[] newBuffer;		
	}

	// If we made it this far, it should be safe to delete all files
	for each (const auto & file in removedFiles) {
		// Try to read the target file (may not exist)
		char *oldBuffer(nullptr);
		size_t oldSize(0ull), oldHash(0ull);

		// Try to read source file
		if (!readFile(file.fullPath, oldSize, &oldBuffer, oldHash))
			std::cout << "The file \"" << file.path << "\" has already been removed, skipping...\n";
		else {
			// Only remove source files if they match entirely
			if (oldHash == file.diff_oldHash)
				if (!std::filesystem::remove(file.fullPath))
					std::cout << "Critical failure: cannot delete file \"" << file.path << "\" from disk\n";
				else
					std::cout << "removing file \"" << file.path << "\"\n";
		}

		// Cleanup and finish
		delete[] file.instructionSet;
		delete[] oldBuffer;
	}

	return true;
}