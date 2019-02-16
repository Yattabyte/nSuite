#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include "common.h"
#include "Threader.h"
#include "lz4.h"


int main()
{
	// Variables
	Threader threader;
	const auto megaPath = get_current_dir() + "\\archive.dat";
	const auto absolute_path_length = get_current_dir().size();
	const auto directoryArray = get_file_paths();
	struct FileData {
		std::string fullpath, trunc_path;
		size_t size, unitSize;
	};
	std::vector<FileData> files(directoryArray.size());

	// Threading logic
	std::cout << "Compress the current directory? (Y/N)" << std::endl;
	char input('N');
	std::cin >> input;
	input = toupper(input);
	if (input == 'Y') {	
		// Calculate final file size
		auto start = std::chrono::system_clock::now();
		size_t archiveSize(0), px(0);
		for each (const auto & entry in directoryArray) {
			std::string path = entry.path().string();
			if (path == megaPath ||
				(path.find("compressor.exe") != std::string::npos) ||
				(path.find("decompressor.exe") != std::string::npos))
				continue;
			path = path.substr(absolute_path_length, path.size() - absolute_path_length);
			size_t pathSize = path.size();
			
			const size_t unitSize = 
				size_t(sizeof(size_t)) +	// size of path size variable in bytes
				pathSize +					// the actual path data
				size_t(sizeof(size_t)) +	// size of the file size variable in bytes
				entry.file_size();			// the actual file data
			archiveSize += unitSize;
			files[px++] = { entry.path().string(), path, entry.file_size(), unitSize };
		}

		// Create buffer for final file data
		char * filebuffer = new char[archiveSize];

		// Modify buffer
		void * pointer = filebuffer;
		std::atomic_size_t filesRead(0);		
		for each (const auto & file in files) {
			// Increment the pointer address by the offset provided
			constexpr auto INCR_PTR = [](void *const ptr, const size_t & offset) {
				void * pointer = (void*)(reinterpret_cast<unsigned char*>(ptr) + offset);
				return pointer;
			};
			std::cout << "..." << file.trunc_path << std::endl;

			threader.addJob([&INCR_PTR, file, pointer, &filesRead]() {				
				// Write the total number of characters in the path string, into the archive
				void * ptr = pointer;
				auto pathSize = file.trunc_path.size();
				memcpy(ptr, reinterpret_cast<char*>(&pathSize), size_t(sizeof(size_t)));
				ptr = INCR_PTR(ptr, size_t(sizeof(size_t)));

				// Write the file path string, into the archive
				memcpy(ptr, file.trunc_path.data(), pathSize);
				ptr = INCR_PTR(ptr, pathSize);

				// Write the file size in bytes, into the archive
				auto fileSize = file.size;
				memcpy(ptr, reinterpret_cast<char*>(&fileSize), size_t(sizeof(size_t)));
				ptr = INCR_PTR(ptr, size_t(sizeof(size_t)));

				// Write the file into the archive
				std::ifstream fileOnDisk(file.fullpath, std::ios::binary | std::ios::beg);
				fileOnDisk.read(reinterpret_cast<char*>(ptr), fileSize);
				fileOnDisk.close();
				filesRead++;
			});

			pointer = INCR_PTR(pointer, file.unitSize);
		}

		// Wait for threaded operations to complete
		while (filesRead != files.size())
			continue;

		// Compress the buffer
		char * compressedBuffer = new char[archiveSize];
		auto result = LZ4_compress_default(
			filebuffer, 
			compressedBuffer, 
			int(archiveSize), 
			int(archiveSize)
		);

		// Write the entire compressed buffer to disk
		if (result > 0) {
			std::ofstream megafile(megaPath, std::ios::binary | std::ios::out);
			if (megafile.is_open()) {
				// First, write the full uncompressed file size
				megafile.write(reinterpret_cast<char*>(&archiveSize), size_t(sizeof(size_t)));
				megafile.write(compressedBuffer, result);
			}
			megafile.close();

			const auto end = std::chrono::system_clock::now();
			const std::chrono::duration<double> elapsed_seconds = end - start;
			std::cout
				<< "Compression into \"" << megaPath << "\" complete.\n"
				<< "Files read: " << filesRead << "/" << files.size() << "\n"
				<< "Bytes read: " << archiveSize << " (compressed to " << result << " bytes) \n"
				<< "Elapsed time: " << elapsed_seconds.count() << "\n";
		}

		// Clean up
		delete[] filebuffer;
		delete[] compressedBuffer;
	}
	system("pause");
	exit(1);
}