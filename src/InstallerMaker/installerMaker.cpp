#include "Archiver.h"
#include "Common.h"
#include "Resource.h"
#include <chrono>
#include <fstream>
#include <iostream>


/** Specify a message to print to the terminal before closing the program. 
@param	message		the message to write-out.*/
inline static void exit_program(const char * message)
{
	std::cout << message;
	system("pause");
	exit(EXIT_FAILURE);
}

/** Displays help information about this program, then terminates it. */
inline static void display_help_and_exit()
{
	exit_program(
		"Usage:\n\n"
		" * if run without any arguments : uses application directory\n"
		" * use command -ovrd to skip user-ready prompt.\n"
		" * use command -src=[path] to specify the directory to package into an installer.\n"
		" * use command -dst=[path] to specify the directory to write the installer.\n"
	);
}

/** Entry point. */
int main(int argc, char *argv[])
{
	// Check command line arguments
	bool skipPrompt = false;
	std::string srcDirectory(""), dstDirectory("");
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
	
	// If no (usable) arguments, use current running directory
	if (srcDirectory == "" || argc == 1) 
		srcDirectory = get_current_directory();
	sanitize_path(srcDirectory);
	
	// Allow null destination directory, will just make it the source directory
	if (dstDirectory == "")
		dstDirectory = srcDirectory;
	dstDirectory += "\\installer.exe";
	sanitize_path(dstDirectory);
	
	// Report an overview of supplied procedure
	std::cout
		<< "Packaging:\t\"" << srcDirectory << "\"\n"
		<< "Into:\t\t\"" << dstDirectory << "\"\n\n";

	// See if we should skip the user-ready prompt
	if (!skipPrompt) {
		std::cout << "Ready?\n";
		system("pause");
		std::cout << std::endl;
	}

	// Package to an output file
	const auto start = std::chrono::system_clock::now();
	size_t fileCount(0ull), byteCount(0ull);
	char * archiveBuffer(nullptr);
	if (!Archiver::Pack(srcDirectory, fileCount, byteCount, &archiveBuffer))
		exit_program("Packaging failed, aborting...");
	else {
		// Write installer to disk
		Resource installer(IDR_INSTALLER, "INSTALLER");
		if (!installer.exists())
			exit_program("Cannot access installer resource, aborting...");
		else {
			std::ofstream file(dstDirectory, std::ios::binary | std::ios::out);
			if (!file.is_open())
				exit_program("Cannot write installer to disk, aborting...");
			file.write(reinterpret_cast<char*>(installer.getPtr()), (std::streamsize)installer.getSize());
			file.close();

			// Update installer's resource
			auto handle = BeginUpdateResource(dstDirectory.c_str(), true);
			if (!(bool)UpdateResource(handle, "ARCHIVE", MAKEINTRESOURCE(IDR_ARCHIVE), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), archiveBuffer, (DWORD)byteCount))
				exit_program("Cannot write archive contents to the installer, aborting...");
			EndUpdateResource(handle, FALSE);
			delete[] archiveBuffer;

			// Success, report results
			const auto end = std::chrono::system_clock::now();
			const std::chrono::duration<double> elapsed_seconds = end - start;
			std::cout
				<< "Files packaged: " << fileCount << "\n"
				<< "Bytes packaged: " << byteCount << "\n"
				<< "Total duration: " << elapsed_seconds.count() << " seconds\n\n";

			// Exit
			system("pause");
			exit(EXIT_SUCCESS);
		}
	}	
}