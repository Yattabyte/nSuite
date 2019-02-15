#include <atomic>
#include <chrono>
#include <deque>
#include <direct.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <shared_mutex>
#include <string>
#include "resource.h"
#include "Windows.h"
#include "lz4.h"


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
	
	// Get archive, first data = uncompressed size
	Archive archive(IDR_ARCHIVE, "ARCHIVE");
	auto archivePtr = archive.GetResource();
	const auto compressedSize = (size_t)archive.GetSize();
	const auto uncompressedSize = *reinterpret_cast<size_t*>(archivePtr);
	archivePtr = reinterpret_cast<unsigned char*>(archivePtr) + unsigned(size_t(sizeof(size_t)));

	std::cout << "Install to the current directory? (Y/N)" << std::endl;
	char input('N');
	std::cin >> input;
	input = toupper(input);
	if (input == 'Y') {
		// Start decompression
		auto start = std::chrono::system_clock::now();
		char * uncompressedBuffer = new char[uncompressedSize];
		auto result = LZ4_decompress_safe(
			reinterpret_cast<char*>(archivePtr), 
			uncompressedBuffer,
			int(compressedSize - size_t(sizeof(size_t))),
			int(uncompressedSize)
		);

		size_t bytesRead(0);
		size_t filesRead(0);
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
			std::unique_lock<std::shared_mutex> writeGuard(jobMutex);
			jobs.push_back([ptr, path, fileSize, &filesFinished]() {	
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
		auto end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		std::cout 
			<< "Decompression into \"" << get_current_dir() << "\" complete.\n"
			<< "Files written: " << filesFinished << "/" << filesRead << "\n"
			<< "Bytes written: " << bytesRead << "/" << uncompressedSize << "\n"
			<< "Elapsed time: " << elapsed_seconds.count() << "\n";

		// Cleanup
		threads.clear();
		delete[] uncompressedBuffer;
	}
	system("pause");
	exit(1);
}