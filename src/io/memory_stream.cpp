/**
 * @file memory_stream.cpp
 * @brief Stream processing
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/io/memory_stream.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace neo::io
{
void MemoryStream::SetPosition(size_t position)
{
    position_ = std::min(position, data_.size());
    clear();
    seekg(static_cast<std::streamoff>(position_));
    seekp(static_cast<std::streamoff>(position_));
}

void MemoryStream::Seek(int64_t offset, SeekOrigin origin)
{
    int64_t base = 0;
    switch (origin)
    {
        case SeekOrigin::Begin:
            base = 0;
            break;
        case SeekOrigin::Current:
            base = static_cast<int64_t>(position_);
            break;
        case SeekOrigin::End:
            base = static_cast<int64_t>(data_.size());
            break;
        default:
            throw std::runtime_error("Unknown seek origin");
    }

    int64_t target = base + offset;
    if (target < 0)
    {
        target = 0;
    }
    else if (target > static_cast<int64_t>(data_.size()))
    {
        target = static_cast<int64_t>(data_.size());
    }

    SetPosition(static_cast<size_t>(target));
}

size_t MemoryStream::Read(uint8_t* buffer, size_t count)
{
    if (!buffer || position_ >= data_.size()) return 0;

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
    if (!buffer || count == 0) return;

    // Ensure we have enough space
    if (position_ + count > data_.size())
    {
        data_.resize(position_ + count);
    }

    std::memcpy(data_.data() + position_, buffer, count);
    position_ += count;
    streamBuf_.Reset();
    SetPosition(position_);
}

ByteVector MemoryStream::ToByteVector() const { return ByteVector(ByteSpan(data_.data(), data_.size())); }
}  // namespace neo::io
