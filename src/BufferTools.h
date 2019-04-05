#pragma once
#ifndef BUFFER_TOOLS_H
#define BUFFER_TOOLS_H


/** Namespace to keep buffer-related operations grouped together. */
namespace BFT {
	/** Compresses a source buffer into an equal or smaller sized destination buffer.
	@param	sourceBuffer		the original buffer to read from.
	@param	sourceSize			the size in bytes of the source buffer.
	@param	destinationBuffer	pointer to the destination buffer, which will hold compressed contents.
	@param	destinationSize		reference updated with the size in bytes of the compressed destinationBuffer.
	@param	headerPadding		amount of headroom to allocate for a header on top of the final compressed buffer. (optional, defaults to 0)
	@return						true if compression success, false otherwise. */
	bool CompressBuffer(char * sourceBuffer, const size_t & sourceSize, char ** destinationBuffer, size_t & destinationSize, const size_t & headerPadding = 0ull);
	/** Decompressess a source buffer into an equal or larger sized destination buffer.
	@param	sourceBuffer		the original buffer to read from.
	@param	sourceSize			the size in bytes of the source buffer.
	@param	destinationBuffer	pointer to the destination buffer, which will hold decompressed contents.
	@param	destinationSize		reference updated with the size in bytes of the decompressed destinationBuffer.
	@return						true if decompression success, false otherwise. */
	bool DecompressBuffer(char * sourceBuffer, const size_t & sourceSize, char ** destinationBuffer, size_t & destinationSize);
	/** Processes both input buffers, differentiating them, generating an output (compressed) diff-buffer, ready to be written to disk.
	@note						caller expected to clean-up buffer_diff on their own
	@param	buffer_old			the older of the 2 buffers.
	@param	size_old			the size of the old buffer.
	@param	buffer_new			the newer of the 2 buffers.
	@param	size_new			the size of the new buffer.
	@param	buffer_diff			pointer to store the diff buffer at.
	@param	size_diff			reference updated with the size of the compressed diff buffer.
	@param	instructionCount	pointer to update with the number of instructions processed. (optional)
	@return						true if diff success, false otherwise. */
	bool DiffBuffers(char * buffer_old, const size_t & size_old, char * buffer_new, const size_t & size_new, char ** buffer_diff, size_t & size_diff, size_t * instructionCount = nullptr);
	/** Uses a compressed diff buffer to patch a source buffer into an updated destination buffer
	@note						caller expected to clean-up buffer_new on their own
	@param	buffer_old			the older of the 2 buffers.
	@param	size_old			the size of the old buffer.
	@param	buffer_new			pointer to store the newer of the 2 buffers.
	@param	size_new			reference updated with the size of the new buffer.
	@param	buffer_diff			the compressed diff buffer.
	@param	size_diff			the size of the compressed diff buffer.
	@param	instructionCount	pointer to update with the number of instructions processed. (optional)
	@return						true if patch success, false otherwise. */
	bool PatchBuffer(char * buffer_old, const size_t & size_old, char ** buffer_new, size_t & size_new, char * buffer_diff, const size_t & size_diff, size_t * instructionCount = nullptr);
	/** Generate a hash value for the buffer provided. 
	@param	buffer				pointer to the buffer to hash.
	@param	size				the size of the buffer.
	@return						hash value for the buffer. */
	size_t HashBuffer(char * buffer, const size_t & size);
};

#endif // BUFFER_TOOLS_H