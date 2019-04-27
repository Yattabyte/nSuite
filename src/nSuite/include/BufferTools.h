#pragma once
#ifndef BUFFER_TOOLS_H
#define BUFFER_TOOLS_H

#include <vector>

/** Generic byte array encapsulation, adaptor for std::vector. */
class Buffer {
public:
	// Public (de)Constructors
	~Buffer() = default;
	Buffer() = default;
	Buffer(const size_t & size);
	Buffer(const void * pointer, const size_t & range);


	// Public Methods

	// Query Methods
	size_t size() const;
	bool hasData() const;

	// Data Accessor Methods
	std::byte & operator[](const size_t & index) const;
	char * cArray() const;
	std::byte * data() const;


	// Data Modifying Methods
	void resize(const size_t & size);


	void release();


private:
	std::vector<std::byte> m_data;
};


/** Namespace to keep buffer-related operations grouped together. */
namespace BFT {
	/** Compresses a source buffer into an equal or smaller-sized destination buffer.
	After compression, it applies a small header describing how large the uncompressed buffer is.
	destinationBuffer format:
	------------------------------------------------------------------
	| header: identifier title, uncompressed size | compressed data  |
	------------------------------------------------------------------
	@param	sourceBuffer		the buffer to read from.
	@param	destinationBuffer	reference updated with a compressed version of the source buffer.
	@return						true if compression success, false otherwise. */
	bool CompressBuffer(const Buffer & sourceBuffer, Buffer & destinationBuffer);
	/** Decompressess a source buffer into an equal or larger-sized destination buffer.
	@param	sourceBuffer		the buffer to read from.
	@param	destinationBuffer	reference updated with a decompressed version of the source buffer.
	@return						true if decompression success, false otherwise. */	
	bool DecompressBuffer(const Buffer & sourceBuffer, Buffer & destinationBuffer);
	/** Parses the header of an nSuite compressed buffer, updating parameters like the data portion of the buffer if successfull.
	@param	buffer				the entire package buffer to parse.
	@param	uncompressedSize	reference updated with the uncompressed size of the package.
	@param	dataBuffer			reference updated with a buffer containing the remaining data portion of the package.
	@return						true if the nSuite compressed buffer is formatted correctly and could be parsed, false otherwise. */
	bool ParseHeader(const Buffer & buffer, size_t & uncompressedSize, Buffer & dataBuffer);
	/** Processes two input buffers, diffing them.
	Generates a compressed instruction set dictating how to get from the old buffer to the new buffer.
	buffer_diff format:
	-----------------------------------------------------------------------------------
	| header: identifier title, final target file size | compressed instruction data  |
	-----------------------------------------------------------------------------------
	@note						caller expected to clean-up buffer_diff on their own
	@param	buffer_old			the older of the 2 buffers.
	@param	buffer_new			the newer of the 2 buffers.
	@param	buffer_diff			reference updated with a buffer containing the diff instructions.
	@param	instructionCount	(optional) pointer to update with the number of instructions processed.
	@return						true if diff success, false otherwise. */
	bool DiffBuffers(const Buffer & buffer_old, const Buffer & buffer_new, Buffer & buffer_diff, size_t * instructionCount = nullptr);
	/** Reads from a compressed instruction set, uses it to patch the 'older' buffer into the 'newer' buffer
	@note						caller expected to clean-up buffer_new on their own
	@param	buffer_old			the older of the 2 buffers.
	@param	size_old			the size of the old buffer.
	@param	buffer_new			pointer to store the newer of the 2 buffers.
	@param	size_new			reference updated with the size of the new buffer.
	@param	buffer_diff			the compressed diff buffer (instruction set).
	@param	size_diff			the size of the compressed diff buffer.
	@param	instructionCount	(optional) pointer to update with the number of instructions processed.
	@return						true if patch success, false otherwise. */
	bool PatchBuffer(char * buffer_old, const size_t & size_old, char ** buffer_new, size_t & size_new, char * buffer_diff, const size_t & size_diff, size_t * instructionCount = nullptr);
	/** Generates a hash value for the buffer provided, using the buffers' contents.
	@param	buffer				pointer to the buffer to hash.
	@param	size				the size of the buffer.
	@return						hash value for the buffer. */
	size_t HashBuffer(char * buffer, const size_t & size);
	/** Increment a pointer's address by the offset provided.
	@param	ptr					the pointer to increment by the offset amount.
	@param	offset				the offset amount to apply to the pointer's address.
	@return						the modified pointer address. */
	void * PTR_ADD(void *const ptr, const size_t & offset);
};

#endif // BUFFER_TOOLS_H