#pragma once
#ifndef YATTA_BUFFER_H
#define YATTA_BUFFER_H

#include "MemoryRange.hpp"
#include <memory>
#include <type_traits>


namespace yatta {
    /** An expandable container representing a contiguous memory space.
    Allocates 2x its creation size, expanding when its capacity is exhausted. */
    class Buffer {
    public:
        // Public (de)Constructors
        /** Destroy the buffer, freeing the memory underlying memory. */
        ~Buffer() = default;
        /** Construct an empty buffer, allocating no memory. */
        Buffer() = default;
        /** Construct a buffer of the specified size in bytes.
        @param	size				the number of bytes to allocate. */
        explicit Buffer(const size_t& size);
        /** Construct a buffer, copying from another buffer.
        @param	other				the buffer to copy from. */
        Buffer(const Buffer& other);
        /** Construct a buffer, moving from another buffer.
        @param	other				the buffer to move from. */
        Buffer(Buffer&& other) noexcept;


        // Public Assignment Operators
        /** Copy-assignment operator.
        @param	other				the buffer to copy from.
        @return						reference to this. */
        Buffer& operator=(const Buffer& other);
        /** Move-assignment operator.
        @param	other				the buffer to move from.
        @return						reference to this. */
        Buffer& operator=(Buffer&& other) noexcept;


        // Public Inquiry Methods
        /** Check if this buffer is empty - has no data allocated.
        @return						true if no memory has been allocated, false otherwise. */
        bool empty() const noexcept;
        /** Retrieves whether or not this buffer's size is greater than zero.
        @return						true if has memory allocated, false otherwise. */
        bool hasData() const noexcept;
        /** Returns the size of memory allocated by this buffer.
        @return						number of bytes allocated. */
        size_t size() const noexcept;
        /** Returns the total size + reserved capacity of memory allocated by this buffer.
        @return						actual number of bytes allocated. */
        size_t capacity() const noexcept;
        /** Generates a hash value derived from this buffer's contents.
        @return						hash value for this buffer. */
        size_t hash() const;


        // Public Manipulation Methods
        /** Retrieves a reference to the data at the byte index specified.
        @param	byteIndex			how many bytes into this buffer to index at.
        @return						reference to data found at the byte index. */
        [[nodiscard]] std::byte& operator[](const size_t& byteIndex);
        /** Retrieves a const reference to the data at the byte index specified.
        @param	byteIndex			how many bytes into this buffer to index at.
        @return						reference to data found at the byte index. */
        [[nodiscard]] const std::byte& operator[](const size_t& byteIndex) const;
        /** Retrieves a char array pointer to this buffer's data.
        Does not copy underlying data.
        @return						data pointer cast to char *. */
        [[nodiscard]] char* charArray() const noexcept;
        /** Retrieves a raw pointer to this buffer's data.
        Does not copy underlying data.
        @return						pointer to this buffer's data. */
        [[nodiscard]] std::byte* bytes() const noexcept;
        /** Changes the size of this buffer, expanding if need be.
        @note	won't ever reduce the capacity of the container.
        @note	will invalidate previous pointers when expanding.
        @param	size				the new size to use. */
        void resize(const size_t& size);
        /** Reduces the capacity of this buffer down to its size. */
        void shrink();
        /** Clear the size and capacity of this buffer, freeing its memory. */
        void clear() noexcept;


        // Public IO Methods
        /** Copies raw data found in the input pointer into this buffer.
        @param	dataPtr				pointer to copy the data from.
        @param	size				the number of bytes to copy.
        @param	byteIndex			the destination index to begin copying to. */
        void in_raw(const void* const dataPtr, const size_t& size, const size_t byteIndex = 0) {
            MemoryRange{ m_size, bytes() }.in_raw(dataPtr, size, byteIndex);
        }
        /** Copies a data object into this buffer.
        @tparam T					the data type (auto-deducible).
        @param	dataObj				const reference to some object to copy from.
        @param	byteIndex			the destination index to begin copying to. */
        template <typename T>
        void in_type(const T& dataObj, const size_t byteIndex = 0) {
            MemoryRange{ m_size, bytes() }.in_type(dataObj, byteIndex);
        }
        /** Copies raw data found in this buffer out to the specified pointer.
        @param	dataPtr				pointer to copy the data into.
        @param	size				the number of bytes to copy.
        @param	byteIndex			the destination index to begin copying from. */
        void out_raw(void* const dataPtr, const size_t& size, const size_t byteIndex = 0) const {
            MemoryRange{ m_size, bytes() }.out_raw(dataPtr, size, byteIndex);
        }
        /** Copies data found in this buffer out to a data object.
        @tparam T					the data type (auto-deducible).
        @param	dataObj				reference to some object to copy into.
        @param	byteIndex			the destination index to begin copying from. */
        template <typename T>
        void out_type(T& dataObj, const size_t byteIndex = 0) const {
            MemoryRange{ m_size, bytes() }.out_type(dataObj, byteIndex);
        }


        // Public Derivation Methods
        /** Compresses this buffer into an equal or smaller-sized buffer.
        @return						a pointer to the compressed buffer on compression success, empty otherwise.
        Buffer format:
        ------------------------------------------------------------------
        | header: identifier title, uncompressed size | compressed data  |
        ------------------------------------------------------------------ */
        [[nodiscard]] Buffer compress() const;
        /** Compresses the supplied buffer into an equal or smaller-sized buffer.
        @param  buffer              the buffer to compress.
        @return						a pointer to the compressed buffer on compression success, empty otherwise.
        Buffer format:
        ------------------------------------------------------------------
        | header: identifier title, uncompressed size | compressed data  |
        ------------------------------------------------------------------ */
        [[nodiscard]] static Buffer compress(const Buffer& buffer);
        /** Compresses the supplied memory range into an equal or smaller-sized buffer.
        @param  memoryRange         the memory range to compress.
        @return						a pointer to the compressed buffer on compression success, empty otherwise.
        Buffer format:
        ------------------------------------------------------------------
        | header: identifier title, uncompressed size | compressed data  |
        ------------------------------------------------------------------ */
        [[nodiscard]] static Buffer compress(const MemoryRange& memoryRange);
        /** Generates a decompressed version of this buffer.
        @return						a pointer to the decompressed buffer on decompression success, empty otherwise. */
        [[nodiscard]] Buffer decompress() const;
        /** Generates a decompressed version of the supplied buffer.
        @param  buffer              the buffer to decompress.
        @return						a pointer to the decompressed buffer on decompression success, empty otherwise. */
        [[nodiscard]] static Buffer decompress(const Buffer& buffer);
        /** Generates a decompressed version of the supplied memory range.
        @param  memoryRange         the memory range to decompress.
        @return						a pointer to the decompressed buffer on decompression success, empty otherwise. */
        [[nodiscard]] static Buffer decompress(const MemoryRange& memoryRange);
        /** Generates a differential buffer containing patch instructions to get from THIS ->to-> TARGET.
        @param	target				the newer of the 2 buffers.
        @param	maxThreads			the number of threads to use in accelerating the operation.
        @return						a pointer to the diff buffer on diff success, empty otherwise.
        Buffer format:
        -----------------------------------------------------------------------------------
        | header: identifier title, final target file size | compressed instruction data  |
        ----------------------------------------------------------------------------------- */
        [[nodiscard]] Buffer diff(const Buffer& target, const size_t& maxThreads) const;
        /** Generates a differential buffer containing patch instructions to get from SOURCE ->to-> TARGET.
        @param	source				the older of the 2 buffers.
        @param	target				the newer of the 2 buffers.
        @param	maxThreads			the number of threads to use in accelerating the operation.
        @return						a pointer to the diff buffer on diff success, empty otherwise.
        Buffer format:
        -----------------------------------------------------------------------------------
        | header: identifier title, final target file size | compressed instruction data  |
        ----------------------------------------------------------------------------------- */
        [[nodiscard]] static Buffer diff(const Buffer& source, const Buffer& target, const size_t& maxThreads);
        /** Generates a differential buffer containing patch instructions to get from SOURCEMEMORY ->to-> TARGETMEMORY.
        @param	sourceMemory		the older of the 2 memory ranges.
        @param	targetMemory    	the newer of the 2 memory ranges.
        @param	maxThreads			the number of threads to use in accelerating the operation.
        @return						a pointer to the diff buffer on diff success, empty otherwise.
        Buffer format:
        -----------------------------------------------------------------------------------
        | header: identifier title, final target file size | compressed instruction data  |
        ----------------------------------------------------------------------------------- */
        [[nodiscard]] static Buffer diff(const MemoryRange& sourceMemory, const MemoryRange& targetMemory, const size_t& maxThreads);
        /** Generates a patched version of this buffer, using data found in the supplied diff buffer.
        @param	diffBuffer			the diff buffer to patch with.
        @return						a pointer to the patched buffer on patch success, empty otherwise. */
        [[nodiscard]] Buffer patch(const Buffer& diffBuffer) const;
        /** Generates a patched version of the supplied source buffer, using data found in the supplied diff buffer.
        @param	sourceBuffer		the source buffer to patch from.
        @param	diffBuffer			the diff buffer to patch with.
        @return						a pointer to the patched buffer on patch success, empty otherwise. */
        [[nodiscard]] static Buffer patch(const Buffer& sourceBuffer, const Buffer& diffBuffer);
        /** Generates a patched version of the supplied source memory, using data found in the supplied diff memory.
        @param	sourceMemory		the source memory range to patch from.
        @param	diffMemory			the diff memory range to patch with.
        @return						a pointer to the patched buffer on patch success, empty otherwise. */
        [[nodiscard]] static Buffer patch(const MemoryRange& sourceMemory, const MemoryRange& diffMemory);


    protected:
        // Protected Attributes
        /** Size of memory in use. */
        size_t m_size = 0ULL;
        /** Size of memory allocated. */
        size_t m_capacity = 0ULL;
        /** Underlying data pointer. */
        std::unique_ptr<std::byte[]> m_data = nullptr;
    };
};

#endif // YATTA_BUFFER_H