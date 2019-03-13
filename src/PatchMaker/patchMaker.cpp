#include "Archiver.h"
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
	const auto path_old = directory + "\\old.exe", path_new = directory + "\\new.exe", path_diff = directory + "\\patch.diff";
	const auto size_old = std::filesystem::file_size(path_old), size_new = std::filesystem::file_size(path_new);
	std::ifstream file_old(path_old, std::ios::binary | std::ios::beg), file_new(path_new, std::ios::binary | std::ios::beg);
	char * buffer_old = new char[size_old], * buffer_new = new char[size_new];
	if (file_old.is_open() && file_new.is_open()) {
		file_old.read(buffer_old, std::streamsize(size_old));
		file_new.read(buffer_new, std::streamsize(size_new));
		file_old.close();
		file_new.close();
	}
	
	char * buffer_diff(nullptr);
	size_t size_diff(0ull), instructionCount(0ull);
	if (BFT::DiffBuffers(buffer_old, size_old, buffer_new, size_new, &buffer_diff, size_diff, &instructionCount)) {
		// Write-out compressed diff to disk
		std::filesystem::create_directories(std::filesystem::path(path_diff).parent_path());
		std::ofstream file(path_diff, std::ios::binary | std::ios::out);
		if (file.is_open())
			file.write(buffer_diff, std::streamsize(size_diff));
		file.close();		
		delete[] buffer_diff;

		// Output results
		const auto end = std::chrono::system_clock::now();
		const std::chrono::duration<double> elapsed_seconds = end - start;
		std::cout
			<< "Successfully created diff file.\n"
			<< "Diff instructions: " << instructionCount << "\n"
			<< "Diff size: " << size_diff << " bytes\n"
			<< "Elapsed time: " << elapsed_seconds.count() << " seconds\n";
	}
	else
		std::cout << "Failed creating compressed diff file.\n";

	// Clean-up and exit
	delete[] buffer_old;
	delete[] buffer_new;
	system("pause");
	exit(1);
}