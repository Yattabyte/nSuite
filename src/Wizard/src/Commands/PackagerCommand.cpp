#include "Commands/PackagerCommand.h"
#include "Directory.h"
#include "StringConversions.h"
#include "Log.h"
#include "Resource.h"
#include <filesystem>
#include <fstream>


int PackagerCommand::execute(const int & argc, char * argv[]) const
{
	// Supply command header to console
	NST::Log::PushText(
		"                             ~\r\n"
		"    Portable Package Maker  /\r\n"
		"  ~------------------------~\r\n"
		" /\r\n"
		"~\r\n\r\n"
	);

	// Check command line arguments
	std::string srcDirectory(""), dstDirectory("");
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
				" -dst=[path to write the portable package] (can omit filename)\r\n"
				"\r\n"
			);
			return EXIT_FAILURE;
		}
	}

	// If user provides a directory only, append a filename
	if (std::filesystem::is_directory(dstDirectory))
		dstDirectory = NST::Directory::SanitizePath(dstDirectory) + "\\package.exe";

	// Ensure a file-extension is chosen
	if (!std::filesystem::path(dstDirectory).has_extension())
		dstDirectory += ".exe";
	
	// Try to compress the directory specified
	bool success = false;
	HANDLE handle(nullptr);
	NST::Directory directory(srcDirectory);
	auto packBuffer = directory.make_package();
	if (!packBuffer)
		NST::Log::PushText("Cannot create package from the directory specified, aborting...\r\n");
	else {
		// Ensure resource exists
		const NST::Resource unpacker(IDR_UNPACKER, "UNPACKER");
		if (!unpacker.exists())
			NST::Log::PushText("Cannot access unpacker resource, aborting...\r\n");
		else {
			// Try to create package file
			std::filesystem::create_directories(std::filesystem::path(dstDirectory).parent_path());
			std::ofstream file(dstDirectory, std::ios::binary | std::ios::out);
			if (!file.is_open())
				NST::Log::PushText("Cannot write package to disk, aborting...\r\n");
			else {
				// Write packager to disk
				file.write(reinterpret_cast<char*>(unpacker.getPtr()), (std::streamsize)unpacker.getSize());
				file.close();

				// Try to update packager's resource
				handle = BeginUpdateResource(dstDirectory.c_str(), false);
				if (!(bool)UpdateResource(handle, "ARCHIVE", MAKEINTRESOURCE(IDR_ARCHIVE), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), packBuffer->data(), (DWORD)packBuffer->size()))
					NST::Log::PushText("Cannot write archive contents to the package, aborting...\r\n");				
				else {
					// Output results
					NST::Log::PushText(
						"Files packaged:  " + std::to_string(directory.fileCount()) + "\r\n" +
						"Bytes packaged:  " + std::to_string(directory.byteCount()) + "\r\n" +
						"Compressed Size: " + std::to_string(packBuffer->size()) + "\r\n"
					);
					success = true;
				}
			}
		}
	}

	// Clean-up
	EndUpdateResource(handle, !success);
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}