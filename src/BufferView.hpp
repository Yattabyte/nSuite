#pragma once
#ifndef BUFFERVIEW_H
#define BUFFERVIEW_H

#include "Buffer.hpp"


namespace yatta {
    /** An expandable container representing a contiguous memory space.
    Allocates 2x its creation size, expanding when its capacity is exhausted. */
    class BufferView : public MemoryRange {
    public:
        // Public Constructors
        /** Construct a buffer view from the specified buffer.
        @param  buffer              the buffer to construct a view from. */
        explicit BufferView(const Buffer& buffer) noexcept;
        /** Construct a buffer view from the specified pointer and size.
        @param	size				the number of bytes in the range.
        @param	dataPtr			    pointer to some data source. */
        BufferView(const size_t& size, std::byte* dataPtr) noexcept;


        // Public Derivation Methods
        /** Compresses this buffer into an equal or smaller-sized version.
        @return						a pointer to the compressed buffer on compression success, empty otherwise.
        Buffer format:
        ------------------------------------------------------------------
        | header: identifier title, uncompressed size | compressed data  |
        ------------------------------------------------------------------ */
        [[nodiscard]] std::optional<Buffer> compress() const;
        /** Generates a decompressed version of this buffer.
        @return						a pointer to the decompressed buffer on decompression success, empty otherwise. */
        [[nodiscard]] std::optional<Buffer> decompress() const;
        /** Generates a differential buffer containing patch instructions to get from THIS ->to-> TARGET.
        @param	target				the newer of the 2 buffers.
        @param	maxThreads			the number of threads to use in accelerating the operation.
        @return						a pointer to the diff buffer on diff success, empty otherwise.
        Buffer format:
        -----------------------------------------------------------------------------------
        | header: identifier title, final target file size | compressed instruction data  |
        ----------------------------------------------------------------------------------- */
        [[nodiscard]] std::optional<Buffer> diff(const BufferView& target, const size_t& maxThreads) const;
        /** Generates a patched version of this buffer, using data found in the supplied diff buffer.
        @param	diffBuffer			the diff buffer to patch with.
        @return						a pointer to the patched buffer on patch success, empty otherwise. */
        [[nodiscard]] std::optional<Buffer> patch(const BufferView& diffBuffer) const;
    };
};

#endif // BUFFERVIEW_H