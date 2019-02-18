#include "Archiver.h"
#include <chrono>
#include <direct.h>
#include <iostream>
#include <string>


/** Get the current directory for this executable. */
static std::string get_current_dir()
{
	char cCurrentPath[FILENAME_MAX];
	if (_getcwd(cCurrentPath, sizeof(cCurrentPath)))
		cCurrentPath[sizeof(cCurrentPath) - 1l] = char('/0');
	return std::string(cCurrentPath);
}

/** Entry point. */
int main()
{
	// Check if user is ready to install
	const auto directory = get_current_dir();
	std::cout << "Install to the current directory: \"" + directory + "\"\nInput (Y/N): ";
	char input('N');
	std::cin >> input;
	input = toupper(input);
	if (input == 'Y') {
		std::cout << "...working..." << std::endl;
		// Unpackage using the resource file
		const auto start = std::chrono::system_clock::now();
		size_t fileCount(0), byteCount(0);
		if (Archiver::unpack(directory, fileCount, byteCount)) {
			// Success, report results
			const auto end = std::chrono::system_clock::now();
			const std::chrono::duration<double> elapsed_seconds = end - start;
			std::cout
				<< "Decompression into \"" << directory << "\" complete.\n"
				<< "Files written: " << fileCount << "\n"
				<< "Bytes written: " << byteCount << "\n"
				<< "Elapsed time: " << elapsed_seconds.count() << "\n";
		}
	}

	// Exit
	system("pause");
	exit(1);
}