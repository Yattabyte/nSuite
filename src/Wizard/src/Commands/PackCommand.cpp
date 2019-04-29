#include "Commands/PackCommand.h"
#include "Buffer.h"
#include "DirectoryTools.h"
#include "StringConversions.h"
#include "Log.h"
#include <fstream>


int PackCommand::execute(const int & argc, char * argv[]) const
{
	// Supply command header to console
	Log::PushText(
		"                     ~\r\n"
		"       Packager     /\r\n"
		"  ~----------------~\r\n"
		" /\r\n"
		"~\r\n\r\n"
	);

	// Create common variables
	bool success = false;
	std::ofstream file;
	char * packBuffer(nullptr);
	size_t packSize(0ull), maxSize(0ull), fileCount(0ull);
	std::string srcDirectory(""), dstDirectory("");

	// Check command line arguments
	for (int x = 2; x < argc; ++x) {
		std::string command = string_to_lower(std::string(argv[x], 5));
		if (command == "-src=")
			srcDirectory = DRT::SanitizePath(std::string(&argv[x][5]));
		else if (command == "-dst=")
			dstDirectory = DRT::SanitizePath(std::string(&argv[x][5]));
		else {
			Log::PushText(
				" Arguments Expected:\r\n"
				" -src=[path to the directory to package]\r\n"
				" -dst=[path to write the package] (can omit filename)\r\n"
				"\r\n"
			);
			return EXIT_FAILURE;
		}
	}	

	// If user provides a directory only, append a filename
	if (std::filesystem::is_directory(dstDirectory)) 
		dstDirectory = DRT::SanitizePath(dstDirectory) + "\\" + std::filesystem::path(srcDirectory).stem().string() + ".npack";

	// Ensure a file-extension is chosen
	if (!std::filesystem::path(dstDirectory).has_extension())
		dstDirectory += ".npack";
	
	// Try to compress the directory specified
	if (!DRT::CompressDirectory(srcDirectory, &packBuffer, packSize, &maxSize, &fileCount))
		Log::PushText("Cannot create package from the directory specified, aborting...\r\n");
	else {
		// Try to create package file
		std::filesystem::create_directories(std::filesystem::path(dstDirectory).parent_path());
		file = std::ofstream(dstDirectory, std::ios::binary | std::ios::out);
		if (!file.is_open())
			Log::PushText("Cannot write package to disk, aborting...\r\n");
		else {
			// Write package to disk
			file.write(packBuffer, (std::streamsize)packSize);

			// Output results
			Log::PushText(
				"Files packaged:  " + std::to_string(fileCount) + "\r\n" +
				"Bytes packaged:  " + std::to_string(maxSize) + "\r\n" +
				"Compressed Size: " + std::to_string(packSize) + "\r\n"
			);
			success = true;
		}
	}

	// Clean-up
	file.close();
	delete[] packBuffer;
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}