#pragma once
#ifndef DIRECTORY_TOOLS_H
#define DIRECTORY_TOOLS_H

#include <string>


/** Namespace to keep directory-related operations grouped together. */
namespace DRT {
	/** Compresses a source directory into an equal or smaller sized destination buffer.
	@note						caller is responsible for cleaning-up packBuffer.
	@param	srcDirectory		the absolute path to the directory to compress.
	@param	packBuffer			pointer to the destination buffer, which will hold compressed contents.
	@param	packSize			reference updated with the size in bytes of the compressed destinationBuffer.
	@param	fileCount			reference updated with the number of files compressed into the package. */	
	void CompressDirectory(const std::string & srcDirectory, char ** packBuffer, size_t & packSize, size_t & fileCount);
	/** Decompresses a packaged buffer into a destination directory.
	@param	dstDirectory		the absolute path to the directory to compress.
	@param	packBuffer			the buffer containing the compressed contents.
	@param	packSize			the size of the buffer in bytes.
	@param	byteCount			reference updated with the number of bytes written to disk.
	@param	fileCount			reference updated with the number of files written to disk. */
	void DecompressDirectory(const std::string & dstDirectory, char * packBuffer, const size_t & packSize, size_t & byteCount, size_t & fileCount);
	/** Processes two input directories and generates a compressed instruction set for transforming the old directory into the new directory.
	@note						caller is responsible for cleaning-up diffBuffer.
	@param	oldDirectory		the older directory.
	@param	newDirectory		the newer directory.
	@param	diffBuffer			pointer to the diff buffer, which will hold compressed diff instructions.
	@param	diffSize			reference updated with the size in bytes of the diff buffer.
	@param	instructionCount	reference updated with the number of instructions compressed into the diff buffer. */
	void DiffDirectory(const std::string & oldDirectory, const std::string & newDirectory, char ** diffBuffer, size_t & diffSize, size_t & instructionCount);
	/** Using a previously-generated diff file, applies the patching procedure to transform an input directory to the 'newer' state.
	@param	dstDirectory		the destination directory to transform.
	@param	diffBufferComp		the buffer containing the compressed diff instructions.
	@param	diffSizeComp		the size in bytes of the compressed diff buffer.
	@param	bytesWritten		reference updated with the number of bytes written to disk.
	@param	instructionsUsed	reference updated with the number of instructions executed. 
	@return						true if patch success, false otherwise. */
	bool PatchDirectory(const std::string & dstDirectory, char * diffBufferComp, const size_t & diffSizeComp, size_t & bytesWritten, size_t & instructionsUsed);
};

#endif // DIRECTORY_TOOLS_H