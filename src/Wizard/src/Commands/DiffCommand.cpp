#include "Commands/DiffCommand.h"
#include "BufferTools.h"
#include "Common.h"
#include "DirectoryTools.h"
#include "TaskLogger.h"
#include <fstream>


void DiffCommand::execute(const int & argc, char * argv[]) const 
{
	// Supply command header to console
	TaskLogger::PushText(
		"                      ~\r\n"
		"      Patch Maker    /\r\n"
		"  ~-----------------~\r\n"
		" /\r\n"
		"~\r\n\r\n"
	);

	// Check command line arguments
	std::string oldDirectory(""), newDirectory(""), dstDirectory("");
	for (int x = 2; x < argc; ++x) {
		std::string command = string_to_lower(std::string(argv[x], 5));
		if (command == "-old=")
			oldDirectory = sanitize_path(std::string(&argv[x][5]));
		else if (command == "-new=")
			newDirectory = sanitize_path(std::string(&argv[x][5]));
		else if (command == "-dst=")
			dstDirectory = sanitize_path(std::string(&argv[x][5]));
		else
			exit_program(
				" Arguments Expected:\r\n"
				" -old=[path to the older directory]\r\n"
				" -new=[path to the newer directory]\r\n"
				" -dst=[path to write the diff file] (can omit filename)\r\n"
				"\r\n"
			);
	}	

	// If user provides a directory only, append a filename
	if (std::filesystem::is_directory(dstDirectory)) {
		const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm bt;
		localtime_s(&bt, &time);
		dstDirectory = sanitize_path(dstDirectory) + "\\" + std::to_string(bt.tm_year) + std::to_string(bt.tm_mon) + std::to_string(bt.tm_mday) + std::to_string(bt.tm_hour) + std::to_string(bt.tm_min) + std::to_string(bt.tm_sec);
	}

	// Ensure a file-extension is chosen
	if (!std::filesystem::path(dstDirectory).has_extension())
		dstDirectory += ".ndiff";
	
	// Diff the 2 directories specified
	char * diffBuffer(nullptr);
	size_t diffSize(0ull), instructionCount(0ull);
	if (!DRT::DiffDirectories(oldDirectory, newDirectory, &diffBuffer, diffSize, &instructionCount))
		exit_program("Cannot diff the two paths chosen, aborting...\r\n");
	
	// Create diff file
	std::filesystem::create_directories(std::filesystem::path(dstDirectory).parent_path());
	std::ofstream file(dstDirectory, std::ios::binary | std::ios::out);
	if (!file.is_open())
		exit_program("Cannot write diff file to disk, aborting...\r\n");

	// Write the diff file to disk
	file.write(diffBuffer, std::streamsize(diffSize));
	file.close();
	delete[] diffBuffer;

	// Output results
	TaskLogger::PushText(
		"Instruction(s): " + std::to_string(instructionCount) + "\r\n" +
		"Bytes written:  " + std::to_string(diffSize) + "\r\n"
	);
}