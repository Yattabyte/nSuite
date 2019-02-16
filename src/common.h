#include <direct.h>
#include <filesystem>
#include <vector>


/** Get the current directory for this executable. */
static std::string get_current_dir()
{
	char cCurrentPath[FILENAME_MAX];
	if (_getcwd(cCurrentPath, sizeof(cCurrentPath)))
		cCurrentPath[sizeof(cCurrentPath) - 1l] = char('/0');
	return std::string(cCurrentPath);
}

/** Return info for all files within this working directory. */
static std::vector<std::filesystem::directory_entry> get_file_paths()
{
	std::vector<std::filesystem::directory_entry> paths;
	for (const auto & entry : std::filesystem::recursive_directory_iterator(get_current_dir()))
		if (entry.is_regular_file())
			paths.push_back(entry);
	return paths;
}