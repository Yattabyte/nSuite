#include "Archiver.h"
#include "Common.h"
#include "Resource.h"
#include <chrono>


/** Displays help information about this program, then terminates it. */
static void display_help_and_exit()
{
	exit_program(
		"Help:\n"
		"~-~-~-~-~-~-~-~-~-~-/\n"
		" * if run without any arguments : uses application directory\n"
		" * use command -ovrd to skip user-ready prompt.\n"
		" * use command -dst=[path] to specify the installation directory.\n"
	);
}

/** Entry point. */
int main(int argc, char *argv[])
{
	// Check command line arguments
	bool skipPrompt = false;
	std::string dstDirectory(get_current_directory());
	for (int x = 1; x < argc; ++x) {
		std::string command(argv[x], 5);
		std::transform(command.begin(), command.end(), command.begin(), ::tolower);
		if (command == "-ovrd")
			skipPrompt = true;
		else if (command == "-dst=")
			dstDirectory = std::string(&argv[x][5]);
		else
			display_help_and_exit();
	}
	sanitize_path(dstDirectory);

	// Report an overview of supplied procedure
	std::cout
		<< "Unpacking into:\t\"" << dstDirectory << "\"\n\n";

	// See if we should skip the user-ready prompt
	if (!skipPrompt) {
		std::cout << "Ready to install?\n";
		system("pause");
		std::cout << std::endl;
	}
	
	// Acquire archive resource
	const auto start = std::chrono::system_clock::now();
	size_t fileCount(0ull), byteCount(0ull);
	Resource archive(IDR_ARCHIVE, "ARCHIVE");
	if (!archive.exists())
		exit_program("Cannot access archive resource (may be absent, corrupt, or have different identifiers), aborting...\n");
	
	// Unpackage using the resource file
	if (!Archiver::Unpack(dstDirectory, fileCount, byteCount, reinterpret_cast<char*>(archive.getPtr()), archive.getSize()))
		exit_program("Unpackaging failed, aborting...\n");

	// Success, report results
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	std::cout
		<< "Files unpackaged: " << fileCount << "\n"
		<< "Bytes unpackaged: " << byteCount << "\n"
		<< "Elapsed time: " << elapsed_seconds.count() << " seconds\n";
	system("pause");
	exit(EXIT_SUCCESS);	
}