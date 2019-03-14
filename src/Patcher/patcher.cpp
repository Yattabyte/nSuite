#include "BufferTools.h"
#include "Common.h"
#include <chrono>
#include <direct.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>


/** Entry point. */
int main()
{
	// Get the proper path names, and read the source files into their appropriate buffers.
	const auto directory = get_current_directory();
	const auto start = std::chrono::system_clock::now();
	const auto path_old = directory + "\\old.exe", path_new = directory + "\\new-patched.exe", path_diff = directory + "\\patch.diff";
	const auto size_old = std::filesystem::file_size(path_old), size_diff = std::filesystem::file_size(path_diff);
	std::ifstream file_old(path_old, std::ios::binary | std::ios::beg), file_diff(path_diff, std::ios::binary | std::ios::beg);
	char * buffer_old = new char[size_old], * buffer_diff = new char[size_diff];
	if (file_old.is_open() && file_diff.is_open()) {
		file_old.read(buffer_old, std::streamsize(size_old));
		file_diff.read(buffer_diff, std::streamsize(size_diff));
		file_old.close();
		file_diff.close();
	}

	// Get the 'old' hash, compare it against the patch to make sure it's valid
	const size_t hash_old = BFT::HashBuffer(buffer_old, size_old);
	const size_t hash_patch_old = *reinterpret_cast<size_t*>(buffer_diff);
	const size_t hash_patch_new = *reinterpret_cast<size_t*>(&buffer_diff[sizeof(size_t)]);

	if (hash_old == hash_patch_old) {
		char * buffer_new(nullptr);
		size_t size_new(0ull), instructionCount(0ull);
		if (BFT::PatchBuffer(buffer_old, size_old, &buffer_new, size_new, &buffer_diff[sizeof(size_t)*2ull], size_diff - (sizeof(size_t) * 2ull), &instructionCount)) {
			// Compare hash of the new file
			const size_t hash_new = BFT::HashBuffer(buffer_new, size_new);
			if (hash_new == hash_patch_new) {
				// Write-out patched file to disk
				std::filesystem::create_directories(std::filesystem::path(path_new).parent_path());
				std::ofstream file(path_new, std::ios::binary | std::ios::out);
				if (file.is_open())
					file.write(buffer_new, (std::streamsize)(size_new));
				file.close();
				delete[] buffer_new;

				// Output results
				const auto end = std::chrono::system_clock::now();
				const std::chrono::duration<double> elapsed_seconds = end - start;
				std::cout
					<< "Successfully patched file.\n"
					<< "Patch instructions: " << instructionCount << "\n"
					<< "Patch size: " << size_diff << " bytes\n"
					<< "Elapsed time: " << elapsed_seconds.count() << " seconds\n";
			}
			else
				std::cout << "Patching resulted in a different file than expected (destination hash mismatch), aborting...\n";
		}
		else
			std::cout << "Failed processing patch file.\n";
	}
	else
		std::cout << "Cannot apply the patch, it affects a different version or a different file (source hash mismatch).\n";	

	// Clean-up and exit
	delete[] buffer_old;
	delete[] buffer_diff;
	system("pause");
	exit(1);
}