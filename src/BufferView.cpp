#include "BufferView.hpp"

using namespace yatta;


// Public (de)Constructors

BufferView::BufferView(const size_t& size, std::byte* const dataPtr) noexcept :
    MemoryRange{ size, dataPtr }
{
}


// Public Derivation Methods

std::optional<Buffer> BufferView::compress() const
{
    return Buffer::compress(*this);
}

std::optional<Buffer> BufferView::decompress() const
{
    return Buffer::decompress(*this);
}

std::optional<Buffer> BufferView::diff(const BufferView& target, const size_t& maxThreads) const
{
    return Buffer::diff(*this, target, maxThreads);
}

std::optional<Buffer> BufferView::patch(const BufferView& diffBuffer) const
{
    return Buffer::patch(*this, diffBuffer);
}