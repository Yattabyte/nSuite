#pragma once
#ifndef DIRECTORY_TOOLS_H
#define DIRECTORY_TOOLS_H

#include <string>


/** Namespace to keep directory-related operations grouped together. */
namespace DRT {
	/***/
	void CompressDirectory(const std::string & srcDirectory, char ** packBuffer, size_t & packSize, size_t & fileCount);
	/***/
	void DecompressDirectory(const std::string & dstDirectory, char * packBuffer, const size_t & packSize, size_t & byteCount, size_t & fileCount);
	/***/
	void DiffDirectory(const std::string & oldDirectory, const std::string & newDirectory, char ** diffBuffer, size_t & diffSize, size_t & instructionCount);
	/***/
	void PatchDirectory(const std::string & dstDirectory, char * diffBufferCompressed, const size_t & diffSizeCompressed, size_t & bytesWritten, size_t & instructionsUsed);
};

#endif // DIRECTORY_TOOLS_H