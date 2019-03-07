#include "Archiver.h"
#include "Resource.h"
#include "Threader.h"
#include "lz4.h"
#include <atomic>
#include <fstream>
#include <filesystem>
#include <vector>


/** Return file-info for all files within the directory specified.
@param	directory	the directory to retrieve file-info from.
@return				a vector of file information, including file names, sizes, meta-data, etc. */
static std::vector<std::filesystem::directory_entry> get_file_paths(const std::string & directory)
{
	std::vector<std::filesystem::directory_entry> paths;
	for (const auto & entry : std::filesystem::recursive_directory_iterator(directory))
		if (entry.is_regular_file())
			paths.emplace_back(entry);
	return paths;
}

/** Increment a pointer's address by the offset provided.
@param	ptr			the pointer to increment by the offset amount.
@param	offset		the offset amount to apply to the pointer's address.
@return				the modified pointer address. */
static void * INCR_PTR(void *const ptr, const size_t & offset) 
{
	return (void*)(reinterpret_cast<unsigned char*>(ptr) + offset);
};

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
	std::atomic_size_t filesRead(0ull);
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

			// Write the file write time into the archive
			auto writeTime = file.writeTime;
			memcpy(ptr, reinterpret_cast<char*>(&writeTime), size_t(sizeof(std::filesystem::file_time_type)));
			ptr = INCR_PTR(ptr, size_t(sizeof(std::filesystem::file_time_type)));

			// Write the file size in bytes, into the archive
			auto fileSize = file.size;
			memcpy(ptr, reinterpret_cast<char*>(&fileSize), size_t(sizeof(size_t)));
			ptr = INCR_PTR(ptr, size_t(sizeof(size_t)));

			// Write the file into the archive
			std::ifstream fileOnDisk(file.fullpath, std::ios::binary | std::ios::beg);
			fileOnDisk.read(reinterpret_cast<char*>(ptr), (std::streamsize)fileSize);
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
	size_t compressedSize(0ull);
	bool returnResult = false;
	if (Archiver::CompressBuffer(filebuffer, archiveSize, &compressedBuffer, compressedSize)) {
		// Update variables
		fileCount = filesRead;
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
	const bool result = Archiver::DecompressBuffer(reinterpret_cast<char*>(archive.getPtr()), archive.getSize(), &decompressedBuffer, decompressedSize);
	if (!result)
		return false;

	// Read the archive
	size_t jobsStarted(0ull), bytesRead(0ull);
	std::atomic_size_t jobsFinished(0ull), filesWritten(0ull), bytesWritten(0ull);
	void * readingPtr = decompressedBuffer;
	while (bytesRead < decompressedSize) {
		// Read the total number of characters from the path string, from the archive
		const auto pathSize = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = INCR_PTR(readingPtr, size_t(sizeof(size_t)));

		// Read the file path string, from the archive
		const char * path_array = reinterpret_cast<char*>(readingPtr);
		const auto path = directory + std::string(path_array, pathSize);
		readingPtr = INCR_PTR(readingPtr, pathSize);

		// Read the file write time into the archive
		auto writeTime = *reinterpret_cast<std::filesystem::file_time_type*>(readingPtr);
		readingPtr = INCR_PTR(readingPtr, size_t(sizeof(std::filesystem::file_time_type)));

		// Read the file size in bytes, from the archive 
		const auto fileSize = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = INCR_PTR(readingPtr, size_t(sizeof(size_t)));

		// Write file out to disk, from the archive
		void * ptrCopy = readingPtr; // needed for lambda, since readingPtr gets incremented
		threader.addJob([ptrCopy, path, fileSize, writeTime, &filesWritten, &bytesWritten, &jobsFinished]() {
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
			jobsFinished++;
		});
		jobsStarted++;

		readingPtr = INCR_PTR(readingPtr, fileSize);
		bytesRead += size_t(sizeof(size_t)) + pathSize + size_t(sizeof(std::filesystem::file_time_type)) + size_t(sizeof(size_t)) + fileSize;
	}

	// Wait for threaded operations to complete
	while (jobsFinished != jobsStarted)
		continue;	

	// Success
	fileCount = filesWritten;
	byteCount = bytesWritten;
	return true;	
}

bool Archiver::CompressBuffer(char * sourceBuffer, const size_t & sourceSize, char ** destinationBuffer, size_t & destinationSize)
{
	// Allocate enough room for the compressed buffer
	*destinationBuffer = new char[sourceSize + sizeof(size_t)];

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
	destinationSize = size_t(result) + sizeof(size_t);
	return (result > 0);
}

bool Archiver::DecompressBuffer(char * sourceBuffer, const size_t & sourceSize, char ** destinationBuffer, size_t & destinationSize)
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