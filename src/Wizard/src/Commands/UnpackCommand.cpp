#include "Commands/UnpackCommand.h"
#include "BufferTools.h"
#include "Common.h"
#include "DirectoryTools.h"
#include "Log.h"
#include <fstream>


void UnpackCommand::execute(const int & argc, char * argv[]) const
{
	// Supply command header to console
	Log::PushText(
		"                      ~\r\n"
		"       Unpacker      /\r\n"
		"  ~-----------------~\r\n"
		" /\r\n"
		"~\r\n\r\n"
	);

	// Check command line arguments
	std::string srcDirectory(""), dstDirectory("");
	for (int x = 2; x < argc; ++x) {
		std::string command = string_to_lower(std::string(argv[x], 5));
		if (command == "-src=")
			srcDirectory = sanitize_path(std::string(&argv[x][5]));
		else if (command == "-dst=")
			dstDirectory = sanitize_path(std::string(&argv[x][5]));
		else
			exit_program(
				" Arguments Expected:\r\n"
				" -src=[path to the package file]\r\n"
				" -dst=[directory to write package contents]\r\n"
				"\r\n"
			);
	}

	// Ensure a file-extension is chosen
	if (!std::filesystem::path(srcDirectory).has_extension())
		srcDirectory += ".npack";

	// Open pack file
	std::ifstream packFile(srcDirectory, std::ios::binary | std::ios::beg);
	const size_t packSize = std::filesystem::file_size(srcDirectory);
	if (!packFile.is_open())
		exit_program("Cannot read diff file, aborting...\r\n");

	// Copy contents into a buffer
	char * packBuffer = new char[packSize];
	packFile.read(packBuffer, std::streamsize(packSize));
	packFile.close();

	// Unpackage using the resource file
	size_t byteCount(0ull), fileCount(0ull);
	if (!DRT::DecompressDirectory(dstDirectory, packBuffer, packSize, &byteCount, &fileCount))
		exit_program("Cannot decompress package file, aborting...\r\n");
	delete[] packBuffer;

	// Output results
	Log::PushText(
		"Files written:   " + std::to_string(fileCount) + "\r\n" +
		"Bytes processed: " + std::to_string(byteCount) + "\r\n"
	);
}