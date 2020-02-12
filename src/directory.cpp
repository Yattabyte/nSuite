#include "directory.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <numeric>

// Convenience definitions
using yatta::Buffer;
using yatta::Directory;
using filepath = std::filesystem::path;
using directory_itt = std::filesystem::directory_iterator;
using directory_rec_itt = std::filesystem::recursive_directory_iterator;
using FileList = std::vector<Directory::VirtualFile>;
using FilePairList =
    std::vector<std::pair<Directory::VirtualFile, Directory::VirtualFile>>;
struct FileInstruction {
    std::string path, fullPath;
    Buffer instructionBuffer;
    size_t diff_oldHash = 0ULL, diff_newHash = 0ULL;
}; /** Contains diff instructions for a specific file. */

// Private Static Methods

/** Attempts to remove a file using an instruction. */
constexpr auto find_file = [](const std::string& path, const size_t& hash,
                              auto& files) noexcept {
    return std::find_if(
        files.begin(), files.end(), [&path, &hash](const auto& file) noexcept {
            // Ensure file path and hash matches
            return file.m_relativePath == path && file.m_data.hash() == hash;
        });
};

static constexpr auto get_file_lists = [](const FileList& srcOld_Files,
                                          const FileList& srcNew_Files) {
    FilePairList commonFiles;
    FileList addFiles;
    FileList delFiles = srcOld_Files;
    for (const auto& nFile : srcNew_Files) {
        bool found = false;
        size_t oIndex(0ULL);
        for (const auto& oFile : delFiles) {
            if (nFile.m_relativePath == oFile.m_relativePath) {
                // Common file found
                commonFiles.push_back({ oFile, nFile });

                // Remove old file from list
                delFiles.erase(delFiles.begin() + oIndex);
                found = true;
                break;
            }
            oIndex++;
        }
        // New file found, add it
        if (!found)
            addFiles.push_back(nFile);
    }

    return std::make_tuple(commonFiles, addFiles, delFiles);
};

void in_files(
    const Buffer& filebuffer, std::vector<Directory::VirtualFile>& files) {
    // Find the file count
    size_t byteIndex(0ULL);
    size_t fileCount(0ULL);
    filebuffer.out_type(fileCount, byteIndex);
    byteIndex += sizeof(size_t);

    // Iterate over all files, writing out all their data
    const auto packSize = filebuffer.size();
    size_t fileIndex(files.size());
    files.resize(files.size() + fileCount);
    const auto finalSize = files.size();
    while (byteIndex < packSize && fileIndex < finalSize) {
        // Read the path string out of the archive
        auto& file = files[fileIndex++];
        filebuffer.out_type(file.m_relativePath, byteIndex);
        byteIndex += sizeof(size_t) +
                     (sizeof(char) * file.m_relativePath.size()) +
                     sizeof(size_t);

        // Write the file size in bytes, into the archive
        size_t bufferSize(0ULL);
        filebuffer.out_type(bufferSize, byteIndex);
        byteIndex += sizeof(size_t);

        // Copy the file data
        file.m_data.resize(bufferSize);
        filebuffer.out_raw(file.m_data.bytes(), file.m_data.size(), byteIndex);
        byteIndex += sizeof(std::byte) * file.m_data.size();
    }
}

/** Attempt to patch a file using an instruction. */
void patch_file(
    Directory::VirtualFile& file, const FileInstruction& instruction) {
    if (auto result = file.m_data.patch(instruction.instructionBuffer))
        // Confirm new hashes match
        if (result->hash() == instruction.diff_newHash)
            // Update virtualized folder
            std::swap(file.m_data, *result);
}

/** Attempt to create a new file using an instruction. */
std::optional<Directory::VirtualFile>
add_file(const FileInstruction& instruction) {
    // Convert the instruction into a virtual file
    if (auto result = Buffer().patch(instruction.instructionBuffer))
        // Confirm new hashes match
        if (result->hash() == instruction.diff_newHash)
            // Emplace the file
            return Directory::VirtualFile{ instruction.path,
                                           std::move(*result) };
    return {};
}

/** Parse in instructions from a memory range, updating instruction vectors. */
void in_instructions(
    const Buffer& instructionBuffer, const size_t& expectedFileCount,
    std::vector<FileInstruction>& diffInstructions,
    std::vector<FileInstruction>& addInstructions,
    std::vector<FileInstruction>& removeInstructions) {
    // Start reading diff file
    size_t files(0ULL);
    size_t byteIndex(0ULL);
    const size_t instBufSize = instructionBuffer.size();
    FileInstruction instruction;
    char flag(0);
    size_t instructionSize(0ULL);
    while (files < expectedFileCount && byteIndex < instBufSize) {
        // Read Attributes
        instructionBuffer.out_type(instruction.path, byteIndex);
        byteIndex += sizeof(size_t) +
                     (sizeof(char) * instruction.path.length()) +
                     sizeof(size_t);
        instructionBuffer.out_type(flag, byteIndex);
        byteIndex += sizeof(char);
        instructionBuffer.out_type(instruction.diff_oldHash, byteIndex);
        byteIndex += sizeof(size_t);
        instructionBuffer.out_type(instruction.diff_newHash, byteIndex);
        byteIndex += sizeof(size_t);
        instructionBuffer.out_type(instructionSize, byteIndex);
        byteIndex += sizeof(size_t);
        instruction.instructionBuffer.resize(instructionSize);
        if (instructionSize != 0ULL)
            instructionBuffer.out_raw(
                instruction.instructionBuffer.bytes(), instructionSize,
                byteIndex);
        byteIndex += instructionSize;

        // Sort instructions
        if (flag == 'U')
            diffInstructions.emplace_back(std::move(instruction));
        else if (flag == 'N')
            addInstructions.emplace_back(std::move(instruction));
        else if (flag == 'D')
            removeInstructions.emplace_back(std::move(instruction));
        files++;
    }
}

/** Write out instructions into a buffer. */
void out_instruction(
    const std::string& path, const size_t& oldHash, const size_t& newHash,
    const Buffer& buffer, const char& flag, Buffer& instructionBuffer) {
    const auto bufferSize = buffer.size();
    const auto pathLength = path.length();
    const size_t instructionSize = (sizeof(size_t) * 4ULL) +
                                   (sizeof(char) * pathLength) + sizeof(char) +
                                   bufferSize;
    instructionBuffer.reserve(instructionBuffer.size() + instructionSize);

    // Write Attributes
    instructionBuffer.push_type(path);
    instructionBuffer.push_type(flag);
    instructionBuffer.push_type(oldHash);
    instructionBuffer.push_type(newHash);
    instructionBuffer.push_type(bufferSize);
    if (bufferSize != 0ULL)
        instructionBuffer.push_raw(
            buffer.bytes(), sizeof(std::byte) * bufferSize);
}

std::pair<Buffer, size_t>
gen_instructions(const FileList& srcFiles, const FileList& dstFiles) {
    // Retrieve all common, added, and removed files
    auto [commonFiles, addedFiles, removedFiles] =
        get_file_lists(srcFiles, dstFiles);

    // These files are common, maybe some have changed
    Buffer instructionBuffer;
    size_t instCount(0ULL);
    for (const auto& [oldFile, newFile] : commonFiles) {
        // Check if a common file has changed
        const auto oldHash = oldFile.m_data.hash();
        const auto newHash = newFile.m_data.hash();
        if (oldHash != newHash) {
            if (const auto diffBuffer = oldFile.m_data.diff(newFile.m_data)) {
                out_instruction(
                    oldFile.m_relativePath, oldHash, newHash, *diffBuffer, 'U',
                    instructionBuffer);
                instCount++;
            }
        }
    }
    commonFiles.clear();

    // These files are brand new
    for (const auto& nFile : addedFiles) {
        if (const auto diffBuffer = Buffer().diff(nFile.m_data)) {
            out_instruction(
                nFile.m_relativePath, 0ULL, nFile.m_data.hash(), *diffBuffer,
                'N', instructionBuffer);
            instCount++;
        }
    }
    addedFiles.clear();

    // These files are deprecated
    for (const auto& oFile : removedFiles) {
        out_instruction(
            oFile.m_relativePath, oFile.m_data.hash(), 0ULL, Buffer(), 'D',
            instructionBuffer);
        instCount++;
    }
    removedFiles.clear();

    // Success
    return { instructionBuffer, instCount };
}

/** Modify files based on the input instruction set. */
void apply_instructions(
    std::vector<FileInstruction>& diffFiles,
    std::vector<FileInstruction>& addedFiles,
    std::vector<FileInstruction>& removedFiles,
    std::vector<Directory::VirtualFile>& files) {
    // Patch all files first
    std::for_each(
        diffFiles.cbegin(), diffFiles.cend(), [&](const FileInstruction& inst) {
            // Try to find the target file
            if (auto dFile = find_file(inst.path, inst.diff_oldHash, files);
                dFile != files.end())
                patch_file(*dFile, inst);
        });
    diffFiles.clear();

    // Add new files
    std::for_each(
        addedFiles.cbegin(), addedFiles.cend(),
        [&](const FileInstruction& inst) {
            // Erase any instances of this file
            const auto prevFile =
                find_file(inst.path, inst.diff_oldHash, files);
            if (prevFile != files.end())
                files.erase(prevFile);
            // Attempt to make a new file
            if (auto newFile = add_file(inst))
                files.emplace_back(std::move(*newFile));
        });
    addedFiles.clear();

    // Delete all files
    std::for_each(
        removedFiles.cbegin(), removedFiles.cend(),
        [&](const FileInstruction& inst) {
            // Erase any instances of this file
            files.erase(
                std::remove_if(
                    files.begin(), files.end(),
                    [&](const Directory::VirtualFile& file) noexcept {
                        return file.m_relativePath == inst.path &&
                               file.m_data.hash() == inst.diff_oldHash;
                    }),
                files.end());
        });
    removedFiles.clear();
}

// Public (de)Constructors

Directory::Directory(
    const filepath& path, const std::vector<std::string>& exclusions) {
    if (std::filesystem::is_directory(path))
        in_folder(path, exclusions);
}

Directory::Directory(const Buffer& packageBuffer) {
    if (packageBuffer.hasData())
        in_package(packageBuffer);
}

// Public Inquiry Methods

bool Directory::empty() const noexcept { return m_files.empty(); }

bool Directory::hasFiles() const noexcept { return !m_files.empty(); }

size_t Directory::fileCount() const noexcept { return m_files.size(); }

size_t Directory::fileSize() const noexcept {
    return std::accumulate(
        m_files.begin(), m_files.end(), 0ULL,
        [](const size_t& currentSum, const VirtualFile& file) noexcept {
            return currentSum + file.m_data.size();
        });
}

size_t Directory::hash() const noexcept {
    // May overflow, but that's okay as long as the accumulation order is the
    // same such that 2 copies of the same directory result in the same hash
    return std::accumulate(
        m_files.begin(), m_files.end(), yatta::ZeroHash,
        [](const size_t& currentHash, const VirtualFile& file) noexcept {
            return currentHash + file.m_data.hash();
        });
}

#ifdef __GNUC__
#include <unistd.h>
#else
#include <direct.h>
#define getcwd _getcwd
#endif // __GNUC__

std::string Directory::GetRunningDirectory() noexcept {
    char cCurrentPath[FILENAME_MAX];
    if (getcwd(cCurrentPath, sizeof(cCurrentPath)) != nullptr)
        cCurrentPath[sizeof(cCurrentPath) - 1ULL] = static_cast<char>('\0');
    return std::string(cCurrentPath);
}

// Public Manipulation Methods

void Directory::clear() noexcept { m_files.clear(); }

// Public IO Methods

bool Directory::in_folder(
    const filepath& path, const std::vector<std::string>& exclusions) {
    // Ensure the directory exists
    if (!std::filesystem::exists(path))
        return false; // Failure

    constexpr auto get_file_paths = [](const filepath& directory,
                                       const std::vector<std::string>& exc) {
        std::vector<std::filesystem::directory_entry> paths;
        if (std::filesystem::is_directory(directory))
            for (const auto& entry : directory_rec_itt(directory))
                if (entry.is_regular_file()) {
                    const auto extension = entry.path().extension();
                    if (!std::any_of(
                            exc.cbegin(), exc.cend(),
                            [extension](const auto& element) {
                                return element == extension;
                            }))
                        paths.emplace_back(entry);
                }
        return paths;
    };

    for (const auto& entry : get_file_paths(path, exclusions)) {
        if (entry.is_regular_file()) {
            // Read the file data
            Buffer fileBuffer(entry.file_size());
            const std::string path_string = entry.path().string();
            constexpr std::ios_base::openmode mode =
                std::ios_base::in | std::ios_base::binary;
            std::ifstream fileOnDisk = std::ifstream(path_string, mode);
            assert(fileOnDisk.is_open());

            fileOnDisk.read(
                fileBuffer.charArray(),
                static_cast<std::streamsize>(fileBuffer.size()));
            fileOnDisk.close();

            m_files.emplace_back(VirtualFile{
                (std::filesystem::relative(entry.path(), path)).string(),
                std::move(fileBuffer) });
        }
    }

    return true; // Success
}

bool Directory::in_package(const Buffer& packageBuffer) {
    // Ensure the package buffer exists
    if (packageBuffer.empty())
        return false; // Failure

    // Read in header
    char packHeaderTitle[16ULL] = { '\0' };
    std::string packHeaderName;
    size_t byteIndex = sizeof(packHeaderTitle);
    packageBuffer.out_type(packHeaderTitle);
    packageBuffer.out_type(packHeaderName, byteIndex);
    byteIndex += sizeof(size_t) + (sizeof(char) * packHeaderName.size()) +
                 sizeof(size_t);

    // Ensure header title matches
    if (std::strcmp(packHeaderTitle, "yatta pack") != 0)
        return false; // Failure

    // Try to decompress the archive buffer
    auto filebuffer = Buffer::decompress(
        packageBuffer.subrange(byteIndex, packageBuffer.size() - byteIndex));
    if (!filebuffer.has_value())
        return false; // Failure

    // Parse and read-in the packaged files
    in_files(*filebuffer, m_files);

    // Success
    return true;
}

bool Directory::in_delta(const Buffer& deltaBuffer) {
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
    const auto instructionBuffer = Buffer::decompress(
        deltaBuffer.subrange(byteIndex, deltaBuffer.size() - byteIndex));
    if (!instructionBuffer.has_value())
        return false;

    // Parse and acquire instructions
    std::vector<FileInstruction> diffFiles;
    std::vector<FileInstruction> addedFiles;
    std::vector<FileInstruction> removedFiles;
    in_instructions(
        *instructionBuffer, deltaHeaderFileCount, diffFiles, addedFiles,
        removedFiles);

    // Consume and apply instructions
    apply_instructions(diffFiles, addedFiles, removedFiles, m_files);
    return true;
}

bool Directory::out_folder(const filepath& path) const {
    // Ensure we have files to output
    if (m_files.empty())
        return false; // Failure

    for (auto& file : m_files) {
        // Write-out the file
        const auto fullPath = path.string() + "/" + file.m_relativePath;
        std::filesystem::create_directories(filepath(fullPath).parent_path());
        constexpr std::ios_base::openmode mode =
            std::ios_base::out | std::ios_base::binary;
        std::ofstream fileOnDisk = std::ofstream(fullPath.c_str(), mode);
        assert(fileOnDisk.is_open());

        fileOnDisk.write(
            file.m_data.charArray(),
            static_cast<std::streamsize>(file.m_data.size()));
        fileOnDisk.close();
    }

    return true; // Success
}

std::optional<Buffer>
Directory::out_package(const std::string& folderName) const {
    // Ensure we have files to output
    if (m_files.empty())
        return {}; // Failure

    // Create a buffer large enough to hold all the files
    Buffer filebuffer;
    filebuffer.reserve(std::accumulate(
        m_files.cbegin(), m_files.cend(),
        sizeof(size_t), // Starting with the file count
        [](const size_t& currentSize, const VirtualFile& file) noexcept {
            return currentSize + sizeof(size_t)                  // Path Size
                   + (sizeof(char) * file.m_relativePath.size()) // Path
                   + sizeof(size_t) // Path Size again for bidirectional reading
                   + sizeof(size_t) // File Size
                   + (sizeof(std::byte) * file.m_data.size()); // File Data
        }));

    // Starting with the file count
    filebuffer.push_type(m_files.size());

    // Iterate over all files, writing in all their data
    for (auto& file : m_files) {
        filebuffer.push_type(file.m_relativePath);
        filebuffer.push_type(file.m_data.size());
        filebuffer.push_raw(file.m_data.bytes(), file.m_data.size());
    }

    // Try to compress the archive buffer
    if (auto result = filebuffer.compress())
        std::swap(filebuffer, *result);
    else
        return {}; // Failure

    // Prepend header information
    constexpr char packHeaderTitle[16ULL] = "yatta pack\0";
    const auto& packHeaderName = folderName;
    const size_t headerSize = sizeof(packHeaderTitle) + sizeof(size_t) +
                              (sizeof(char) * folderName.size()) +
                              sizeof(size_t);
    Buffer bufferWithHeader;
    bufferWithHeader.reserve(filebuffer.size() + headerSize);

    // Copy header data into new buffer at the beginning
    bufferWithHeader.push_type(packHeaderTitle);
    bufferWithHeader.push_type(packHeaderName);
    bufferWithHeader.push_raw(filebuffer.bytes(), filebuffer.size());

    return bufferWithHeader; // Success
}

std::optional<Buffer>
Directory::out_delta(const Directory& targetDirectory) const {
    // Ensure we have files to diff
    if (fileCount() == 0 && targetDirectory.fileCount() == 0)
        return {}; // Failure

    // Retrieve all common, added, and removed files as instructions
    auto [instructionBuffer, instCount] =
        gen_instructions(m_files, targetDirectory.m_files);

    // Try to compress the instruction buffer
    if (auto result = instructionBuffer.compress())
        std::swap(instructionBuffer, *result);
    else
        return {}; // Failure

    // Prepend header information
    constexpr char deltaHeaderTitle[16ULL] = "yatta delta";
    const auto& deltaHeaderFileCount = instCount;
    constexpr size_t headerSize = sizeof(deltaHeaderTitle) + sizeof(size_t);
    Buffer bufferWithHeader;
    bufferWithHeader.reserve(instructionBuffer.size() + headerSize);

    // Copy header data into new buffer at the beginning
    bufferWithHeader.push_type(deltaHeaderTitle);
    bufferWithHeader.push_type(deltaHeaderFileCount);
    bufferWithHeader.push_raw(
        instructionBuffer.bytes(), instructionBuffer.size());

    return bufferWithHeader; // Success
}