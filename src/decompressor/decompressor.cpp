#include <chrono>
#include <iostream>
#include <string>
#include "resource.h"
#include "common.h"
#include "Archive.h"


int main()
{
	// Check if user is ready to install
	std::cout << "Install to the current directory? (Y/N)" << std::endl;
	char input('N');
	std::cin >> input;
	input = toupper(input);
	if (input == 'Y') {
		// Unpackage using the resource file
		const auto start = std::chrono::system_clock::now();
		Archive archive(IDR_ARCHIVE, "ARCHIVE");
		size_t fileCount(0), byteCount(0);
		if (archive.unpack(fileCount, byteCount)) {
			// Success, report results
			const auto end = std::chrono::system_clock::now();
			const std::chrono::duration<double> elapsed_seconds = end - start;
			std::cout
				<< "Decompression into \"" << get_current_dir() << "\" complete.\n"
				<< "Files written: " << fileCount << "\n"
				<< "Bytes written: " << byteCount << "\n"
				<< "Elapsed time: " << elapsed_seconds.count() << "\n";
		}
	}

	// Exit
	system("pause");
	exit(1);
}