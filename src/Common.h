#pragma once
#ifndef COMMON_H
#define COMMON_H

#include "TaskLogger.h"
#include <cstddef>
#include <direct.h>
#include <filesystem>
#include <iostream>
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

/** Cleans up a target string representing a file path, removing leading and trailing quotes.
@param	path		reference to the path to be sanitized. */
inline static void sanitize_path(std::string & path)
{
	if (path.front() == '"' || path.front() == '\'')
		path.erase(0ull, 1ull);
	if (path.back() == '"' || path.back() == '\'')
		path.erase(path.size() - 1ull);
}

/** Return file-info for all files within the directory specified.
@param	directory	the directory to retrieve file-info from.
@return				a vector of file information, including file names, sizes, meta-data, etc. */
inline static auto get_file_paths(const std::string & directory)
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

/** Specify a message to print to the terminal before closing the program.
@param	message		the message to write-out.*/
inline static void exit_program(const char * message)
{
	TaskLogger::PushText(message);
	system("pause");
	exit(EXIT_FAILURE);
}

/** Print a message to the console, and pause the program until the user presses any key.
@param	message		pause message to show the user. */
inline static void pause_program(const char * message)
{
	TaskLogger::PushText(message + ' ');
	system("pause");
	std::printf("\033[A\33[2K\r");
	std::printf("\033[A\33[2K\r\n");
}

#endif // COMMON_H