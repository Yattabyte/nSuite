#include "BufferTools.h"
#include "Common.h"
#include <chrono>
#include <fstream>
#include <filesystem>


/** Displays help information about this program, then terminates it. */
static void display_help_and_exit()
{
	exit_program(
		"Help:\n"
		"~-~-~-~-~-~-~-~-~-~-/\n"
		" * if run without any arguments : uses application directory\n"
		" * use command -ovrd to skip user-ready prompt.\n"
		" * use command -old=[path] to specify a path to the older file.\n"
		" * use command -new=[path] to specify a path to the newer file.\n"
		" * use command -dst=[path] to specify a path to store the diff file.\n"
	);
}

/** Entry point. */
int main(int argc, char *argv[])
{
	// Check command line arguments
	bool skipPrompt = false;
	std::string oldPath(get_current_directory()), newPath(oldPath), dstDirectory(oldPath);
	oldPath += "\\old.exe";
	newPath += "\\new.exe";
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

	// Get the proper path names, and read the source files into their appropriate buffers.
	const auto start = std::chrono::system_clock::now();
	const auto oldSize = std::filesystem::file_size(oldPath), newSize = std::filesystem::file_size(newPath);
	std::ifstream oldFile(oldPath, std::ios::binary | std::ios::beg), newFile(newPath, std::ios::binary | std::ios::beg);
	char * buffer_old = new char[oldSize], * buffer_new = new char[newSize];
	if (!oldFile.is_open() || !newFile.is_open())
		exit_program("Cannot access both source files, aborting...\n");
	oldFile.read(buffer_old, std::streamsize(oldSize));
	newFile.read(buffer_new, std::streamsize(newSize));
	oldFile.close();
	newFile.close();	

	// Hash these files first, get the buffer contents cached sooner
	// Hash values are separated from rest of diff buffer so they can be read before decompression ops.
	const size_t oldHash = BFT::HashBuffer(buffer_old, oldSize);
	const size_t newHash = BFT::HashBuffer(buffer_new, newSize);
	
	// Begin diffing the buffers
	char * buffer_diff(nullptr);
	size_t size_diff(0ull), instructionCount(0ull);
	if (!BFT::DiffBuffers(buffer_old, oldSize, buffer_new, newSize, &buffer_diff, size_diff, &instructionCount)) 
		exit_program("Diffing failed, aborting...\n");
	delete[] buffer_old;
	delete[] buffer_new;
	
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
	
	// Write hash values, than the compressed diff buffer
	file.write(reinterpret_cast<char*>(const_cast<size_t*>(&oldHash)), std::streamsize(sizeof(size_t)));
	file.write(reinterpret_cast<char*>(const_cast<size_t*>(&newHash)), std::streamsize(sizeof(size_t)));
	file.write(buffer_diff, std::streamsize(size_diff));	
	file.close();
	delete[] buffer_diff;

	// Output results
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	std::cout
		<< "Successfully created diff file.\n"
		<< "Diff instructions: " << instructionCount << "\n"
		<< "Diff size: " << size_diff << " bytes\n"
		<< "Elapsed time: " << elapsed_seconds.count() << " seconds\n";
	system("pause");
	exit(EXIT_SUCCESS);
}