#include "Common.h"
#include "DirectoryTools.h"
#include "Resource.h"
#include <chrono>


/** Entry point. */
int main(int argc, char *argv[])
{
	// Check command line arguments
	std::string dstDirectory(get_current_directory());
	for (int x = 1; x < argc; ++x) {
		std::string command(argv[x], 5);
		std::transform(command.begin(), command.end(), command.begin(), ::tolower);
		if (command == "-dst=")
			dstDirectory = std::string(&argv[x][5]);
		else
			exit_program(
				"        Help:       /\n"
				" ~-----------------~\n"
				"/\n"
				" * if run without any arguments : uses application directory\n"
				" * use command -dst=[path] to specify the installation directory.\n\n"
			);
	}
	sanitize_path(dstDirectory);

	// Report an overview of supplied procedure
	std::cout
		<< "Unpacking into:\t\"" << dstDirectory << "\"\n\n";
	std::cout << "Ready to install?\n";
	system("pause");
	std::cout << std::endl;	
	
	// Acquire archive resource
	const auto start = std::chrono::system_clock::now();
	size_t fileCount(0ull), byteCount(0ull);
	Resource archive(IDR_ARCHIVE, "ARCHIVE");
	if (!archive.exists())
		exit_program("Cannot access archive resource (may be absent, corrupt, or have different identifiers), aborting...\n");
	
	// Unpackage using the resource file
	DRT::DecompressDirectory(dstDirectory, reinterpret_cast<char*>(archive.getPtr()), archive.getSize(), byteCount, fileCount);

	// Success, report results
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	std::cout
		<< "Files written:  " << fileCount << "\n"
		<< "Bytes written:  " << byteCount << "\n"
		<< "Total duration: " << elapsed_seconds.count() << " seconds\n\n";
	system("pause");
	exit(EXIT_SUCCESS);	
}