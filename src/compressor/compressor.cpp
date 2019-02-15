#include <atomic>
#include <chrono>
#include <deque>
#include <direct.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <shared_mutex>
#include <string>
#include <thread>
#include <memory>
#include <functional>
#include <future>
#include "lz4.h"


/** Retrieve the working directory for this executable file. */
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

int main()
{
	// Variables
	const auto megaPath = get_current_dir() + "\\archive.dat";
	const auto absolute_path_length = get_current_dir().size();
	const auto directoryArray = get_file_paths();
	struct FileData {
		std::string fullpath, trunc_path;
		size_t size, unitSize;
	};
	std::vector<FileData> files(directoryArray.size());

	// Threading logic
	std::shared_mutex jobMutex;
	std::deque<std::function<void()>> jobs;
	std::vector<std::thread> threads;
	for (size_t x = 0; x < std::thread::hardware_concurrency(); ++x) {
		std::thread thread([&jobMutex, &jobs]() {
			while (true) {
				// Check if there is a job to do
				std::unique_lock<std::shared_mutex> writeGuard(jobMutex);
				if (jobs.size()) {
					// Get the first job, remove it from the list
					auto job = jobs.front();
					jobs.pop_front();
					// Unlock
					writeGuard.unlock();
					writeGuard.release();
					// Do Job
					job();
				}
			}
		});
		thread.detach();
		threads.emplace_back(std::move(thread));
	}

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
				continue; // Ignore the mega file we're creating
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

			std::unique_lock<std::shared_mutex> writeGuard(jobMutex);
			jobs.push_back([&INCR_PTR, file, pointer, &filesRead]() {				
				// Write the size in bytes, of the file path string, into the buffer
				void * ptr = pointer;
				auto pathSize = file.trunc_path.size();
				memcpy(ptr, reinterpret_cast<char*>(&pathSize), size_t(sizeof(size_t)));
				ptr = INCR_PTR(ptr, size_t(sizeof(size_t)));

				// Write the file path string into the buffer
				memcpy(ptr, file.trunc_path.data(), pathSize);
				ptr = INCR_PTR(ptr, pathSize);

				// Write the size in bytes of the file, into the buffer
				auto fileSize = file.size;
				memcpy(ptr, reinterpret_cast<char*>(&fileSize), size_t(sizeof(size_t)));
				ptr = INCR_PTR(ptr, size_t(sizeof(size_t)));

				// Copy file to mega file
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

		char * compressedBuffer = new char[archiveSize];
		auto result = LZ4_compress_default(
			filebuffer, 
			compressedBuffer, 
			int(archiveSize), 
			int(archiveSize)
		);

		// Write entire buffer to disk
		if (result > 0) {
			std::ofstream megafile(megaPath, std::ios::binary | std::ios::out);
			if (megafile.is_open()) {
				// First, write the full uncompressed file size
				megafile.write(reinterpret_cast<char*>(&archiveSize), size_t(sizeof(size_t)));
				megafile.write(compressedBuffer, result);
			}
			megafile.close();

			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = end - start;
			std::cout
				<< "Compression into \"" << megaPath << "\" complete.\n"
				<< "Files read: " << filesRead << "/" << files.size() << "\n"
				<< "Bytes read: " << archiveSize << " (compressed to " << result << " bytes) \n"
				<< "Elapsed time: " << elapsed_seconds.count() << "\n";
		}

		// Clean up
		threads.clear();
		delete[] filebuffer;
		delete[] compressedBuffer;
	}
	system("pause");
	exit(1);
}