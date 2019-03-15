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
		" * use command -src=[path] to specify a path to the file-to-be-patched.\n"
		" * use command -dst=[path] to specify a path to write the new file (w/o replaces src file).\n"
		" * use command -dif=[path] to specify a path to the diff file.\n\n"
	);
}

/** Entry point. */
int main(int argc, char *argv[])
{
	// Check command line arguments
	bool skipPrompt = false;
	std::string srcPath, diffPath, dstPath("");
	for (int x = 1; x < argc; ++x) {
		std::string command(argv[x], 5);
		std::transform(command.begin(), command.end(), command.begin(), ::tolower);
		if (command == "-ovrd")
			skipPrompt = true;
		else if (command == "-src=")
			srcPath = std::string(&argv[x][5]);
		else if (command == "-dst=")
			dstPath = std::string(&argv[x][5]);
		else if (command == "-dif=")
			diffPath = std::string(&argv[x][5]);
		else
			display_help_and_exit();
	}
	if (dstPath == "")
		dstPath = srcPath;
	sanitize_path(srcPath);
	sanitize_path(diffPath);
	sanitize_path(dstPath);

	// Make sure user inputted both paths
	if (srcPath.length() == 0 || diffPath.length() == 0)
		display_help_and_exit();

	// Report an overview of supplied procedure
	std::cout
		<< "Patching:\t\"" << srcPath << "\"\n"
		<< "-> using ->:\t\"" << diffPath << "\"\n\n";
	if (dstPath != srcPath)
		std::cout << "Writing to:\t\"" << dstPath << "\"\n\n";

	// See if we should skip the user-ready prompt
	if (!skipPrompt) {
		std::cout << "Ready to apply patch?\n";
		system("pause");
		std::cout << std::endl;
	}

	// Read the source files into their appropriate buffers.
	const auto start = std::chrono::system_clock::now();
	const auto srcSize = std::filesystem::file_size(srcPath), diffSize = std::filesystem::file_size(diffPath);
	std::ifstream srcFile(srcPath, std::ios::binary | std::ios::beg), difFile(diffPath, std::ios::binary | std::ios::beg);
	char * srcBuffer = new char[srcSize], * difBuffer = new char[diffSize];
	if (!srcFile.is_open() || !difFile.is_open())
		exit_program("Cannot access both source files, aborting...\n");
	srcFile.read(srcBuffer, std::streamsize(srcSize));
	difFile.read(difBuffer, std::streamsize(diffSize));
	srcFile.close();
	difFile.close();	

	// Get the 'old' hash, compare it against the patch to make sure it's valid
	const auto srcHash = BFT::HashBuffer(srcBuffer, srcSize);
	const auto patch_srcHash = *reinterpret_cast<size_t*>(difBuffer);
	const auto patch_dstHash = *reinterpret_cast<size_t*>(&difBuffer[sizeof(size_t)]);

	// Verify source hashes match
	if (srcHash != patch_srcHash)
		exit_program("Cannot apply the patch to this file (hashes don't match), aborting...\n");
	 
	// Begin patching the buffer
	char * dstBuffer(nullptr);
	size_t dstSize(0ull), instructionCount(0ull);
	if (!BFT::PatchBuffer(srcBuffer, srcSize, &dstBuffer, dstSize, &difBuffer[sizeof(size_t) * 2ull], diffSize - (sizeof(size_t) * 2ull), &instructionCount))
		exit_program("Patching failed, aborting...\n");
	delete[] srcBuffer;
	delete[] difBuffer;
	
	// Compare hash of the new file
	const auto dstHash = BFT::HashBuffer(dstBuffer, dstSize);
	if (dstHash != patch_dstHash)
		exit_program("Critical failure, patched file different than expected (hashes don't match), aborting...\n");
		
	// Write-out patched file to disk
	std::ofstream dstFile(dstPath, std::ios::binary | std::ios::out);
	if (!dstFile.is_open())
		exit_program("Cannot write patched file to disk, aborting...\n");
	dstFile.write(dstBuffer, (std::streamsize)(dstSize));
	dstFile.close();
	delete[] dstBuffer;

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