#include "Commands/InstallerCommand.h"
#include "Buffer.h"
#include "DirectoryTools.h"
#include "StringConversions.h"
#include "Log.h"
#include "Resource.h"
#include <fstream>


int InstallerCommand::execute(const int & argc, char * argv[]) const
{
	// Supply command header to console
	Log::PushText(
		"                      ~\r\n"
		"    Installer Maker  /\r\n"
		"  ~-----------------~\r\n"
		" /\r\n"
		"~\r\n\r\n"
	);

	// Create common variables
	bool success = false;
	HANDLE handle(nullptr);
	std::ofstream instFile;
	std::ifstream maniFile;
	char * packBuffer(nullptr), *maniBuffer(nullptr);
	size_t packSize(0ull), maxSize(0ull), fileCount(0ull);
	std::string srcDirectory(""), dstDirectory("");
	const Resource installer(IDR_INSTALLER, "INSTALLER");

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
				" -dst=[path to write the installer] (can omit filename)\r\n"
				"\r\n"
			);
			return EXIT_FAILURE;
		}
	}

	// If user provides a directory only, append a filename
	if (std::filesystem::is_directory(dstDirectory)) 
		dstDirectory = DRT::SanitizePath(dstDirectory) + "\\installer.exe";

	// Ensure a file-extension is chosen
	if (!std::filesystem::path(dstDirectory).has_extension())
		dstDirectory += ".exe";

	// Try to compress the directory specified
	if (!DRT::CompressDirectory(srcDirectory, &packBuffer, packSize, &maxSize, &fileCount, { "\\manifest.nman" }))
		Log::PushText("Cannot create installer from the directory specified, aborting...\r\n");
	else {
		// Ensure resource exists
		if (!installer.exists()) 
			Log::PushText("Cannot access installer resource, aborting...\r\n");		
		else {
			// Try to create installer file
			std::filesystem::create_directories(std::filesystem::path(dstDirectory).parent_path());
			instFile = std::ofstream(dstDirectory, std::ios::binary | std::ios::out);
			if (!instFile.is_open()) 
				Log::PushText("Cannot write installer to disk, aborting...\r\n");
			else {
				// Write installer to disk
				instFile.write(reinterpret_cast<char*>(installer.getPtr()), (std::streamsize)installer.getSize());
				instFile.close();

				// Try to update installer's resource
				handle = BeginUpdateResource(dstDirectory.c_str(), false);
				if (!(bool)UpdateResource(handle, "ARCHIVE", MAKEINTRESOURCE(IDR_ARCHIVE), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), packBuffer, (DWORD)packSize))
					Log::PushText("Cannot write archive contents to the installer, aborting...\r\n");				
				else {
					// Try to find manifest file
					if (std::filesystem::exists(srcDirectory + "\\manifest.nman")) {
						const auto manifestSize = std::filesystem::file_size(srcDirectory + "\\manifest.nman");
						maniFile = std::ifstream(srcDirectory + "\\manifest.nman", std::ios::binary | std::ios::beg);
						if (!maniFile.is_open())
							Log::PushText("Cannot open manifest file!\r\n");
						else {
							// Read manifest file
							maniBuffer = new char[manifestSize];
							maniFile.read(maniBuffer, (std::streamsize)manifestSize);

							// Update installers' manifest resource
							if (!(bool)UpdateResource(handle, "MANIFEST", MAKEINTRESOURCE(IDR_MANIFEST), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), maniBuffer, (DWORD)manifestSize))
								Log::PushText("Cannot write manifest contents to the installer!\r\n");						
						}
					}
					// Output results
					Log::PushText(
						"Files packaged:  " + std::to_string(fileCount) + "\r\n" +
						"Bytes packaged:  " + std::to_string(maxSize) + "\r\n" +
						"Compressed Size: " + std::to_string(packSize) + "\r\n"
					);
					success = true;
				}
			}
		}
	}

	// Clean-up
	maniFile.close();
	instFile.close();
	EndUpdateResource(handle, !success);
	delete[] packBuffer;
	delete[] maniBuffer;
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}