#include "nSuite.h"
#include "Log.h"
#include <chrono>
#include <fstream>
#include <iostream>


/** Fetch all patch files from the directory supplied.
@param	srcDirectory	the directory to find patch files.
@return					list of all patch files within the directory supplied. */
static auto get_patches(const std::string & srcDirectory)
{
	std::vector<std::filesystem::directory_entry> patches;
	for (const auto & entry : std::filesystem::recursive_directory_iterator(srcDirectory))
		if (entry.is_regular_file() && entry.path().has_extension() && (entry.path().extension().string().find("ndiff") != std::string::npos))
			patches.emplace_back(entry);
	return patches;
}

/** Entry point. */
int main()
{
	// Tap-in to the log, have it redirect to the console
	auto index = NST::Log::AddObserver([&](const std::string & message) {
		std::cout << message;
	});

	// Find all patch files?
	const auto dstDirectory = NST::SanitizePath(NST::GetRunningDirectory());
	const auto patches = get_patches(dstDirectory);

	// Report an overview of supplied procedure
	NST::Log::PushText(
		"                       ~\r\n"
		"        Updater       /\r\n"
		"  ~------------------~\r\n"
		" /\r\n"
		"~\r\n\r\n"
		"There are " + std::to_string(patches.size()) + " patches(s) found.\r\n"
		"\r\n"
	);
	if (patches.size()) {
		NST::Log::PushText(std::string("Ready to update?") + ' ');
		system("pause");
		std::printf("\033[A\33[2K\r");
		std::printf("\033[A\33[2K\r\n");

		// Begin updating
		const auto start = std::chrono::system_clock::now();
		size_t bytesWritten(0ull), patchesApplied(0ull);
		for each (const auto & patch in patches) {
			// Open diff file
			std::ifstream diffFile(patch, std::ios::binary | std::ios::beg);
			if (!diffFile.is_open()) {
				NST::Log::PushText("Cannot read diff file, skipping...\r\n");
				continue;
			}
			else {
				// Apply patch
				NST::Buffer diffBuffer(std::filesystem::file_size(patch));
				diffFile.read(diffBuffer.cArray(), std::streamsize(diffBuffer.size()));
				diffFile.close();
				NST::Directory directory(dstDirectory);
				const bool updateResult = directory.apply_delta(diffBuffer);

				// If patching success, write changes to disk
				if (updateResult) {
					directory.make_folder();
					patchesApplied++;

					// Delete patch file at very end
					if (!std::filesystem::remove(patch))
						NST::Log::PushText("Cannot delete diff file \"" + patch.path().string() + "\" from disk, make sure to delete it manually.\r\n");
				}
				else {
					NST::Log::PushText("skipping patch...\r\n");
					continue;
				}
			}
		}

		// Success, report results
		const auto end = std::chrono::system_clock::now();
		const std::chrono::duration<double> elapsed_seconds = end - start;
		NST::Log::PushText(
			"Patches used:   " + std::to_string(patchesApplied) + " out of " + std::to_string(patches.size()) + "\r\n" +
			"Bytes written:  " + std::to_string(bytesWritten) + "\r\n" +
			"Total duration: " + std::to_string(elapsed_seconds.count()) + " seconds\r\n\r\n"
		);
	}

	// Pause and exit
	NST::Log::RemoveObserver(index);
	system("pause");
	exit(EXIT_SUCCESS);	
}