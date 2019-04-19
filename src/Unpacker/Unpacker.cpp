#include "Common.h"
#include "DirectoryTools.h"
#include "Resource.h"
#include <chrono>


/** Entry point. */
int main()
{
	// Check command line arguments
	const std::string dstDirectory(get_current_directory());

	// Report an overview of supplied procedure
	std::cout <<
		"                       ~\r\n"
		"       Unpackager     /\r\n"
		"  ~------------------~\r\n"
		" /\r\n"
		"~\r\n\r\n";

	// Acquire archive resource
	const auto start = std::chrono::system_clock::now();
	size_t fileCount(0ull), byteCount(0ull);
	Resource archive(IDR_ARCHIVE, "ARCHIVE");
	if (!archive.exists())
		exit_program("Cannot access archive resource (may be absent, corrupt, or have different identifiers), aborting...\r\n");

	// Read the package header
	char * packBufferOffset = reinterpret_cast<char*>(archive.getPtr());
	const auto folderSize = *reinterpret_cast<size_t*>(packBufferOffset);
	packBufferOffset = reinterpret_cast<char*>(PTR_ADD(packBufferOffset, size_t(sizeof(size_t))));
	const char * folderArray = reinterpret_cast<char*>(packBufferOffset);
	const auto finalDestionation = dstDirectory + "\\" + std::string(folderArray, folderSize);
	packBufferOffset = reinterpret_cast<char*>(PTR_ADD(packBufferOffset, folderSize));
	   	// Report an overview of supplied procedure
	std::cout <<
		"Unpacking to the following directory:\r\n"
		"\t> " + finalDestionation +
		"\r\n";

	// Unpackage using the resource file
	if (!DRT::DecompressDirectory(finalDestionation, packBufferOffset, archive.getSize() - (size_t(sizeof(size_t)) + folderSize), byteCount, fileCount))
		exit_program("Cannot decompress embedded package resource, aborting...\r\n");

	// Success, report results
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	std::cout
		<< "Files written:  " << fileCount << "\r\n"
		<< "Bytes written:  " << byteCount << "\r\n"
		<< "Total duration: " << elapsed_seconds.count() << " seconds\r\n\r\n";
	system("pause");
	exit(EXIT_SUCCESS);
}