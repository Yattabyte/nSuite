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
        inline ~Directory() = default;
		/***/
		inline Directory() = default;
		/** Constructs a virtualized directory from a path on disk.
		@param	path				the absolute path to a desired folder or a package file.
		@param	exclusions			(optional) list of filenames/types to skip. "string" matches relative path, ".ext" matches extension. */
		explicit Directory(const std::filesystem::path& path, const std::vector<std::string>& exclusions = {});
        /***/
		inline Directory(const Directory& other) = default;
        /***/
		inline Directory(Directory&& other) noexcept = default;
		

		//
		/***/
		void in_folder(const std::filesystem::path& path, const std::vector<std::string>& exclusions = {});
		/***/
		void out_folder(const std::filesystem::path& path);


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
		/***/
		static std::string GetRunningDirectory() noexcept;


	protected:
		// Protected Attributes
		/***/
		struct VirtualFile {
			std::string m_relativePath = "";
			Buffer m_data;
		};
		std::vector<VirtualFile> m_files;
		std::string m_name = "";
	};
}
#endif // DIRECTORY_H