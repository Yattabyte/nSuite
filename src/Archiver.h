#include <string>


/** Utility class responsible for packing and unpacking archive files for installation. */
class Archiver {
public:
	// Public Methods
	/** Pack the archive directory into installer, overwriting its contents. */
	static bool pack(const std::string & directory, size_t & fileCount, size_t & byteCount);
	/** Unpack the installer's contents, overwriting the archive directory. */
	static bool unpack(const std::string & directory, size_t & fileCount, size_t & byteCount);


private:
	// Private Methods
	/** Compress the archive. */
	static bool compressArchive(char * sourceBuffer, const size_t & sourceSize, char ** destinationBuffer, size_t & destinationSize);
	/** Decompress the archive. */
	static bool decompressArchive(char * sourceBuffer, const size_t & sourceSize, char ** destinationBuffer, size_t & destinationSize);
};