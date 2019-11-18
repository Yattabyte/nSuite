#include "Commands/PackCommand.h"
#include "Directory.h"
#include "StringConversions.h"
#include "Log.h"
#include <filesystem>
#include <fstream>


int PackCommand::execute(const int& argc, char* argv[]) const
{
	// Supply command header to console
	NST::Log::PushText(
		"                     ~\r\n"
		"       Packager     /\r\n"
		"  ~----------------~\r\n"
		" /\r\n"
		"~\r\n\r\n"
	);

	// Check command line arguments
	std::string srcDirectory;
	std::string dstDirectory;
	for (int x = 2; x < argc; ++x) {
		std::string command = NST::string_to_lower(std::string(argv[x], 5));
		if (command == "-src=")
			srcDirectory = NST::Directory::SanitizePath(std::string(&argv[x][5]));
		else if (command == "-dst=")
			dstDirectory = NST::Directory::SanitizePath(std::string(&argv[x][5]));
		else {
			NST::Log::PushText(
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
		dstDirectory = NST::Directory::SanitizePath(dstDirectory) + "\\" + std::filesystem::path(srcDirectory).stem().string() + ".npack";

	// Ensure a file-extension is chosen
	if (!std::filesystem::path(dstDirectory).has_extension())
		dstDirectory += ".npack";

	// Try to compress the directory specified
	NST::Directory directory(srcDirectory);
	auto packBuffer = directory.make_package();
	if (!packBuffer)
		NST::Log::PushText("Cannot create package from the directory specified, aborting...\r\n");
	else {
		// Try to create package file
		std::filesystem::create_directories(std::filesystem::path(dstDirectory).parent_path());
		std::ofstream file(dstDirectory, std::ios::binary | std::ios::out);
		if (!file.is_open())
			NST::Log::PushText("Cannot write package to disk, aborting...\r\n");
		else {
			// Write package to disk
			file.write(packBuffer->cArray(), (std::streamsize)packBuffer->size());
			file.close();

			// Output results
			NST::Log::PushText(
				"Files packaged:  " + std::to_string(directory.fileCount()) + "\r\n" +
				"Bytes packaged:  " + std::to_string(directory.byteCount()) + "\r\n" +
				"Compressed Size: " + std::to_string(packBuffer->size()) + "\r\n"
			);

			return EXIT_SUCCESS;
		}
	}

	return EXIT_FAILURE;
}