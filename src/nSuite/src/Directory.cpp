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
				const auto pathSize = file.relativePath.size();
				byteIndex = filebuffer.writeData(&pathSize, size_t(sizeof(size_t)), byteIndex);

				// Write the file path string, into the archive
				byteIndex = filebuffer.writeData(file.relativePath.data(), size_t(sizeof(char)) * pathSize, byteIndex);

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
			const auto fullPath = finalDestionation + file.relativePath;
			std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());
			std::ofstream fileWriter(fullPath, std::ios::binary | std::ios::out);
			if (!fileWriter.is_open())
				Log::PushText("Error writing file: \"" + file.relativePath + "\" to disk.\r\n");
			else {
				Log::PushText("Writing file: \"" + file.relativePath + "\"\r\n");
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

std::optional<Buffer> NST::Directory::diff(const Directory & newDirectory)
{
	// Declarations that will only be used here	
	typedef std::vector<DirFile> PathList;
	typedef std::vector<std::pair<DirFile, DirFile>> PathPairList;
	static constexpr auto getFileLists = [](const Directory & oldDirectory, const Directory & newDirectory, PathPairList & commonFiles, PathList & addFiles, PathList & delFiles) {
		// Find all common and new files first
		auto srcOld_Files = oldDirectory.m_files;
		auto srcNew_Files = newDirectory.m_files;
		for each (const auto & nFile in srcNew_Files) {
			bool found = false;
			size_t oIndex(0ull);
			for each (const auto & oFile in srcOld_Files) {
				if (nFile.relativePath == oFile.relativePath) {
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
	static constexpr auto writeInstructions = [](const std::string & path, const size_t & oldHash, const size_t & newHash, const Buffer & buffer, const char & flag, Buffer & instructionBuffer) {
		const auto bufferSize = buffer.size();
		auto pathLength = path.length();
		const size_t instructionSize = (sizeof(size_t) * 4) + (size_t(sizeof(char)) * pathLength) + size_t(sizeof(char)) + bufferSize;
		const size_t bufferOldSize = instructionBuffer.size();
		instructionBuffer.resize(bufferOldSize + instructionSize);

		// Write file path length
		size_t byteIndex = instructionBuffer.writeData(&pathLength, size_t(sizeof(size_t)), bufferOldSize);

		// Write file path
		byteIndex = instructionBuffer.writeData(path.data(), size_t(sizeof(char)) * pathLength, byteIndex);

		// Write operation flag
		byteIndex = instructionBuffer.writeData(&flag, size_t(sizeof(char)), byteIndex);

		// Write old hash
		byteIndex = instructionBuffer.writeData(&oldHash, size_t(sizeof(size_t)), byteIndex);

		// Write new hash
		byteIndex = instructionBuffer.writeData(&newHash, size_t(sizeof(size_t)), byteIndex);

		// Write buffer size
		byteIndex = instructionBuffer.writeData(&bufferSize, size_t(sizeof(size_t)), byteIndex);

		// Write buffer
		byteIndex = instructionBuffer.writeData(buffer.data(), (size_t(sizeof(std::byte)) * bufferSize), byteIndex);
	};
	if (file_count() <= 0 && newDirectory.file_count() <= 0)
		Log::PushText("Error: input directories are empty!\r\n");
	else {
		// Retrieve all common, added, and removed files
		PathPairList commonFiles;
		PathList addedFiles, removedFiles;
		getFileLists(*this, newDirectory, commonFiles, addedFiles, removedFiles);
		// Generate Instructions from file lists, store them in this expanding buffer
		Buffer instructionBuffer;

		// These files are common, maybe some have changed
		size_t fileCount(0ull);
		for each (const auto & cFiles in commonFiles) {
			Buffer oldBuffer(cFiles.first.data, cFiles.first.size), newBuffer(cFiles.second.data, cFiles.second.size);
			size_t oldHash(oldBuffer.hash()), newHash(newBuffer.hash());
			if (oldHash != newHash) {
				// Files are different versions
				auto diffBuffer = oldBuffer.diff(newBuffer);
				if (diffBuffer) {
					Log::PushText("Diffing file: \"" + cFiles.first.relativePath + "\"\r\n");
					writeInstructions(cFiles.first.relativePath, oldHash, newHash, *diffBuffer, 'U', instructionBuffer);
					fileCount++;
				}
			}
		}
		commonFiles.clear();
		commonFiles.shrink_to_fit();

		// These files are brand new
		for each (const auto & nFile in addedFiles) {
			Buffer newBuffer(nFile.data, nFile.size);
			size_t newHash(newBuffer.hash());
			auto diffBuffer = Buffer().diff(newBuffer);
			if (diffBuffer) {
				Log::PushText("Adding file: \"" + nFile.relativePath + "\"\r\n");
				writeInstructions(nFile.relativePath, 0ull, newHash, *diffBuffer, 'N', instructionBuffer);
				fileCount++;
			}
		}
		addedFiles.clear();
		addedFiles.shrink_to_fit();

		// These files are deprecated
		for each (const auto & oFile in removedFiles) {
			Buffer oldBuffer(oFile.data, oFile.size);
			size_t oldHash(oldBuffer.hash());
			Log::PushText("Removing file: \"" + oFile.relativePath + "\"\r\n");
			writeInstructions(oFile.relativePath, oldHash, 0ull, Buffer(), 'D', instructionBuffer);
			fileCount++;			
		}
		removedFiles.clear();
		removedFiles.shrink_to_fit();

		// Try to compress the instruction buffer
		auto compressedBuffer = instructionBuffer.compress();
		instructionBuffer.release();
		if (!compressedBuffer)
			Log::PushText("Critical failure: cannot compress diff instructions!\r\n");
		else {
			// Prepend header information
			Directory::PatchHeader header(fileCount);
			compressedBuffer->writeHeader(&header);

			// Success
			return compressedBuffer;
		}
	}

	// Failure
	return {};
}


// Private Methods
/***/

std::vector<std::filesystem::directory_entry> NST::Directory::GetFilePaths(const std::string & directory, const std::vector<std::string> & exclusions)
{
	std::vector<std::filesystem::directory_entry> paths;
	if (std::filesystem::is_directory(directory))
		for (const auto & entry : std::filesystem::recursive_directory_iterator(directory))
			if (entry.is_regular_file()) {
				const auto extension = std::filesystem::path(entry).extension();
				auto path = entry.path().string();
				path = path.substr(directory.size(), path.size() - directory.size());
				bool useEntry(true);
				for each (const auto & excl in exclusions) {
					if (excl.empty())
						continue;
					// Compare Paths && Extensions
					if (path == excl || extension == excl) {
						useEntry = false;
						break;
					}
				}
				if (useEntry)
					paths.emplace_back(entry);
			}
	return paths;
}

void NST::Directory::virtualize_from_path(const std::string & input_path)
{
	if (std::filesystem::is_directory(input_path)) {
		m_directoryName = std::filesystem::path(m_directoryPath).stem().string();
		for (const auto & entry : GetFilePaths(input_path, m_exclusions)) {
			if (entry.is_regular_file()) {
				const auto extension = std::filesystem::path(entry).extension();
				auto path = entry.path().string();
				path = path.substr(input_path.size(), path.size() - input_path.size());
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
