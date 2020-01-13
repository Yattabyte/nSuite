#include "Directory.hpp"
#include <fstream>
#include <numeric>

using yatta::Buffer;
using yatta::Directory;


// Public (de)Constructors

Directory::Directory(const std::filesystem::path& path, const std::vector<std::string>& exclusions)
{
    if (std::filesystem::is_directory(path))
        in_folder(path, exclusions);
}

void Directory::in_folder(const std::filesystem::path& path, const std::vector<std::string>& exclusions)
{
    constexpr auto get_file_paths = [](const std::filesystem::path& , const std::vector<std::string>&) {
        //constexpr auto check_exclusion = [](const std::filesystem::path& path, const std::vector<std::string>& exclusions) {
        //    const auto extension = path.extension();
        //    for (const auto& excl : exclusions) {
        //        if (excl.empty())
        //            continue;
        //        // Compare Paths && Extensions
        //        if (path == excl || extension == excl) {
        //            // Don't use path
        //            return false;
        //        }
        //    }
        //    // Safe to use path
        //    return true;
        //};
        std::vector<std::filesystem::directory_entry> paths;
       // if (std::filesystem::is_directory(directory)) {}
            /*for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
                if (entry.is_regular_file()) {
                    auto path = entry.path().string();
                    path = path.substr(directory.string().size(), path.size() - directory.string().size());
                    //if (check_exclusion(path, exclusions))
                        paths.emplace_back(entry);
                }*/
        return paths;
    };
    for (const auto& entry : get_file_paths(path, exclusions)) {
        if (entry.is_regular_file()) {
            /*// Read the file data
            Buffer fileBuffer(entry.file_size());
            const std::string path_string = entry.path().string();
            constexpr std::ios_base::openmode mode = std::ios_base::in | std::ios_base::binary;
            std::ifstream fileOnDisk = std::ifstream(path_string, mode);
            if (!fileOnDisk.is_open())
                throw std::runtime_error("Cannot read the file" + entry.path().string());

            fileOnDisk.read(fileBuffer.charArray(), static_cast<std::streamsize>(fileBuffer.size()));
            fileOnDisk.close();

            m_files.emplace_back(VirtualFile{ (std::filesystem::relative(entry.path(), path)).string(), std::move(fileBuffer) });*/
        }
    }
}

void Directory::out_folder(const std::filesystem::path& path)
{
    for (auto& file : m_files) {
        // Write-out the file
        const auto fullPath = path.string() + file.m_relativePath;
        std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());
        std::ofstream fileOnDisk = std::ofstream(fullPath.c_str(), std::ofstream::binary | std::ofstream::out);
        if (!fileOnDisk.is_open())
            throw std::runtime_error("Cannot write the file" + file.m_relativePath);

        fileOnDisk.write(file.m_data.charArray(), static_cast<std::streamsize>(file.m_data.size()));
        fileOnDisk.close();
    }
}


// Public Inquiry Methods

bool Directory::empty() const noexcept
{
    return m_files.empty();
}

bool Directory::hasFiles() const noexcept { return !m_files.empty(); }

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
        cCurrentPath[sizeof(cCurrentPath) - 1ULL] = char('\0');
    return std::string(cCurrentPath);
}