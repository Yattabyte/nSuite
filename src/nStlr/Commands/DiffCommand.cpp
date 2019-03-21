#include "DiffCommand.h"
#include "BufferTools.h"
#include "DirectoryTools.h"
#include "Common.h"
#include <fstream>


void DiffCommand::execute(const int & argc, char * argv[]) const 
{
	// Check command line arguments
	std::string oldDirectory(""), newDirectory(""), dstDirectory("");
	for (int x = 2; x < argc; ++x) {
		std::string command(argv[x], 5);
		std::transform(command.begin(), command.end(), command.begin(), ::tolower);
		if (command == "-old=")
			oldDirectory = std::string(&argv[x][5]);
		else if (command == "-new=")
			newDirectory = std::string(&argv[x][5]);
		else if (command == "-dst=")
			dstDirectory = std::string(&argv[x][5]);
		else
			exit_program("\n"
				"        Help:       /\n"
				" ~-----------------~\n"
				"/\n"
				"\n\n"
			);
	}
	
	// Diff the 2 directories specified
	char * diffBuffer(nullptr);
	size_t diffSize(0ull), instructionCount(0ull);
	DRT::DiffDirectory(oldDirectory, newDirectory, &diffBuffer, diffSize, instructionCount);
	
	// Create diff file
	const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm bt;
	localtime_s(&bt, &time);
	std::string diffPath = dstDirectory + "\\"
		+ std::to_string(bt.tm_year) + std::to_string(bt.tm_mon) + std::to_string(bt.tm_mday) + std::to_string(bt.tm_hour) + std::to_string(bt.tm_min) + std::to_string(bt.tm_sec)
		+ ".diff";
	std::filesystem::create_directories(std::filesystem::path(diffPath).parent_path());
	std::ofstream file(diffPath, std::ios::binary | std::ios::out);
	if (!file.is_open())
		exit_program("Cannot write diff file to disk, aborting...\n");

	// Write the diff file to disk
	file.write(diffBuffer, std::streamsize(diffSize));
	file.close();
	delete[] diffBuffer;

	// Output results
	std::cout << std::endl
		<< "Instruction(s): " << instructionCount << "\n"
		<< "Bytes written:  " << diffSize << "\n";
}