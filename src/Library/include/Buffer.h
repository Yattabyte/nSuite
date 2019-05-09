#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include <optional>


namespace NST {
	/** An expandable container representing a contiguous memory space. 
	Allocates 2x its creation size, expanding when its capacity is exhausted. */
	class Buffer {
	public:
		struct Header;
		struct CompressionHeader;
		struct DiffHeader;


		// Public (de)Constructors
		/** Destroy the buffer, erasing all buffer contents and freeing its memory. */
		~Buffer();
		/** Constructs an empty buffer, allocating no memory. */
		Buffer() = default;
		/** Constructs a buffer of the specified size in bytes.
		@param	size				the number of bytes to have available. */
		Buffer(const size_t & size);
		/** Constructs a buffer from another region of memory, pointing to it or copying from it.
		@param	pointer				pointer to a region of memory to adopt from.
		@param	range				the number of bytes to use.
		@param	hardCopy			if true: hard-copies the region's data, if false: adopts the pointer and NEVER calls delete[] on it.
		@note	resizing may eventually force a hard copy, and the old pointer will be discarded (preserving data). */
		Buffer(std::byte * pointer, const size_t & range, bool hardCopy = true);
		/** Constructs a buffer from another region of memory, pointing to it or copying from it.
		@param	pointer				pointer to a region of memory to adopt from.
		@param	range				the number of bytes to use.
		@param	hardCopy			if true: hard-copies the region's data, if false: adopts the pointer and NEVER calls delete[] on it.
		@note	resizing may eventually force a hard copy, and the old pointer will be discarded (preserving data). */
		inline Buffer(void * pointer, const size_t & range, bool hardCopy = true)
			: Buffer(reinterpret_cast<std::byte*>(pointer), range, hardCopy) { }
		/** Copy Constructor. 
		@param	other				the buffer to copy from. */
		Buffer(const Buffer & other);
		/** Move Constructor. 
		@param	other				the buffer to move from and invalidate. */
		Buffer(Buffer && other);


		


		// Public Operators
		/** Copy-assignment operator.
		@param	other				the buffer to copy from.
		@return						reference to this. */
		Buffer & operator=(const Buffer & other);
		/** Move-assignment operator.
		@param	other				the buffer to move from.
		@return						reference to this. */
		Buffer & operator=(Buffer && other);


		// Public Methods
		/** Retrieves whether or not this buffer is not zero-sized.
		@return						true if has memory allocated, false otherwise. */
		const bool hasData() const;
		/** Returns the size of memory allocated by this buffer.
		@return						number of bytes allocated. */
		const size_t & size() const;
		/** Generates a hash value derived from this buffer's contents.
		@return						hash value for this buffer. */
		const size_t hash() const;
		/** Changes the size of this buffer to the new size specified.
		@note	Will expand if the size requested is greater than the buffer's internal capacity.
		@note	Expansion will copy over all data that will fit.
		@note	Current memory model doesn't shrink, so no data should be lost.
		@note	When capacity is expdended, invalidates previous pointers retrieved by cArray(), data(), [], etc.
		@param	size				the new size to use. */
		void resize(const size_t & size);
		/** Erases all buffer content and frees the memory this buffer allocated.
		@note	Invalidates previous pointers retrieved by cArray(), data(), [], etc. */
		void release();


		// Public Mutable Data Accessors
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
		/** Writes-in data from an input pointer, from a specific byte offset and size, to this buffer.
		@param	inputPtr			pointer to copy the data from.
		@param	size				the number of bytes to copy.
		@param	byteOffset			how many bytes into this buffer to begin copying to.
		@return						new byte offset = byteOffset + size. */
		size_t writeData(const void * inputPtr, const size_t & size, const size_t byteOffset = 0);


		// Public Manipulation Methods
		/** Compresses this buffer into an equal or smaller-sized version.
		@return						a pointer to the compressed buffer on compression succes, empty otherwise.		
		Buffer format:
		------------------------------------------------------------------
		| header: identifier title, uncompressed size | compressed data  |
		------------------------------------------------------------------ */
		std::optional<Buffer> compress() const;
		/** Generates a decompressed version of this buffer.
		@return						a pointer to the decompressed buffer on decompression succes, empty otherwise. */
		std::optional<Buffer> decompress() const;
		/** Generates a differential buffer containing patch instructions to get from THIS ->to-> TARGET.
		@param	target				the newer of the 2 buffers.
		@return						a pointer to the diff buffer on diff succes, empty otherwise. 		
		Buffer format:
		-----------------------------------------------------------------------------------
		| header: identifier title, final target file size | compressed instruction data  |
		----------------------------------------------------------------------------------- */
		std::optional<Buffer> diff(const Buffer & target) const;
		/** Generates a patched version of this buffer, using data found in the supplied diff buffer.
		@param	diffBuffer			the diff buffer to patch with.
		@return						a pointer to the patched buffer on patch succes, empty otherwise. */
		std::optional<Buffer> patch(const Buffer & diffBuffer) const;


		// Public Header Methods
		/** Reads-in data from an input buffer's header, updating the data in the parameter pointers supplied.
		@param	header				specific subclass of header to hold the header contents.
		@param	dataPtr				pointer updated with a pointer to the remaining data in the buffer, found after the header.
		@param	dataSize			reference updated with the size of the remaining data ( = buffer size - header size ). */
		void readHeader(Header * header, std::byte ** dataPtr, size_t & dataSize) const;
		/** Writes-in data from an input header to the beginning of this buffer. Shifts old data down by the header size.
		@note	Invalidates previous pointers retrieved by cArray(), data(), [], etc.
		@param	header				header to copy the data from. */
		void writeHeader(const Header * header);


		// Public Header Structs
		/** A structure for holding and performing header I/O operations on buffers. */
		struct Header {
			// Attributes
			static constexpr const size_t TITLE_SIZE = 16ull;
			char m_title[TITLE_SIZE] = { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };


			// (de)Constructors
			~Header() = default;
			Header() = default;
			/** Constructs a header with the title specified. */
			inline Header(const char * title) {
				strcpy_s(&m_title[0], TITLE_SIZE, title);
			}


			// Interface
			/** Retrieve whether or not this header is valid, after populating it.
			@return					true if valid, false otherwise. */
			virtual bool isValid() const = 0;
			/** Retrieves the size of this header.
			@return					the size of this header. */
			virtual size_t size() const = 0;
			/** Updates THIS with the data found in the input pointer.
			@param	ptr				the pointer to source data from.
			@return	offset version of the input ptr by the number of bytes read from it. */
			inline virtual std::byte * operator << (std::byte * ptr) {
				std::copy(ptr, ptr + TITLE_SIZE, reinterpret_cast<std::byte*>(&m_title[0]));
				return &ptr[TITLE_SIZE];
			}
			/** Updates the data in the input pointer with the data found in THIS.
			@param	ptr				the pointer to write data to.
			@return	offset version of the input ptr by the number of bytes written to it. */
			inline virtual std::byte * operator >> (std::byte * ptr) const {
				std::copy(m_title, m_title + TITLE_SIZE, reinterpret_cast<char*>(ptr));
				return &ptr[TITLE_SIZE];
			}
		};
		/** Holds and performs Compression I/O operations on buffers. */
		struct CompressionHeader : Header {
			// Attributes
			size_t m_uncompressedSize = 0ull;


			// (de)Constructors
			inline CompressionHeader(const size_t size = 0ull) : Header("nSuite cmpress"), m_uncompressedSize(size) {}


			// Interface Implementation
			inline virtual bool isValid() const override {
				return (std::strcmp(m_title, "nSuite cmpress") == 0);
			}
			inline virtual size_t size() const override {
				return size_t(sizeof(size_t));
			}
			inline virtual std::byte * operator << (std::byte * ptr) override {
				ptr = Header::operator<<(ptr);
				std::copy(ptr, &ptr[size()], reinterpret_cast<std::byte*>(&m_uncompressedSize));
				return &ptr[size()];
			}
			inline virtual std::byte *operator >> (std::byte * ptr) const override {
				ptr = Header::operator>>(ptr);
				*reinterpret_cast<size_t*>(ptr) = m_uncompressedSize;
				return &ptr[size()];
			}
		};
		/** Holds and performs Diff I/O operations on buffers. */
		struct DiffHeader : Header {
			// Attributes
			size_t m_targetSize = 0ull;


			// (de)Constructors
			inline DiffHeader(const size_t size = 0ull) : Header("nSuite differ"), m_targetSize(size) {}


			// Interface Implementation
			inline virtual bool isValid() const override {
				return (std::strcmp(m_title, "nSuite differ") == 0);
			}
			inline virtual size_t size() const override {
				return size_t(sizeof(size_t));
			}
			inline virtual std::byte * operator << (std::byte * ptr) override {
				ptr = Header::operator>>(ptr);
				std::copy(ptr, &ptr[size()], reinterpret_cast<std::byte*>(&m_targetSize));
				return &ptr[size()];
			}
			inline virtual std::byte *operator >> (std::byte * ptr) const override {
				ptr = Header::operator>>(ptr);
				*reinterpret_cast<size_t*>(ptr) = m_targetSize;
				return &ptr[size()];
			}
		};


		// Public Diff-Instruction Structure
		/** Super-class for buffer diff instructions. */
		struct Differential_Instruction {
			// Constructor
			inline Differential_Instruction(const char & t) : m_type(t) {}


			// Interface Declaration
			/** Retrieve the bytesize of this instruction. */
			virtual size_t size() const = 0;
			/** Exectute this instruction. */
			virtual void execute(NST::Buffer & bufferNew, const NST::Buffer & bufferOld) const = 0;
			/** Write-out this instruction to a buffer. */
			virtual void write(NST::Buffer & outputBuffer, size_t & byteIndex) const = 0;
			/** Read-in this instruction from a buffer. */
			virtual void read(const NST::Buffer & inputBuffer, size_t & byteIndex) = 0;


			// Attributes
			const char m_type = 0;
			size_t m_index = 0ull;
		};

		
	private:
		// Private Methods
		/** Allocate @capacity amount of memory for this buffer.
		@param	capacity			the amount of bytes to allocate. */
		void alloc(const size_t & capacity);

		
		// Private Attributes
		std::byte * m_data = nullptr;
		size_t m_size = 0ull, m_capacity = 0ull;
		bool m_ownsData = false;
	};
};

#endif // BUFFER_H