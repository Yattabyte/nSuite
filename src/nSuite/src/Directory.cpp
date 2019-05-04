#include "Directory.h"
#include "Log.h"
#include "Threader.h"
#include <fstream>

using namespace NST;


// Public (de)Constructors

NST::Directory::~Directory()
{
	unloadFromMemory();
}

Directory::Directory(const std::string & directoryPath, const std::vector<std::string> & exclusions)
	: m_directoryPath(directoryPath), m_exclusions(exclusions)
{
	parse();
}

void NST::Directory::changeDirectory(const std::string & directoryPath)
{
	m_directoryPath = directoryPath;
	parse();
}

size_t NST::Directory::file_count() const
{
	return m_files.size();
}

size_t NST::Directory::space_used() const
{
	size_t size(0ull);
	for each (const auto & file in m_files)
		size += file.unitSize;
	return size;
}


// Public Methods


// Public Derivation Methods

std::optional<Buffer> NST::Directory::package()
{
	// Ensure the source-directory has files
	if (!m_files.size())
		Log::PushText("Error: this directory with no (useable) files to package!\r\n");
	else {
		// Get a folder name, to name the package after
		auto srcPath = std::filesystem::path(m_directoryPath);
		auto folderName = srcPath.stem().string();
		while (folderName.empty()) {
			srcPath = srcPath.parent_path();
			folderName = srcPath.stem().string();
		};

		// Ensure we have a non-zero sized archive
		size_t archiveSize(space_used());
		if (archiveSize <= 0ull)
			Log::PushText("Error: the archive has no data in it!\r\n");
		else {
			// Create file buffer to contain all the file data
			loadIntoMemory();
			Buffer filebuffer(archiveSize);

			// Write file data into the buffer
			size_t byteIndex(0ull);
			for each (const auto & file in m_files) {
				// Write the total number of characters in the path string, into the archive
				const auto pathSize = file.trunc_path.size();
				byteIndex = filebuffer.writeData(&pathSize, size_t(sizeof(size_t)), byteIndex);

				// Write the file path string, into the archive
				byteIndex = filebuffer.writeData(file.trunc_path.data(), size_t(sizeof(char)) * pathSize, byteIndex);

				// Write the file size in bytes, into the archive
				byteIndex = filebuffer.writeData(&file.size, size_t(sizeof(size_t)), byteIndex);

				// Copy the file data
				if (file.size > 0ull && file.data != nullptr)
					std::copy(file.data, file.data + file.size, &filebuffer[byteIndex]);
				byteIndex += file.size;
			}
			unloadFromMemory();

			// Compress the archive
			auto compressedBuffer = filebuffer.compress();
			filebuffer.release();
			if (!compressedBuffer)
				Log::PushText("Error: cannot compress the file-buffer for the chosen source directory.\r\n");
			else {
				// Prepend header information
				PackageHeader header(folderName.size(), folderName.c_str());
				compressedBuffer->writeHeader(&header);

				// Success
				return compressedBuffer;
			}
		}
	}

	// Failure
	return {};
}

bool NST::Directory::unpackage(Directory & destinationDirectory)
{
	/*// Ensure buffer at least *exists*
	if (!packBuffer.hasData())
		Log::PushText("Error: package buffer has no content.\r\n");
	else {
		// Read in header
		Directory::PackageHeader header;
		std::byte * dataPtr(nullptr);
		size_t dataSize(0ull);
		packBuffer.readHeader(&header, &dataPtr, dataSize);

		// Ensure header title matches
		if (std::strcmp(header.m_title, Directory::PackageHeader::TITLE) != 0)
			Log::PushText("Critical failure: supplied an invalid nSuite package!\r\n");
		else {
			// Try to decompress the buffer
			auto decompressedBuffer = Buffer(dataPtr, dataSize).decompress();
			if (!decompressedBuffer)
				Log::PushText("Error: cannot complete directory decompression, as the package file cannot be decompressed.\r\n");
			else {
				// Begin reading the archive
				Threader threader;
				size_t byteIndex(0ull);
				std::atomic_size_t filesProcessed(0ull);
				Progress::SetRange(decompressedBuffer->size() + 100);
				const auto finalDestionation = SanitizePath(dstDirectory + "\\" + header.m_folderName);
				while (byteIndex < decompressedBuffer->size()) {
					// Read path char count, path string, and file size
					size_t pathSize(0ull);
					byteIndex = decompressedBuffer->readData(&pathSize, size_t(sizeof(size_t)), byteIndex);
					char * pathArray = new char[pathSize];
					byteIndex = decompressedBuffer->readData(pathArray, pathSize, byteIndex);
					const std::string path(pathArray, pathSize);
					delete[] pathArray;
					size_t fileSize(0ull);
					byteIndex = decompressedBuffer->readData(&fileSize, size_t(sizeof(size_t)), byteIndex);

					// Write file out to disk, from the archive
					const auto fullPath = finalDestionation + path;
					const char * data = &((decompressedBuffer->cArray())[byteIndex]);
					threader.addJob([fullPath, data, fileSize, &filesProcessed]() {
						// Write-out the file if it doesn't exist yet, if the size is different, or if it's older
						std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());
						std::ofstream file(fullPath, std::ios::binary | std::ios::out);
						if (!file.is_open())
							Log::PushText("Error writing file: \"" + fullPath + "\" to disk.\r\n");
						else
							file.write(data, (std::streamsize)fileSize);
						file.close();
						filesProcessed++;
					});

					byteIndex += fileSize;
					Log::PushText("Writing file: \"" + path + "\"\r\n");
					Progress::SetProgress(byteIndex);
				}

				// Wait for threaded operations to complete
				threader.prepareForShutdown();
				while (!threader.isFinished())
					continue;
				threader.shutdown();
				Progress::SetProgress(byteIndex + 100);

				// Update optional parameters
				if (byteCount != nullptr)
					*byteCount = byteIndex;
				if (fileCount != nullptr)
					*fileCount = filesProcessed;

				// Success
				return true;
			}
		}
	}

	// Failure*/
	return false;
}

void NST::Directory::parse()
{
	// Form a list of all files and their attributes
	unloadFromMemory();
	m_files.clear();
	for (const auto & entry : std::filesystem::recursive_directory_iterator(m_directoryPath)) {
		if (entry.is_regular_file()) {
			const auto extension = std::filesystem::path(entry).extension();
			auto path = entry.path().string();
			path = path.substr(m_directoryPath.size(), path.size() - m_directoryPath.size());
			bool useEntry(true);
			for each (const auto & excl in m_exclusions) {
				if (excl.empty())
					continue;
				// Compare Paths && Extensions
				if (path == excl || extension == excl) {
					useEntry = false;
					break;
				}
			}
			if (useEntry) {
				const auto pathSize = path.size();
				const size_t unitSize =
					size_t(sizeof(size_t)) +	// size of path size variable in bytes
					pathSize +					// the actual path data
					size_t(sizeof(size_t)) +	// size of the file size variable in bytes
					entry.file_size();			// the actual file data
				m_files.push_back({ entry.path().string(), path, entry.file_size(), unitSize, nullptr });
			}
		}
	}
}

void NST::Directory::loadIntoMemory()
{
	unloadFromMemory();
	for (auto & file : m_files) {
		// Read the file
		std::ifstream fileOnDisk(file.fullpath, std::ios::binary | std::ios::beg | std::ios::in);
		if (fileOnDisk.is_open()) {
			file.data = new std::byte[file.size];
			fileOnDisk.read(reinterpret_cast<char*>(file.data), (std::streamsize)file.size);
			fileOnDisk.close();
		}
	}
}

void NST::Directory::unloadFromMemory()
{
	for (auto & file : m_files) {
		if (file.data != nullptr) {
			delete[] file.data;
			file.data = nullptr;
		}
	}
}
