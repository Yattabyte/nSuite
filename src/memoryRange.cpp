#include "memoryRange.hpp"

using yatta::MemoryRange;


// Public Constructor

MemoryRange::MemoryRange(const size_t& size, std::byte* dataPtr) noexcept :
    m_range(size), m_dataPtr(dataPtr)
{
}


// Public Inquiry Methods

bool MemoryRange::empty() const noexcept
{
    return m_dataPtr == nullptr || m_range == 0ULL;
}

bool MemoryRange::hasData() const noexcept
{
    return m_dataPtr != nullptr && m_range > 0ULL;
}

size_t MemoryRange::size() const noexcept
{
    return m_range;
}

size_t MemoryRange::hash() const noexcept
{
    size_t value(ZeroHash);

    // Ensure data is valid
    if (m_dataPtr != nullptr) {
        if (const auto* const pointer = reinterpret_cast<size_t*>(m_dataPtr)) {
            // Use data 8-bytes at a time, until end of data or less than 8 bytes
            // remains
            size_t x(0ULL);
            const size_t max(m_range / 8ULL);
            for (; x < max; ++x)
                value = ((value << 5ULL) + value) + pointer[x]; // use 8 bytes

              // If any bytes remain, switch technique to work byte-wise instead of
              // 8-byte-wise
            if (const auto* const remainderPtr = reinterpret_cast<char*>(m_dataPtr)) {
                x *= 8ULL;
                for (; x < m_range; ++x)
                    value = ((value << 5ULL) + value) +
                    remainderPtr[x]; // use remaining bytes
            }
        }
    }

    return value;
}


// Public Manipulation Methods

std::byte& MemoryRange::operator[](const size_t& byteIndex)
{
    if (byteIndex >= m_range)
        throw std::runtime_error("Memory Range index out of bounds");
    return m_dataPtr[byteIndex];
}

const std::byte& MemoryRange::operator[](const size_t& byteIndex) const
{
    if (byteIndex >= m_range)
        throw std::runtime_error("Memory Range index out of bounds");
    return m_dataPtr[byteIndex];
}

char* MemoryRange::charArray() const noexcept
{
    return reinterpret_cast<char*>(&m_dataPtr[0]);
}

std::byte* MemoryRange::bytes() const noexcept
{
    return m_dataPtr;
}

MemoryRange MemoryRange::subrange(const size_t& offset, const size_t& length) const
{
    // Ensure pointers are valid
    if (m_dataPtr == nullptr)
        throw std::runtime_error("Invalid Memory Range (null pointer)");

    // Ensure data won't exceed range
    if ((offset + length) > m_range)
        throw std::runtime_error("Memory Range index out of bounds");

    // Return a sub-range
    return MemoryRange{ length, &m_dataPtr[offset] };
}

std::byte* MemoryRange::begin() noexcept
{
    return &m_dataPtr[0];
}

const std::byte* MemoryRange::cbegin() const noexcept
{
    return &m_dataPtr[0];
}

std::byte* MemoryRange::end() noexcept
{
    return &m_dataPtr[m_range];
}

const std::byte* MemoryRange::cend() const noexcept
{
    return &m_dataPtr[m_range];
}


// Public IO Methods

void MemoryRange::in_raw(const void* const dataPtr, const size_t& size, const size_t byteIndex)
{
    // Ensure pointers are valid
    if (m_dataPtr == nullptr)
        throw std::runtime_error("Invalid Memory Range (null pointer)");
    if (dataPtr == nullptr)
        throw std::runtime_error("Invalid argument (null pointer)");

    // Ensure data won't exceed range
    if ((size + byteIndex) > m_range)
        throw std::runtime_error("Memory Range index out of bounds");

    // Copy Data
    std::memcpy(&this->operator[](byteIndex), dataPtr, size);
}

void MemoryRange::out_raw(void* const dataPtr, const size_t& size, const size_t byteIndex) const
{
    // Ensure pointers are valid
    if (m_dataPtr == nullptr)
        throw std::runtime_error("Invalid Memory Range (null pointer)");
    if (dataPtr == nullptr)
        throw std::runtime_error("Invalid argument (null pointer)");

    // Ensure data won't exceed range
    if ((size + byteIndex) > m_range)
        throw std::runtime_error("Memory Range index out of bounds");

    // Copy Data
    std::memcpy(dataPtr, &this->operator[](byteIndex), size);
}