#pragma once
#ifndef DIRECTORY_TOOLS_H
#define DIRECTORY_TOOLS_H

#include "Buffer.h"
#include <filesystem>
#include <string>
#include <vector>


/** Namespace declaration for all nSuite methods and classes. */
namespace NST {
	/** Compresses all disk contents found within a source directory into an .npack - package formatted buffer.
	After compression, it applies a small header dictating packaged folders' name.
	packBuffer format:
	-------------------------------------------------------------------------------------------
	| header: identifier title, package name size, package name  | compressed directory data  |
	-------------------------------------------------------------------------------------------
	@note						caller is responsible for cleaning-up packBuffer.
	@param	srcDirectory		the absolute path to the directory to compress.
	@param	byteCount			(optional) pointer updated with the number of bytes written into the package
	@param	fileCount			(optional) pointer updated with the number of files written into the package.
	@param	exclusions			(optional) list of filenames/types to skip. "string" match relative path, ".ext" match extension.
	@return						a pointer to a buffer holding the packaged directory contents on package success, empty otherwise. */
	std::optional<Buffer> CompressDirectory(const std::string & srcDirectory, size_t * byteCount = nullptr, size_t * fileCount = nullptr, const std::vector<std::string> & exclusions = std::vector<std::string>());
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


// Public Header Information
/** Holds and performs Package I/O operations on buffers. */
struct PackageHeader : NST::Buffer::Header {
	// Attributes
	static constexpr const char TITLE[] = "nSuite package";
	size_t m_charCount = 0ull;
	std::string m_folderName = "";


	// (de)Constructors
	PackageHeader() = default;
	PackageHeader(const size_t & folderNameSize, const char * folderName) : Header(TITLE), m_charCount(folderNameSize) {
		m_folderName = std::string(folderName, folderNameSize);
	}


	// Interface Implementation
	virtual size_t size() const override {
		return size_t(sizeof(size_t) + (sizeof(char) * m_charCount));
	}
	virtual std::byte * operator << (std::byte * ptr) override {
		ptr = Header::operator<<(ptr);
		std::copy(ptr, &ptr[size_t(sizeof(size_t))], reinterpret_cast<std::byte*>(&m_charCount));
		ptr = &ptr[size_t(sizeof(size_t))];
		char * folderArray = new char[m_charCount];
		std::copy(ptr, &ptr[size_t(sizeof(char) * m_charCount)], reinterpret_cast<std::byte*>(&folderArray[0]));
		m_folderName = std::string(folderArray, m_charCount);
		delete[] folderArray;
		return &ptr[size_t(sizeof(char) * m_charCount)];
	}
	virtual std::byte *operator >> (std::byte * ptr) const override {
		ptr = Header::operator>>(ptr);
		*reinterpret_cast<size_t*>(ptr) = m_charCount;
		ptr = &ptr[size_t(sizeof(size_t))];
		std::copy(m_folderName.begin(), m_folderName.end(), reinterpret_cast<char*>(ptr));
		return &ptr[size_t(sizeof(char) * m_charCount)];
	}
};
/** Holds and performs Patch I/O operations on buffers. */
struct PatchHeader : NST::Buffer::Header {
	// Attributes
	static constexpr const char TITLE[] = "nSuite patch";
	size_t m_fileCount = 0ull;


	// (de)Constructors
	PatchHeader(const size_t size = 0ull) : Header(TITLE), m_fileCount(size) {}


	// Interface Implementation
	virtual size_t size() const override {
		return size_t(sizeof(size_t));
	}
	virtual std::byte * operator << (std::byte * ptr) override {
		ptr = Header::operator<<(ptr);
		std::copy(ptr, &ptr[size()], reinterpret_cast<std::byte*>(&m_fileCount));
		return &ptr[size()];
	}
	virtual std::byte *operator >> (std::byte * ptr) const override {
		ptr = Header::operator>>(ptr);
		*reinterpret_cast<size_t*>(ptr) = m_fileCount;
		return &ptr[size()];
	}
};

#endif // DIRECTORY_TOOLS_H