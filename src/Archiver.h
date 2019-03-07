#include <string>


/** Utility class responsible for packing and unpacking archive files for installation. */
class Archiver {
public:
	// Public Methods
	/** Packs the archive directory into an installer application, either creating one or updating one if it exists.
	@param	directory	the directory to compress / create an installer in.
	@param	fileCount	reference updated with the number of files packaged. 
	@param	byteCount	reference updated with the number of bytes packaged. 
	@return				true if packing success, false otherwise. */
	static bool Pack(const std::string & directory, size_t & fileCount, size_t & byteCount);
	/** Unpacks the installer's contents into a directory.
	@param	directory	the directory to decompress into.
	@param	fileCount	reference updated with the number of files unpackaged.
	@param	byteCount	reference updated with the number of bytes unpackaged.
	@return				true if unpacking success, false otherwise. */
	static bool Unpack(const std::string & directory, size_t & fileCount, size_t & byteCount);

	/** Compresses a source buffer into an equal or smaller sized destination buffer. 
	@param	sourceBuffer		the original buffer to read from.
	@param	sourceSize			the size in bytes of the source buffer.
	@param	destinationBuffer	pointer to the destination buffer, which will hold compressed contents.
	@param	destinationSize		reference updated with the size in bytes of the compressed destinationBuffer. 
	@return						true if compression success, false otherwise. */
	static bool CompressBuffer(char * sourceBuffer, const size_t & sourceSize, char ** destinationBuffer, size_t & destinationSize);
	/** Decompressess a source buffer into an equal or larger sized destination buffer.
	@param	sourceBuffer		the original buffer to read from.
	@param	sourceSize			the size in bytes of the source buffer.
	@param	destinationBuffer	pointer to the destination buffer, which will hold decompressed contents.
	@param	destinationSize		reference updated with the size in bytes of the decompressed destinationBuffer.
	@return						true if decompression success, false otherwise. */
	static bool DecompressBuffer(char * sourceBuffer, const size_t & sourceSize, char ** destinationBuffer, size_t & destinationSize);
};