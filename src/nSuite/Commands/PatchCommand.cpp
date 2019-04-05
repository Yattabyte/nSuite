#include "PatchCommand.h"
#include "BufferTools.h"
#include "Common.h"
#include "DirectoryTools.h"
#include "TaskLogger.h"
#include <fstream>


void PatchCommand::execute(const int & argc, char * argv[]) const
{
	// Supply command header to console
	TaskLogger::PushText(
		"                      ~\r\n"
		"    Patcher          /\r\n"
		"  ~-----------------~\r\n"
		" /\r\n"
		"~\r\n\r\n"
	);

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
				" Arguments Expected:\r\n"
				" -src=[path to the .ndiff file]\r\n"
				" -dst=[path to the directory to patch]\r\n"
				"\r\n"
			);
	}

	// Open diff file
	std::ifstream diffFile(srcDirectory, std::ios::binary | std::ios::beg);
	const size_t diffSize = std::filesystem::file_size(srcDirectory);
	if (!diffFile.is_open())
		exit_program("Cannot read diff file, aborting...\r\n");

	// Patch the directory specified
	char * diffBuffer = new char[diffSize];
	diffFile.read(diffBuffer, std::streamsize(diffSize));
	diffFile.close();
	size_t bytesWritten(0ull), instructionsUsed(0ull);
	if (!DRT::PatchDirectory(dstDirectory, diffBuffer, diffSize, bytesWritten, instructionsUsed))
		exit_program("aborting...\r\n");
	delete[] diffBuffer;

	// Output results
	TaskLogger::PushText(
		"Instruction(s): " + std::to_string(instructionsUsed) + "\r\n" +
		"Bytes written:  " + std::to_string(bytesWritten) + "\r\n"
	);
}