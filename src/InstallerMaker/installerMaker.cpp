#include "Archiver.h"
#include "Common.h"
#include "Resource.h"
#include <algorithm>
#include <chrono>
#include <fstream>


/** Displays help information about this program, then terminates it. */
static void display_help_and_exit()
{
	exit_program("\n"
		"Help:\n"
		"~-~-~-~-~-~-~-~-~-~-/\n"
		" * if run without any arguments : uses application directory\n"
		" * use command -ovrd to skip user-ready prompt.\n"
		" * use command -src=[path] to specify a directory to package into an installer.\n"
		" * use command -dst=[path] to specify a directory to write the installer.\n"
	);
}

/** Entry point. */
int main(int argc, char *argv[])
{
	// Check command line arguments
	bool skipPrompt = false;
	std::string srcDirectory(get_current_directory()), dstDirectory("");
	for (int x = 1; x < argc; ++x) {		
		std::string command(argv[x], 5);
		std::transform(command.begin(), command.end(), command.begin(), ::tolower);
		if (command == "-ovrd")
			skipPrompt = true;
		else if (command == "-src=")
			srcDirectory = std::string(&argv[x][5]);
		else if (command == "-dst=") 
			dstDirectory = std::string(&argv[x][5]);		
		else 
			display_help_and_exit();
	}
	// Set dstDirectory to src directory if it wasn't explicitly set. 
	// Done after args are processed, in case srcDirectory was set as an arg.
	if (dstDirectory == "")
		dstDirectory = srcDirectory + "\\installer.exe";
	sanitize_path(srcDirectory);
	sanitize_path(dstDirectory);
	
	// Report an overview of supplied procedure
	std::cout
		<< "Packaging:\t\"" << srcDirectory << "\"\n"
		<< "Into:\t\t\"" << dstDirectory << "\"\n\n";

	// See if we should skip the user-ready prompt
	if (!skipPrompt) {
		std::cout << "Ready to generate installer?\n";
		system("pause");
		std::cout << std::endl;
	}

	// Package to an output file
	const auto start = std::chrono::system_clock::now();
	size_t fileCount(0ull), byteCount(0ull);
	char * archiveBuffer(nullptr);
	if (!Archiver::Pack(srcDirectory, fileCount, byteCount, &archiveBuffer))
		exit_program("Packaging failed, aborting...\n");

	// Acquire installer resource
	Resource installer(IDR_INSTALLER, "INSTALLER");
	if (!installer.exists())
		exit_program("Cannot access installer resource, aborting...\n");

	// Write installer to disk
	std::ofstream file(dstDirectory, std::ios::binary | std::ios::out);
	if (!file.is_open())
		exit_program("Cannot write installer to disk, aborting...\n");
	file.write(reinterpret_cast<char*>(installer.getPtr()), (std::streamsize)installer.getSize());
	file.close();

	// Update installer's resource
	auto handle = BeginUpdateResource(dstDirectory.c_str(), true);
	if (!(bool)UpdateResource(handle, "ARCHIVE", MAKEINTRESOURCE(IDR_ARCHIVE), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), archiveBuffer, (DWORD)byteCount))
		exit_program("Cannot write archive contents to the installer, aborting...\n");
	EndUpdateResource(handle, FALSE);
	delete[] archiveBuffer;

	// Success, report results
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	std::cout
		<< "Files packaged: " << fileCount << "\n"
		<< "Bytes packaged: " << byteCount << "\n"
		<< "Total duration: " << elapsed_seconds.count() << " seconds\n\n";
	system("pause");
	exit(EXIT_SUCCESS);
}