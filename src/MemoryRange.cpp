#include "MemoryRange.hpp"

using namespace yatta;


// Public Inquiry Methods

bool MemoryRange::empty() const noexcept
{
    return m_range == 0ULL;
}

bool MemoryRange::hasData() const noexcept
{
    return m_range > 0ULL;
}

size_t MemoryRange::size() const noexcept
{
    return m_range;
}

size_t MemoryRange::hash() const noexcept
{
	// Use data 8-bytes at a time, until end of data or less than 8 bytes remains
	size_t value(1234567890ULL);
	const auto* const pointer = reinterpret_cast<size_t*>(m_dataPtr);
	size_t x(0ULL);
	const size_t max(m_range / 8ULL);
	for (; x < max; ++x)
		value = ((value << 5) + value) + pointer[x]; // use 8 bytes

	// If any bytes remain, switch technique to work byte-wise instead of 8-byte-wise
	x *= 8ULL;
	const auto* const remainderPtr = reinterpret_cast<char*>(m_dataPtr);
	for (; x < m_range; ++x)
		value = ((value << 5) + value) + remainderPtr[x]; // use remaining bytes

	return value;
}


// Public Manipulation Methods

std::byte& MemoryRange::operator[](const size_t& byteIndex) const noexcept
{
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


// Public IO Methods

void MemoryRange::in_raw(const void* const dataPtr, const size_t& size, const size_t byteIndex) noexcept
{
    // Ensure pointers are valid
    if (m_dataPtr == nullptr || dataPtr == nullptr)
        return; // Failure

    // Ensure data won't exceed range
    if ((size + byteIndex) > m_range)
        return; // Failure

    // Copy Data
    std::memcpy(&bytes()[byteIndex], dataPtr, size);
}

void MemoryRange::out_raw(void* const dataPtr, const size_t& size, const size_t byteIndex) const noexcept
{
    // Ensure pointers are valid
    if (m_dataPtr == nullptr || dataPtr == nullptr)
        return; // Failure

    // Ensure data won't exceed range
    if ((size + byteIndex) > m_range)
        return; // Failure

    // Copy Data
    std::memcpy(dataPtr, &bytes()[byteIndex], size);
}