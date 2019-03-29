#include "InstallerCommand.h"
#include "BufferTools.h"
#include "DirectoryTools.h"
#include "Resource.h"
#include "Common.h"
#include <fstream>


void InstallerCommand::execute(const int & argc, char * argv[]) const
{
	// Supply command header to console
	std::cout <<
		"                      ~\n"
		"    Installer Maker  /\n"
		"  ~-----------------~\n"
		" /\n"
		"~\n\n";

	// Check command line arguments
	std::string srcDirectory(""), dstDirectory("");
	for (int x = 2; x < argc; ++x) {
		std::string command(argv[x], 5);
		std::transform(command.begin(), command.end(), command.begin(), ::tolower);
		if (command == "-src=")
			srcDirectory = std::string(&argv[x][5]);
		else if (command == "-dst=")
			dstDirectory = std::string(&argv[x][5]);
		else
			exit_program(
				" Arguments Expected:\n"
				" -src=[path to the directory to compress]\n"
				" -dst=[path to write the installer] (can omit filename)\n"
				"\n"
			);
	}

	// If user provides a directory only, append a filename
	if (std::filesystem::is_directory(dstDirectory))
		dstDirectory += "\\installer.exe";

	// Ensure a file-extension is chosen
	if (!std::filesystem::path(dstDirectory).has_extension())
		dstDirectory += ".exe";

	// Compress the directory specified
	char * packBuffer(nullptr);
	size_t packSize(0ull), fileCount(0ull);
	if (!DRT::CompressDirectory(srcDirectory, &packBuffer, packSize, fileCount))
		exit_program("Cannot create installer from the directory specified, aborting...\n");

	// Acquire installer resource
	Resource installer(IDR_INSTALLER, "INSTALLER");
	if (!installer.exists())
		exit_program("Cannot access installer resource, aborting...\n");

	// Write installer to disk
	std::filesystem::create_directories(std::filesystem::path(dstDirectory).parent_path());
	std::ofstream file(dstDirectory, std::ios::binary | std::ios::out);
	if (!file.is_open())
		exit_program("Cannot write installer to disk, aborting...\n");
	file.write(reinterpret_cast<char*>(installer.getPtr()), (std::streamsize)installer.getSize());
	file.close();

	// Update installer's resource
	auto handle = BeginUpdateResource(dstDirectory.c_str(), false);
	if (!(bool)UpdateResource(handle, "ARCHIVE", MAKEINTRESOURCE(IDR_ARCHIVE), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), packBuffer, (DWORD)packSize))
		exit_program("Cannot write archive contents to the installer, aborting...\n");
	EndUpdateResource(handle, FALSE);	
	delete[] packBuffer;

	// Output results
	std::cout
		<< "Files packaged: " << fileCount << "\n"
		<< "Bytes packaged: " << packSize << "\n";
}