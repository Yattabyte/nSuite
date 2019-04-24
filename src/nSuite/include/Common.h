#pragma once
#ifndef COMMON_H
#define COMMON_H

#include "Log.h"
#include <cstddef>
#include <direct.h>
#include <filesystem>
#include <iostream>
#include <shlobj.h>
#include <string>
#include <stdio.h>
#include <vector>
#include <windows.h>


/** Changes an input string to lower case, and returns it.
@param	string		the input string.
@return				lower case version of the string. */
inline static std::string string_to_lower(const std::string & string)
{
	std::string input = string;
	std::transform(input.begin(), input.end(), input.begin(), [](const int & character){ return static_cast<char>(::tolower(character)); });
	return input;
}

/** Converts the input string into a wide string.
@param	str			the input string.
@return				wide string version of the string. */
inline static std::wstring to_wideString(const std::string & str)
{
	if (!str.empty()) {
		if (const auto size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0)) {
			std::wstring wstr(size_needed, 0);
			MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
			return wstr;
		}
	}
	return std::wstring();
}

/** Converts the input wide string into a string.
@param	str			the input wide string.
@return				string version of the wide string. */
inline static std::string from_wideString(const std::wstring & wstr)
{
	if (!wstr.empty()) {
		if (const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL)) {
			std::string str(size_needed, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
			return str;
		}
	}
	return std::string();
}

/** Creates a shortcut file for the paths chosen.
@param	srcPath		path to the target file that the shortcut will link to.
@param	wrkPath		path to the working directory for the shortcut.
@param	dstPath		path to where the shortcut should be placed. */
inline static void create_shortcut(const std::string & srcPath, const std::string & wrkPath, const std::string & dstPath)
{
	IShellLink* psl;
	if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl))) {
		IPersistFile* ppf;

		// Set the path to the shortcut target and add the description. 
		psl->SetPath(srcPath.c_str());
		psl->SetWorkingDirectory(wrkPath.c_str());
		psl->SetIconLocation(srcPath.c_str(), 0);

		// Query IShellLink for the IPersistFile interface, used for saving the 
		// shortcut in persistent storage. 
		if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf))) {
			WCHAR wsz[MAX_PATH];

			// Ensure that the string is Unicode. 
			MultiByteToWideChar(CP_ACP, 0, (dstPath + ".lnk").c_str(), -1, wsz, MAX_PATH);

			// Save the link by calling IPersistFile::Save. 
			ppf->Save(wsz, TRUE);
			ppf->Release();
		}
		psl->Release();
	}
}

/** Increment a pointer's address by the offset provided.
@param	ptr			the pointer to increment by the offset amount.
@param	offset		the offset amount to apply to the pointer's address.
@return				the modified pointer address. */
inline static void * PTR_ADD(void *const ptr, const size_t & offset)
{
	return static_cast<std::byte*>(ptr) + offset;
};

/** Cleans up a target string representing a file path
@param	path		reference to the path to be sanitized. 
@return				sanitized version of path. */
inline static std::string sanitize_path(const std::string & path)
{
	std::string cpy(path);
	while (cpy.front() == '"' || cpy.front() == '\'' || cpy.front() == '\"' || cpy.front() == '\\')
		cpy.erase(0ull, 1ull);
	while (cpy.back() == '"' || cpy.back() == '\'' || cpy.back() == '\"' || cpy.back() == '\\')
		cpy.erase(cpy.size() - 1ull);
	return cpy;
}

/** Return file-info for all files within the directory specified.
@param	directory	the directory to retrieve file-info from.
@return				a vector of file information, including file names, sizes, meta-data, etc. */
inline static auto get_file_paths(const std::string & directory)
{
	std::vector<std::filesystem::directory_entry> paths;
	if (std::filesystem::is_directory(directory))
		for (const auto & entry : std::filesystem::recursive_directory_iterator(directory))
			if (entry.is_regular_file())
				paths.emplace_back(entry);
	return paths;
}

/** Retrieve the start menu directory.
@return		path to the user's start menu. */
inline static std::string get_users_startmenu()
{
	// Get the running directory
	char cPath[FILENAME_MAX];
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, cPath)))
		return std::string(cPath);
	return std::string();
}

/** Retrieve the desktop directory.
@return		path to the user's desktop. */
inline static std::string get_users_desktop()
{
	// Get the running directory
	char cPath[FILENAME_MAX];
	if (SHGetSpecialFolderPathA(HWND_DESKTOP, cPath, CSIDL_DESKTOP, FALSE))
		return std::string(cPath);
	return std::string();
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
	Log::PushText(message);
	system("pause");
	exit(EXIT_FAILURE);
}

/** Print a message to the console, and pause the program until the user presses any key.
@param	message		pause message to show the user. */
inline static void pause_program(const char * message)
{
	Log::PushText(std::string(message) + ' ');
	system("pause");
	std::printf("\033[A\33[2K\r");
	std::printf("\033[A\33[2K\r\n");
}

#endif // COMMON_H