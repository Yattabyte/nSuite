#include "Directory.hpp"
#include "BufferView.hpp"
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
        size_t bufferSize(0ull);
        filebuffer.out_type(bufferSize, byteIndex);
        byteIndex += sizeof(size_t);

        // Copy the file data
        file.m_data.resize(bufferSize);
        filebuffer.out_raw(file.m_data.bytes(), file.m_data.size(), byteIndex);
        byteIndex += sizeof(std::byte) * file.m_data.size();
        ++fileIndex;
    }
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
            sizeof(size_t), // Starting with the file count
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
    const auto packHeaderName = folderName;
    const auto headerSize = sizeof(packHeaderTitle) + sizeof(size_t) + (sizeof(char) * folderName.size());
    Buffer bufferWithHeader(filebuffer.size() + headerSize);

    // Copy header data into new buffer at the beginning
    bufferWithHeader.in_type(packHeaderTitle);
    bufferWithHeader.in_type(packHeaderName, sizeof(packHeaderTitle));

    // Copy remaining data
    bufferWithHeader.in_raw(filebuffer.bytes(), filebuffer.size(), headerSize);

    return bufferWithHeader; // Success
}