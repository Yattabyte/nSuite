#pragma once
#ifndef DIRECTORY_TOOLS_H
#define DIRECTORY_TOOLS_H

#include "Buffer.h"
#include "Directory.h"
#include <filesystem>
#include <string>
#include <vector>


/** Namespace declaration for all nSuite methods and classes. */
namespace NST {
	/** Decompresses an .npack - package formatted buffer into its component files in the destination directory.
	@param	dstDirectory		the absolute path to the directory to decompress.
	@param	packBuffer			the buffer containing the compressed package contents.
	@param	byteCount			(optional) pointer updated with the number of bytes written to disk.
	@param	fileCount			(optional) pointer updated with the number of files written to disk.
	@return						true if decompression success, false otherwise. */
	bool DecompressDirectory(const std::string & dstDirectory, const Buffer & buffer, size_t * byteCount = nullptr, size_t * fileCount = nullptr);
	/** Processes two input directories and generates a compressed instruction set for transforming the old directory into the new directory.
	diffBuffer format:
	-------------------------------------------------------------------------------
	| header: identifier title, modified file count  | compressed directory data  |
	-------------------------------------------------------------------------------
	@note						caller is responsible for cleaning-up diffBuffer.
	@param	oldDirectory		the older directory or path to an .npack file.  
	@param	newDirectory		the newer directory or path to an .npack file.  
	@return						a pointer to a buffer holding the patch instructions. */
	std::optional<Buffer> DiffDirectories(const std::string & oldDirectory, const std::string & newDirectory);
	/** Decompresses and executes the instructions contained within a previously - generated diff buffer.
	Transforms the contents of an 'old' directory into that of the 'new' directory.
	@param	dstDirectory		the destination directory to transform.
	@param	diffBuffer			the buffer containing the compressed diff instructions.
	@param	bytesWritten		(optional) pointer updated with the number of bytes written to disk.
	@return						true if patch success, false otherwise. */
	bool PatchDirectory(const std::string & dstDirectory, const Buffer & diffBuffer, size_t * bytesWritten = nullptr);
	/** Returns a list of file information for all files within the directory specified.
	@param	directory			the directory to retrieve file-info from.
	@return						a vector of file information, including file names, sizes, meta-data, etc. */
	std::vector<std::filesystem::directory_entry> GetFilePaths(const std::string & directory);
	/** Retrieves the path to the user's start-menu folder.
	@return						the path to the user's start menu folder. */
	std::string GetStartMenuPath();
	/** Retrieve the path to the user's desktop.
	@return						the path to the user's desktop folder. */
	std::string GetDesktopPath();
	/** Retrieve the directory this executable is running from.
	@return						the current directory for this program. */
	std::string GetRunningDirectory();
	/** Cleans up the target string representing a file path, specifically targeting the number of slashes.
	@param	path				the path to be sanitized.
	@return						the sanitized version of path. */
	std::string SanitizePath(const std::string & path);
};

#endif // DIRECTORY_TOOLS_H