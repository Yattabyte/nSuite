#include "Commands/PatchCommand.h"
#include "Buffer.h"
#include "DirectoryTools.h"
#include "StringConversions.h"
#include "Log.h"
#include <fstream>


int PatchCommand::execute(const int & argc, char * argv[]) const
{
	// Supply command header to console
	Log::PushText(
		"                      ~\r\n"
		"        Patcher      /\r\n"
		"  ~-----------------~\r\n"
		" /\r\n"
		"~\r\n\r\n"
	);

	// Create common variables
	bool success = false;
	char * diffBuffer(nullptr);
	std::ifstream diffFile;
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
				" -src=[path to the .ndiff file]\r\n"
				" -dst=[path to the directory to patch]\r\n"
				"\r\n"
			);
			return EXIT_FAILURE;
		}
	}

	// Ensure a file-extension is chosen
	if (!std::filesystem::path(srcDirectory).has_extension())
		srcDirectory += ".ndiff";

	// Try to open diff file
	diffFile = std::ifstream(srcDirectory, std::ios::binary | std::ios::beg);
	if (!diffFile.is_open())
		Log::PushText("Cannot read diff file, aborting...\r\n");
	else {
		// Try to patch the directory specified
		const size_t diffSize = std::filesystem::file_size(srcDirectory);
		diffBuffer = new char[diffSize];
		diffFile.read(diffBuffer, std::streamsize(diffSize));
		size_t bytesWritten(0ull);
		if (!DRT::PatchDirectory(dstDirectory, diffBuffer, diffSize, &bytesWritten))
			Log::PushText("aborting...\r\n");
		else {
			// Output results
			Log::PushText(
				"Bytes written:  " + std::to_string(bytesWritten) + "\r\n"
			);
			success = true;
		}
	}

	// Clean-up
	diffFile.close();
	delete[] diffBuffer;
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}