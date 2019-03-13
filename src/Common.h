#pragma once
#ifndef COMMON_H
#define COMMON_H
#include <cstddef>
#include <direct.h>
#include <filesystem>
#include <string>
#include <vector>


/** Increment a pointer's address by the offset provided.
@param	ptr			the pointer to increment by the offset amount.
@param	offset		the offset amount to apply to the pointer's address.
@return				the modified pointer address. */
inline static void * PTR_ADD(void *const ptr, const size_t & offset)
{
	return static_cast<std::byte*>(ptr) + offset;
};

/** Return file-info for all files within the directory specified.
@param	directory	the directory to retrieve file-info from.
@return				a vector of file information, including file names, sizes, meta-data, etc. */
inline static std::vector<std::filesystem::directory_entry> get_file_paths(const std::string & directory)
{
	std::vector<std::filesystem::directory_entry> paths;
	for (const auto & entry : std::filesystem::recursive_directory_iterator(directory))
		if (entry.is_regular_file())
			paths.emplace_back(entry);
	return paths;
}

/** Retrieve the directory this executable is running out-of.
@return		the current directory of this program. */
inline static std::string get_current_directory()
{
	// Get the running directory
	char cCurrentPath[FILENAME_MAX];
	if (_getcwd(cCurrentPath, sizeof(cCurrentPath)))
		cCurrentPath[sizeof(cCurrentPath) - 1ull] = char('\0');
	return std::string(cCurrentPath);
}

#endif // COMMON_H