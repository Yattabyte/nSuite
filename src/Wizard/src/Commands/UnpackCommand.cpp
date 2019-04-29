#include "Commands/UnpackCommand.h"
#include "Buffer.h"
#include "DirectoryTools.h"
#include "StringConversions.h"
#include "Log.h"
#include <fstream>


int UnpackCommand::execute(const int & argc, char * argv[]) const
{
	// Supply command header to console
	Log::PushText(
		"                      ~\r\n"
		"       Unpacker      /\r\n"
		"  ~-----------------~\r\n"
		" /\r\n"
		"~\r\n\r\n"
	);

	// Create common variables
	bool success = false;
	char * packBuffer(nullptr);
	std::ifstream packFile;
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
				" -src=[path to the package file]\r\n"
				" -dst=[directory to write package contents]\r\n"
				"\r\n"
			);
			return EXIT_FAILURE;
		}
	}

	// Ensure a file-extension is chosen
	if (!std::filesystem::path(srcDirectory).has_extension())
		srcDirectory += ".npack";

	// Try to open pack file
	packFile = std::ifstream(srcDirectory, std::ios::binary | std::ios::beg);
	const size_t packSize = std::filesystem::file_size(srcDirectory);
	if (!packFile.is_open())
		Log::PushText("Cannot read diff file, aborting...\r\n");
	else {
		// Copy contents into a buffer
		packBuffer = new char[packSize];
		packFile.read(packBuffer, std::streamsize(packSize));

		// Try to unpackage using the resource file
		size_t byteCount(0ull), fileCount(0ull);
		if (!DRT::DecompressDirectory(dstDirectory, packBuffer, packSize, &byteCount, &fileCount))
			Log::PushText("Cannot decompress package file, aborting...\r\n");
		else {
			// Output results
			Log::PushText(
				"Files written:   " + std::to_string(fileCount) + "\r\n" +
				"Bytes processed: " + std::to_string(byteCount) + "\r\n"
			);
			success = true;
		}
	}

	// Clean-up
	packFile.close();
	delete[] packBuffer;
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}