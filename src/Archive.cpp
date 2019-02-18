#include "Archive.h"
#include "common.h"
#include "Threader.h"
#include "lz4.h"
#include <atomic>
#include <fstream>
#include "Windows.h"


Archive::~Archive()
{
	UnlockResource(m_hMemory);
	FreeResource(m_hMemory);

	if (m_decompressedPtr)
		delete[] m_decompressedPtr;
}

Archive::Archive(int resource_id, const std::string & resource_class) 
{
	m_hResource = FindResourceA(nullptr, MAKEINTRESOURCEA(resource_id), resource_class.c_str());
	m_hMemory = LoadResource(nullptr, m_hResource);
	m_ptr = LockResource(m_hMemory);
	m_size = SizeofResource(nullptr, m_hResource);
}

bool Archive::decompressArchive()
{
	m_decompressedSize = *reinterpret_cast<size_t*>(m_ptr);
	m_decompressedPtr = new char[m_decompressedSize];
	auto result = LZ4_decompress_safe(
		reinterpret_cast<char*>(reinterpret_cast<unsigned char*>(m_ptr) + size_t(sizeof(size_t))),
		reinterpret_cast<char*>(m_decompressedPtr),
		int(m_size - size_t(sizeof(size_t))),
		int(m_decompressedSize)
	);

	return (result > 0);
}

bool Archive::pack(size_t & fileCount, size_t & byteCount)
{
	// Variables
	Threader threader;
	const auto absolute_path_length = get_current_dir().size();
	const auto directoryArray = get_file_paths();
	struct FileData {
		std::string fullpath, trunc_path;
		size_t size, unitSize;
	};
	std::vector<FileData> files(directoryArray.size());

	// Calculate final file size
	auto start = std::chrono::system_clock::now();
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

	// Allocate enough room for the compressed buffer
	char * compressedBuffer = new char[archiveSize + size_t(sizeof(size_t))];
	// First chunk of data = the total uncompressed size
	*reinterpret_cast<size_t*>(compressedBuffer) = archiveSize;
	// Increment pointer so that the compression works on the remaining part of the buffer
	compressedBuffer = reinterpret_cast<char*>(compressedBuffer) + size_t(sizeof(size_t));
	// Compress the buffer
	auto result = LZ4_compress_default(
		filebuffer,
		compressedBuffer,
		int(archiveSize),
		int(archiveSize)
	);
	// Decrement pointer
	compressedBuffer = reinterpret_cast<char*>(compressedBuffer) - size_t(sizeof(size_t));

	// Update variables
	fileCount = filesRead;
	byteCount = result + size_t(sizeof(size_t));

	// Update the installer executable file
	bool returnResult = result > 0;
	if (returnResult) {
		auto handle = BeginUpdateResourceA("decompressor.exe", true);
		auto updateResult = UpdateResourceA(handle, "ARCHIVE", MAKEINTRESOURCEA(101), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), compressedBuffer, byteCount);
		EndUpdateResource(handle, FALSE);
	}

	// Clean up
	delete[] filebuffer;
	delete[] compressedBuffer;
	return returnResult;
}

bool Archive::unpack(size_t & fileCount, size_t & byteCount)
{
	Threader threader;
	if (!decompressArchive())
		return false;

	// Read the archive
	fileCount = 0;
	byteCount = 0;
	std::atomic_size_t filesFinished(0);
	void * readingPtr = m_decompressedPtr;
	while (byteCount < m_decompressedSize) {
		// Read the total number of characters from the path string, from the archive
		const auto pathSize = *reinterpret_cast<size_t*>(readingPtr);
		readingPtr = INCR_PTR(readingPtr, size_t(sizeof(size_t)));

		// Read the file path string, from the archive
		const char * path_array = reinterpret_cast<char*>(readingPtr);
		const auto path = get_current_dir() + std::string(path_array, pathSize);
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
