#include "DirectoryTools.h"
#include "BufferTools.h"
#include "Resource.h"
#include "Threader.h"
#include "Log.h"
#include "Progress.h"
#include <atomic>
#include <direct.h>
#include <filesystem>
#include <fstream>
#include <shlobj.h>
#include <vector>


bool DRT::CompressDirectory(const std::string & srcDirectory, char ** packBuffer, size_t & packSize, size_t * byteCount, size_t * fileCount, const std::vector<std::string> & exclusions)
{
	// Variables
	Threader threader;
	const auto absolute_path_length = srcDirectory.size();
	const auto directoryArray = GetFilePaths(srcDirectory);
	struct FileData {
		std::string fullpath, trunc_path;
		size_t size, unitSize;
	};
	std::vector<FileData> files;
	files.reserve(directoryArray.size());

	// Get path name
	if (!directoryArray.size()) {
		Log::PushText("Critical failure: the source path specified \"" + srcDirectory + "\" is not a (useable) directory.\r\n");
		return false;
	}
	std::filesystem::path srcPath = std::filesystem::path(srcDirectory);
	std::string folderName = srcPath.stem().string();
	while (folderName.empty()) {
		srcPath = srcPath.parent_path();
		folderName = srcPath.stem().string();
	};

	// Calculate final file size using all the files in this directory,
	// and make a list containing all relevant files and their attributes
	size_t archiveSize(0ull);
	for each (const auto & entry in directoryArray) {
		const auto extension = std::filesystem::path(entry).extension();
		auto path = entry.path().string();
		path = path.substr(absolute_path_length, path.size() - absolute_path_length);
		bool useEntry = true;
		for each (const auto & excl in exclusions) {
			if (excl.empty())
				continue;
			// Compare Paths
			if (path == excl) {
				useEntry = false;
				break;
			}
			// Compare Extensions
			else if (extension == excl) {
				useEntry = false;
				break;
			}			
		}
		if (useEntry) {
			const auto pathSize = path.size();

			const size_t unitSize =
				size_t(sizeof(size_t)) +							// size of path size variable in bytes
				pathSize +											// the actual path data
				size_t(sizeof(size_t)) +							// size of the file size variable in bytes
				entry.file_size();									// the actual file data
			archiveSize += unitSize;
			files.push_back({ entry.path().string(), path, entry.file_size(), unitSize });
		}
	}

	// Update optional parameters
	if (byteCount != nullptr)
		*byteCount = archiveSize;
	if (fileCount != nullptr)
		*fileCount = files.size();

	// Create buffer for final file data
	char * filebuffer = new char[archiveSize];

	// Write file data into the buffer
	void * pointer = filebuffer;
	for each (const auto & file in files) {
		threader.addJob([file, pointer]() {
			// Write the total number of characters in the path string, into the archive
			void * ptr = pointer;
			auto pathSize = file.trunc_path.size();
			memcpy(ptr, reinterpret_cast<char*>(&pathSize), size_t(sizeof(size_t)));
			ptr = BFT::PTR_ADD(ptr, size_t(sizeof(size_t)));

			// Write the file path string, into the archive
			memcpy(ptr, file.trunc_path.data(), pathSize);
			ptr = BFT::PTR_ADD(ptr, pathSize);

			// Write the file size in bytes, into the archive
			auto size = file.size;
			memcpy(ptr, reinterpret_cast<char*>(&size), size_t(sizeof(size_t)));
			ptr = BFT::PTR_ADD(ptr, size_t(sizeof(size_t)));

			// Read the file
			std::ifstream fileOnDisk(file.fullpath, std::ios::binary | std::ios::beg);
			fileOnDisk.read(reinterpret_cast<char*>(ptr), (std::streamsize)size);
			fileOnDisk.close();
		});

		pointer = BFT::PTR_ADD(pointer, file.unitSize);
	}

	// Wait for threaded operations to complete
	threader.prepareForShutdown();
	while (!threader.isFinished())
		continue;
	threader.shutdown();

	// Compress the archive
	char * compBuffer(nullptr);
	size_t compSize(0ull);
	if (!BFT::CompressBuffer(filebuffer, archiveSize, &compBuffer, compSize)) {
		Log::PushText("Critical failure: cannot perform compression operation on the set of joined files.\r\n");
		return false;
	}
	delete[] filebuffer;

	// Move compressed buffer data to a new buffer that has a special header
	packSize = compSize + sizeof(size_t) + folderName.size();
	*packBuffer = new char[packSize];
	memcpy(&(*packBuffer)[sizeof(size_t) + folderName.size()], compBuffer, compSize);
	delete[] compBuffer;

	// Write the header (root folder name and size)
	pointer = *packBuffer;
	auto pathSize = folderName.size();	
	memcpy(pointer, reinterpret_cast<char*>(&pathSize), size_t(sizeof(size_t)));
	pointer = BFT::PTR_ADD(pointer, size_t(sizeof(size_t)));
	memcpy(pointer, folderName.data(), pathSize);
	return true;
}

bool DRT::DecompressDirectory(const std::string & dstDirectory, char * packBuffer, const size_t & packSize, size_t * byteCount, size_t * fileCount)
{
	Threader threader;
	if (packSize <= 0ull) {
		Log::PushText("Critical failure: package buffer has no content.\r\n");
		return false;
	}

	// Read the directory header
	char * packBufferOffset = packBuffer;
	const auto folderSize = *reinterpret_cast<size_t*>(packBufferOffset);
	packBufferOffset = reinterpret_cast<char*>(BFT::PTR_ADD(packBufferOffset, size_t(sizeof(size_t))));
	const char * folderArray = reinterpret_cast<char*>(packBufferOffset);
	const auto finalDestionation = SanitizePath(dstDirectory + "\\" + std::string(folderArray, folderSize));
	packBufferOffset = reinterpret_cast<char*>(BFT::PTR_ADD(packBufferOffset, folderSize));

	char * decompressedBuffer(nullptr);
	size_t decompressedSize(0ull);
	if (!BFT::DecompressBuffer(packBufferOffset, packSize - (size_t(sizeof(size_t)) + folderSize), &decompressedBuffer, decompressedSize)) {
		Log::PushText("Critical failure: cannot decompress package file.\r\n");
		return false;
	}

	// Begin reading the archive
	void * readingPtr = decompressedBuffer;
	size_t bytesRead(0ull);
	std::atomic_size_t filesWritten(0ull);
	Progress::SetRange(decompressedSize + 100);
	while (bytesRead < decompressedSize) {
		// Read the total number of characters from the path string, from the archive
		const auto pathSize = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = BFT::PTR_ADD(readingPtr, size_t(sizeof(size_t)));

		// Read the file path string, from the archive
		const char * path_array = reinterpret_cast<char*>(readingPtr);
		std::string fullPath = finalDestionation + std::string(path_array, pathSize);
		readingPtr = BFT::PTR_ADD(readingPtr, pathSize);

		// Read the file size in bytes, from the archive 
		const auto fileSize = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = BFT::PTR_ADD(readingPtr, size_t(sizeof(size_t)));

		// Write file out to disk, from the archive
		void * ptrCopy = readingPtr; // needed for lambda, since readingPtr gets incremented
		threader.addJob([ptrCopy, fullPath, fileSize, &filesWritten]() {
			// Write-out the file if it doesn't exist yet, if the size is different, or if it's older
			std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());
			std::ofstream file(fullPath, std::ios::binary | std::ios::out);
			if (!file.is_open())
				Log::PushText("Error writing file: \"" + fullPath + "\" to disk");
			else {
				file.write(reinterpret_cast<char*>(ptrCopy), (std::streamsize)fileSize);
				file.close();
			}
			filesWritten++;			
		});

		readingPtr = BFT::PTR_ADD(readingPtr, fileSize);
		bytesRead += size_t(sizeof(size_t)) + pathSize + size_t(sizeof(size_t)) + fileSize;
		Log::PushText("Writing file: \"" + std::string(path_array, pathSize) + "\"\r\n");
		Progress::SetProgress(bytesRead);
	}

	// Wait for threaded operations to complete
	threader.prepareForShutdown();
	while (!threader.isFinished())
		continue;	
	threader.shutdown();
	Progress::SetProgress(bytesRead + 100);

	// Update optional parameters
	if (byteCount != nullptr)
		*byteCount = bytesRead;
	if (fileCount != nullptr)
		*fileCount = filesWritten;

	// Success
	delete[] decompressedBuffer;
	return true;
}

bool DRT::DiffDirectories(const std::string & oldDirectory, const std::string & newDirectory, char ** diffBuffer, size_t & diffSize, size_t * instructionCount)
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
	static constexpr auto getFiles = [](const std::string & directory, char ** snapshot, size_t & size, std::vector<File*> & files) -> bool {		
		if (std::filesystem::is_directory(directory)) {
			for each (const auto & srcFile in GetFilePaths(directory)) {
				const std::string path = srcFile.path().string();
				size += srcFile.file_size();
				files.push_back(new File(path, getRelativePath(path, directory), srcFile.file_size()));
			}
			return true;
		}
		// See if the file is actually a program with an embedded archive
		char * packBuffer(nullptr);
		size_t packSize(0ull);
		bool canLoad = LoadLibraryA(directory.c_str());
		auto handle = GetModuleHandle(directory.c_str());
		Resource fileResource(IDR_ARCHIVE, "ARCHIVE", handle);
		if (canLoad && handle != NULL && fileResource.exists()) {
			// Extract the archive
			packBuffer = reinterpret_cast<char*>(fileResource.getPtr());
			packSize = fileResource.getSize();
		}
		else {
			// Last resort, treat as a snapshot file if it isn't a directory
			// Open diff file
			std::ifstream packFile(directory, std::ios::binary | std::ios::beg);
			packSize = std::filesystem::file_size(directory);
			if (!packFile.is_open()) {
				Log::PushText("Critical failure: cannot read package file.\r\n");
				return false;
			}
			packBuffer = new char[packSize];
			packFile.read(packBuffer, std::streamsize(packSize));
			packFile.close();
		}
		// We don't care about the package's header (folder name + name size)
		char * packBufferOffset = packBuffer;
		const auto folderSize = *reinterpret_cast<size_t*>(packBuffer);
		packBufferOffset = reinterpret_cast<char*>(BFT::PTR_ADD(packBufferOffset, size_t(sizeof(size_t))));
		packBufferOffset = reinterpret_cast<char*>(BFT::PTR_ADD(packBufferOffset, folderSize));

		// Decompress
		size_t snapSize(0ull);
		if (!BFT::DecompressBuffer(packBufferOffset, packSize - (size_t(sizeof(size_t)) + folderSize), snapshot, snapSize)) {
			Log::PushText("Critical failure: cannot decompress package file.\r\n");
			return false;
		}

		// Check if we need to delete the packBuffer, or if it is an embedded resource
		// If it's an embedded resource, the fileResource's destructor will satisfy, else do below
		if (!canLoad || handle == NULL || !fileResource.exists())
			delete[] packBuffer;
		else if (canLoad && handle != NULL)
			FreeLibrary(handle);

		// Get lists of all files involved
		size_t bytesRead(0ull);
		void * ptr = *snapshot;
		while (bytesRead < snapSize) {
			// Read the total number of characters from the path string, from the archive
			const auto pathSize = *reinterpret_cast<size_t*>(ptr);
			ptr = BFT::PTR_ADD(ptr, size_t(sizeof(size_t)));

			// Read the file path string, from the archive
			const char * path_array = reinterpret_cast<char*>(ptr);
			const auto path = std::string(path_array, pathSize);
			ptr = BFT::PTR_ADD(ptr, pathSize);

			// Read the file size in bytes, from the archive 
			const auto fileSize = *reinterpret_cast<size_t*>(ptr);
			ptr = BFT::PTR_ADD(ptr, size_t(sizeof(size_t)));

			files.push_back(new FileMem(path, path, fileSize, ptr));
			ptr = BFT::PTR_ADD(ptr, fileSize);
			bytesRead += size_t(sizeof(size_t)) + pathSize + size_t(sizeof(size_t)) + fileSize;
			size += fileSize;
		}
		return true;		
	};
	static constexpr auto getFileLists = [](const std::string & oldDirectory, char ** oldSnapshot, const std::string & newDirectory, char ** newSnapshot, size_t & reserveSize, PathPairList & commonFiles, PathList & addFiles, PathList & delFiles) {
		// Get files
		size_t size1(0ull), size2(0ull);
		std::vector<File*> srcOld_Files, srcNew_Files;
		if (!getFiles(oldDirectory, oldSnapshot, size1, srcOld_Files) || !getFiles(newDirectory, newSnapshot, size2, srcNew_Files))
			return false;
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
		return true;
	};
	static constexpr auto writeInstructions = [](const std::string & path, const size_t & oldHash, const size_t & newHash, char * buffer, const size_t & bufferSize, const char & flag, std::vector<char> & vecBuffer) {
		auto pathLength = path.length();
		const size_t instructionSize = (sizeof(size_t) * 4) + (sizeof(char) * pathLength) + sizeof(char) + bufferSize;
		const size_t bufferOldSize = vecBuffer.size();
		vecBuffer.resize(bufferOldSize + instructionSize);
		void * ptr = BFT::PTR_ADD(vecBuffer.data(), bufferOldSize);

		// Write file path length
		std::memcpy(ptr, reinterpret_cast<char*>(&pathLength), size_t(sizeof(size_t)));
		ptr = BFT::PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Write file path
		std::memcpy(ptr, path.data(), pathLength);
		ptr = BFT::PTR_ADD(ptr, pathLength);

		// Write operation flag
		std::memcpy(ptr, &flag, size_t(sizeof(char)));
		ptr = BFT::PTR_ADD(ptr, size_t(sizeof(char)));

		// Write old hash
		std::memcpy(ptr, reinterpret_cast<char*>(const_cast<size_t*>(&oldHash)), size_t(sizeof(size_t)));
		ptr = BFT::PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Write new hash
		std::memcpy(ptr, reinterpret_cast<char*>(const_cast<size_t*>(&newHash)), size_t(sizeof(size_t)));
		ptr = BFT::PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Write buffer size
		std::memcpy(ptr, reinterpret_cast<char*>(const_cast<size_t*>(&bufferSize)), size_t(sizeof(size_t)));
		ptr = BFT::PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Write buffer
		std::memcpy(ptr, buffer, bufferSize);
		ptr = BFT::PTR_ADD(ptr, bufferSize);
	};

	// Retrieve all common, added, and removed files
	PathPairList commonFiles;
	PathList addedFiles, removedFiles;
	char * oldSnap(nullptr), * newSnap(nullptr);
	size_t reserveSize(0ull);
	if (!getFileLists(oldDirectory, &oldSnap, newDirectory, &newSnap, reserveSize, commonFiles, addedFiles, removedFiles))
		return false;

	// Generate Instructions from file lists, store them in this expanding buffer
	std::vector<char> vecBuffer;
	vecBuffer.reserve(reserveSize);

	// These files are common, maybe some have changed
	size_t instructionNum(0ull);
	for each (const auto & cFiles in commonFiles) {
		char * oldBuffer(nullptr), * newBuffer(nullptr);
		size_t oldHash(0ull), newHash(0ull);
		if (cFiles.first->open(&oldBuffer, oldHash) && cFiles.second->open(&newBuffer, newHash)) {
			if (oldHash != newHash) {
				// Files are different versions
				char * buffer(nullptr);
				size_t size(0ull);
				if (BFT::DiffBuffers(oldBuffer, cFiles.first->size, newBuffer, cFiles.second->size, &buffer, size, &instructionNum)) {
					Log::PushText("Diffing file: \"" + cFiles.first->relative + "\"\r\n");
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
			if (BFT::DiffBuffers(nullptr, 0ull, newBuffer, nFile->size, &buffer, size, &instructionNum)) {
				Log::PushText("Adding file: \"" + nFile->relative + "\"\r\n");
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
			instructionNum++;
			Log::PushText("Removing file: \"" + oFile->relative + "\"\r\n");
			writeInstructions(oFile->relative, oldHash, 0ull, nullptr, 0ull, 'D', vecBuffer);
		}
		delete[] oldBuffer;
		delete oFile;
	}
	removedFiles.clear();
	delete[] oldSnap;

	// Compress final buffer
	if (!BFT::CompressBuffer(vecBuffer.data(), vecBuffer.size(), diffBuffer, diffSize)) {
		Log::PushText("Critical failure: cannot compress diff file.\r\n");
		return false;
	}

	// Update optional parameters
	if (instructionCount != nullptr)
		*instructionCount = instructionNum;
	return true;
}

bool DRT::PatchDirectory(const std::string & dstDirectory, char * diffBufferCompressed, const size_t & diffSizeCompressed, size_t * bytesWritten, size_t * instructionsUsed)
{
	char * diffBuffer(nullptr);
	size_t diffSize(0ull);
	if (!BFT::DecompressBuffer(diffBufferCompressed, diffSizeCompressed, &diffBuffer, diffSize)) {
		Log::PushText("Critical failure: cannot decompress diff file.\r\n");
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
		ptr = BFT::PTR_ADD(ptr, size_t(sizeof(size_t)));
		FI.path.resize(pathLength);

		// Read file path
		std::memcpy(FI.path.data(), ptr, pathLength);
		ptr = BFT::PTR_ADD(ptr, pathLength);
		FI.fullPath = dstDirectory + FI.path;

		// Read operation flag
		char flag;
		std::memcpy(&flag, ptr, size_t(sizeof(char)));
		ptr = BFT::PTR_ADD(ptr, size_t(sizeof(char)));

		// Read old hash
		std::memcpy(reinterpret_cast<char*>(const_cast<size_t*>(&FI.diff_oldHash)), ptr, size_t(sizeof(size_t)));
		ptr = BFT::PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Read new hash
		std::memcpy(reinterpret_cast<char*>(const_cast<size_t*>(&FI.diff_newHash)), ptr, size_t(sizeof(size_t)));
		ptr = BFT::PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Read buffer size
		std::memcpy(reinterpret_cast<char*>(const_cast<size_t*>(&FI.instructionSize)), ptr, size_t(sizeof(size_t)));
		ptr = BFT::PTR_ADD(ptr, size_t(sizeof(size_t)));

		// Read buffer
		FI.instructionSet = new char[FI.instructionSize];
		std::memcpy(FI.instructionSet, ptr, FI.instructionSize);
		ptr = BFT::PTR_ADD(ptr, FI.instructionSize);

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
	size_t byteNum(0ull), instructionNum(0ull);
	for each (const auto & file in diffFiles) {
		// Try to read the target file
		char *oldBuffer(nullptr), *newBuffer(nullptr);
		size_t oldSize(0ull), oldHash(0ull);
	
		// Try to read source file
		if (!readFile(file.fullPath, oldSize, &oldBuffer, oldHash)) {
			Log::PushText("Critical failure: Cannot read source file from disk.\r\n");
			return false;
		}

		// Patch if this source file hasn't been patched yet
		if (oldHash == file.diff_newHash)
			Log::PushText("The file \"" + file.path + "\" is already up to date, skipping...\r\n");
		else if (oldHash != file.diff_oldHash) {
			Log::PushText("Critical failure: the file \"" + file.path + "\" is of an unexpected version. \r\n");
			return false;
		}
		else {
			// Patch buffer
			Log::PushText("patching file \"" + file.path + "\"\r\n");
			size_t newSize(0ull);
			if (!BFT::PatchBuffer(oldBuffer, oldSize, &newBuffer, newSize, file.instructionSet, file.instructionSize, &instructionNum)) {
				Log::PushText("Critical failure: patching failed!.\r\n");
				return false;
			}

			const size_t newHash = BFT::HashBuffer(newBuffer, newSize);

			// Confirm new hashes match
			if (newHash != file.diff_newHash) {
				Log::PushText("Critical failure: patched file is corrupted (hash mismatch).\r\n");
				return false;
			}

			// Write patched buffer to disk
			std::ofstream newFile(file.fullPath, std::ios::binary | std::ios::out);
			if (!newFile.is_open()) {
				Log::PushText("Critical failure: cannot write patched file to disk.\r\n");
				return false;
			}
			newFile.write(newBuffer, std::streamsize(newSize));
			newFile.close();
			byteNum += newSize;
		}

		// Cleanup and finish
		delete[] file.instructionSet;
		delete[] newBuffer;
		delete[] oldBuffer;
	}

	// By this point all files matched, safe to add new ones
	for each (const auto & file in addedFiles) {
		std::filesystem::create_directories(std::filesystem::path(file.fullPath).parent_path());
		Log::PushText("Writing file: \"" + file.path + "\"\r\n");
		
		// Write the 'insert' instructions
		// Remember that we use the diff/patch function to add new files too
		char * newBuffer(nullptr);
		size_t newSize(0ull);
		BFT::PatchBuffer(nullptr, 0ull, &newBuffer, newSize, file.instructionSet, file.instructionSize, &instructionNum);
		const size_t newHash = BFT::HashBuffer(newBuffer, newSize);

		// Confirm new hashes match
		if (newHash != file.diff_newHash) {
			Log::PushText("Critical failure: new file is corrupted (hash mismatch).\r\n");
			return false;
		}

		// Write new file to disk
		std::ofstream newFile(file.fullPath, std::ios::binary | std::ios::out);
		if (!newFile.is_open()) {
			Log::PushText("Critical failure: cannot write new file to disk.\r\n");
			return false;
		}
		newFile.write(newBuffer, std::streamsize(newSize));
		newFile.close();
		byteNum += newSize;

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
			Log::PushText("The file \"" + file.path + "\" has already been removed, skipping...\r\n");
		else {
			// Only remove source files if they match entirely
			if (oldHash == file.diff_oldHash)
				if (!std::filesystem::remove(file.fullPath))
					Log::PushText("Error: cannot delete file \"" + file.path + "\" from disk, delete this file manually if you can. \r\n");
				else
					Log::PushText("Removing file: \"" + file.path + "\"\r\n");
		}

		// Cleanup and finish
		delete[] file.instructionSet;
		delete[] oldBuffer;
	}

	// Update optional parameters
	if (bytesWritten != nullptr)
		*bytesWritten = byteNum;
	if (instructionsUsed != nullptr)
		*instructionsUsed = instructionNum;
	return true;
}

std::vector<std::filesystem::directory_entry> DRT::GetFilePaths(const std::string & directory)
{
	std::vector<std::filesystem::directory_entry> paths;
	if (std::filesystem::is_directory(directory))
		for (const auto & entry : std::filesystem::recursive_directory_iterator(directory))
			if (entry.is_regular_file())
				paths.emplace_back(entry);
	return paths;
}

std::string DRT::GetStartMenuPath()
{
	char cPath[FILENAME_MAX];
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, cPath)))
		return std::string(cPath);
	return std::string();
}

std::string DRT::GetDesktopPath()
{
	char cPath[FILENAME_MAX];
	if (SHGetSpecialFolderPathA(HWND_DESKTOP, cPath, CSIDL_DESKTOP, FALSE))
		return std::string(cPath);
	return std::string();
}

std::string DRT::GetRunningDirectory()
{
	char cCurrentPath[FILENAME_MAX];
	if (_getcwd(cCurrentPath, sizeof(cCurrentPath)))
		cCurrentPath[sizeof(cCurrentPath) - 1ull] = char('\0');
	return std::string(cCurrentPath);
}

std::string DRT::SanitizePath(const std::string & path)
{
	std::string cpy(path);
	while (cpy.front() == '"' || cpy.front() == '\'' || cpy.front() == '\"' || cpy.front() == '\\')
		cpy.erase(0ull, 1ull);
	while (cpy.back() == '"' || cpy.back() == '\'' || cpy.back() == '\"' || cpy.back() == '\\')
		cpy.erase(cpy.size() - 1ull);
	return cpy;
}
