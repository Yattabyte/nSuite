#pragma once
#ifndef BUFFER_TOOLS_H
#define BUFFER_TOOLS_H

#include <optional>
#include <vector>


/** Generic byte array encapsulation, adaptor for std::vector. */
class Buffer {
public:
	// Public (de)Constructors
	/***/
	~Buffer() = default;
	/***/
	Buffer() = default;
	/***/
	Buffer(const size_t & size);
	/***/
	Buffer(const void * pointer, const size_t & range);


	// Public Accessors
	/***/
	bool hasData() const;
	/***/
	size_t size() const;
	/** Generates a hash value for this buffer, using it's contents.
	@return						hash value for this buffer. */
	const size_t hash() const;
	/***/
	char * cArray() const;
	/***/
	std::byte * data() const;
	/***/
	std::byte & operator[](const size_t & index) const;


	// Public Derivators
	/** Compresses a source buffer into an equal or smaller-sized destination buffer.
	After compression, it applies a small header describing how large the uncompressed buffer is.
	destinationBuffer format:
	------------------------------------------------------------------
	| header: identifier title, uncompressed size | compressed data  |
	------------------------------------------------------------------
	@param	sourceBuffer		the buffer to read from.
	@param	destinationBuffer	reference updated with a compressed version of the source buffer.
	@return						true if compression success, false otherwise. */
	std::optional<Buffer> compress() const;
	/** Decompressess a source buffer into an equal or larger-sized destination buffer.
	@param	sourceBuffer		the buffer to read from.
	@param	destinationBuffer	reference updated with a decompressed version of the source buffer.
	@return						true if decompression success, false otherwise. */
	std::optional<Buffer> decompress() const;
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
	std::optional<Buffer> diff(const Buffer & target);
	/** Reads from a compressed instruction set, uses it to patch the 'older' buffer into the 'newer' buffer
	@note						caller expected to clean-up buffer_new on their own
	@param	buffer_old			the older of the 2 buffers.
	@param	buffer_new			reference updated with the 'new' buffer, derived from the 'old' + diff instructions.
	@param	buffer_diff			the compressed diff buffer (instruction set).
	@param	instructionCount	(optional) pointer to update with the number of instructions processed.
	@return						true if patch success, false otherwise. */
	std::optional<Buffer> patch(const Buffer & diffBuffer);

	
	// Public Modifiers
	/***/
	void resize(const size_t & size);
	/***/
	void release();


private:
	// Private Attributes
	std::vector<std::byte> m_data;
};


/** Namespace to keep buffer-related operations grouped together. */
namespace BFT {
	/** Parses the header of an nSuite compressed buffer, updating parameters like the data portion of the buffer if successfull.
	@param	buffer				the entire package buffer to parse.
	@param	uncompressedSize	reference updated with the uncompressed size of the package.
	@param	dataBuffer			reference updated with a buffer containing the remaining data portion of the package.
	@return						true if the nSuite compressed buffer is formatted correctly and could be parsed, false otherwise. */
	bool ParseHeader(const Buffer & buffer, size_t & uncompressedSize, Buffer & dataBuffer);
	/** Increment a pointer's address by the offset provided.
	@param	ptr					the pointer to increment by the offset amount.
	@param	offset				the offset amount to apply to the pointer's address.
	@return						the modified pointer address. */
	void * PTR_ADD(void *const ptr, const size_t & offset);
};

#endif // BUFFER_TOOLS_H