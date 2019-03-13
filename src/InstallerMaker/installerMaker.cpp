#include "Archiver.h"
#include "Common.h"
#include <chrono>
#include <iostream>


/** Entry point. */
int main()
{
	const auto directory = get_current_directory();

	// Check if user is ready to compress
	std::cout << "Compress the current directory: \"" + directory + "\"\nInput (Y/N): ";
	char input('N');
	std::cin >> input;
	input = (char)toupper((int)input);
	if (input == 'Y') {
		// Package to an output file
		const auto start = std::chrono::system_clock::now();
		std::cout << "...working..." << std::endl;
		size_t fileCount(0ull), byteCount(0ull);
		if (Archiver::Pack(directory, fileCount, byteCount)) {
			// Success, report results
			const auto end = std::chrono::system_clock::now();
			const std::chrono::duration<double> elapsed_seconds = end - start;
			std::cout
				<< "Compression into \"" << directory << "\" complete.\n"
				<< "Files packaged: " << fileCount << "\n"
				<< "Bytes packaged: " << byteCount << "\n"
				<< "Elapsed time: " << elapsed_seconds.count() << " seconds\n";
		}		
	}

	// Exit
	system("pause");
	exit(1);
}