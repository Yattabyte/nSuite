#pragma once
#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "buffer.hpp"
#include <filesystem>
#include <string>
#include <vector>


namespace yatta {
    /** A virtual file folder, sourcing its data from disk or pre-packaged buffers.
    Has functions for compressing, decompressing, diffing, and patching. */
    class Directory {
    public:
        // Public (de)Constructors
        /** Destroy this directory. */
        ~Directory() = default;
        /** Construct an empty directory. */
        Directory() = default;
        /** Constructs a directory from a path on disk.
        @param  path            the absolute path to a desired folder or a package file.
        @param  exclusions      list of files or extensions to exclude. */
        explicit Directory(const std::filesystem::path& path, const std::vector<std::string>& exclusions = {});
        /** Constructs a directory from a packaged buffer. 
        @param  packageBuffer   the package to source data from. */
        explicit Directory(const Buffer& packageBuffer);
        /** Construct a directory, copying from another.
        @param  other           the directory to copy from. */
        Directory(const Directory& other) = default;
        /** Construct a directory, moving from another.
        @param  other           the directory to move from. */
        Directory(Directory&& other) noexcept = default;


        // Public Assignment Operators
        /** Copy-assignment operator.
        @param  other           the directory to copy from.
        @return                 reference to this. */
        Directory& operator=(const Directory& other) = default;
        /** Move-assignment operator.
        @param  other           the directory to move from.
        @return                 reference to this. */
        Directory& operator=(Directory&& other) noexcept = default;


        // Public Inquiry Methods
        /** Check if this directory is empty.
        @return                 true if this has no files, false otherwise. */
        bool empty() const noexcept;
        /** Check if this directory has files.
        @return                 true if at least one file exists. */
        bool hasFiles() const noexcept;
        /** Returns the number of files in this directory.
        @return                 the number of files in this directory. */
        size_t fileCount() const noexcept;
        /** Returns the sum of all file sizes in this directory.
        @return                 the total number of bytes of all files in this directory. */
        size_t fileSize() const noexcept;
        /** Generates a hash value derived from this directory's contents.
        @return                 hash value for this directory, derived from its buffers. */
        size_t hash() const noexcept;
        /** Retrieve the running directory for this application. 
        @return                 the directory this application launched from. */
        static std::string GetRunningDirectory() noexcept;


        // Public Manipulation Methods
        /** Remove all files from this directory, freeing its memory. */
        void clear() noexcept;


        // Public IO Methods
        /** Copies in the files found on disk at the path specified.
        @param  path            the path to look for files at. 
        @param  exclusions      list of filenames/types to skip. "string" matches relative path, ".ext" matches extension. 
        @return                 true on success, false otherwise. */
        bool in_folder(const std::filesystem::path& path, const std::vector<std::string>& exclusions = {});
        /** Parses and expands the contents of a package into this directory.
        @param  packageBuffer   the package to source data from. 
        @return                 true on success, false otherwise. */
        bool in_package(const Buffer& packageBuffer);
        /** Updates and patches the files in this directory using the specified patch file. 
        @param  deltaBuffer     the patch to apply. 
        @return                 true on success, false otherwise. */
        bool in_delta(const Buffer& deltaBuffer);
        /** Copies out the files found in this directory to the path on disk specified.
        @param  path            the path to write the files at.
        @return                 true on success, false otherwise. */
        bool out_folder(const std::filesystem::path& path) const;
        /** Generate a package buffer from this directory.
        @param  folderName      the name to give this package.
        @return                 packaged version of this directory on success, empty otherwise. */
        std::optional<Buffer> out_package(const std::string& folderName) const;
        /** Generate a patch buffer from this directory against the specified target directory.
        @param  targetDirectory the target to diff against.
        @return                 patch buffer on success, empty otherwise. */
        std::optional<Buffer> out_delta(const Directory& targetDirectory) const;


    protected:
        // Protected Attributes
        /** File data and path container. */
        struct VirtualFile {
            std::string m_relativePath = "";
            Buffer m_data;
        };
        std::vector<VirtualFile> m_files;
    };
}
#endif // DIRECTORY_H