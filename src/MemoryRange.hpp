#pragma once
#ifndef YATTA_MEMORYRANGE_H
#define YATTA_MEMORYRANGE_H

#include <memory>
#include <stdexcept>


namespace yatta {
    /** A range of pre-defined memory. */
    class MemoryRange {
    public:
        // Public Constructor
        /** Construct a memory range from the specified pointer and size.
        @param	size				the number of bytes in the range.
        @param	dataPtr			    pointer to some data source. */
        MemoryRange(const size_t& size, std::byte* dataPtr) noexcept;


        // Public Inquiry Methods
        /** Check if this buffer is empty - has no data allocated.
        @return						true if no memory has been allocated, false otherwise. */
        [[nodiscard]] bool empty() const noexcept;
        /** Retrieves whether or not this buffer's size is greater than zero.
        @return						true if has memory allocated, false otherwise. */
        [[nodiscard]] bool hasData() const noexcept;
        /** Returns the size of memory allocated by this buffer.
        @return						number of bytes allocated. */
        [[nodiscard]] size_t size() const noexcept;
        /** Generates a hash value derived from this buffer's contents.
        @return						hash value for this buffer. */
        [[nodiscard]] size_t hash() const;


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


        // Public IO Methods
        /** Copies raw data found in the input pointer into this buffer.
        @param	dataPtr				pointer to copy the data from.
        @param	size				the number of bytes to copy.
        @param	byteIndex			the destination index to begin copying to. */
        void in_raw(const void* const dataPtr, const size_t& size, const size_t byteIndex = 0);
        /** Copies a data object into this buffer.
        @tparam T					the data type (auto-deducible).
        @param	dataObj				const reference to some object to copy from.
        @param	byteIndex			the destination index to begin copying to. */
        template <typename T>
        void in_type(const T& dataObj, const size_t byteIndex = 0) {
            // Ensure pointers are valid
            if (m_dataPtr == nullptr)
                throw std::runtime_error("Invalid Memory Range (null pointer)");

            // Ensure data won't exceed range
            if ((sizeof(T) + byteIndex) > m_range)
                throw std::runtime_error("Memory Range index out of bounds");

            // Copy Data
            // Only reinterpret-cast if T is not std::byte
            if constexpr (std::is_same<T, std::byte>::value)
                m_dataPtr[byteIndex] = dataObj;
            else
                *reinterpret_cast<T*>(&m_dataPtr[byteIndex]) = dataObj;
        }
        /** Copies raw data found in this buffer out to the specified pointer.
        @param	dataPtr				pointer to copy the data into.
        @param	size				the number of bytes to copy.
        @param	byteIndex			the destination index to begin copying from. */
        void out_raw(void* const dataPtr, const size_t& size, const size_t byteIndex = 0) const;
        /** Copies data found in this buffer out to a data object.
        @tparam T					the data type (auto-deducible).
        @param	dataObj				reference to some object to copy into.
        @param	byteIndex			the destination index to begin copying from. */
        template <typename T>
        void out_type(T& dataObj, const size_t byteIndex = 0) const {
            // Ensure pointers are valid
            if (m_dataPtr == nullptr)
                throw std::runtime_error("Invalid Memory Range (null pointer)");

            // Ensure data won't exceed range
            if ((sizeof(T) + byteIndex) > m_range)
                throw std::runtime_error("Memory Range index out of bounds");

            // Copy Data
            // Only reinterpret-cast if T is not std::byte
            if constexpr (std::is_same<T, std::byte>::value)
                dataObj = m_dataPtr[byteIndex];
            else
                dataObj = *reinterpret_cast<T*>(&m_dataPtr[byteIndex]);
        }


    protected:
        // Protected Attributes
        /** Range of memory in use. */
        size_t m_range = 0ULL;
        /** Underlying data pointer. */
        std::byte* m_dataPtr = nullptr;
    };
};

#endif // YATTA_MEMORYRANGE_H