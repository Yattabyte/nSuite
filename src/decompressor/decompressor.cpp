#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include "resource.h"
#include "common.h"
#include "Archive.h"
#include "Threader.h"


int main()
{
	// Variables	
	Threader threader;
	Archive archive(IDR_ARCHIVE, "ARCHIVE");

	std::cout << "Install to the current directory? (Y/N)" << std::endl;
	char input('N');
	std::cin >> input;
	input = toupper(input);
	if (input == 'Y') {
		// Start decompression
		const auto start = std::chrono::system_clock::now();
		void * archivePtr = nullptr;
		if (archive.uncompressArchive(&archivePtr)) {
			// Read the archive
			size_t filesRead(0), bytesRead(0), uncompressedSize(archive.getUncompressedSize());
			std::atomic_size_t filesFinished(0);
			while (bytesRead < uncompressedSize) {
				// Increment the pointer address by the offset provided
				constexpr auto INCR_PTR = [](void * archivePtr, const size_t & offset) {
					void * pointer = (void*)(reinterpret_cast<unsigned char*>(archivePtr) + offset);
					return pointer;
				};

				// Read the total number of characters from the path string, from the archive
				const auto pathSize = *reinterpret_cast<size_t*>(archivePtr);
				archivePtr = INCR_PTR(archivePtr, size_t(sizeof(size_t)));

				// Read the file path string, from the archive
				const char * path_array = reinterpret_cast<char*>(archivePtr);
				const auto path = get_current_dir() + std::string(path_array, pathSize);
				archivePtr = INCR_PTR(archivePtr, pathSize);

				// Read the file size in bytes, from the archive 
				const auto fileSize = *reinterpret_cast<size_t*>(archivePtr);
				archivePtr = INCR_PTR(archivePtr, size_t(sizeof(size_t)));

				// Write file out to disk, from the archive
				threader.addJob([archivePtr, path, fileSize, &filesFinished]() {
					std::filesystem::create_directories(std::filesystem::path(path).parent_path());
					std::ofstream file(path, std::ios::binary | std::ios::out);
					if (file.is_open())
						file.write(reinterpret_cast<char*>(archivePtr), fileSize);
					file.close();
					filesFinished++;
				});

				archivePtr = INCR_PTR(archivePtr, fileSize);
				bytesRead += size_t(sizeof(size_t)) + pathSize + size_t(sizeof(size_t)) + fileSize;
				filesRead++;
				std::cout << "..." << std::string(path_array, pathSize) << std::endl;
			}

			// Wait for threaded operations to complete
			while (filesRead != filesFinished)
				continue;

			const auto end = std::chrono::system_clock::now();
			const std::chrono::duration<double> elapsed_seconds = end - start;
			std::cout 
				<< "Decompression into \"" << get_current_dir() << "\" complete.\n"
				<< "Files written: " << filesFinished << "/" << filesRead << "\n"
				<< "Bytes written: " << bytesRead << "/" << uncompressedSize << "\n"
				<< "Elapsed time: " << elapsed_seconds.count() << "\n";
		}
	}
	system("pause");
	exit(1);
}