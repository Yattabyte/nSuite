#include "Directory.h"
#include "Log.h"
#include "Progress.h"
#include "Resource.h"
#include "Threader.h"
#include <fstream>

using namespace NST;

static std::string SanitizePath(const std::string & path)
{
	std::string cpy(path);
	while (cpy.front() == '"' || cpy.front() == '\'' || cpy.front() == '\"' || cpy.front() == '\\')
		cpy.erase(0ull, 1ull);
	while (cpy.back() == '"' || cpy.back() == '\'' || cpy.back() == '\"' || cpy.back() == '\\')
		cpy.erase(cpy.size() - 1ull);
	return cpy;
}

// Public (de)Constructors

NST::Directory::~Directory()
{
	release();
}

Directory::Directory(const std::string & path, const std::vector<std::string> & exclusions)
	: m_directoryPath(path), m_exclusions(exclusions)
{	
	virtualize_from_path(path);
}

NST::Directory::Directory(const Buffer & package)
{	
	virtualize_from_buffer(package);
}

size_t NST::Directory::file_count() const
{
	return m_files.size();
}

size_t NST::Directory::space_used() const
{
	return m_spaceUsed;
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
			Buffer filebuffer(archiveSize);

			// Write file data into the buffer
			size_t byteIndex(0ull);
			for (auto & file : m_files) {
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
				delete[] file.data;
				file.data = nullptr;
				byteIndex += file.size;
			}

			// Compress the archive
			auto compressedBuffer = filebuffer.compress();
			filebuffer.release();
			if (!compressedBuffer)
				Log::PushText("Error: cannot compress the file-buffer for the chosen source directory!\r\n");
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

bool NST::Directory::unpackage(const std::string & outputPath)
{
	// Ensure the source-directory has files
	if (!m_files.size())
		Log::PushText("Error: this virtual-directory has no (useable) files to unpackage!\r\n");
	else {
		const auto finalDestionation = SanitizePath(outputPath + "\\" + m_directoryName);
		Progress::SetRange(m_files.size());
		size_t fileCount(0ull);
		for (auto & file : m_files) {
			// Write-out the file
			const auto fullPath = finalDestionation + file.trunc_path;
			std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());
			std::ofstream fileWriter(fullPath, std::ios::binary | std::ios::out);
			if (!fileWriter.is_open())
				Log::PushText("Error writing file: \"" + file.trunc_path + "\" to disk.\r\n");
			else {
				Log::PushText("Writing file: \"" + file.trunc_path + "\"\r\n");
				fileWriter.write(reinterpret_cast<char*>(file.data), (std::streamsize)file.size);
			}
			Progress::SetProgress(++fileCount);
			fileWriter.close();
		}		

		// Success
		return true;
	}

	// Failure
	return false;
}

void NST::Directory::virtualize_from_path(const std::string & input_path)
{
	if (std::filesystem::is_directory(input_path)) {
		m_directoryName = std::filesystem::path(m_directoryPath).stem().string();
		for (const auto & entry : std::filesystem::recursive_directory_iterator(input_path)) {
			if (entry.is_regular_file()) {
				const auto extension = std::filesystem::path(entry).extension();
				auto path = entry.path().string();
				path = path.substr(input_path.size(), path.size() - input_path.size());
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
					m_spaceUsed += unitSize;

					// Read the file data
					DirFile file{ path, entry.file_size(), nullptr };
					std::ifstream fileOnDisk(input_path + path, std::ios::binary | std::ios::beg | std::ios::in);
					if (fileOnDisk.is_open()) {
						file.data = new std::byte[file.size];
						fileOnDisk.read(reinterpret_cast<char*>(file.data), (std::streamsize)file.size);
						fileOnDisk.close();
					}
					m_files.push_back(file);
				}
			}
		}
	}
	else {
		// See if the file is actually a program with an embedded archive
		NST::Buffer packBuffer;
		bool success = std::filesystem::path(input_path).extension() == ".exe" ? LoadLibraryA(input_path.c_str()) : false;
		auto handle = GetModuleHandle(input_path.c_str());
		Resource fileResource(IDR_ARCHIVE, "ARCHIVE", handle);
		if (success && handle != NULL && fileResource.exists()) {
			packBuffer = NST::Buffer(reinterpret_cast<std::byte*>(fileResource.getPtr()), fileResource.getSize());
			FreeLibrary(handle);
		}
		else {
			// Last resort: try to treat the file as an nSuite package
			std::ifstream packFile(input_path, std::ios::binary | std::ios::beg);
			if (!packFile.is_open())
				Log::PushText("Error: cannot open the package file specified!\r\n");
			else {
				packBuffer = NST::Buffer(std::filesystem::file_size(input_path));
				packFile.read(packBuffer.cArray(), std::streamsize(packBuffer.size()));
				packFile.close();
				success = true;
			}
		}
		if (success)
			virtualize_from_buffer(packBuffer);
	}
}

void NST::Directory::virtualize_from_buffer(const Buffer & buffer)
{
	// Read in header		
	Directory::PackageHeader header;
	std::byte * dataPtr(nullptr);
	size_t dataSize(0ull);
	buffer.readHeader(&header, &dataPtr, dataSize);
	m_directoryName = header.m_folderName;

	// Ensure header title matches
	if (std::strcmp(header.m_title, Directory::PackageHeader::TITLE) != 0)
		NST::Log::PushText("Critical failure: cannot parse package header!\r\n");
	else {
		// Try to decompress the buffer
		auto decompressedBuffer = Buffer(dataPtr, dataSize).decompress();
		if (!decompressedBuffer)
			Log::PushText("Critical failure: cannot decompress package file!\r\n");
		else {
			// Get lists of all files involved
			size_t byteIndex(0ull);
			Progress::SetRange(decompressedBuffer->size() + 100);
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

				const size_t unitSize =
					size_t(sizeof(size_t)) +	// size of path size variable in bytes
					pathSize +					// the actual path data
					size_t(sizeof(size_t)) +	// size of the file size variable in bytes
					fileSize;					// the actual file data
				m_spaceUsed += unitSize;

				// Save the file data
				DirFile file{ path, fileSize, nullptr };
				file.data = new std::byte[file.size];
				std::copy(&((*decompressedBuffer)[byteIndex]), &((*decompressedBuffer)[byteIndex + fileSize]), file.data);
				m_files.push_back(file);
				byteIndex += fileSize;
			}
		}
	}
}

void NST::Directory::release()
{
	for (auto & file : m_files) {
		if (file.data != nullptr) {
			delete[] file.data;
			file.data = nullptr;
		}
	}
}
