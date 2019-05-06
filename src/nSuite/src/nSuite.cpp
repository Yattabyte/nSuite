#include "nSuite.h"
#include "Resource.h"
#include "Log.h"
#include "Threader.h"
#include "Progress.h"
#include <atomic>
#include <direct.h>
#include <filesystem>
#include <fstream>
#include <shlobj.h>
#include <vector>

using namespace NST;


std::vector<std::filesystem::directory_entry> NST::GetFilePaths(const std::string & directory)
{
	std::vector<std::filesystem::directory_entry> paths;
	if (std::filesystem::is_directory(directory))
		for (const auto & entry : std::filesystem::recursive_directory_iterator(directory))
			if (entry.is_regular_file())
				paths.emplace_back(entry);
	return paths;
}

std::string NST::GetStartMenuPath()
{
	char cPath[FILENAME_MAX];
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, cPath)))
		return std::string(cPath);
	return std::string();
}

std::string NST::GetDesktopPath()
{
	char cPath[FILENAME_MAX];
	if (SHGetSpecialFolderPathA(HWND_DESKTOP, cPath, CSIDL_DESKTOP, FALSE))
		return std::string(cPath);
	return std::string();
}

std::string NST::GetRunningDirectory()
{
	char cCurrentPath[FILENAME_MAX];
	if (_getcwd(cCurrentPath, sizeof(cCurrentPath)))
		cCurrentPath[sizeof(cCurrentPath) - 1ull] = char('\0');
	return std::string(cCurrentPath);
}

std::string NST::SanitizePath(const std::string & path)
{
	std::string cpy(path);
	while (cpy.front() == '"' || cpy.front() == '\'' || cpy.front() == '\"' || cpy.front() == '\\')
		cpy.erase(0ull, 1ull);
	while (cpy.back() == '"' || cpy.back() == '\'' || cpy.back() == '\"' || cpy.back() == '\\')
		cpy.erase(cpy.size() - 1ull);
	return cpy;
}