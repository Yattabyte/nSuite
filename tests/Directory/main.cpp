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

int main() noexcept
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
    if (directory.empty()) {
        // Ensure we can virtualize directories
        Directory dirA(Directory::GetRunningDirectory() + "/old");
        if (dirA.hasFiles()) {
            // Ensure move constructor works
            Directory moveDirectory = Directory(Directory::GetRunningDirectory() + "/old");
            if (moveDirectory.fileCount() == dirA.fileCount()) {
                // Ensure copy constructor works
                const Directory& copyDir(moveDirectory);
                if (copyDir.fileSize() == moveDirectory.fileSize()) {
                    return true; // Success
                }
            }
        }
    }

    return false; // Failure
}

bool Directory_MethodTest()
{
    // Verify empty directories
    Directory directory;
    if (directory.empty()) {
        // Verify that we can input folders
        directory.in_folder(Directory::GetRunningDirectory() + "/old");
        if (directory.hasFiles()) {
            // Ensure we have 4 files all together
            if (directory.fileCount() == 4ULL) {
                // Ensure the total size is as expected
                if (directory.fileSize() == 147777ULL) {
                    // Ensure we can hash an actual directory
                    if (const auto hash = directory.hash(); hash != yatta::ZeroHash) {
                        // Ensure we can clear a directory
                        directory.clear();
                        if (directory.empty()) {
                            // Ensure we can hash an empty directory
                            if (directory.hash() == yatta::ZeroHash) {
                                return true; // Success
                            }
                        }
                    }
                }
            }
        }
    }

    return false; // Failure
}

bool Directory_ManipulationTest()
{
    // Verify empty directories
    Directory directory;
    if (directory.empty()) {
        // Verify that we can add multiple folders together
        directory.in_folder(Directory::GetRunningDirectory() + "/old");
        directory.in_folder(Directory::GetRunningDirectory() + "/new");
        // Ensure we have 8 files all together
        if (directory.fileCount() == 8ULL) {
            // Ensure the total size is as expected
            if (directory.fileSize() == 189747ULL) {
                // Reset the directory to just the 'old' folder, hash it
                directory = Directory(Directory::GetRunningDirectory() + "/old");
                if (const auto oldHash = directory.hash(); oldHash != yatta::ZeroHash) {
                    // Overwrite the /old folder, make sure hashes match
                    directory.out_folder(Directory::GetRunningDirectory() + "/old");
                    directory = Directory(Directory::GetRunningDirectory() + "/old");
                    if (const auto newHash = directory.hash(); newHash != yatta::ZeroHash && newHash == oldHash) {
                        // Ensure we can dump a directory as a package
                        if (const auto package = directory.out_package("package")) {
                            if (package->hasData()) {
                                // Ensure we can import a package
                                directory.clear();
                                directory.in_package(*package);
                                // Ensure this new directory matches the old one
                                if (directory.fileSize() == 147777ULL && directory.fileCount() == 4ULL) {
                                    // Ensure new hash matches
                                    if (const auto packHash = directory.hash(); newHash != yatta::ZeroHash && packHash == newHash) {
                                        // Try to diff the old and new directories
                                        Directory newDirectory(Directory::GetRunningDirectory() + "/new");
                                        if (const auto deltaBuffer = directory.out_delta(newDirectory)) {
                                            // Try to patch the old directory into the new directory
                                            if (directory.in_delta(*deltaBuffer)) {
                                                // Ensure the hashes match
                                                if (directory.hash() == newDirectory.hash()) {
                                                    // Overwrite the /new folder, make sure the hashes match
                                                    directory.out_folder(Directory::GetRunningDirectory() + "/new");
                                                    directory = Directory(Directory::GetRunningDirectory() + "/new");
                                                    if (directory.hash() == newDirectory.hash()) {
                                                        return true; // Success
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return false; // Failure
}