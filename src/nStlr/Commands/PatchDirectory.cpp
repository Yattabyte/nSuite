#include "PatchDirectory.h"
#include "BufferTools.h"
#include "Common.h"
#include "Threader.h"
#include <fstream>


static bool read_file(const std::string & path, const size_t & size, char ** buffer, size_t & hash)
{
	std::ifstream f(path, std::ios::binary | std::ios::beg);
	if (f.is_open()) {
		*buffer = new char[size];
		f.read(*buffer, std::streamsize(size));
		f.close();
		hash = BFT::HashBuffer(*buffer, size);
		return true;
	}
	return false;
}

void PatchDirectory::execute(const int & argc, char * argv[]) const
{
	// Check command line arguments
	Threader threader;
	std::string diffDirectory(""), srcDirectory("");
	for (int x = 2; x < argc; ++x) {
		std::string command(argv[x], 5);
		std::transform(command.begin(), command.end(), command.begin(), ::tolower);
		if (command == "-src=")
			diffDirectory = std::string(&argv[x][5]);
		else if (command == "-old=")
			srcDirectory = std::string(&argv[x][5]);
		else
			exit_program("\n"
				"        Help:       /\n"
				" ~-----------------~\n"
				"/\n"
				"\n\n"
			);
	}

	// Open diff file
	std::ifstream diffFile(diffDirectory, std::ios::binary | std::ios::beg);
	const size_t diffSize = std::filesystem::file_size(diffDirectory);
	if (!diffFile.is_open())
		exit_program("Cannot read diff file, aborting...\n");

	// Start reading diff file
	size_t bytesRead(0ull);
	std::atomic_size_t instructionCount(0ull);
	while (bytesRead < diffSize) {
		// Read file path
		std::string path;
		size_t pathLength = 0ull;
		diffFile.read(reinterpret_cast<char*>(&pathLength), std::streamsize(sizeof(size_t)));
		path.resize(pathLength);
		diffFile.read(path.data(), std::streamsize(pathLength));
		const auto fullPath = srcDirectory + path;

		// Read operation flag
		char flag;
		diffFile.read(&flag, std::streamsize(sizeof(char)));

		// Read diff hash values
		size_t diff_oldHash = 0ull, diff_newHash = 0ull;
		diffFile.read(reinterpret_cast<char*>(const_cast<size_t*>(&diff_oldHash)), std::streamsize(sizeof(size_t)));
		diffFile.read(reinterpret_cast<char*>(const_cast<size_t*>(&diff_newHash)), std::streamsize(sizeof(size_t)));

		// Read buffer into memory
		size_t instructionSize = 0ull;
		diffFile.read(reinterpret_cast<char*>(const_cast<size_t*>(&instructionSize)), std::streamsize(sizeof(size_t)));
		char * instructionSet = new char[instructionSize];
		diffFile.read(instructionSet, std::streamsize(instructionSize));

		// Update range of memory read
		bytesRead += (sizeof(size_t) * 4) + (sizeof(char) * pathLength) + sizeof(char) + instructionSize;

		// Process instruction set
		threader.addJob([path, fullPath, flag, diff_oldHash, diff_newHash, instructionSet, instructionSize, &instructionCount]() {
			// Try to read the target file (may not exist)
			char *oldBuffer = nullptr;
			size_t oldSize = 0ull, oldHash = 0ull;
			if (std::filesystem::exists(fullPath))
				oldSize = std::filesystem::file_size(fullPath);
			const bool srcRead = read_file(fullPath, oldSize, &oldBuffer, oldHash);

			switch (flag) {			
			case 'D': // Delete source file
				// Only remove source files if they match entirely
				if (srcRead && oldHash == diff_oldHash)
					if (!std::filesystem::remove(fullPath))
						exit_program("Critical failure: cannot delete file from disk, aborting...\n");
				break; // Done deletion procedure
			
			case 'N': // Create source file
				std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());

			case 'U': // Update source file
			default:
				if (flag == 'U' && !srcRead)
					exit_program("Cannot read source file from disk, aborting...\n");
				if (oldHash == diff_newHash) {
					std::cout << "The file \"" << path << "\" is already up to date, skipping...\n";
					break;
				}

				// Patch buffer
				char * newBuffer = nullptr;
				size_t newSize(0ull), instructions(0ull);
				BFT::PatchBuffer(oldBuffer, oldSize, &newBuffer, newSize, instructionSet, instructionSize, &instructions);
				size_t newHash = BFT::HashBuffer(newBuffer, newSize);
				instructionCount += instructions;
				delete[] instructionSet;

				// Confirm new hashes match
				if (newHash != diff_newHash)
					exit_program("Critical failure: patched file is corrupted (hash mismatch), aborting...\n");

				// Write patched buffer to disk
				std::ofstream newFile(fullPath, std::ios::binary | std::ios::out);
				if (!newFile.is_open())
					exit_program("Cannot write updated file to disk, aborting...\n");
				newFile.write(newBuffer, std::streamsize(newSize));
				newFile.close();
				delete[] newBuffer;

				// Cleanup and finish
				if (oldBuffer != nullptr)
					delete[] oldBuffer;
			}
		});		
	}

	// Wait for jobs to finish
	while (!threader.isFinished())
		continue;

	// Output results
	std::cout
		<< "Instruction(s): " << instructionCount << "\n"
		<< "Bytes written:  " << bytesRead << "\n";
}
