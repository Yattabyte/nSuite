#include "Commands/InstallerCommand.h"
#include "Directory.h"
#include "StringConversions.h"
#include "Log.h"
#include "Resource.h"
#include <filesystem>
#include <fstream>


int InstallerCommand::execute(const int& argc, char* argv[]) const
{
	// Supply command header to console
	yatta::Log::PushText(
		"                      ~\r\n"
		"    Installer Maker  /\r\n"
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
				" -src=[path to the directory to package]\r\n"
				" -dst=[path to write the Installer] (can omit filename)\r\n"
				"\r\n"
			);
			return EXIT_FAILURE;
		}
	}

	// If user provides a directory only, append a filename
	if (std::filesystem::is_directory(dstDirectory))
		dstDirectory = yatta::Directory::SanitizePath(dstDirectory) + "\\Installer.exe";

	// Ensure a file-extension is chosen
	if (!std::filesystem::path(dstDirectory).has_extension())
		dstDirectory += ".exe";

	// Try to compress the directory specified
	bool success = false;
	HANDLE handle(nullptr);
	yatta::Directory directory(srcDirectory, { "\\manifest.nman" });
	auto packBuffer = directory.make_package();
	if (!packBuffer)
		yatta::Log::PushText("Cannot create Installer from the directory specified, aborting...\r\n");
	else {
		// Ensure resource exists
		const yatta::Resource Installer(IDR_InstallER, "InstallER");
		if (!Installer.exists())
			yatta::Log::PushText("Cannot access Installer resource, aborting...\r\n");
		else {
			// Try to create Installer file
			std::filesystem::create_directories(std::filesystem::path(dstDirectory).parent_path());
			std::ofstream iyattaFile(dstDirectory, std::ios::binary | std::ios::out);
			if (!iyattaFile.is_open())
				yatta::Log::PushText("Cannot write Installer to disk, aborting...\r\n");
			else {
				// Write Installer to disk
				iyattaFile.write(reinterpret_cast<char*>(Installer.getPtr()), (std::streamsize)Installer.getSize());
				iyattaFile.close();

				// Try to update Installer's resource
				handle = BeginUpdateResource(dstDirectory.c_str(), 0);
				if (!(bool)UpdateResource(handle, "ARCHIVE", MAKEINTRESOURCE(IDR_ARCHIVE), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), packBuffer->data(), (DWORD)packBuffer->size()))
					yatta::Log::PushText("Cannot write archive contents to the Installer, aborting...\r\n");
				else {
					// Try to find manifest file
					if (std::filesystem::exists(srcDirectory + "\\manifest.nman")) {
						const auto manifestSize = std::filesystem::file_size(srcDirectory + "\\manifest.nman");
						std::ifstream maniFile(srcDirectory + "\\manifest.nman", std::ios::binary | std::ios::beg);
						if (!maniFile.is_open())
							yatta::Log::PushText("Cannot open manifest file!\r\n");
						else {
							// Read manifest file
							yatta::Buffer manifestBuffer(manifestSize);
							maniFile.read(manifestBuffer.cArray(), (std::streamsize)manifestSize);
							maniFile.close();

							// Update Installers' manifest resource
							if (!(bool)UpdateResource(handle, "MANIFEST", MAKEINTRESOURCE(IDR_MANIFEST), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), manifestBuffer.data(), (DWORD)manifestSize))
								yatta::Log::PushText("Cannot write manifest contents to the Installer!\r\n");
						}
					}
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