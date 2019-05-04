#include "Commands/InstallerCommand.h"
#include "nSuite.h"
#include "StringConversions.h"
#include "Log.h"
#include "Resource.h"
#include <fstream>


int InstallerCommand::execute(const int & argc, char * argv[]) const
{
	// Supply command header to console
	NST::Log::PushText(
		"                      ~\r\n"
		"    Installer Maker  /\r\n"
		"  ~-----------------~\r\n"
		" /\r\n"
		"~\r\n\r\n"
	);

	// Check command line arguments
	std::string srcDirectory(""), dstDirectory("");
	for (int x = 2; x < argc; ++x) {
		std::string command = string_to_lower(std::string(argv[x], 5));
		if (command == "-src=")
			srcDirectory = NST::SanitizePath(std::string(&argv[x][5]));
		else if (command == "-dst=")
			dstDirectory = NST::SanitizePath(std::string(&argv[x][5]));
		else {
			NST::Log::PushText(
				" Arguments Expected:\r\n"
				" -src=[path to the directory to package]\r\n"
				" -dst=[path to write the installer] (can omit filename)\r\n"
				"\r\n"
			);
			return EXIT_FAILURE;
		}
	}

	// If user provides a directory only, append a filename
	if (std::filesystem::is_directory(dstDirectory)) 
		dstDirectory = NST::SanitizePath(dstDirectory) + "\\installer.exe";

	// Ensure a file-extension is chosen
	if (!std::filesystem::path(dstDirectory).has_extension())
		dstDirectory += ".exe";

	// Try to compress the directory specified
	bool success = false;
	HANDLE handle(nullptr);
	NST::Directory directory(srcDirectory, { "\\manifest.nman" });
	auto packBuffer = directory.package();
	if (!packBuffer)
		NST::Log::PushText("Cannot create installer from the directory specified, aborting...\r\n");
	else {
		// Ensure resource exists
		const NST::Resource installer(IDR_INSTALLER, "INSTALLER");
		if (!installer.exists()) 
			NST::Log::PushText("Cannot access installer resource, aborting...\r\n");		
		else {
			// Try to create installer file
			std::filesystem::create_directories(std::filesystem::path(dstDirectory).parent_path());
			std::ofstream instFile(dstDirectory, std::ios::binary | std::ios::out);
			if (!instFile.is_open()) 
				NST::Log::PushText("Cannot write installer to disk, aborting...\r\n");
			else {
				// Write installer to disk
				instFile.write(reinterpret_cast<char*>(installer.getPtr()), (std::streamsize)installer.getSize());
				instFile.close();

				// Try to update installer's resource
				handle = BeginUpdateResource(dstDirectory.c_str(), false);
				if (!(bool)UpdateResource(handle, "ARCHIVE", MAKEINTRESOURCE(IDR_ARCHIVE), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), packBuffer->data(), (DWORD)packBuffer->size()))
					NST::Log::PushText("Cannot write archive contents to the installer, aborting...\r\n");				
				else {
					// Try to find manifest file
					if (std::filesystem::exists(srcDirectory + "\\manifest.nman")) {
						const auto manifestSize = std::filesystem::file_size(srcDirectory + "\\manifest.nman");
						std::ifstream maniFile(srcDirectory + "\\manifest.nman", std::ios::binary | std::ios::beg);
						if (!maniFile.is_open())
							NST::Log::PushText("Cannot open manifest file!\r\n");
						else {
							// Read manifest file
							NST::Buffer manifestBuffer(manifestSize);
							maniFile.read(manifestBuffer.cArray(), (std::streamsize)manifestSize);
							maniFile.close();

							// Update installers' manifest resource
							if (!(bool)UpdateResource(handle, "MANIFEST", MAKEINTRESOURCE(IDR_MANIFEST), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), manifestBuffer.data(), (DWORD)manifestSize))
								NST::Log::PushText("Cannot write manifest contents to the installer!\r\n");
						}
					}
					// Output results
					NST::Log::PushText(
						"Files packaged:  " + std::to_string(directory.file_count()) + "\r\n" +
						"Bytes packaged:  " + std::to_string(directory.space_used()) + "\r\n" +
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