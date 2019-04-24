#include "Commands/DiffCommand.h"
#include "BufferTools.h"
#include "DirectoryTools.h"
#include "StringConversions.h"
#include "Log.h"
#include <fstream>


int DiffCommand::execute(const int & argc, char * argv[]) const 
{
	// Supply command header to log
	Log::PushText(
		"                      ~\r\n"
		"      Patch Maker    /\r\n"
		"  ~-----------------~\r\n"
		" /\r\n"
		"~\r\n\r\n"
	);

	// Create common variables
	bool success = false;
	std::ofstream file;
	char * diffBuffer(nullptr);
	size_t diffSize(0ull), instructionCount(0ull);
	std::string oldDirectory(""), newDirectory(""), dstDirectory("");

	// Check command line arguments
	for (int x = 2; x < argc; ++x) {
		std::string command = string_to_lower(std::string(argv[x], 5));
		if (command == "-old=")
			oldDirectory = DRT::SanitizePath(std::string(&argv[x][5]));
		else if (command == "-new=")
			newDirectory = DRT::SanitizePath(std::string(&argv[x][5]));
		else if (command == "-dst=")
			dstDirectory = DRT::SanitizePath(std::string(&argv[x][5]));
		else {
			Log::PushText(
				" Arguments Expected:\r\n"
				" -old=[path to the older directory]\r\n"
				" -new=[path to the newer directory]\r\n"
				" -dst=[path to write the diff file] (can omit filename)\r\n"
				"\r\n"
			);
			return EXIT_FAILURE;
		}
	}	

	// If user provides a directory only, append a filename
	if (std::filesystem::is_directory(dstDirectory)) {
		const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm bt;
		localtime_s(&bt, &time);
		dstDirectory = DRT::SanitizePath(dstDirectory) + "\\" + std::to_string(bt.tm_year) + std::to_string(bt.tm_mon) + std::to_string(bt.tm_mday) + std::to_string(bt.tm_hour) + std::to_string(bt.tm_min) + std::to_string(bt.tm_sec);
	}

	// Ensure a file-extension is chosen
	if (!std::filesystem::path(dstDirectory).has_extension())
		dstDirectory += ".ndiff";
	
	// Try to diff the 2 directories specified
	if (!DRT::DiffDirectories(oldDirectory, newDirectory, &diffBuffer, diffSize, &instructionCount))
		Log::PushText("Cannot diff the two paths chosen, aborting...\r\n");
	else {
		// Try to create diff file
		std::filesystem::create_directories(std::filesystem::path(dstDirectory).parent_path());
		file = std::ofstream(dstDirectory, std::ios::binary | std::ios::out);
		if (!file.is_open())
			Log::PushText("Cannot write diff file to disk, aborting...\r\n");
		else {
			// Write the diff file to disk
			file.write(diffBuffer, std::streamsize(diffSize));

			// Output results
			Log::PushText(
				"Instruction(s): " + std::to_string(instructionCount) + "\r\n" +
				"Bytes written:  " + std::to_string(diffSize) + "\r\n"
			);
			success = true;
		}
	}

	// Clean-up
	file.close();
	delete[] diffBuffer;
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}