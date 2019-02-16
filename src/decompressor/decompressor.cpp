#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include "resource.h"
#include "common.h"
#include "lz4.h"


int main()
{
	// Variables	
	Threader threader;
	Archive archive(IDR_ARCHIVE, "ARCHIVE");
	auto archivePtr = archive.m_ptr;
	const auto compressedSize = archive.m_size;
	const auto uncompressedSize = *reinterpret_cast<size_t*>(archivePtr);
	archivePtr = reinterpret_cast<unsigned char*>(archivePtr) + unsigned(size_t(sizeof(size_t)));

	std::cout << "Install to the current directory? (Y/N)" << std::endl;
	char input('N');
	std::cin >> input;
	input = toupper(input);
	if (input == 'Y') {
		// Start decompression
		const auto start = std::chrono::system_clock::now();
		char * uncompressedBuffer = new char[uncompressedSize];
		auto result = LZ4_decompress_safe(
			reinterpret_cast<char*>(archivePtr), 
			uncompressedBuffer,
			int(compressedSize - size_t(sizeof(size_t))),
			int(uncompressedSize)
		);

		// Read the buffer
		size_t bytesRead(0), filesRead(0);
		std::atomic_size_t filesFinished(0);
		void * ptr = uncompressedBuffer;
		while (bytesRead < uncompressedSize) {
			// Increment the pointer address by the offset provided
			constexpr auto INCR_PTR = [](void * ptr, const size_t & offset) {
				void * pointer = (void*)(reinterpret_cast<unsigned char*>(ptr) + offset);
				return pointer;
			};
			
			// Read file path size from mega file
			const auto pathSize = *reinterpret_cast<size_t*>(ptr);
			ptr = INCR_PTR(ptr, size_t(sizeof(size_t)));

			// Read file path from mega file
			const char * path_array = reinterpret_cast<char*>(ptr);
			const auto path = get_current_dir() + std::string(path_array, pathSize);
			ptr = INCR_PTR(ptr, pathSize);

			// Read file size to mega file
			const size_t fileSize = *reinterpret_cast<size_t*>(ptr);
			ptr = INCR_PTR(ptr, size_t(sizeof(size_t)));

			// Write file out to disk
			threader.addJob([ptr, path, fileSize, &filesFinished]() {	
				std::filesystem::create_directories(std::filesystem::path(path).parent_path());
				std::ofstream file(path, std::ios::binary | std::ios::out);
				if (file.is_open())
					file.write(reinterpret_cast<char*>(ptr), fileSize);
				file.close();
				filesFinished++;
			});

			ptr = INCR_PTR(ptr, fileSize);
			bytesRead += size_t(sizeof(size_t)) + pathSize + size_t(sizeof(size_t)) + fileSize;
			filesRead++;
			std::cout << "..." << std::string(path_array, pathSize) << std::endl;
		}

		// Wait for threaded operations to complete
		while (filesRead != filesFinished)
			continue;

		/*while (bytesRead < uncompressedSize) {
			// Read file path size from mega file
			const size_t * pathSize = reinterpret_cast<size_t*>(offsetPtr(uncompressedBuffer, bytesRead));
			bytesRead += size_t(sizeof(size_t));

			// Read file path from mega file
			const char * path_array = reinterpret_cast<char*>(offsetPtr(uncompressedBuffer, bytesRead));
			bytesRead += *pathSize;
			const std::string path = get_current_dir() + std::string(path_array, *pathSize);

			// Read file size to mega file
			const size_t fileSize = *reinterpret_cast<size_t*>(offsetPtr(uncompressedBuffer, bytesRead));
			bytesRead += size_t(sizeof(size_t));

			// Copy file from mega file
			const char * file_array = reinterpret_cast<char*>(offsetPtr(uncompressedBuffer, bytesRead));
			bytesRead += fileSize;
			std::filesystem::create_directories(std::filesystem::path(path).parent_path());
			std::ofstream file(path, std::ios::binary | std::ios::out);
			if (file.is_open())
				file.write(file_array, fileSize);
			file.close();

			filesRead++;
			std::cout << "..." << std::string(path_array, *pathSize) << std::endl;
		}*/
		const auto end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		std::cout 
			<< "Decompression into \"" << get_current_dir() << "\" complete.\n"
			<< "Files written: " << filesFinished << "/" << filesRead << "\n"
			<< "Bytes written: " << bytesRead << "/" << uncompressedSize << "\n"
			<< "Elapsed time: " << elapsed_seconds.count() << "\n";

		// Clean up
		delete[] uncompressedBuffer;
	}
	system("pause");
	exit(1);
}