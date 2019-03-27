#include "PatchCommand.h"
#include "BufferTools.h"
#include "DirectoryTools.h"
#include "Common.h"
#include <fstream>


void PatchCommand::execute(const int & argc, char * argv[]) const
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
				" -src=[path to the .ndiff file]\n"
				" -dst=[path to the directory to patch]\n"
				"\n\n"
			);
	}

	// Open diff file
	std::ifstream diffFile(srcDirectory, std::ios::binary | std::ios::beg);
	const size_t diffSize = std::filesystem::file_size(srcDirectory);
	if (!diffFile.is_open())
		exit_program("Cannot read diff file, aborting...\n");

	// Patch the directory specified
	char * diffBuffer = new char[diffSize];
	diffFile.read(diffBuffer, std::streamsize(diffSize));
	diffFile.close();
	size_t bytesWritten(0ull), instructionsUsed(0ull);
	if (!DRT::PatchDirectory(dstDirectory, diffBuffer, diffSize, bytesWritten, instructionsUsed))
		exit_program("aborting...\n");
	delete[] diffBuffer;

	// Output results
	std::cout
		<< std::endl
		<< "Instruction(s): " << instructionsUsed << "\n"
		<< "Bytes written:  " << bytesWritten << "\n";
}