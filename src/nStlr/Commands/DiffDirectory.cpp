#include "DiffDirectory.h"
#include "BufferTools.h"
#include "Instructions.h"
#include <fstream>


static bool read_file(const std::filesystem::directory_entry & file, char ** buffer, size_t & hash)
{
	std::ifstream f(file, std::ios::binary | std::ios::beg);
	*buffer = new char[file.file_size()];
	if (f.is_open()) {
		f.read(*buffer, std::streamsize(file.file_size()));
		f.close();
		hash = BFT::HashBuffer(*buffer, file.file_size());
		return true;
	}
	return false;
}

void DiffDirectory::execute(const int & argc, char * argv[]) const 
{
	// Check command line arguments
	const auto start = std::chrono::system_clock::now();
	std::string oldDirectory(""), newDirectory(""), dstDirectory("");
	for (int x = 2; x < argc; ++x) {
		std::string command(argv[x], 5);
		std::transform(command.begin(), command.end(), command.begin(), ::tolower);
		if (command == "-old=")
			oldDirectory = std::string(&argv[x][5]);
		else if (command == "-new=")
			newDirectory = std::string(&argv[x][5]);
		else if (command == "-dst=")
			dstDirectory = std::string(&argv[x][5]);
		else
			exit_program("\n"
				"        Help:       /\n"
				" ~-----------------~\n"
				"/\n"
				"\n\n"
			);
	}

	// Create diff file
	const auto time = std::chrono::system_clock::to_time_t(start);
	std::tm bt;
	localtime_s(&bt, &time);
	std::string diffPath = dstDirectory + "\\"
		+ std::to_string(bt.tm_year) + std::to_string(bt.tm_mon) + std::to_string(bt.tm_mday) + std::to_string(bt.tm_hour) + std::to_string(bt.tm_min) + std::to_string(bt.tm_sec)
		+ ".diff";
	std::filesystem::create_directories(std::filesystem::path(diffPath).parent_path());
	std::ofstream file(diffPath, std::ios::binary | std::ios::out);
	if (!file.is_open())
		exit_program("Cannot write diff file to disk, aborting...\n");

	// Find all files in both directories
	const auto oldFiles = get_file_paths(oldDirectory);
	const auto newFiles = get_file_paths(newDirectory);
	size_t instructionCount(0ull), bytesWritten(0ull);
	static auto writeInstructions = [&file, &bytesWritten](const std::string & path, const size_t & oldHash, const size_t & newHash, char * buffer, const size_t & bufferSize, const char & flag) {
		auto pathLength = path.length();
		file.write(reinterpret_cast<char*>(&pathLength), std::streamsize(sizeof(size_t)));
		file.write(path.data(), std::streamsize(pathLength));
		file.write(&flag, std::streamsize(sizeof(char)));
		file.write(reinterpret_cast<char*>(const_cast<size_t*>(&oldHash)), std::streamsize(sizeof(size_t)));
		file.write(reinterpret_cast<char*>(const_cast<size_t*>(&newHash)), std::streamsize(sizeof(size_t)));
		file.write(reinterpret_cast<char*>(const_cast<size_t*>(&bufferSize)), std::streamsize(sizeof(size_t)));
		file.write(buffer, std::streamsize(bufferSize));
		bytesWritten += (sizeof(size_t) * 4) + (sizeof(char) * pathLength) + sizeof(char) + bufferSize;
	};

	// Find all common files, new files, generating instructions
	for each (const auto & nFile in newFiles) {
		const auto newPath = nFile.path().string();
		const auto newRelativePath = newPath.substr(newDirectory.length(), newPath.length() - newDirectory.length());
		char *newBuffer = nullptr;
		size_t newHash = 0ull;
		if (read_file(nFile, &newBuffer, newHash)) {
			bool found = false;
			for each (const auto & oFile in oldFiles) {
				const auto oldPath = oFile.path().string();
				const auto oldRelativePath = oldPath.substr(oldDirectory.length(), oldPath.length() - oldDirectory.length());
				// Common file found
				if (oldRelativePath == newRelativePath) {
					char *oldBuffer = nullptr;
					size_t oldHash = 0ull;
					if (read_file(oFile, &oldBuffer, oldHash)) {
						// Compare hash values, if different then we have to diff the files
						if (oldHash != newHash) {
							char * buffer = nullptr;
							size_t size = 0ull;
							if (BFT::DiffBuffers(oldBuffer, oFile.file_size(), newBuffer, nFile.file_size(), &buffer, size, &instructionCount)) {
								writeInstructions(newRelativePath, oldHash, newHash, buffer, size, 'U');
								found = true;
							}
						}

						delete[] oldBuffer;
						found = true;
						break;
					}
				}
			}
			// New file found, add it
			if (!found) {
				// Generate 1 'insert' instruction, the length of the entire buffer
				char * bufferCompressed = nullptr;
				size_t sizeCompressed = 0ull;
				if (BFT::DiffBuffers(nullptr, 0ull, newBuffer, nFile.file_size(), &bufferCompressed, sizeCompressed, &instructionCount)) {
					writeInstructions(newRelativePath, 0ull, newHash, bufferCompressed, sizeCompressed, 'N');
					delete[] bufferCompressed;
				}
			}
		}
		delete[] newBuffer;
	}

	// Find all deleted files	
	for each (const auto & oFile in oldFiles) {
		const auto oldPath = oFile.path().string();
		const auto oldRelativePath = oldPath.substr(oldDirectory.length(), oldPath.length() - oldDirectory.length());
		bool found = false;
		char *oldBuffer = nullptr;
		size_t oldHash = 0ull;
		if (read_file(oFile, &oldBuffer, oldHash)) {
			for each (const auto & nFile in newFiles) {
				const auto newPath = nFile.path().string();
				const auto newRelativePath = newPath.substr(newDirectory.length(), newPath.length() - newDirectory.length());
				// Common file found
				if (oldRelativePath == newRelativePath) {
					found = true;
					break;
				}
			}
			// Old file found, delete it
			if (!found) {
				// Signify deletion by using flag 'D'
				instructionCount++;
				writeInstructions(oldRelativePath, oldHash, 0ull, nullptr, 0ull, 'D');
			}
		}
	}

	// Output results
	std::cout
		<< "Instruction(s): " << instructionCount << "\n"
		<< "Bytes written:  " << bytesWritten << "\n";
}
