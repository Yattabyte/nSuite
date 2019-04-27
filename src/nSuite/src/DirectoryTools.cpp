#include "DirectoryTools.h"
#include "BufferTools.h"
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

#define PDIRECTORY_HEADER_TEXT "nSuite package"
#define DDIRECTORY_HEADER_TEXT "nSuite diff"


bool DRT::CompressDirectory(const std::string & srcDirectory, char ** packBuffer, size_t & packSize, size_t * byteCount, size_t * fileCount, const std::vector<std::string> & exclusions)
{
	// Variables
	const auto absolute_path_length = srcDirectory.size();
	const auto directoryArray = GetFilePaths(srcDirectory);
	struct FileData {
		std::string fullpath, trunc_path;
		size_t size, unitSize;
	};
	std::vector<FileData> files;
	files.reserve(directoryArray.size());

	// Ensure the source-directory has files
	if (!directoryArray.size()) {
		Log::PushText("Error: the source path specified \"" + srcDirectory + "\" is not a (useable) directory.\r\n");
		return false;
	}

	// Get a folder name, to name the package after
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

	// Ensure we have a non-zero sized archive
	if (archiveSize <= 0ull) {
		Log::PushText("Error: the chosen source directory has no useable files\r\n");
		return false;
	}

	// Update optional parameter
	if (byteCount != nullptr)
		*byteCount = archiveSize;

	// Create file buffer to contain all the file data
	char * filebuffer = new char[archiveSize];

	// Write file data into the buffer
	Threader threader;
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

	// Update optional parameters
	if (fileCount != nullptr)
		*fileCount = files.size();

	// Free up memory
	files.clear();
	files.shrink_to_fit();

	// Compress the archive
	char * compressedBuffer(nullptr);
	size_t compressedSize(0ull);
	const bool result = BFT::CompressBuffer(filebuffer, archiveSize, &compressedBuffer, compressedSize);
	delete[] filebuffer;
	if (!result) {
		Log::PushText("Error: cannot compress the file-buffer for the chosen source directory.\r\n");		
		return false;
	}

	// Expected header information
	constexpr const char HEADER_TITLE[] = PDIRECTORY_HEADER_TEXT;
	constexpr const size_t HEADER_TITLE_SIZE = (sizeof(PDIRECTORY_HEADER_TEXT) / sizeof(*PDIRECTORY_HEADER_TEXT));
	const size_t HEADER_SIZE = HEADER_TITLE_SIZE + folderName.size() + size_t(sizeof(size_t));
	packSize = HEADER_SIZE + size_t(compressedSize);
	*packBuffer = new char[packSize];
	char * HEADER_ADDRESS = *packBuffer;
	char * DATA_ADDRESS = HEADER_ADDRESS + HEADER_SIZE;

	// Write header information
	std::memcpy(HEADER_ADDRESS, &HEADER_TITLE[0], HEADER_TITLE_SIZE); // Header Title
	HEADER_ADDRESS = reinterpret_cast<char*>(BFT::PTR_ADD(HEADER_ADDRESS, HEADER_TITLE_SIZE));
	auto pathSize = folderName.size();
	std::memcpy(HEADER_ADDRESS, &pathSize, size_t(sizeof(size_t))); // Folder character num
	HEADER_ADDRESS = reinterpret_cast<char*>(BFT::PTR_ADD(HEADER_ADDRESS, size_t(sizeof(size_t))));	
	std::memcpy(HEADER_ADDRESS, folderName.data(), pathSize); // Folder name

	// Copy compressed data over
	std::memcpy(DATA_ADDRESS, compressedBuffer, compressedSize);	
	delete[] compressedBuffer;

	// Success
	return true;
}

bool DRT::DecompressDirectory(const std::string & dstDirectory, char * packBuffer, const size_t & packSize, size_t * byteCount, size_t * fileCount)
{
	// Ensure buffer at least *exists*
	if (packSize <= size_t(sizeof(size_t)) || packBuffer == nullptr) {
		Log::PushText("Error: package buffer doesn't exist or has no content.\r\n");
		return false;
	}

	// Parse the header
	std::string packageName("");
	char * packagedData(nullptr);
	size_t packagedDataSize(0ull);
	const bool result = ParsePackage(packBuffer, packSize, packageName, &packagedData, packagedDataSize);
	if (!result) {
		Log::PushText("Error: cannot begin directory decompression, as package header cannot be parsed.\r\n");
		return false;
	}	

	// Try to decompress the buffer
	const auto finalDestionation = SanitizePath(dstDirectory + "\\" + packageName);
	char * decompressedBuffer(nullptr);
	size_t decompressedSize(0ull);
	if (!BFT::DecompressBuffer(packagedData, packagedDataSize, &decompressedBuffer, decompressedSize)) {
		Log::PushText("Error: cannot complete directory decompression, as the package file cannot be decompressed.\r\n");
		return false;
	}

	// Begin reading the archive
	Threader threader;
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
				Log::PushText("Error writing file: \"" + fullPath + "\" to disk.\r\n");
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

bool DRT::ParsePackage(char * packageBuffer, const size_t & packageSize, std::string & packageName, char ** dataPointer, size_t & dataSize)
{
	// Ensure buffer at least *exists*
	if (packageSize <= size_t(sizeof(size_t)) || packageBuffer == nullptr) {
		Log::PushText("Error: package buffer doesn't exist or has no content.\r\n");
		return false;
	}

	// Expected header information
	constexpr const char HEADER_TITLE[] = PDIRECTORY_HEADER_TEXT;
	constexpr const size_t HEADER_TITLE_SIZE = (sizeof(PDIRECTORY_HEADER_TEXT) / sizeof(*PDIRECTORY_HEADER_TEXT));
	size_t HEADER_SIZE = HEADER_TITLE_SIZE;
	if (packageSize < HEADER_SIZE) {
		Log::PushText("Error: package buffer is malformed.\r\n");
		return false;
	}
	char headerTitle_In[HEADER_TITLE_SIZE];
	char * HEADER_ADDRESS = packageBuffer;
	char * DATA_ADDRESS = HEADER_ADDRESS;

	// Read in the header title of this buffer, ENSURE it matches
	std::memcpy(headerTitle_In, HEADER_ADDRESS, HEADER_TITLE_SIZE);
	if (std::strcmp(headerTitle_In, HEADER_TITLE) != 0) {
		Log::PushText("Error: the package buffer specified is not a valid nSuite package buffer.\r\n");
		return false;
	}

	// Get the folder size + name
	HEADER_ADDRESS = reinterpret_cast<char*>(BFT::PTR_ADD(HEADER_ADDRESS, HEADER_TITLE_SIZE));
	const auto folderSize = *reinterpret_cast<size_t*>(HEADER_ADDRESS);
	HEADER_SIZE += folderSize + size_t(sizeof(size_t));
	DATA_ADDRESS += HEADER_SIZE;
	HEADER_ADDRESS = reinterpret_cast<char*>(BFT::PTR_ADD(HEADER_ADDRESS, size_t(sizeof(size_t))));
	packageName = std::string(reinterpret_cast<char*>(HEADER_ADDRESS), folderSize);

	// Get the data portion
	*dataPointer = DATA_ADDRESS;
	dataSize = packageSize - HEADER_SIZE;
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
		// See if the input path is a directory
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
		bool canLoad = std::filesystem::path(directory).extension() == ".exe" ? LoadLibraryA(directory.c_str()) : false;
		auto handle = GetModuleHandle(directory.c_str());
		Resource fileResource(IDR_ARCHIVE, "ARCHIVE", handle);
		if (canLoad && handle != NULL && fileResource.exists()) {
			// Extract the archive
			packBuffer = reinterpret_cast<char*>(fileResource.getPtr());
			packSize = fileResource.getSize();
		}
		else {
			// Last resort: try to treat the file as an nSuite package
			std::ifstream packFile(directory, std::ios::binary | std::ios::beg);
			packSize = std::filesystem::file_size(directory);
			if (!packFile.is_open()) {
				Log::PushText("Error: cannot open the package file specified.\r\n");
				return false;
			}
			packBuffer = new char[packSize];
			packFile.read(packBuffer, std::streamsize(packSize));
			packFile.close();
		}

		// Parse the header
		std::string packageName("");
		char * packagedData(nullptr);
		size_t packagedDataSize(0ull);
		const bool result = ParsePackage(packBuffer, packSize, packageName, &packagedData, packagedDataSize);
		if (!result) {
			Log::PushText("Error: cannot begin directory decompression, as package header cannot be parsed.\r\n");
			return false;
		}

		// Try to decompress the buffer
		size_t snapSize(0ull);
		const bool decompressResult = BFT::DecompressBuffer(packagedData, packagedDataSize, snapshot, snapSize);

		// Check if we need to delete the packBuffer, or if it is an embedded resource
		// If it's an embedded resource, the fileResource's destructor will satisfy, else do below
		if (!canLoad || handle == NULL || !fileResource.exists())
			delete[] packBuffer;
		else if (canLoad && handle != NULL)
			FreeLibrary(handle);

		// Handle decompression failure now
		if (!decompressResult) {
			Log::PushText("Critical failure: cannot decompress package file.\r\n");
			return false;
		}

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

			// Save file path + size + pointer info
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
		if (!getFiles(oldDirectory, oldSnapshot, size1, srcOld_Files) || !getFiles(newDirectory, newSnapshot, size2, srcNew_Files)) {
			Log::PushText("Error: cannot retrieve both lists of files.\r\n");
			return false;
		}
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
	if (!getFileLists(oldDirectory, &oldSnap, newDirectory, &newSnap, reserveSize, commonFiles, addedFiles, removedFiles)) {
		Log::PushText("Error: could not retrieve all requisite file lists for diffing.\r\n");
		return false;
	}

	// Generate Instructions from file lists, store them in this expanding buffer
	std::vector<char> vecBuffer;
	vecBuffer.reserve(reserveSize);

	// These files are common, maybe some have changed
	size_t fileCount(0ull), instructionNum(0ull);
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
					fileCount++;
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
	commonFiles.shrink_to_fit();

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
				fileCount++;
			}
			delete[] buffer;
		}
		delete[] newBuffer;
		delete nFile;
	}
	addedFiles.clear();
	addedFiles.shrink_to_fit();
	delete[] newSnap;

	// These files are deprecated
	for each (const auto & oFile in removedFiles) {
		char * oldBuffer(nullptr);
		size_t oldHash(0ull);
		if (oFile->open(&oldBuffer, oldHash)) {
			instructionNum++;
			Log::PushText("Removing file: \"" + oFile->relative + "\"\r\n");
			writeInstructions(oFile->relative, oldHash, 0ull, nullptr, 0ull, 'D', vecBuffer);
			fileCount++;
		}
		delete[] oldBuffer;
		delete oFile;
	}
	removedFiles.clear();
	removedFiles.shrink_to_fit();
	delete[] oldSnap;

	// Ensure we have data in the instruction buffer
	if (vecBuffer.size() <= size_t(sizeof(size_t))) {
		Log::PushText("Error: diff instruction buffer is empty!\r\n");
		return false;
	}

	// Try to compress the instruction buffer
	char * compressedBuffer(nullptr);
	size_t compressedSize(0ull);
	const bool compressedResult = BFT::CompressBuffer(vecBuffer.data(), vecBuffer.size(), &compressedBuffer, compressedSize);
	vecBuffer.clear();
	vecBuffer.shrink_to_fit();
	if (!compressedResult) {
		Log::PushText("Critical failure: cannot compress diff file.\r\n");
		return false;
	}

	// Expected header information
	constexpr const char HEADER_TITLE[] = DDIRECTORY_HEADER_TEXT;
	constexpr const size_t HEADER_TITLE_SIZE = (sizeof(DDIRECTORY_HEADER_TEXT) / sizeof(*DDIRECTORY_HEADER_TEXT));
	constexpr const size_t HEADER_SIZE = HEADER_TITLE_SIZE + size_t(sizeof(size_t));
	diffSize = HEADER_SIZE + compressedSize;
	*diffBuffer = new char[diffSize];
	char * HEADER_ADDRESS = *diffBuffer;
	char * DATA_ADDRESS = HEADER_ADDRESS + HEADER_SIZE;

	// Write header information
	std::memcpy(HEADER_ADDRESS, &HEADER_TITLE[0], HEADER_TITLE_SIZE);
	HEADER_ADDRESS = reinterpret_cast<char*>(BFT::PTR_ADD(HEADER_ADDRESS, HEADER_TITLE_SIZE));
	std::memcpy(HEADER_ADDRESS, &fileCount, size_t(sizeof(size_t)));

	// Copy compressed data over
	std::memcpy(DATA_ADDRESS, compressedBuffer, compressedSize);
	delete[] compressedBuffer;

	// Update optional parameter
	if (instructionCount != nullptr)
		*instructionCount = instructionNum;

	// Success
	return true;
}

bool DRT::PatchDirectory(const std::string & dstDirectory, char * diffBuffer, const size_t & diffSize, size_t * bytesWritten, size_t * instructionsUsed)
{
	// Ensure buffer at least *exists*
	if (diffSize <= size_t(sizeof(size_t)) || diffBuffer == nullptr) {
		Log::PushText("Error: diff buffer doesn't exist or has no content.\r\n");
		return false;
	}

	// Expected header information
	constexpr const char HEADER_TITLE[] = DDIRECTORY_HEADER_TEXT;
	constexpr const size_t HEADER_TITLE_SIZE = (sizeof(DDIRECTORY_HEADER_TEXT) / sizeof(*DDIRECTORY_HEADER_TEXT));
	constexpr const size_t HEADER_SIZE = HEADER_TITLE_SIZE + size_t(sizeof(size_t));
	const size_t DATA_SIZE = diffSize - HEADER_SIZE;
	char headerTitle_In[HEADER_TITLE_SIZE];
	char * HEADER_ADDRESS = diffBuffer;
	char * DATA_ADDRESS = HEADER_ADDRESS + HEADER_SIZE;

	// Read in the header title of this buffer, ENSURE it matches
	std::memcpy(headerTitle_In, HEADER_ADDRESS, HEADER_TITLE_SIZE);
	if (std::strcmp(headerTitle_In, HEADER_TITLE) != 0) {
		Log::PushText("Error: the diff buffer specified is not a valid nSuite diff buffer.\r\n");
		return false;
	}

	// Get instruction count
	HEADER_ADDRESS = reinterpret_cast<char*>(BFT::PTR_ADD(HEADER_ADDRESS, HEADER_TITLE_SIZE));
	const auto fileCount = *reinterpret_cast<size_t*>(HEADER_ADDRESS);

	// Try to decompress the instruction buffer
	char * instructionBuffer(nullptr);
	size_t instructionBufferSize(0ull);
	if (!BFT::DecompressBuffer(DATA_ADDRESS, DATA_SIZE, &instructionBuffer, instructionBufferSize)) {
		Log::PushText("Error: cannot complete directory patching, as the instruction buffer cannot be decompressed.\r\n");
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
	void * ptr = instructionBuffer;
	size_t files(0ull);
	while (files < fileCount) {
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

		// Sort instructions
		if (flag == 'U')
			diffFiles.push_back(FI);
		else if (flag == 'N')
			addedFiles.push_back(FI);
		else if (flag == 'D')
			removedFiles.push_back(FI);
		files++;
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
				Log::PushText("Critical failure: patching failed!\r\n");
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
		if (!BFT::PatchBuffer(nullptr, 0ull, &newBuffer, newSize, file.instructionSet, file.instructionSize, &instructionNum)) {
			Log::PushText("Critical failure: cannot derive new file from patch instructions.\r\n");
			return false;
		}

		// Confirm new hashes match
		const size_t newHash = BFT::HashBuffer(newBuffer, newSize);
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
			if (oldHash == file.diff_oldHash) {
				if (!std::filesystem::remove(file.fullPath))
					Log::PushText("Error: cannot delete file \"" + file.path + "\" from disk, delete this file manually if you can.\r\n");
				else {
					Log::PushText("Removing file: \"" + file.path + "\"\r\n");
					instructionNum++;
				}
			}
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

	// Success
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
