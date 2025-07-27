#include <algorithm>
#include <cstring>
#include <neo/io/memory_stream.h>

namespace neo::io
{
void MemoryStream::SetPosition(size_t position)
{
    position_ = std::min(position, data_.size());
    seekg(position_);
    seekp(position_);
}

size_t MemoryStream::Read(uint8_t* buffer, size_t count)
{
    if (!buffer || position_ >= data_.size())
        return 0;

    size_t available = data_.size() - position_;
    size_t to_read = std::min(count, available);

    if (to_read > 0)
    {
        std::memcpy(buffer, data_.data() + position_, to_read);
        position_ += to_read;
    }

    return to_read;
}

void MemoryStream::Write(const uint8_t* buffer, size_t count)
{
    if (!buffer || count == 0)
        return;

    // Ensure we have enough space
    if (position_ + count > data_.size())
    {
        data_.resize(position_ + count);
        streamBuf_.Reset();
    }

    std::memcpy(data_.data() + position_, buffer, count);
    position_ += count;
}

ByteVector MemoryStream::ToByteVector() const
{
    return ByteVector(ByteSpan(data_.data(), data_.size()));
}
}  // namespace neo::io