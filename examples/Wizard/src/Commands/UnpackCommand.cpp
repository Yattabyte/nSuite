#include "Commands/UnpackCommand.h"
#include "Directory.h"
#include "StringConversions.h"
#include "Log.h"
#include <filesystem>
#include <fstream>


int UnpackCommand::execute(const int& argc, char* argv[]) const
{
	// Supply command header to console
	yatta::Log::PushText(
		"                      ~\r\n"
		"       Unpacker      /\r\n"
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
	std::ifstream packFile(srcDirectory, std::ios::binary | std::ios::beg);
	if (!packFile.is_open())
		yatta::Log::PushText("Cannot read package file, aborting...\r\n");
	else {
		// Copy contents into a buffer
		yatta::Buffer packBuffer(std::filesystem::file_size(srcDirectory));
		packFile.read(packBuffer.cArray(), std::streamsize(packBuffer.size()));
		packFile.close();

		// Try to un-package using the resource file
		yatta::Directory directory(packBuffer, dstDirectory);
		if (!directory.apply_folder())
			yatta::Log::PushText("Cannot decompress package file, aborting...\r\n");
		else {
			// Output results
			yatta::Log::PushText(
				"Files processed: " + std::to_string(directory.fileCount()) + "\r\n" +
				"Bytes processed: " + std::to_string(directory.byteCount()) + "\r\n"
			);

			return EXIT_SUCCESS;
		}
	}

	return EXIT_FAILURE;
}