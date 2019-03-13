#include "Archiver.h"
#include "Common.h"
#include <chrono>
#include <iostream>


/** Entry point. */
int main()
{
	const auto directory = get_current_directory();

	// Check if user is ready to install
	std::cout << "Install to the current directory: \"" + directory + "\"\nInput (Y/N): ";
	char input('N');
	std::cin >> input;
	input = (char)toupper((int)input);
	if (input == 'Y') {
		// Unpackage using the resource file
		const auto start = std::chrono::system_clock::now();
		std::cout << "...working..." << std::endl;
		size_t fileCount(0ull), byteCount(0ull);
		if (Archiver::Unpack(directory, fileCount, byteCount)) {
			// Success, report results
			const auto end = std::chrono::system_clock::now();
			const std::chrono::duration<double> elapsed_seconds = end - start;
			std::cout
				<< "Decompression into \"" << directory << "\" complete.\n"
				<< "Files unpackaged: " << fileCount << "\n"
				<< "Bytes unpackaged: " << byteCount << "\n"
				<< "Elapsed time: " << elapsed_seconds.count() << " seconds\n";
		}
	}

	// Exit
	system("pause");
	exit(1);
}