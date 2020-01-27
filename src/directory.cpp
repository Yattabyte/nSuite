#include "Directory.hpp"
#include "BufferView.hpp"
#include <algorithm>
#include <fstream>
#include <numeric>


// Convenience definitions
using yatta::Buffer;
using yatta::BufferView;
using yatta::Directory;
using filepath = std::filesystem::path;
using directory_itt = std::filesystem::directory_iterator;
using directory_rec_itt = std::filesystem::recursive_directory_iterator;


// Public (de)Constructors

Directory::Directory(const filepath& path, const std::vector<std::string>& exclusions)
{
    if (std::filesystem::is_directory(path))
        in_folder(path, exclusions);
}

Directory::Directory(const Buffer& packageBuffer)
{
    if (packageBuffer.hasData())
        in_package(packageBuffer);
}


// Public Inquiry Methods

bool Directory::empty() const noexcept
{
    return m_files.empty();
}

bool Directory::hasFiles() const noexcept
{
    return !m_files.empty();
}

size_t Directory::fileCount() const noexcept
{
    return m_files.size();
}

size_t Directory::fileSize() const noexcept
{
    return std::accumulate(
        m_files.begin(),
        m_files.end(),
        0ULL,
        [](const size_t& currentSum, const VirtualFile& file) noexcept {
            return currentSum + file.m_data.size();
        }
    );
}

size_t Directory::hash() const noexcept
{
    // May overflow, but that's okay as long as the accumulation order is the same
    // such that 2 copies of the same directory result in the same hash
    return std::accumulate(
        m_files.begin(),
        m_files.end(),
        yatta::ZeroHash,
        [](const size_t& currentHash, const VirtualFile& file) noexcept {
            return currentHash + file.m_data.hash();
        }
    );
}

#ifdef __GNUC__
#include <unistd.h>
#else
#include <direct.h>
#define getcwd _getcwd
#endif // __GNUC__

std::string Directory::GetRunningDirectory() noexcept
{
    char cCurrentPath[FILENAME_MAX];
    if (getcwd(cCurrentPath, sizeof(cCurrentPath)) != nullptr)
        cCurrentPath[sizeof(cCurrentPath) - 1ULL] = static_cast<char>('\0');
    return std::string(cCurrentPath);
}


// Public Manipulation Methods

void Directory::clear() noexcept
{
    m_files.clear();
}

void Directory::in_folder(const filepath& path, const std::vector<std::string>& exclusions)
{
    constexpr auto get_file_paths = [](const filepath& directory, const std::vector<std::string>& exc) {
        constexpr auto check_exclusion = [](const filepath& p, const std::vector<std::string>& e) {
            const auto extension = p.extension();
            for (const auto& excl : e) {
                if (excl.empty())
                    continue;
                // Compare Paths && Extensions
                if (p == excl || extension == excl) {
                    // Don't use path
                    return false;
                }
            }
            // Safe to use path
            return true;
        };
        std::vector<std::filesystem::directory_entry> paths;
        if (std::filesystem::is_directory(directory))
            for (const auto& entry : directory_rec_itt(directory))
                if (entry.is_regular_file()) {
                    auto subpath = entry.path().string();
                    subpath = subpath.substr(
                        directory.string().size(), subpath.size() - directory.string().size()
                    );
                    if (check_exclusion(subpath, exc))
                        paths.emplace_back(entry);
                }
        return paths;
    };

    for (const auto& entry : get_file_paths(path, exclusions)) {
        if (entry.is_regular_file()) {
            // Read the file data
            Buffer fileBuffer(entry.file_size());
            const std::string path_string = entry.path().string();
            constexpr std::ios_base::openmode mode = std::ios_base::in | std::ios_base::binary;
            std::ifstream fileOnDisk = std::ifstream(path_string, mode);
            if (!fileOnDisk.is_open())
                throw std::runtime_error("Cannot read the file" + entry.path().string());

            fileOnDisk.read(
                fileBuffer.charArray(),
                static_cast<std::streamsize>(fileBuffer.size())
            );
            fileOnDisk.close();

            m_files.emplace_back(
                VirtualFile{
                    (std::filesystem::relative(entry.path(), path)).string(),
                    std::move(fileBuffer)
                }
            );
        }
    }
}

void Directory::in_package(const Buffer& packageBuffer)
{
    // Ensure the package buffer exists
    if (packageBuffer.empty())
        throw std::runtime_error("Invalid arguments, the package buffer is empty");

    // Read in header
    char packHeaderTitle[16ULL] = { '\0' };
    std::string packHeaderName;
    size_t byteIndex = sizeof(packHeaderTitle);
    packageBuffer.out_type(packHeaderTitle);
    packageBuffer.out_type(packHeaderName, byteIndex);
    byteIndex += sizeof(size_t) + (sizeof(char) * packHeaderName.size());

    // Ensure header title matches
    if (std::strcmp(packHeaderTitle, "yatta pack") != 0)
        throw std::runtime_error("Header mismatch");

    // Try to decompress the archive buffer
    const auto filebuffer = BufferView{ packageBuffer.size() - byteIndex, &packageBuffer.bytes()[byteIndex] }.decompress();
    byteIndex = 0ULL; // reset byte index

    // Find the file count
    size_t fileCount(0ULL);
    filebuffer.out_type(fileCount, byteIndex);
    byteIndex += sizeof(size_t);

    // Iterate over all files, writing out all their data
    const auto packSize = filebuffer.size();
    size_t fileIndex(m_files.size());
    m_files.resize(m_files.size() + fileCount);
    const auto finalSize = m_files.size();
    while (byteIndex < packSize && fileIndex < finalSize) {
        auto& file = m_files[fileIndex];
        // Read the path string out of the archive
        filebuffer.out_type(file.m_relativePath, byteIndex);
        byteIndex += sizeof(size_t) + (sizeof(char) * file.m_relativePath.size());

        // Write the file size in bytes, into the archive
        size_t bufferSize(0ULL);
        filebuffer.out_type(bufferSize, byteIndex);
        byteIndex += sizeof(size_t);

        // Copy the file data
        file.m_data.resize(bufferSize);
        filebuffer.out_raw(file.m_data.bytes(), file.m_data.size(), byteIndex);
        byteIndex += static_cast<size_t>(sizeof(std::byte))* file.m_data.size();
        ++fileIndex;
    }
}

bool Directory::in_delta(const Buffer& deltaBuffer)
{
    // Ensure buffer at least *exists*
    if (deltaBuffer.empty())
        return false; // Failure

    // Read in header
    char deltaHeaderTitle[16ULL] = { '\0' };
    size_t deltaHeaderFileCount(0ULL);
    size_t byteIndex = sizeof(deltaHeaderTitle);
    deltaBuffer.out_type(deltaHeaderTitle);
    deltaBuffer.out_type(deltaHeaderFileCount, byteIndex);
    byteIndex += sizeof(size_t);

    // Ensure header title matches
    if (std::strcmp(deltaHeaderTitle, "yatta delta") != 0)
        return false; // Failure

    // Try to decompress the instruction buffer
    auto instructionBuffer = BufferView{ deltaBuffer.size() - byteIndex, &deltaBuffer.bytes()[byteIndex] }.decompress();
    byteIndex = 0ULL; // reset byte index

    // Start reading diff file
    struct FileInstruction {
        std::string path = "", fullPath = "";
        Buffer instructionBuffer;
        size_t diff_oldHash = 0ULL, diff_newHash = 0ULL;
    };
    std::vector<FileInstruction> diffFiles;
    std::vector<FileInstruction> addedFiles;
    std::vector<FileInstruction> removedFiles;
    size_t files(0ULL);
    while (files < deltaHeaderFileCount) {
        FileInstruction FI;

        // Read file path
        instructionBuffer.out_type(FI.path, byteIndex);
        byteIndex += sizeof(size_t) + (sizeof(char) * FI.path.length());

        // Read operation flag
        char flag(0);
        instructionBuffer.out_type(flag, byteIndex);
        byteIndex += sizeof(char);

        // Read old hash
        instructionBuffer.out_type(FI.diff_oldHash, byteIndex);
        byteIndex += sizeof(size_t);

        // Read new hash
        instructionBuffer.out_type(FI.diff_newHash, byteIndex);
        byteIndex += sizeof(size_t);

        // Read buffer size
        size_t instructionSize(0ULL);
        instructionBuffer.out_type(instructionSize, byteIndex);
        byteIndex += sizeof(size_t);

        // Copy buffer
        if (instructionSize > 0) {
            FI.instructionBuffer.resize(instructionSize);
            instructionBuffer.out_raw(FI.instructionBuffer.bytes(), instructionSize, byteIndex);
            byteIndex += instructionSize;
        }

        // Sort instructions
        if (flag == 'U')
            diffFiles.emplace_back(std::move(FI));
        else if (flag == 'N')
            addedFiles.emplace_back(std::move(FI));
        else if (flag == 'D')
            removedFiles.emplace_back(std::move(FI));
        files++;
    }
    instructionBuffer.clear();

    // Patch all files first
    for (FileInstruction& inst : diffFiles) {
        // Try to find the target file
        auto storedFile = std::find_if(
            m_files.begin(),
            m_files.end(),
            [&](const VirtualFile& file) noexcept {
                return (file.m_relativePath == inst.path);
            }
        );
        if (storedFile == m_files.end())
            continue; // Error

        // Ensure hashes match
        if (storedFile->m_data.hash() != inst.diff_oldHash)
            continue; // Error

        // Attempt Patching Process
        auto newBuffer = storedFile->m_data.patch(inst.instructionBuffer);

        // Confirm new hashes match
        if (newBuffer.hash() != inst.diff_newHash)
            continue; // Error

        // Update virtualized folder
        std::swap(storedFile->m_data, newBuffer);
    }
    diffFiles.clear();

    // Add new files
    for (FileInstruction& inst : addedFiles) {
        // Convert the instruction into a virtual file
        auto newBuffer = Buffer().patch(inst.instructionBuffer);
        if (newBuffer.hash() != inst.diff_newHash)
            continue;

        // Erase any instances of this file
        m_files.erase(std::find_if(
            m_files.cbegin(),
            m_files.cend(),
            [&](const VirtualFile& file) noexcept {
                return file.m_relativePath == inst.path;
            }
        ));

        // Emplace the file
        m_files.emplace_back(VirtualFile{ inst.path, std::move(newBuffer) });
    }
    addedFiles.clear();

    // Delete all files
    for (FileInstruction& inst : removedFiles) {
        // Erase any instances of this file if the file name and hash matches
        m_files.erase(std::find_if(
            m_files.cbegin(),
            m_files.cend(),
            [&](const VirtualFile& file) noexcept {
                return
                    file.m_relativePath == inst.path &&
                    file.m_data.hash() == inst.diff_oldHash;
            }
        ));
    }
    removedFiles.clear();

    // Success
    return true;
}

void Directory::out_folder(const filepath& path) const
{
    for (auto& file : m_files) {
        // Write-out the file
        const auto fullPath = path.string() + "/" + file.m_relativePath;
        std::filesystem::create_directories(filepath(fullPath).parent_path());
        constexpr std::ios_base::openmode mode = std::ios_base::out | std::ios_base::binary;
        std::ofstream fileOnDisk = std::ofstream(fullPath.c_str(), mode);
        if (!fileOnDisk.is_open())
            throw std::runtime_error("Cannot write the file" + file.m_relativePath);

        fileOnDisk.write(
            file.m_data.charArray(),
            static_cast<std::streamsize>(file.m_data.size())
        );
        fileOnDisk.close();
    }
}

Buffer Directory::out_package(const std::string& folderName) const
{
    // Create a buffer large enough to hold all the files
    Buffer filebuffer(
        std::accumulate(
            m_files.cbegin(),
            m_files.cend(),
            static_cast<size_t>(sizeof(size_t)), // Starting with the file count
            [](const size_t& currentSize, const VirtualFile& file) noexcept {
                return currentSize
                    + sizeof(size_t) // Path Size
                    + (sizeof(char) * file.m_relativePath.size()) // Path
                    + sizeof(size_t) // File Size
                    + (sizeof(std::byte) * file.m_data.size()); // File Data
            }
        )
    );

    // Starting with the file count
    filebuffer.in_type(m_files.size());
    size_t byteIndex(sizeof(size_t));

    // Iterate over all files, writing in all their data
    for (auto& file : m_files) {
        // Write the path string into the archive
        filebuffer.in_type(file.m_relativePath, byteIndex);
        byteIndex += sizeof(size_t) + (sizeof(char) * file.m_relativePath.size());

        // Write the file size in bytes, into the archive
        filebuffer.in_type(file.m_data.size(), byteIndex);
        byteIndex += sizeof(size_t);

        // Copy the file data
        filebuffer.in_raw(file.m_data.bytes(), file.m_data.size(), byteIndex);
        byteIndex += sizeof(std::byte) * file.m_data.size();
    }

    // Try to compress the archive buffer
    filebuffer = filebuffer.compress();

    // Prepend header information
    constexpr char packHeaderTitle[16ULL] = "yatta pack\0";
    const auto& packHeaderName = folderName;
    const size_t headerSize = sizeof(packHeaderTitle) + sizeof(size_t) + (sizeof(char) * folderName.size());
    Buffer bufferWithHeader(filebuffer.size() + headerSize);

    // Copy header data into new buffer at the beginning
    bufferWithHeader.in_type(packHeaderTitle);
    bufferWithHeader.in_type(packHeaderName, sizeof(packHeaderTitle));

    // Copy remaining data
    bufferWithHeader.in_raw(filebuffer.bytes(), filebuffer.size(), headerSize);

    return bufferWithHeader; // Success
}

std::optional<Buffer> Directory::out_delta(const Directory& targetDirectory) const
{
    // Ensure we have files to diff
    if (fileCount() == 0 && targetDirectory.fileCount() == 0)
        return {}; // Failure

    // Find all common and new files first
    using PathList = std::vector<VirtualFile>;
    using PathPairList = std::vector<std::pair<VirtualFile, VirtualFile>>;
    static constexpr auto getFileLists = [](
        const Directory& oldDirectory,
        const Directory& newDirectory,
        PathPairList& commonFiles,
        PathList& addFiles,
        PathList& delFiles
        )
    {
        auto srcOld_Files = oldDirectory.m_files;
        auto srcNew_Files = newDirectory.m_files;
        for (const auto& nFile : srcNew_Files) {
            bool found = false;
            size_t oIndex(0ull);
            for (const auto& oFile : srcOld_Files) {
                if (nFile.m_relativePath == oFile.m_relativePath) {
                    // Common file found
                    commonFiles.push_back({ oFile, nFile });

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
    static constexpr auto writeInstructions = [](
        const std::string& path,
        const size_t& oldHash,
        const size_t& newHash,
        const Buffer& buffer,
        const char& flag,
        Buffer& instructionBuffer
        )
    {
        const auto bufferSize = buffer.size();
        const auto pathLength = path.length();
        const size_t instructionSize = (sizeof(size_t) * 4ULL) + (sizeof(char) * pathLength) + sizeof(char) + bufferSize;
        size_t byteIndex = instructionBuffer.size();
        instructionBuffer.resize(byteIndex + instructionSize);

        // Write file path
        instructionBuffer.in_type(path, byteIndex);
        byteIndex += sizeof(size_t) + (sizeof(char) * pathLength);

        // Write operation flag
        instructionBuffer.in_type(flag, byteIndex);
        byteIndex += sizeof(char);

        // Write old hash
        instructionBuffer.in_type(oldHash, byteIndex);
        byteIndex += sizeof(size_t);

        // Write new hash
        instructionBuffer.in_type(newHash, byteIndex);
        byteIndex += sizeof(size_t);

        // Write buffer size
        instructionBuffer.in_type(bufferSize, byteIndex);
        byteIndex += sizeof(size_t);

        // Write buffer
        instructionBuffer.in_raw(buffer.bytes(), sizeof(std::byte) * bufferSize, byteIndex);
        byteIndex += sizeof(std::byte) * bufferSize;
    };

    // Retrieve all common, added, and removed files
    PathPairList commonFiles;
    PathList addedFiles;
    PathList removedFiles;
    getFileLists(*this, targetDirectory, commonFiles, addedFiles, removedFiles);

    // Generate Instructions from file lists, store them in this expanding buffer
    Buffer instructionBuffer;

    // These files are common, maybe some have changed
    size_t fileCount(0ULL);
    for (const auto& [oldFile, newFile] : commonFiles) {
        const auto& oldBuffer = oldFile.m_data;
        const auto& newBuffer = newFile.m_data;
        const auto oldHash = oldBuffer.hash();
        const auto newHash = newBuffer.hash();
        // Check if a common file has changed
        if (oldHash != newHash) {
            const auto diffBuffer = oldBuffer.diff(newBuffer);
            writeInstructions(oldFile.m_relativePath, oldHash, newHash, diffBuffer, 'U', instructionBuffer);
            fileCount++;
        }
    }
    commonFiles.clear();

    // These files are brand new
    for (const auto& nFile : addedFiles) {
        const auto& newBuffer = nFile.m_data;
        const auto newHash = newBuffer.hash();
        auto diffBuffer = Buffer().diff(newBuffer);
        writeInstructions(nFile.m_relativePath, 0ULL, newHash, diffBuffer, 'N', instructionBuffer);
        fileCount++;
    }
    addedFiles.clear();

    // These files are deprecated
    for (const auto& oFile : removedFiles) {
        const auto& oldBuffer = oFile.m_data;
        const auto oldHash = oldBuffer.hash();
        writeInstructions(oFile.m_relativePath, oldHash, 0ULL, Buffer(), 'D', instructionBuffer);
        fileCount++;
    }
    removedFiles.clear();

    // Try to compress the instruction buffer
    instructionBuffer = instructionBuffer.compress();

    // Prepend header information
    constexpr char deltaHeaderTitle[16ULL] = "yatta delta";
    const auto& deltaHeaderFileCount = fileCount;
    constexpr size_t headerSize = sizeof(deltaHeaderTitle) + sizeof(size_t);
    Buffer bufferWithHeader(instructionBuffer.size() + headerSize);

    // Copy header data into new buffer at the beginning
    bufferWithHeader.in_type(deltaHeaderTitle);
    bufferWithHeader.in_type(deltaHeaderFileCount, sizeof(deltaHeaderTitle));

    // Copy remaining data
    bufferWithHeader.in_raw(instructionBuffer.bytes(), instructionBuffer.size(), headerSize);

    return bufferWithHeader; // Success
}
