#include "Archiver.h"
#include "BufferTools.h"
#include "Common.h"
#include "Resource.h"
#include "Threader.h"
#include <atomic>
#include <fstream>
#include <vector>


bool Archiver::Pack(const std::string & directory, size_t & fileCount, size_t & byteCount)
{
	// Variables
	fileCount = 0ull;
	byteCount = 0ull;
	Threader threader;
	const auto absolute_path_length = directory.size();
	const auto directoryArray = get_file_paths(directory);
	struct FileData {
		std::string fullpath, trunc_path;
		size_t size, unitSize;
		std::filesystem::file_time_type writeTime;
	};
	std::vector<FileData> files;
	files.reserve(directoryArray.size());

	// Calculate final file size using all the files in this directory,
	// and make a list containing all relevant files and their attributes
	size_t archiveSize(0);
	for each (const auto & entry in directoryArray) {
		auto path = entry.path().string();
		// Blacklist
		if (
			(path.find("installer.exe") != std::string::npos) 
			|| 
			(path.find("installerMaker.exe") != std::string::npos)
			)
			continue;
		path = path.substr(absolute_path_length, path.size() - absolute_path_length);
		const auto pathSize = path.size();

		const size_t unitSize =
			size_t(sizeof(size_t)) +							// size of path size variable in bytes
			pathSize +											// the actual path data
			size_t(sizeof(std::filesystem::file_time_type)) +	// size of the file write time
			size_t(sizeof(size_t)) +							// size of the file size variable in bytes
			entry.file_size();									// the actual file data
		archiveSize += unitSize;
		files.push_back({ entry.path().string(), path, entry.file_size(), unitSize, entry.last_write_time() });
	}

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

			// Write the file write time into the archive
			auto writeTime = file.writeTime;
			memcpy(ptr, reinterpret_cast<char*>(&writeTime), size_t(sizeof(std::filesystem::file_time_type)));
			ptr = PTR_ADD(ptr, size_t(sizeof(std::filesystem::file_time_type)));

			// Write the file size in bytes, into the archive
			auto fileSize = file.size;
			memcpy(ptr, reinterpret_cast<char*>(&fileSize), size_t(sizeof(size_t)));
			ptr = PTR_ADD(ptr, size_t(sizeof(size_t)));

			// Write the file into the archive
			std::ifstream fileOnDisk(file.fullpath, std::ios::binary | std::ios::beg);
			fileOnDisk.read(reinterpret_cast<char*>(ptr), (std::streamsize)fileSize);
			fileOnDisk.close();
		});

		pointer = PTR_ADD(pointer, file.unitSize);
	}

	// Wait for threaded operations to complete
	while (!threader.isFinished())
		continue;
	threader.shutdown();

	// Compress the archive
	char * compressedBuffer(nullptr);
	size_t compressedSize(0ull);
	bool returnResult = false;
	if (BFT::CompressBuffer(filebuffer, archiveSize, &compressedBuffer, compressedSize)) {
		// Update variables
		fileCount = files.size();
		byteCount = compressedSize;

		// Write installer to disk
		Resource installer(IDR_INSTALLER, "INSTALLER");
		std::ofstream file(directory + "\\installer.exe", std::ios::binary | std::ios::out);
		if (file.is_open())
			file.write(reinterpret_cast<char*>(installer.getPtr()), (std::streamsize)installer.getSize());
		file.close();

		// Update installer's resource
		auto handle = BeginUpdateResource("installer.exe", true);
		returnResult = (bool)UpdateResource(handle, "ARCHIVE", MAKEINTRESOURCE(IDR_ARCHIVE), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), compressedBuffer, (DWORD)byteCount);
		EndUpdateResource(handle, FALSE);
	}

	// Clean up
	delete[] filebuffer;
	delete[] compressedBuffer;
	return returnResult;
}

bool Archiver::Unpack(const std::string & directory, size_t & fileCount, size_t & byteCount)
{
	// Decompress the archive
	fileCount = 0ull;
	byteCount = 0ull;
	Threader threader;
	Resource archive(IDR_ARCHIVE, "ARCHIVE");
	char * decompressedBuffer(nullptr);
	size_t decompressedSize(0ull);
	const bool result = BFT::DecompressBuffer(reinterpret_cast<char*>(archive.getPtr()), archive.getSize(), &decompressedBuffer, decompressedSize);
	if (!result)
		return false;

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
		const auto path = directory + std::string(path_array, pathSize);
		readingPtr = PTR_ADD(readingPtr, pathSize);

		// Read the file write time into the archive
		auto writeTime = *reinterpret_cast<std::filesystem::file_time_type*>(readingPtr);
		readingPtr = PTR_ADD(readingPtr, size_t(sizeof(std::filesystem::file_time_type)));

		// Read the file size in bytes, from the archive 
		const auto fileSize = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = PTR_ADD(readingPtr, size_t(sizeof(size_t)));

		// Write file out to disk, from the archive
		void * ptrCopy = readingPtr; // needed for lambda, since readingPtr gets incremented
		threader.addJob([ptrCopy, path, fileSize, writeTime, &filesWritten, &bytesWritten]() {
			// Write-out the file if it doesn't exist yet, if the size is different, or if it's older
			if (!std::filesystem::exists(path) || std::filesystem::file_size(path) != fileSize || std::filesystem::last_write_time(path) < writeTime) {
				std::filesystem::create_directories(std::filesystem::path(path).parent_path());
				std::ofstream file(path, std::ios::binary | std::ios::out);
				if (file.is_open())
					file.write(reinterpret_cast<char*>(ptrCopy), (std::streamsize)fileSize);
				file.close();
				bytesWritten += fileSize;
				filesWritten++;
			}
		});

		readingPtr = PTR_ADD(readingPtr, fileSize);
		bytesRead += size_t(sizeof(size_t)) + pathSize + size_t(sizeof(std::filesystem::file_time_type)) + size_t(sizeof(size_t)) + fileSize;
	}

	// Wait for threaded operations to complete
	while (!threader.isFinished())
		continue;
	threader.shutdown();

	// Success
	fileCount = filesWritten;
	byteCount = bytesWritten;
	return true;	
}