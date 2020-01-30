#include "yatta.hpp"
#include <iostream>


// Convenience Definitions
constexpr int SUCCESS = 0;
constexpr int FAILURE = 1;
using yatta::Directory;

// Forward Declarations
bool Directory_ConstructionTest();
bool Directory_MethodTest();
bool Directory_ManipulationTest();

int main()
{
    if (Directory_ConstructionTest() &&
        Directory_MethodTest() &&
        Directory_ManipulationTest())
        return SUCCESS;

    std::cout << "Error while running Directory Tests\n";
    return FAILURE;
}

bool Directory_ConstructionTest()
{
    // Ensure we can make an empty directory
    Directory directory;
    if (!directory.empty())
        return false;

    // Ensure we can virtualize directories
    Directory dirA(Directory::GetRunningDirectory() + "/old");
    if (!dirA.hasFiles())
        return false;

    // Ensure move constructor works
    Directory moveDirectory = Directory(Directory::GetRunningDirectory() + "/old");
    if (moveDirectory.fileCount() != dirA.fileCount())
        return false;

    // Ensure copy constructor works
    const Directory& copyDir(moveDirectory);
    if (copyDir.fileSize() != moveDirectory.fileSize())
        return false;

    // Success
    return true;
}

bool Directory_MethodTest()
{
    // Verify empty directories
    Directory directory;
    if (!directory.empty())
        return false;

    // Verify that we can input folders
    directory.in_folder(Directory::GetRunningDirectory() + "/old");
    if (!directory.hasFiles())
        return false;

    // Ensure we have 4 files all together
    if (directory.fileCount() != 4ULL)
        return false;

    // Ensure the total size is as expected
    if (directory.fileSize() != 147777ULL)
        return false;

    // Ensure we can hash an actual directory
    if (const auto hash = directory.hash(); hash == yatta::ZeroHash)
        return false;

    // Ensure we can clear a directory
    directory.clear();
    if (!directory.empty() || directory.hasFiles())
        return false;

    // Ensure we can hash an empty directory
    if (directory.hash() != yatta::ZeroHash)
        return false;

    // Success
    return true;
}

bool Directory_ManipulationTest()
{
    // Verify empty directories
    Directory directory;
    if (!directory.empty())
        return false;

    // Ensure we have 8 files all together
    directory.in_folder(Directory::GetRunningDirectory() + "/old");
    directory.in_folder(Directory::GetRunningDirectory() + "/new");
    if (directory.fileCount() != 8ULL)
        return false;

    // Ensure the total size is as expected
    if (directory.fileSize() != 189747ULL)
        return false;

    // Reset the directory to just the 'old' folder, hash it
    directory = Directory(Directory::GetRunningDirectory() + "/old");
    const auto oldHash = directory.hash();
    if (oldHash == yatta::ZeroHash)
        return false;

    // Overwrite the /old folder, make sure hashes match
    directory.out_folder(Directory::GetRunningDirectory() + "/old");
    directory = Directory(Directory::GetRunningDirectory() + "/old");
    if (const auto reHash = directory.hash(); reHash == yatta::ZeroHash || reHash != oldHash)
        return false;

    // Ensure we can dump a directory as a package
    const auto package = directory.out_package("package");
    if (!package.has_value() || !package->hasData())
        return false;

    // Ensure we can import a package and that it matches the old one
    directory.clear();
    directory.in_package(*package);
    if (directory.fileSize() != 147777ULL || directory.fileCount() != 4ULL)
        return false;

    // Ensure new hash matches
    if (const auto packHash = directory.hash(); packHash == yatta::ZeroHash || packHash != oldHash)
        return false;

    // Try to diff the old and new directories
    Directory newDirectory(Directory::GetRunningDirectory() + "/new");
    const auto deltaBuffer = directory.out_delta(newDirectory);
    if (!deltaBuffer.has_value() || !deltaBuffer->hasData())
        return false;

    // Try to patch the old directory into the new directory
    if (!directory.in_delta(*deltaBuffer))
        return false;

    // Ensure the hashes match
    if (directory.hash() != newDirectory.hash())
        return false;

    // Overwrite the /new folder, make sure the hashes match
    directory.out_folder(Directory::GetRunningDirectory() + "/new");
    directory = Directory(Directory::GetRunningDirectory() + "/new");
    if (directory.hash() != newDirectory.hash())
        return false;

    // Success
    return true;
}