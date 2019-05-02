#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include <optional>


/** Add Buffer to nSuite NST namespace. */
namespace NST {
	/** Generic byte array encapsulation, adaptor for std::vector. */
	class Buffer {
	public:
		struct Header;
		struct CompressionHeader;
		struct DiffHeader;

		// Public (de)Constructors
		/** Destroy the buffer, erasing all buffer contents and freeing its memory. */
		~Buffer();
		/** Constructs an empty buffer, allocating no memory. */
		Buffer();
		/** Constructs a buffer of the specified size.
		@param	size				the amount of memory to allocate. */
		Buffer(const size_t & size);
		/** Constructs a buffer from another region of memory, pointing to it or copying from it.
		@param	pointer				pointer to a region of memory to adopt from.
		@param	range				the number of bytes to use.
		@param	hardCopy			when true, copies the region, when false safely points to the region and NEVER deletes it. */
		Buffer(std::byte * pointer, const size_t & range, bool hardCopy = false);
		/** Copy Constructor. 
		@param	other				the buffer to copy from. */
		Buffer(const Buffer & other);
		/** Move Constructor. 
		@param	other				the buffer to move from and invalidate. */
		Buffer(Buffer && other);



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
		/***/
		void readHeader(Header * header, std::byte ** dataPtr, size_t & dataSize) const;


		// Public Modifiers
		/** Copy-assignment operator. 
		@param	other				the buffer to copy from. 
		@return						reference to this. */
		Buffer & operator=(const Buffer & other);
		/** move-assignment operator. 
		@param	other				the buffer to move from. 
		@return						reference to this. */
		Buffer & operator=(Buffer && other);
		/** Writes-in data from an input pointer, from a specific byte offset and size, to this buffer.
		@param	inputPtr			pointer to copy the data from.
		@param	size				the number of bytes to copy.
		@param	byteOffset			how many bytes into this buffer to begin copying to.
		@return						new byte offset = byteOffset + size. */
		size_t writeData(const void * inputPtr, const size_t & size, const size_t byteOffset = 0);
		/** Writes-in data from an input header buffer to the beginning of this buffer. Shifts old data down by the header size.
		@note	Invalidates previous pointers retrieved by cArray(), data(), [], etc
		@param	header				header to copy the data from. */
		void writeHeader(const Header * header);
		/** Changes the size of this buffer to the new size specified.
		@note	Requires allocating a new buffer of the target size and copying over old data.
		@note	If the new size is smaller than the old one, the trailing data will be lost.
		@note	Invalidates previous pointers retrieved by cArray(), data(), [], etc
		@param	size				the new size to use. */
		void resize(const size_t & size);
		/** Erases all buffer content and frees the memory this buffer allocated.
		@note	Invalidates previous pointers retrieved by cArray(), data(), [], etc */
		void release();


		// Public Header Information
		/** A structure for holding and performing header I/O operations on buffers. */
		struct Header {
			// Attributes
			static constexpr const size_t TITLE_SIZE = 16ull;
			char m_title[TITLE_SIZE] = { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };


			// (de) Constructors
			~Header() = default;
			Header() = default;
			inline Header(const char * title) {
				strcpy_s(&m_title[0], TITLE_SIZE, title);
			}


			// Interface
			virtual size_t size() const = 0;
			/** Updates THIS with the data found in the input pointer. */
			inline virtual void * operator << (void * ptr) {
				std::memcpy(&m_title[0], ptr, TITLE_SIZE);
				return &(reinterpret_cast<char*>(ptr)[TITLE_SIZE]);
			}
			/** Updates the data in the input pointer with the data found in THIS. */
			inline virtual void * operator >> (void * ptr) const {
				std::memcpy(ptr, &m_title[0], TITLE_SIZE);
				return &reinterpret_cast<char*>(ptr)[TITLE_SIZE];
			}
		};
		/** Holds and performs Compression I/O operations on buffers. */
		struct CompressionHeader : Header {
			// Attributes
			static constexpr const char TITLE[] = "nSuite cmpress";
			size_t m_uncompressedSize = 0ull;


			// (de)Constructors
			inline CompressionHeader(const size_t size = 0ull) : Header(TITLE), m_uncompressedSize(size) {}


			// Interface Implementation
			inline virtual size_t size() const override {
				return size_t(sizeof(size_t));
			}
			inline virtual void * operator << (void * ptr) override {
				ptr = Header::operator<<(ptr);
				std::memcpy(&m_uncompressedSize, ptr, size());
				return &(reinterpret_cast<char*>(ptr)[size()]);
			}
			inline virtual void *operator >> (void * ptr) const override {
				ptr = Header::operator>>(ptr);
				std::memcpy(ptr, &m_uncompressedSize, size());
				return &reinterpret_cast<char*>(ptr)[size()];
			}
		};
		/** Holds and performs Diff I/O operations on buffers. */
		struct DiffHeader : Header {
			// Attributes
			static constexpr const char TITLE[] = "nSuite differ";
			size_t m_targetSize = 0ull;


			// (de)Constructors
			inline DiffHeader(const size_t size = 0ull) : Header(TITLE), m_targetSize(size) {}


			// Interface Implementation
			inline virtual size_t size() const override {
				return size_t(sizeof(size_t));
			}
			inline virtual void * operator << (void * ptr) override {
				ptr = Header::operator>>(ptr);
				std::memcpy(&m_targetSize, ptr, size());
				return &(reinterpret_cast<char*>(ptr)[size()]);
			}
			inline virtual void *operator >> (void * ptr) const override {
				ptr = Header::operator>>(ptr);
				std::memcpy(ptr, &m_targetSize, size());
				return &reinterpret_cast<char*>(ptr)[size()];
			}
		};


	private:
		// Private Attributes
		bool m_ownsData = true;
		std::byte * m_data = nullptr;
		size_t m_size = 0ull, m_capacity = 0ull;
	};
};

#endif // BUFFER_H