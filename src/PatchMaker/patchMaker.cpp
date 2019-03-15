#include "BufferTools.h"
#include "Common.h"
#include <chrono>
#include <fstream>
#include <filesystem>


/** Displays help information about this program, then terminates it. */
static void display_help_and_exit()
{
	exit_program(
		"        Help:       /\n"
		" ~-----------------~\n"
		"/\n"
		" * use command -ovrd to skip user-ready prompt.\n"
		" * use command -old=[path] to specify a path to the older file.\n"
		" * use command -new=[path] to specify a path to the newer file.\n"
		" * use command -dst=[path] to specify a path to store the diff file.\n\n"
	);
}

/** Entry point. */
int main(int argc, char *argv[])
{
	// Check command line arguments
	bool skipPrompt = false;
	std::string oldPath, newPath, dstDirectory;
	for (int x = 1; x < argc; ++x) {
		std::string command(argv[x], 5);
		std::transform(command.begin(), command.end(), command.begin(), ::tolower);
		if (command == "-ovrd")
			skipPrompt = true;
		else if (command == "-old=")
			oldPath = std::string(&argv[x][5]);
		else if (command == "-new=")
			newPath = std::string(&argv[x][5]);
		else if (command == "-dst=")
			dstDirectory = std::string(&argv[x][5]);
		else
			display_help_and_exit();
	}
	sanitize_path(oldPath);
	sanitize_path(newPath);
	sanitize_path(dstDirectory);

	// Make sure user inputted all 3 paths
	if (oldPath.length() == 0 || newPath.length() == 0 || dstDirectory.length() == 0)
		display_help_and_exit();

	// Report an overview of supplied procedure
	std::cout
		<< "Generate diff of:\t\"" << oldPath << "\"\n"
		<< "-> to ->:\t\t\"" << newPath << "\"\n"
		<< "Storing into:\t\t\"" << dstDirectory << "\"\n\n";

	// See if we should skip the user-ready prompt
	if (!skipPrompt) {
		std::cout << "Ready to begin diffing?\n";
		system("pause");
		std::cout << std::endl;
	}

	// Read the source files into their appropriate buffers.
	const auto start = std::chrono::system_clock::now();
	const auto oldSize = std::filesystem::file_size(oldPath), newSize = std::filesystem::file_size(newPath);
	std::ifstream oldFile(oldPath, std::ios::binary | std::ios::beg), newFile(newPath, std::ios::binary | std::ios::beg);
	char * oldBuffer = new char[oldSize], * newBuffer = new char[newSize];
	if (!oldFile.is_open() || !newFile.is_open())
		exit_program("Cannot access both source files, aborting...\n");
	oldFile.read(oldBuffer, std::streamsize(oldSize));
	newFile.read(newBuffer, std::streamsize(newSize));
	oldFile.close();
	newFile.close();	

	// Hash these files first, get the buffer contents cached sooner
	const auto oldHash = BFT::HashBuffer(oldBuffer, oldSize);
	const auto newHash = BFT::HashBuffer(newBuffer, newSize);
	
	// Begin diffing the buffers
	char * diffBuffer(nullptr);
	size_t diffSize(0ull), instructionCount(0ull);
	if (!BFT::DiffBuffers(oldBuffer, oldSize, newBuffer, newSize, &diffBuffer, diffSize, &instructionCount)) 
		exit_program("Diffing failed, aborting...\n");
	delete[] oldBuffer;
	delete[] newBuffer;
	
	// Write-out compressed diff to disk
	const auto time = std::chrono::system_clock::to_time_t(start);
	std::tm bt;
	localtime_s(&bt, &time);
	std::string diffPath = dstDirectory + "\\"
		+ std::to_string(bt.tm_year) + std::to_string(bt.tm_mon) + std::to_string(bt.tm_mday) + std::to_string(bt.tm_hour) + std::to_string(bt.tm_min) + std::to_string(bt.tm_sec)
		+ ".diff";
	std::filesystem::create_directories(std::filesystem::path(diffPath).parent_path());
	std::ofstream file(diffPath, std::ios::binary | std::ios::out);
	if (!file.is_open()) 
		exit_program("Cannot write diff to disk, aborting...\n");
	
	// Write hash values first, than the compressed diff buffer
	// Hashes separated from compressed buffer so the reader can disqualify a patch quicker (w/o decompressing)
	file.write(reinterpret_cast<char*>(const_cast<size_t*>(&oldHash)), std::streamsize(sizeof(size_t)));
	file.write(reinterpret_cast<char*>(const_cast<size_t*>(&newHash)), std::streamsize(sizeof(size_t)));
	file.write(diffBuffer, std::streamsize(diffSize));	
	file.close();
	diffSize += (sizeof(size_t) * 2ull);
	delete[] diffBuffer;

	// Output results
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	std::cout
		<< "Instruction(s): " << instructionCount << "\n"
		<< "Bytes written:  " << diffSize << "\n"
		<< "Total duration: " << elapsed_seconds.count() << " seconds\n\n";
	system("pause");
	exit(EXIT_SUCCESS);
}