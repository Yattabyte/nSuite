#pragma once
#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "Buffer.hpp"
#include <filesystem>
#include <string>
#include <vector>


namespace yatta {
    /***/
    class Directory {
    public:
        // Public (de)Constructors
        /***/
        ~Directory() = default;
        /***/
        Directory() = default;
        /** Constructs a virtualized directory from a path on disk.
        @param	path				the absolute path to a desired folder or a package file.
        @param	exclusions			(optional) list of filenames/types to skip. "string" matches relative path, ".ext" matches extension. */
        explicit Directory(const std::filesystem::path& path, const std::vector<std::string>& exclusions = {});
        /***/
        explicit Directory(const Buffer& packageBuffer);
        /***/
        Directory(const Directory& other) = default;
        /***/
        Directory(Directory&& other) noexcept = default;


        // Public Assignment Operators
        /** Copy-assignment operator.
        @param	other				the directory to copy from.
        @return						reference to this. */
        Directory& operator=(const Directory& other) = default;
        /** Move-assignment operator.
        @param	other				the directory to move from.
        @return						reference to this. */
        Directory& operator=(Directory&& other) noexcept = default;


        // Public Inquiry Methods
        /** Check if this directory is empty - has no files.
        @return						true if no files exist in this directory, false otherwise. */
        bool empty() const noexcept;
        /** Retrieves whether or not this directory has files.
        @return						true if at least one file exists in this directory. */
        bool hasFiles() const noexcept;
        /** Returns the number of files in this directory.
        @return						number of files in this directory. */
        size_t fileCount() const noexcept;
        /** Returns the sum of all file sizes in this directory.
        @return						the total number of bytes of all files in this directory. */
        size_t fileSize() const noexcept;
        /** Generates a hash value derived from this directory's contents.
        @return						hash value for this directory, derived from its buffers. */
        size_t hash() const noexcept;
        /***/
        static std::string GetRunningDirectory() noexcept;


        // Public Manipulation Methods
        /** Clears out the contents of this virtual directory, freeing its memory. */
        void clear() noexcept;
        /***/
        void in_folder(const std::filesystem::path& path, const std::vector<std::string>& exclusions = {});
        /***/
        void in_package(const Buffer& packageBuffer);
        /***/
        void out_folder(const std::filesystem::path& path) const;
        /***/
        Buffer out_package(const std::string& folderName) const;


    protected:
        // Protected Attributes
        /***/
        struct VirtualFile {
            std::string m_relativePath = "";
            Buffer m_data;
        };
        std::vector<VirtualFile> m_files;
    };
}
#endif // DIRECTORY_H