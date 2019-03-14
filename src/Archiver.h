#pragma once
#ifndef ARCHIVER_H
#define ARCHIVER_H
#include <string>


/** Utility class responsible for packing and unpacking archive files for installation. */
class Archiver {
public:
	// Public Methods
	/** Packs the archive directory into an installer application, either creating one or updating one if it exists.
	@param	directory	the directory to compress / create an installer in.
	@param	fileCount	reference updated with the number of files packaged. 
	@param	byteCount	reference updated with the number of bytes packaged. 
	@param	archBuffer	pointer to the archive buffer.
	@return				true if packing success, false otherwise. */
	static bool Pack(const std::string & directory, size_t & fileCount, size_t & byteCount, char ** archBuffer);
	/** Unpacks the installer's contents into a directory.
	@param	directory	the directory to decompress into.
	@param	fileCount	reference updated with the number of files unpackaged.
	@param	byteCount	reference updated with the number of bytes unpackaged.
	@return				true if unpacking success, false otherwise. */
	static bool Unpack(const std::string & directory, size_t & fileCount, size_t & byteCount);
};

#endif // ARCHIVER_H