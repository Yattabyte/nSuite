#include "Common.h"
#include "DirectoryTools.h"
#include "TaskLogger.h"
#include <chrono>
#include <fstream>


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
	TaskLogger::AddCallback_TextAdded([&](const std::string & message) {
		std::cout << message;
	});

	// Find all patch files?
	const auto dstDirectory(get_current_directory());
	const auto patches = get_patches(dstDirectory);

	// Report an overview of supplied procedure
	TaskLogger::PushText(
		"                       ~\r\n"
		"        Updater       /\r\n"
		"  ~------------------~\r\n"
		" /\r\n"
		"~\r\n\r\n"
		"There are " + std::to_string(patches.size()) + " patches(s) found.\r\n"
		"\r\n"
	);
	if (patches.size()) {
		pause_program("Ready to update?");

		// Begin updating
		const auto start = std::chrono::system_clock::now();
		size_t bytesWritten(0ull), instructionsUsed(0ull), patchesApplied(0ull);
		for each (const auto & patch in patches) {
			// Open diff file
			std::ifstream diffFile(patch, std::ios::binary | std::ios::beg);
			const size_t diffSize = std::filesystem::file_size(patch);
			if (!diffFile.is_open()) {
				TaskLogger::PushText("Cannot read diff file, skipping...\r\n");
				continue;
			}
			else {
				// Apply patch
				char * diffBuffer = new char[diffSize];
				diffFile.read(diffBuffer, std::streamsize(diffSize));
				diffFile.close();
				if (!DRT::PatchDirectory(dstDirectory, diffBuffer, diffSize, bytesWritten, instructionsUsed)) {
					TaskLogger::PushText("skipping patch...\r\n");
					delete[] diffBuffer;
					continue;
				}

				// Delete patch file at very end
				if (!std::filesystem::remove(patch))
					TaskLogger::PushText("Cannot delete diff file \"" + patch.path().string() + "\" from disk, make sure to delete it manually.\r\n");
				patchesApplied++;
				delete[] diffBuffer;
			}
		}

		// Success, report results
		const auto end = std::chrono::system_clock::now();
		const std::chrono::duration<double> elapsed_seconds = end - start;
		TaskLogger::PushText(
			"Patches used:   " + std::to_string(patchesApplied) + " out of " + std::to_string(patches.size()) + "\r\n" +
			"Bytes written:  " + std::to_string(bytesWritten) + "\r\n" +
			"Total duration: " + std::to_string(elapsed_seconds.count()) + " seconds\r\n\r\n"
		);
	}

	// Pause and exit
	system("pause");
	exit(EXIT_SUCCESS);	
}