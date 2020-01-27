#include "BufferView.hpp"

using yatta::Buffer;
using yatta::BufferView;
using yatta::MemoryRange;


// Public Constructor

BufferView::BufferView(const Buffer& buffer) noexcept :
    BufferView(buffer.size(), buffer.bytes())
{
}

BufferView::BufferView(const size_t& size, std::byte* dataPtr) noexcept :
    MemoryRange{ size, dataPtr }
{
}


// Public Derivation Methods

Buffer BufferView::compress() const
{
    return Buffer::compress(*this);
}

Buffer BufferView::decompress() const
{
    return Buffer::decompress(*this);
}

Buffer BufferView::diff(const BufferView& target) const
{
    return Buffer::diff(*this, target);
}

Buffer BufferView::patch(const BufferView& diffBuffer) const
{
    return Buffer::patch(*this, diffBuffer);
}