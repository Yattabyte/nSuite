#pragma once
#ifndef YATTA_BUFFER_H
#define YATTA_BUFFER_H

#include "memoryRange.hpp"
#include <memory>
#include <optional>
#include <type_traits>


namespace yatta {
    /** An expandable container representing a contiguous memory space.
    Allocates 2x its creation size, expanding when its capacity is exhausted. */
    class Buffer : public MemoryRange {
    public:
        // Public (de)Constructors
        /** Destroy the buffer, freeing the memory underlying memory. */
        ~Buffer() = default;
        /** Construct an empty buffer, allocating no memory. */
        Buffer();
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
        /** Returns the total size + reserved capacity of memory allocated by this buffer.
        @return						actual number of bytes allocated. */
        size_t capacity() const noexcept;


        // Public Manipulation Methods
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
        // push_type
        // push_raw
        // pop_type
        // pop_raw


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
        @return						a pointer to the diff buffer on diff success, empty otherwise.
        Buffer format:
        -----------------------------------------------------------------------------------
        | header: identifier title, final target file size | compressed instruction data  |
        ----------------------------------------------------------------------------------- */
        [[nodiscard]] Buffer diff(const Buffer& target) const;
        /** Generates a differential buffer containing patch instructions to get from SOURCE ->to-> TARGET.
        @param	source				the older of the 2 buffers.
        @param	target				the newer of the 2 buffers.
        @return						a pointer to the diff buffer on diff success, empty otherwise.
        Buffer format:
        -----------------------------------------------------------------------------------
        | header: identifier title, final target file size | compressed instruction data  |
        ----------------------------------------------------------------------------------- */
        [[nodiscard]] static Buffer diff(const Buffer& source, const Buffer& target);
        /** Generates a differential buffer containing patch instructions to get from SOURCEMEMORY ->to-> TARGETMEMORY.
        @param	sourceMemory		the older of the 2 memory ranges.
        @param	targetMemory    	the newer of the 2 memory ranges.
        @return						a pointer to the diff buffer on diff success, empty otherwise.
        Buffer format:
        -----------------------------------------------------------------------------------
        | header: identifier title, final target file size | compressed instruction data  |
        ----------------------------------------------------------------------------------- */
        [[nodiscard]] static Buffer diff(const MemoryRange& sourceMemory, const MemoryRange& targetMemory);
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
        /** Size of memory allocated. */
        size_t m_capacity = 0ULL;
        /** Underlying data pointer. */
        std::unique_ptr<std::byte[]> m_data = nullptr;
    };
};

#endif // YATTA_BUFFER_H