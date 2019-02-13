#include <direct.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>


static std::string get_current_dir() 
{
	char cCurrentPath[FILENAME_MAX];
	if (_getcwd(cCurrentPath, sizeof(cCurrentPath)))
		cCurrentPath[sizeof(cCurrentPath) - 1l] = char('/0');
	return std::string(cCurrentPath);
}

static std::vector<std::filesystem::directory_entry> get_file_paths()
{
	std::vector<std::filesystem::directory_entry> paths;
	for (const auto & entry : std::filesystem::recursive_directory_iterator(get_current_dir())) 
		if (entry.is_regular_file())
			paths.push_back(entry);	
	return paths;
}

int main()
{
	std::cout << "Compress the current directory? (Y/N)" << std::endl;
	char input('N');
	std::cin >> input;
	input = toupper(input);
	if (input == 'Y') {
		const auto megaPath = get_current_dir() + "\\archive.dat";
		std::ofstream megafile(megaPath, std::ios::binary | std::ios::out);
		size_t bytesWritten(0);
		size_t filesWritten(0);
		if (megafile.is_open()) {
			const auto absolute_path_length = get_current_dir().size();
			for each (const auto & entry in get_file_paths()) {
				// Write file path size to mega file
				std::string path = entry.path().string();
				if (path == megaPath || 
					(path.find("compressor.exe") != std::string::npos) || 
					(path.find("decompressor.exe") != std::string::npos))
					continue; // Ignore the mega file we're creating
				path = path.substr(absolute_path_length, path.size() - absolute_path_length);
				size_t pathSize = path.size();
				megafile.write(reinterpret_cast<char*>(&pathSize), sizeof(size_t));
				bytesWritten += size_t(sizeof(size_t));

				// Write file path to mega file
				megafile.write(path.data(), pathSize);
				bytesWritten += pathSize;

				// Write file size to mega file
				std::ifstream file(entry, std::ios::binary | std::ios::ate);
				std::ifstream::pos_type pos = file.tellg();
				size_t fileSize = size_t(pos);
				megafile.write(reinterpret_cast<char*>(&fileSize), sizeof(size_t));
				bytesWritten += size_t(sizeof(size_t));

				// Copy file to mega file
				std::vector<char> file_binaryData(fileSize);
				file.seekg(0, std::ios::beg);
				file.read(&file_binaryData[0], fileSize);
				megafile.write(file_binaryData.data(), fileSize);
				bytesWritten += fileSize;

				filesWritten++;
				std::cout << "..." << path << std::endl;
			}
		}
		megafile.close();
		std::cout << "Compressed " << filesWritten << " files (" << bytesWritten << " bytes) into \"" << megaPath << "\"" << std::endl;
	}
	system("pause");
	exit(1);
}