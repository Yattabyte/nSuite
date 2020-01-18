#include "Commands/PackagerCommand.h"
#include "Directory.h"
#include "StringConversions.h"
#include "Log.h"
#include "Resource.h"
#include <filesystem>
#include <fstream>


int PackagerCommand::execute(const int& argc, char* argv[]) const
{
	// Supply command header to console
	yatta::Log::PushText(
		"                             ~\r\n"
		"    Portable Package Maker  /\r\n"
		"  ~------------------------~\r\n"
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
				" -src=[path to the directory to package]\r\n"
				" -dst=[path to write the portable package] (can omit filename)\r\n"
				"\r\n"
			);
			return EXIT_FAILURE;
		}
	}

	// If user provides a directory only, append a filename
	if (std::filesystem::is_directory(dstDirectory))
		dstDirectory = yatta::Directory::SanitizePath(dstDirectory) + "\\package.exe";

	// Ensure a file-extension is chosen
	if (!std::filesystem::path(dstDirectory).has_extension())
		dstDirectory += ".exe";

	// Try to compress the directory specified
	bool success = false;
	HANDLE handle(nullptr);
	yatta::Directory directory(srcDirectory);
	auto packBuffer = directory.make_package();
	if (!packBuffer)
		yatta::Log::PushText("Cannot create package from the directory specified, aborting...\r\n");
	else {
		// Ensure resource exists
		const yatta::Resource unpacker(IDR_UNPACKER, "UNPACKER");
		if (!unpacker.exists())
			yatta::Log::PushText("Cannot access unpacker resource, aborting...\r\n");
		else {
			// Try to create package file
			std::filesystem::create_directories(std::filesystem::path(dstDirectory).parent_path());
			std::ofstream file(dstDirectory, std::ios::binary | std::ios::out);
			if (!file.is_open())
				yatta::Log::PushText("Cannot write package to disk, aborting...\r\n");
			else {
				// Write packager to disk
				file.write(reinterpret_cast<char*>(unpacker.getPtr()), (std::streamsize)unpacker.getSize());
				file.close();

				// Try to update packager's resource
				handle = BeginUpdateResource(dstDirectory.c_str(), 0);
				if (!(bool)UpdateResource(handle, "ARCHIVE", MAKEINTRESOURCE(IDR_ARCHIVE), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), packBuffer->data(), (DWORD)packBuffer->size()))
					yatta::Log::PushText("Cannot write archive contents to the package, aborting...\r\n");
				else {
					// Output results
					yatta::Log::PushText(
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
	EndUpdateResource(handle, static_cast<BOOL>(!success));
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}