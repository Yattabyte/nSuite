#include "UnpackCommand.h"
#include "BufferTools.h"
#include "DirectoryTools.h"
#include "Common.h"
#include <fstream>


void UnpackCommand::execute(const int & argc, char * argv[]) const
{
	// Supply command header to console
	std::cout << 
		"                      ~\n"
		"    Unpacker         /\n"
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
				" -src=[path to the package file]\n"
				" -dst=[directory to write package contents]\n"
				"\n"
			);
	}

	// Open pack file
	std::ifstream packFile(srcDirectory, std::ios::binary | std::ios::beg);
	const size_t packSize = std::filesystem::file_size(srcDirectory);
	if (!packFile.is_open())
		exit_program("Cannot read diff file, aborting...\n");

	// Copy contents into a buffer
	char * packBuffer = new char[packSize];
	packFile.read(packBuffer, std::streamsize(packSize));
	packFile.close();

	// Unpackage using the resource file
	size_t fileCount(0ull), byteCount(0ull);
	if (!DRT::DecompressDirectory(dstDirectory, packBuffer, packSize, byteCount, fileCount))
		exit_program("Cannot decompress package file, aborting...\n");

	// Output results
	std::cout
		<< "Files written:  " << fileCount << "\n"
		<< "Bytes written:  " << byteCount << "\n";
}