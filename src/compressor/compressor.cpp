#include <chrono>
#include <iostream>
#include <string>
#include "common.h"
#include "Archive.h"


int main()
{
	// Check if user is ready to compress
	std::cout << "Compress the current directory? (Y/N)" << std::endl;
	char input('N');
	std::cin >> input;
	input = toupper(input);
	if (input == 'Y') {
		// Package to an output file
		const auto start = std::chrono::system_clock::now();
		size_t fileCount(0), byteCount(0);
		if (Archive::pack(fileCount, byteCount)) {
			// Success, report results
			const auto end = std::chrono::system_clock::now();
			const std::chrono::duration<double> elapsed_seconds = end - start;
			std::cout
				<< "Compression into \"" << get_current_dir() << "\" complete.\n"
				<< "Files read: " << fileCount << "\n"
				<< "Bytes read: " << byteCount << "\n"
				<< "Elapsed time: " << elapsed_seconds.count() << "\n";
		}		
	}

	// Exit
	system("pause");
	exit(1);
}