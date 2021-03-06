#include "Commands/PatchCommand.h"
#include "Directory.h"
#include "StringConversions.h"
#include "Log.h"
#include <fstream>
#include <filesystem>


int PatchCommand::execute(const int& argc, char* argv[]) const
{
	// Supply command header to console
	yatta::Log::PushText(
		"                      ~\r\n"
		"        Patcher      /\r\n"
		"  ~-----------------~\r\n"
		" /\r\n"
		"~\r\n\r\n"
	);

	// Check command line arguments
	std::string srcDirectory;
	std::string dstDirectory;
	for (int x = 2; x < argc; ++x) {
		std::string command = yatta::string_to_lower(std::string(argv[x], 5));
		if (command == "-src=")
			srcDirectory = yatta::Directory::SanitizePath(std::string(&argv[x][5]));
		else if (command == "-dst=")
			dstDirectory = yatta::Directory::SanitizePath(std::string(&argv[x][5]));
		else {
			yatta::Log::PushText(
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
	std::ifstream diffFile(srcDirectory, std::ios::binary | std::ios::beg);
	if (!diffFile.is_open())
		yatta::Log::PushText("Cannot read diff file, aborting...\r\n");
	else {
		// Try to patch the directory specified
		yatta::Buffer diffBuffer(std::filesystem::file_size(srcDirectory));
		diffFile.read(diffBuffer.cArray(), std::streamsize(diffBuffer.size()));
		diffFile.close();

		// Try to patch the destination directory
		yatta::Directory directory(dstDirectory);
		if (directory.apply_delta(diffBuffer)) {
			// Output results
			/*yatta::Log::PushText(
				"Bytes written:  " + std::to_string(bytesWritten) + "\r\n"
			);*/

			return EXIT_SUCCESS;
		}
	}

	return EXIT_FAILURE;
}