#pragma once
#ifndef YATTA_BUFFER_H
#define YATTA_BUFFER_H

#include "memoryRange.hpp"
#include <memory>
#include <optional>
#include <type_traits>


namespace yatta {
    /** An expandable contiguous memory range, similar to a std::vector<std::byte>.
    Allocates double its size, and may reallocate when the size > capacity.
    Inherits all memory range functions, and provides pushing, popping,
    compressing, expanding, diffing, and patching operations. */
    class Buffer : public MemoryRange {
    public:
        // Public (de)Constructors
        /** Destroy the buffer, freeing any allocated memory. */
        ~Buffer() = default;
        /** Construct an empty buffer. */
        Buffer() = default;
        /** Construct a buffer of the specified byte size.
        @param  size            the number of bytes to allocate. */
        explicit Buffer(const size_t& size);
        /** Construct a buffer, copying from another buffer.
        @param  other           the buffer to copy from. */
        Buffer(const Buffer& other);
        /** Construct a buffer, moving from another buffer.
        @param  other           the buffer to move from. */
        Buffer(Buffer&& other) noexcept;


        // Public Assignment Operators
        /** Copy-assignment operator.
        @param  other           the buffer to copy from.
        @return                 reference to this. */
        Buffer& operator=(const Buffer& other);
        /** Move-assignment operator.
        @param  other           the buffer to move from.
        @return                 reference to this. */
        Buffer& operator=(Buffer&& other) noexcept;


        // Public Inquiry Methods
        /** Check if this buffer is empty.
        @return                 true if no memory has been allocated, false otherwise. */
        bool empty() const noexcept;
        /** Retrieve the total number of bytes allocated.
        @return                 the number of bytes allocated. */
        size_t capacity() const noexcept;


        // Public Manipulation Methods
        /** Change the size of this buffer, reallocating if size > capacity.
        @note   will invalidate previous pointers when reallocating.
        @param  size            the new size to use. */
        void resize(const size_t& size);
        /** Change the buffer's memory allocation to a specific capacity.
        @note   will invalidate previous pointers when reallocating.
        @param  capacity        the new memory capacity to use. */
        void reserve(const size_t& capacity);
        /** Reduces the capacity of this buffer down to its size,
        reallocating an equal or smaller size chunk of memory.
        @note   will invalidate previous pointers when reallocating. */
        void shrink();
        /** Free all allocated memory, and set the size and capacity to zero. */
        void clear() noexcept;


        // Public IO Methods
        /** Insert raw data onto the end of this buffer, increasing its size.
        @param  dataPtr         pointer to the data to insert.
        @param  size            the number of bytes to insert. */
        void push_raw(const void* const dataPtr, const size_t& size);
        /** Insert a specific object onto the end of this buffer, increasing its size.
        @tparam T               the type of the object to insert (auto-deducible).
        @param  dataObject      the specific object to insert. */
        template <typename T>
        void push_type(const T& dataObj) {
            const auto byteIndex = m_range;
            resize(m_range + sizeof(T));
            // Only reinterpret-cast if T is not std::byte
            if constexpr (std::is_same<T, std::byte>::value)
                m_dataPtr[byteIndex] = dataObj;
            else {
                // Instead of casting the buffer to type T, std::copy the range
                const auto dataObjPtr = reinterpret_cast<const std::byte*>(&dataObj);
                std::copy(dataObjPtr, dataObjPtr + sizeof(T), &m_dataPtr[byteIndex]);
            }
        }
        /** Retrieve raw data from the end of this buffer, decreasing its size.
        @param  dataPtr         pointer to the data to retrieve.
        @param  size            the number of bytes to retrieve. */
        void pop_raw(void* const dataPtr, const size_t& size);
        /** Retrieve a specific object from the end of this buffer, decreasing its size.
        @tparam T               the type of the object to retrieve (auto-deducible).
        @param  dataObject      the specific object to retrieve. */
        template <typename T>
        void pop_type(T& dataObj) {
            const auto byteIndex = m_range - sizeof(T);
            resize(m_range - sizeof(T));
            // Only reinterpret-cast if T is not std::byte
            if constexpr (std::is_same<T, std::byte>::value)
                dataObj = m_dataPtr[byteIndex];
            else {
                // Instead of casting the buffer to type T, std::copy the range
                auto dataObjPtr = reinterpret_cast<std::byte*>(&dataObj);
                std::copy(&m_dataPtr[byteIndex], &m_dataPtr[byteIndex + sizeof(T)], dataObjPtr);
            }
        }


        // Public Derivation Methods
        /** Compresses the contents of this buffer into a new buffer.
        @return                 the compressed buffer on success, empty otherwise. */
        [[nodiscard]] std::optional<Buffer> compress() const;
        /** Compresses the contents of the supplied buffer into a new buffer.
        @param  buffer          the buffer to compress.
        @return                 the compressed buffer on success, empty otherwise. */
        [[nodiscard]] static std::optional<Buffer> compress(const Buffer& buffer);
        /** Compresses the supplied memory range into a new buffer.
        @param  memoryRange     the memory range to compress.
        @return                 the compressed buffer on success, empty otherwise. */
        [[nodiscard]] static std::optional<Buffer> compress(const MemoryRange& memoryRange);
        /** Decompress the contents of this buffer into a new buffer.
        @return                 the decompressed buffer on success, empty otherwise. */
        [[nodiscard]] std::optional<Buffer> decompress() const;
        /** Decompress the contents of the supplied buffer into a new buffer.
        @param  buffer          the buffer to decompress.
        @return                 the decompressed buffer on success, empty otherwise. */
        [[nodiscard]] static std::optional<Buffer> decompress(const Buffer& buffer);
        /** Decompress the supplied memory range into a new buffer.
        @param  memoryRange     the memory range to decompress.
        @return                 the decompressed buffer on success, empty otherwise. */
        [[nodiscard]] static std::optional<Buffer> decompress(const MemoryRange& memoryRange);
        /** Diff this buffer against the supplied buffer, generating a patch instruction set.
        @param  target          the buffer to diff against.
        @return                 the diff buffer on success, empty otherwise. */
        [[nodiscard]] std::optional<Buffer> diff(const Buffer& target) const;
        /** Diff the supplied buffers against each other, generating a patch instruction set.
        @param  sourceBuffer    the buffer to diff from.
        @param  targetBuffer    the buffer to diff against.
        @return                 the diff buffer on success, empty otherwise. */
        [[nodiscard]] static std::optional<Buffer> diff(const Buffer& sourceBuffer, const Buffer& targetBuffer);
        /** Diff the supplied memory ranges against each other, generating a patch instruction set.
        @param  sourceMemory    the range to diff from.
        @param  targetMemory    the range to diff against.
        @return                 the diff buffer on success, empty otherwise. */
        [[nodiscard]] static std::optional<Buffer> diff(const MemoryRange& sourceMemory, const MemoryRange& targetMemory);
        /** Patch the contents of this buffer into a new buffer, using the supplied diff buffer.
        @param  diffBuffer      the patch instruction set to use.
        @return                 the patched buffer on success, empty otherwise. */
        [[nodiscard]] std::optional<Buffer> patch(const Buffer& diffBuffer) const;
        /** Patch the contents of the supplied buffer into a new buffer, using the supplied diff buffer.
        @param  sourceBuffer    the source buffer to patch from.
        @param  diffBuffer      the patch instruction set to use.
        @return                 the patched buffer on success, empty otherwise. */
        [[nodiscard]] static std::optional<Buffer> patch(const Buffer& sourceBuffer, const Buffer& diffBuffer);
        /** Patch the contents of the supplied memory range into a new buffer, using the supplied diff memory range.
        @param  sourceMemory    the source memory range to patch from.
        @param  diffMemory      the patch instruction set to use.
        @return                 the patched buffer on success, empty otherwise. */
        [[nodiscard]] static std::optional<Buffer> patch(const MemoryRange& sourceMemory, const MemoryRange& diffMemory);


    protected:
        // Protected Attributes
        /** Size of memory allocated. */
        size_t m_capacity = 0ULL;
        /** Underlying data pointer. */
        std::unique_ptr<std::byte[]> m_data = nullptr;
    };


    // Template Specializations

    template <> inline void Buffer::push_type(const std::string& dataObj) {
        // Copy in string size for forwards reading
        push_type(dataObj.size());
        // Copy in char data
        const auto stringSize = dataObj.size() * sizeof(char);
        push_raw(dataObj.data(), stringSize);
        // Copy in string size again for backwards reading
        push_type(dataObj.size());
    }
    template <> inline void Buffer::pop_type(std::string& dataObj) {
        // Copy out string size
        size_t stringSize(0ULL);
        pop_type(stringSize);
        // Copy out char data
        const auto chars = std::make_unique<char[]>(stringSize);
        pop_raw(chars.get(), stringSize);
        dataObj = std::string(chars.get(), stringSize);
        // Pop size again for forwards direction
        pop_type(stringSize);
    }
};

#endif // YATTA_BUFFER_H