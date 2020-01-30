#pragma once
#ifndef YATTA_MEMORYRANGE_H
#define YATTA_MEMORYRANGE_H

#include <cstddef>
#include <cstring>
#include <memory>
#include <stdexcept>


namespace yatta {
    constexpr size_t ZeroHash = 1234567890ULL;

    /** A range of contiguous memory. 
    Provides means of indexing and manipulating the data it represents. */
    class MemoryRange {
    public:
        // Public Constructor
        /** Construct a memory range from the specified pointer and size.
        @param  size            the number of bytes in the range.
        @param  dataPtr         pointer to some data source. */
        explicit MemoryRange(const size_t& size = 0ULL, std::byte* dataPtr = nullptr) noexcept;


        // Public Inquiry Methods
        /** Check if this memory range is empty.
        @return                 true if the pointer is null or the size is zero. */
        bool empty() const noexcept;
        /** Retrieves whether or not this memory range's size is greater than zero.
        @return                 true if non-zero range, false otherwise. */
        bool hasData() const noexcept;
        /** Returns the length of memory held by this range.
        @return                 number of bytes in this range. */
        size_t size() const noexcept;
        /** Generates a hash value derived from this range's contents.
        @return                 hash value calculated for this range's memory. */
        size_t hash() const noexcept;


        // Public Manipulation Methods
        /** Retrieves a reference to the data at the byte index specified.
        @note   will throw if accessed out of range.
        @param  byteIndex       how many bytes into this range to index at.
        @return                 reference to data found at the byte index. */
        std::byte& operator[](const size_t& byteIndex);
        /** Retrieves a const reference to the data at the byte index specified.
        @note   will throw if accessed out of range.
        @param  byteIndex       how many bytes into this range to index at.
        @return                 reference to data found at the byte index. */
        const std::byte& operator[](const size_t& byteIndex) const;
        /** Retrieves a character array pointer to this range's data.
        @return                 data pointer cast to char *. */
        char* charArray() const noexcept;
        /** Retrieves a raw pointer to this range's data.
        @return                 pointer to this range's data. */
        std::byte* bytes() const noexcept;
        /** Generate a sub-range from this memory range.
        @note   will throw if accessed out of range.
        @param  offset          the byte index to begin the sub-range at.
        @param  length          the byte length of the sub-range. */
        MemoryRange subrange(const size_t& offset, const size_t& length) const;
        /** Retrieve an iterator to the beginning of this memory range. 
        @return                 beginning iterator. */
        std::byte* begin() noexcept;
        /** Retrieve a const iterator to the beginning of this memory range.
        @return                 const beginning iterator. */
        const std::byte* cbegin() const noexcept;
        /** Retrieve an iterator of <T> to the beginning of this memory range.
        @return                 beginning iterator. */
        template <typename T>
        T* begin_t() noexcept {
            return reinterpret_cast<T*>(&m_dataPtr[0]);
        }
        /** Retrieve a const iterator of <T> to the beginning of this memory range.
        @return                 const beginning iterator. */
        template <typename T>
        const T* cbegin_t() const noexcept {
            return reinterpret_cast<T*>(&m_dataPtr[0]);
        }
        /** Retrieve an iterator to the end of this memory range.
        @return                 ending iterator. */
        std::byte* end() noexcept;
        /** Retrieve a const iterator to the ending of this memory range.
        @return                 const ending iterator. */
        const std::byte* cend() const noexcept;
        /** Retrieve an iterator of <T> to the ending of this memory range.
        @return                 ending iterator. */
        template <typename T>
        T* end_t() noexcept {
            const auto lastFullElement = static_cast<size_t>(m_range / sizeof(T)) * sizeof(T);
            return reinterpret_cast<T*>(&m_dataPtr[lastFullElement]);
        }
        /** Retrieve a const iterator of <T> to the ending of this memory range.
        @return                 const ending iterator. */
        template <typename T>
        const T* cend_t() const noexcept {
            const auto lastFullElement = static_cast<size_t>(m_range / sizeof(T)) * sizeof(T);
            return reinterpret_cast<T*>(&m_dataPtr[lastFullElement]);
        }


        // Public IO Methods
        /** Copies raw data into this memory range.
        @note   will throw if accessed out of range.
        @param  dataPtr         pointer to copy the data from.
        @param  size            the number of bytes to copy.
        @param  byteIndex       the destination index to begin copying to. */
        void in_raw(const void* const dataPtr, const size_t& size, const size_t byteIndex = 0);
        /** Copies any data object into this memory range.
        @note   will throw if accessed out of range.
        @tparam T               the data type (auto-deducible).
        @param  dataObj         the object to copy from.
        @param  byteIndex       the destination index to begin copying to. */
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
            else {
                // Instead of casting the data to type T, std::copy the range
                const auto dataObjPtr = reinterpret_cast<const std::byte*>(&dataObj);
                std::copy(dataObjPtr, dataObjPtr + sizeof(T), &m_dataPtr[byteIndex]);
            }
        }
        /** Copies raw data out of this memory range.
        @note   will throw if accessed out of range.
        @param  dataPtr         pointer to copy the data into.
        @param  size            the number of bytes to copy.
        @param  byteIndex       the destination index to begin copying from. */
        void out_raw(void* const dataPtr, const size_t& size, const size_t byteIndex = 0) const;
        /** Copies any data object out of this memory range.
        @note   will throw if accessed out of range.
        @tparam T               the data type (auto-deducible).
        @param  dataObj         reference to some object to copy into.
        @param  byteIndex       the destination index to begin copying from. */
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
            else {
                // Instead of casting the data to type T, std::copy the range
                const auto dataObjPtr = reinterpret_cast<std::byte*>(&dataObj);
                std::copy(&m_dataPtr[byteIndex], &m_dataPtr[byteIndex] + sizeof(T), dataObjPtr);
            }
        }


    protected:
        // Protected Attributes
        /** Range of memory in use. */
        size_t m_range = 0ULL;
        /** Underlying data pointer. */
        std::byte* m_dataPtr = nullptr;
    };


    // Template Specializations

    template <> inline void MemoryRange::in_type(const std::string& dataObj, const size_t byteIndex) {
        // Ensure pointers are valid
        if (m_dataPtr == nullptr)
            throw std::runtime_error("Invalid Memory Range (null pointer)");

        // Ensure data won't exceed range
        const auto stringSize = static_cast<size_t>(sizeof(char))* dataObj.size();
        if ((sizeof(size_t) + stringSize + byteIndex) > m_range)
            throw std::runtime_error("Memory Range index out of bounds");

        // Copy in string size
        in_type(dataObj.size(), byteIndex);
        // Copy in char data
        in_raw(dataObj.data(), stringSize, byteIndex + sizeof(size_t));
    }
    template <> inline void MemoryRange::out_type(std::string& dataObj, const size_t byteIndex) const {
        // Ensure pointers are valid
        if (m_dataPtr == nullptr)
            throw std::runtime_error("Invalid Memory Range (null pointer)");

        // Ensure data won't exceed range
        if ((sizeof(size_t) + byteIndex) > m_range)
            throw std::runtime_error("Memory Range index out of bounds");

        // Copy out string size
        size_t stringSize(0ULL);
        out_type(stringSize, byteIndex);

        // Ensure string won't exceed range
        if ((sizeof(size_t) + (sizeof(char) * stringSize) + byteIndex) > m_range)
            throw std::runtime_error("Memory Range index out of bounds");

        // Copy out char data
        const auto chars = std::make_unique<char[]>(stringSize);
        out_raw(chars.get(), stringSize, byteIndex + sizeof(size_t));
        dataObj = std::string(chars.get(), stringSize);
    }
};

#endif // YATTA_MEMORYRANGE_H