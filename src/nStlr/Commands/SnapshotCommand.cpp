#include "SnapshotCommand.h"
#include "BufferTools.h"
#include "DirectoryTools.h"
#include "Common.h"
#include <fstream>


void SnapshotCommand::execute(const int & argc, char * argv[]) const
{
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
			exit_program("\n"
				"        Help:       /\n"
				" ~-----------------~\n"
				"/\n"
				" Arguments Expected:\n"
				" -src=[path to the directory to compress]\n"
				" -dst=[path to write the snapshot] (can omit filename)\n"
				"\n\n"
			);
	}

	// If user provides a directory only, append a filename
	if (std::filesystem::is_directory(dstDirectory)) 
		dstDirectory += "\\" + std::filesystem::path(srcDirectory).stem().generic_string() + " - snapshot.nsnap";	

	// Ensure a file-extension is chosen
	if (!std::filesystem::path(dstDirectory).has_extension())
		dstDirectory += ".nsnap";

	// Compress the directory specified
	char * packBuffer(nullptr);
	size_t packSize(0ull), fileCount(0ull);
	DRT::CompressDirectory(srcDirectory, &packBuffer, packSize, fileCount);
	
	// Write snapshot to disk
	std::filesystem::create_directories(std::filesystem::path(dstDirectory).parent_path());
	std::ofstream file(dstDirectory, std::ios::binary | std::ios::out);
	if (!file.is_open())
		exit_program("Cannot write snapshot to disk, aborting...\n");
	file.write(packBuffer, (std::streamsize)packSize);
	file.close();
	delete[] packBuffer;

	// Output results
	std::cout
		<< std::endl
		<< "Files packaged: " << fileCount << "\n"
		<< "Bytes packaged: " << packSize << "\n";
}