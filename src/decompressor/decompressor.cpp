#include <direct.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include "resource.h"
#include "Windows.h"


class Archive {
public:
	struct Parameters {
		std::size_t size_bytes = 0;
		void* ptr = nullptr;
	};


private:
	HRSRC hResource = nullptr;
	HGLOBAL hMemory = nullptr;

	Parameters p;


public:
	Archive(int resource_id, const std::string &resource_class) {
		hResource = FindResourceA(nullptr, MAKEINTRESOURCEA(resource_id), resource_class.c_str());
		hMemory = LoadResource(nullptr, hResource);

		p.size_bytes = SizeofResource(nullptr, hResource);
		p.ptr = LockResource(hMemory);
	}

	auto GetResource() const {
		return p.ptr;
	}

	auto GetSize() const {
		return p.size_bytes;
	}
};

static std::string get_current_dir()
{
	char cCurrentPath[FILENAME_MAX];
	if (_getcwd(cCurrentPath, sizeof(cCurrentPath)))
		cCurrentPath[sizeof(cCurrentPath) - 1l] = char('/0');
	return std::string(cCurrentPath);
}

int main()
{
	std::cout << "Install to the current directory? (Y/N)" << std::endl;
	char input('N');
	std::cin >> input;
	input = toupper(input);
	if (input == 'Y') {
		Archive archive(IDR_ARCHIVE, "ARCHIVE");
		void * megafile = archive.GetResource();
		const auto megaSize = (size_t)archive.GetSize();
		size_t bytesRead(0);
		size_t filesRead(0);
		constexpr auto offsetPtr = [](void * ptr, size_t offset) {
			return (void*)(reinterpret_cast<unsigned char*>(ptr) + unsigned(offset));
		};
		while (bytesRead < megaSize) {
			// Read file path size from mega file
			const size_t * pathSize = reinterpret_cast<size_t*>(offsetPtr(megafile, bytesRead));
			bytesRead += size_t(sizeof(size_t));

			// Read file path from mega file
			const char * path_array = reinterpret_cast<char*>(offsetPtr(megafile, bytesRead));
			bytesRead += *pathSize;
			const std::string path = get_current_dir() + std::string(path_array, *pathSize);

			// Read file size to mega file
			const size_t fileSize = *reinterpret_cast<size_t*>(offsetPtr(megafile, bytesRead));
			bytesRead += size_t(sizeof(size_t));

			// Copy file from mega file
			const char * file_array = reinterpret_cast<char*>(offsetPtr(megafile, bytesRead));
			bytesRead += fileSize;
			std::filesystem::create_directories(std::filesystem::path(path).parent_path());
			std::ofstream file(path, std::ios::binary | std::ios::out);
			if (file.is_open())
				file.write(file_array, fileSize);
			file.close();

			filesRead++;
			std::cout << "..." << std::string(path_array, *pathSize) << std::endl;
		}
		std::cout << "Decompressed " << filesRead << " files (" << bytesRead << " out of " << megaSize << " bytes)" << std::endl;
	}
	system("pause");
	exit(1);
}