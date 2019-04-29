#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include <optional>
#include <vector>

constexpr const char CBUFFER_H_TITLE[] = "nSuite cbuffer";
constexpr const char DBUFFER_H_TITLE[] = "nSuite dbuffer";
constexpr const size_t CBUFFER_H_TITLE_SIZE = (sizeof(CBUFFER_H_TITLE) / sizeof(*CBUFFER_H_TITLE));
constexpr const size_t DBUFFER_H_TITLE_SIZE = (sizeof(DBUFFER_H_TITLE) / sizeof(*DBUFFER_H_TITLE));
constexpr const size_t CBUFFER_H_SIZE = CBUFFER_H_TITLE_SIZE + size_t(sizeof(size_t));
constexpr const size_t DBUFFER_H_SIZE = DBUFFER_H_TITLE_SIZE + size_t(sizeof(size_t));


/** Add Buffer to nSuite NST namespace. */
namespace NST {
	/** Generic byte array encapsulation, adaptor for std::vector. */
	class Buffer {
	public:
		// Public (de)Constructors
		/** Destroy the buffer, erasing all buffer contents and freeing its memory. */
		~Buffer();
		/** Constructs an empty buffer, allocating no memory. */
		Buffer();
		/** Constructs a buffer of the specified size.
		@param	size				the amount of memory to allocate. */
		Buffer(const size_t & size);
		/** Constructs a buffer that copies another region of memory.
		@param	pointer				pointer to a region of memory to copy from.
		@param	range				the number of bytes to copy. */
		Buffer(const void * pointer, const size_t & range);


		// Public Derivators
		/** Generates a compressed version of this buffer.
		Compressed buffer format:
		------------------------------------------------------------------
		| header: identifier title, uncompressed size | compressed data  |
		------------------------------------------------------------------
		@return						a pointer to the compressed buffer on compression succes, empty otherwise. */
		std::optional<Buffer> compress() const;
		/** Generates a decompressed version of this buffer.
		@return						a pointer to the decompressed buffer on decompression succes, empty otherwise. */
		std::optional<Buffer> decompress() const;
		/** Generates a differential buffer containing patch instructions to get from THIS ->to-> TARGET.
		Diff buffer format:
		-----------------------------------------------------------------------------------
		| header: identifier title, final target file size | compressed instruction data  |
		-----------------------------------------------------------------------------------
		@param	target				the newer of the 2 buffers.
		@return						a pointer to the diff buffer on diff succes, empty otherwise. */
		std::optional<Buffer> diff(const Buffer & target) const;
		/** Generates a patched version of this buffer, using data found in the supplied diff buffer.
		@param	diffBuffer			the diff buffer to patch with.
		@return						a pointer to the patched buffer on patch succes, empty otherwise. */
		std::optional<Buffer> patch(const Buffer & diffBuffer) const;


		// Public Accessors
		/** Retrieves whether or not this buffer is not zero-sized.
		@return						true if has memory allocated, false otherwise. */
		bool hasData() const;
		/** Returns the size of memory allocated by this buffer.
		@return						number of bytes allocated. */
		size_t size() const;
		/** Generates a hash value derived from this buffer's contents.
		@return						hash value for this buffer. */
		const size_t hash() const;
		/** Retrieves a char array pointer to this buffer's data.
		Does not copy underlying data.
		@return						data pointer cast to char *. */
		char * cArray() const;
		/** Retrieves a raw pointer to this buffer's data.
		Does not copy underlying data.
		@return						pointer to this buffer's data. */
		std::byte * data() const;
		/** Retrieves the data found in this bufffer at the byte offset specified.
		@param	byteOffset			how many bytes into this buffer to index at.
		@return						reference to data found at the byte offset. */
		std::byte & operator[](const size_t & byteOffset) const;
		/** Reads-out data found in this buffer, from a specific byte offset and size, to a given output pointer.
		@param	outputPtr			pointer to copy the data to.
		@param	size				the number of bytes to copy.
		@param	byteOffset			how many bytes into this buffer to begin copying from.
		@return						new byte offset = byteOffset + size. */
		size_t readData(void * outputPtr, const size_t & size, const size_t byteOffset = 0) const;


		// Public Modifiers
		/** Writes-in data from an input pointer, from a specific byte offset and size, to this buffer.
		@param	inputPtr			pointer to copy the data from.
		@param	size				the number of bytes to copy.
		@param	byteOffset			how many bytes into this buffer to begin copying to.
		@return						new byte offset = byteOffset + size. */
		size_t writeData(const void * inputPtr, const size_t & size, const size_t byteOffset = 0);
		/** Changes the size of this buffer to the new size specified.
		@note	Requires allocating a new buffer of the target size and copying over old data.
		@note	If the new size is smaller than the old one, the trailing data will be lost.
		@param	size				the new size to use. */
		void resize(const size_t & size);
		/** Erases all buffer content and frees the memory this buffer allocated.
		@note	All pointers given by this buffer are invalidated. */
		void release();


	private:
		// Private Attributes
		std::vector<std::byte> m_data;
	};
};

#endif // BUFFER_H