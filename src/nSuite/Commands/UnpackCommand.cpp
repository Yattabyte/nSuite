#include "UnpackCommand.h"
#include "BufferTools.h"
#include "Common.h"
#include "DirectoryTools.h"
#include "TaskLogger.h"
#include <fstream>


void UnpackCommand::execute(const int & argc, char * argv[]) const
{
	// Supply command header to console
	TaskLogger::PushText(
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
			srcDirectory = std::string(&argv[x][5]);
		else if (command == "-dst=")
			dstDirectory = std::string(&argv[x][5]);
		else
			exit_program(
				" Arguments Expected:\r\n"
				" -src=[path to the package file]\r\n"
				" -dst=[directory to write package contents]\r\n"
				"\r\n"
			);
	}

	// Open pack file
	std::ifstream packFile(srcDirectory, std::ios::binary | std::ios::beg);
	const size_t packSize = std::filesystem::file_size(srcDirectory);
	if (!packFile.is_open())
		exit_program("Cannot read diff file, aborting...\r\n");

	// Copy contents into a buffer
	char * packBuffer = new char[packSize];
	packFile.read(packBuffer, std::streamsize(packSize));
	packFile.close();

	// Read the package header
	char * packBufferOffset = packBuffer;
	const auto folderSize = *reinterpret_cast<size_t*>(packBufferOffset);
	packBufferOffset = reinterpret_cast<char*>(PTR_ADD(packBufferOffset, size_t(sizeof(size_t))));
	const char * folderArray = reinterpret_cast<char*>(packBufferOffset);
	const auto finalDestionation = dstDirectory + "\\" + std::string(folderArray, folderSize);
	packBufferOffset = reinterpret_cast<char*>(PTR_ADD(packBufferOffset, folderSize));

	// Unpackage using the resource file
	size_t fileCount(0ull), byteCount(0ull);
	if (!DRT::DecompressDirectory(finalDestionation, packBufferOffset, packSize - (size_t(sizeof(size_t)) + folderSize), byteCount, fileCount))
		exit_program("Cannot decompress package file, aborting...\r\n");
	delete[] packBuffer;

	// Output results
	TaskLogger::PushText(
		"Files written:   " + std::to_string(fileCount) + "\r\n" +
		"Bytes processed: " + std::to_string(byteCount) + "\r\n"
	);
}