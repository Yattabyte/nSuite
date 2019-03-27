#include "Common.h"
#include "DirectoryTools.h"
#include <chrono>
#include <fstream>


static std::vector<std::filesystem::directory_entry> get_patches(const std::string & srcDirectory)
{
	std::vector<std::filesystem::directory_entry> patches;
	for (const auto & entry : std::filesystem::recursive_directory_iterator(srcDirectory))
		if (entry.is_regular_file() && entry.path().has_extension() && (entry.path().extension().string().find("ndiff") != std::string::npos))
			patches.emplace_back(entry);
	return patches;
}

/** Entry point. */
int main(int argc, char *argv[])
{
	// Find all patch files?
	const auto dstDirectory(get_current_directory());
	const auto patches = get_patches(dstDirectory);

	// Report an overview of supplied procedure
	std::cout << patches.size() << " updates found.\n";
	if (patches.size()) {
		std::cout << "Ready to update?\n";
		system("pause");
		std::cout << std::endl;

		// Begin updating
		const auto start = std::chrono::system_clock::now();
		size_t bytesWritten(0ull), instructionsUsed(0ull), patchesApplied(0ull);
		for each (const auto & patch in patches) {
			// Open diff file
			std::ifstream diffFile(patch, std::ios::binary | std::ios::beg);
			const size_t diffSize = std::filesystem::file_size(patch);
			if (!diffFile.is_open()) {
				std::cout << "Cannot read diff file, skipping...\n";
				continue;
			}
			else {
				// Apply patch
				char * diffBuffer = new char[diffSize];
				diffFile.read(diffBuffer, std::streamsize(diffSize));
				diffFile.close();
				if (!DRT::PatchDirectory(dstDirectory, diffBuffer, diffSize, bytesWritten, instructionsUsed)) {
					std::cout << "skipping patch...\n";
					delete[] diffBuffer;
					continue;
				}

				// Delete patch file at very end
				if (!std::filesystem::remove(patch))
					std::cout << "Cannot delete diff file \"" << patch << "\" from disk, make sure to delete it manually.\n";
				patchesApplied++;
				delete[] diffBuffer;
			}
		}

		// Success, report results
		const auto end = std::chrono::system_clock::now();
		const std::chrono::duration<double> elapsed_seconds = end - start;
		std::cout
			<< "Patches used:   " << patchesApplied << " out of " << patches.size() << "\n"
			<< "Bytes written:  " << bytesWritten << "\n"
			<< "Total duration: " << elapsed_seconds.count() << " seconds\n\n";
	}

	system("pause");
	exit(EXIT_SUCCESS);	
}