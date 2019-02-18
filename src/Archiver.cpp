#include "Archiver.h"
#include "Resource.h"
#include "Threader.h"
#include "lz4.h"
#include <atomic>
#include <fstream>
#include <filesystem>
#include <vector>


/** Return info for all files within this working directory. */
static std::vector<std::filesystem::directory_entry> get_file_paths(const std::string & directory)
{
	std::vector<std::filesystem::directory_entry> paths;
	for (const auto & entry : std::filesystem::recursive_directory_iterator(directory))
		if (entry.is_regular_file())
			paths.push_back(entry);
	return paths;
}

/** Increment a pointer's address by the offset provided. */
static void * INCR_PTR(void *const ptr, const size_t & offset) 
{
	void * pointer = (void*)(reinterpret_cast<unsigned char*>(ptr) + offset);
	return pointer;
};

bool Archiver::pack(const std::string & directory, size_t & fileCount, size_t & byteCount)
{
	// Variables
	Threader threader;
	const auto absolute_path_length = directory.size();
	const auto directoryArray = get_file_paths(directory);
	struct FileData {
		std::string fullpath, trunc_path;
		size_t size, unitSize;
	};
	std::vector<FileData> files(directoryArray.size());

	// Calculate final file size
	size_t archiveSize(0), px(0);
	for each (const auto & entry in directoryArray) {
		std::string path = entry.path().string();
		if ((path.find("compressor.exe") != std::string::npos) || (path.find("decompressor.exe") != std::string::npos))
			continue;
		path = path.substr(absolute_path_length, path.size() - absolute_path_length);
		size_t pathSize = path.size();

		const size_t unitSize =
			size_t(sizeof(size_t)) +	// size of path size variable in bytes
			pathSize +					// the actual path data
			size_t(sizeof(size_t)) +	// size of the file size variable in bytes
			entry.file_size();			// the actual file data
		archiveSize += unitSize;
		files[px++] = { entry.path().string(), path, entry.file_size(), unitSize };
	}

	// Create buffer for final file data
	char * filebuffer = new char[archiveSize];

	// Modify buffer
	void * pointer = filebuffer;
	std::atomic_size_t filesRead(0);
	for each (const auto & file in files) {
		threader.addJob([file, pointer, &filesRead]() {
			// Write the total number of characters in the path string, into the archive
			void * ptr = pointer;
			auto pathSize = file.trunc_path.size();
			memcpy(ptr, reinterpret_cast<char*>(&pathSize), size_t(sizeof(size_t)));
			ptr = INCR_PTR(ptr, size_t(sizeof(size_t)));

			// Write the file path string, into the archive
			memcpy(ptr, file.trunc_path.data(), pathSize);
			ptr = INCR_PTR(ptr, pathSize);

			// Write the file size in bytes, into the archive
			auto fileSize = file.size;
			memcpy(ptr, reinterpret_cast<char*>(&fileSize), size_t(sizeof(size_t)));
			ptr = INCR_PTR(ptr, size_t(sizeof(size_t)));

			// Write the file into the archive
			std::ifstream fileOnDisk(file.fullpath, std::ios::binary | std::ios::beg);
			fileOnDisk.read(reinterpret_cast<char*>(ptr), fileSize);
			fileOnDisk.close();
			filesRead++;
		});

		pointer = INCR_PTR(pointer, file.unitSize);
	}

	// Wait for threaded operations to complete
	while (filesRead != files.size())
		continue;

	// Compress the archive
	char * compressedBuffer(nullptr);
	size_t compressedSize(0);
	const bool result = Archiver::compressArchive(filebuffer, archiveSize, &compressedBuffer, compressedSize);	

	// Update variables
	fileCount = filesRead;
	byteCount = compressedSize + size_t(sizeof(size_t));

	if (result) {
		// Write installer to disk
		Resource installer(IDR_INSTALLER, "INSTALLER");
		std::ofstream file(directory + "\\decompressor.exe", std::ios::binary | std::ios::out);
		if (file.is_open())
			file.write(reinterpret_cast<char*>(installer.getPtr()), installer.getSize());
		file.close();

		// Update installer's resource
		auto handle = BeginUpdateResource("decompressor.exe", true);
		auto updateResult = UpdateResource(handle, "ARCHIVE", MAKEINTRESOURCE(IDR_ARCHIVE), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), compressedBuffer, (DWORD)byteCount);
		EndUpdateResource(handle, FALSE);
	}

	// Clean up
	delete[] filebuffer;
	delete[] compressedBuffer;
	return result;
}

bool Archiver::unpack(const std::string & directory, size_t & fileCount, size_t & byteCount)
{
	// Decompress the archive
	Threader threader;
	Resource archive(IDR_ARCHIVE, "ARCHIVE");
	char * decompressedBuffer(nullptr);
	size_t decompressedSize(0);
	const bool result = Archiver::decompressArchive(reinterpret_cast<char*>(archive.getPtr()), archive.getSize(), &decompressedBuffer, decompressedSize);
	if (!result)
		return false;

	// Read the archive
	fileCount = 0;
	byteCount = 0;
	std::atomic_size_t filesFinished(0);
	void * readingPtr = decompressedBuffer;
	while (byteCount < decompressedSize) {
		// Read the total number of characters from the path string, from the archive
		const auto pathSize = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = INCR_PTR(readingPtr, size_t(sizeof(size_t)));

		// Read the file path string, from the archive
		const char * path_array = reinterpret_cast<char*>(readingPtr);
		const auto path = directory + std::string(path_array, pathSize);
		readingPtr = INCR_PTR(readingPtr, pathSize);

		// Read the file size in bytes, from the archive 
		const auto fileSize = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = INCR_PTR(readingPtr, size_t(sizeof(size_t)));

		// Write file out to disk, from the archive
		void * ptrCopy = readingPtr; // needed for lambda, since readingPtr gets incremented
		threader.addJob([ptrCopy, path, fileSize, &filesFinished]() {
			std::filesystem::create_directories(std::filesystem::path(path).parent_path());
			std::ofstream file(path, std::ios::binary | std::ios::out);
			if (file.is_open())
				file.write(reinterpret_cast<char*>(ptrCopy), fileSize);
			file.close();
			filesFinished++;
		});

		readingPtr = INCR_PTR(readingPtr, fileSize);
		byteCount += size_t(sizeof(size_t)) + pathSize + size_t(sizeof(size_t)) + fileSize;
		fileCount++;
	}

	// Wait for threaded operations to complete
	while (fileCount != filesFinished)
		continue;	

	// Success
	return true;	
}

bool Archiver::compressArchive(char * sourceBuffer, const size_t & sourceSize, char ** destinationBuffer, size_t & destinationSize)
{
	// Allocate enough room for the compressed buffer
	*destinationBuffer = new char[sourceSize + size_t(sizeof(size_t))];

	// First chunk of data = the total uncompressed size
	*reinterpret_cast<size_t*>(*destinationBuffer) = sourceSize;

	// Increment pointer so that the compression works on the remaining part of the buffer
	*destinationBuffer = reinterpret_cast<char*>(*destinationBuffer) + size_t(sizeof(size_t));

	// Compress the buffer
	auto result = LZ4_compress_default(
		sourceBuffer,
		*destinationBuffer,
		int(sourceSize),
		int(sourceSize)
	);

	// Decrement pointer
	*destinationBuffer = reinterpret_cast<char*>(*destinationBuffer) - size_t(sizeof(size_t));
	destinationSize = size_t(result);
	return (result > 0);
}

bool Archiver::decompressArchive(char * sourceBuffer, const size_t & sourceSize, char ** destinationBuffer, size_t & destinationSize)
{
	destinationSize = *reinterpret_cast<size_t*>(sourceBuffer);
	*destinationBuffer = new char[destinationSize];
	auto result = LZ4_decompress_safe(
		reinterpret_cast<char*>(reinterpret_cast<unsigned char*>(sourceBuffer) + size_t(sizeof(size_t))),
		reinterpret_cast<char*>(*destinationBuffer),
		int(sourceSize - size_t(sizeof(size_t))),
		int(destinationSize)
	);

	return (result > 0);
}