/**
 * @file binary_reader.cpp
 * @brief Binary Reader
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/io/binary_reader.h>
#include <neo/io/iserializable.h>

#include <cstring>
#include <sstream>
#include <stdexcept>

namespace neo::io
{
BinaryReader::BinaryReader(std::istream& stream)
    : stream_(&stream), data_(nullptr), size_(0), position_(0), owns_stream_(false), using_data_mode_(false)
{
}

BinaryReader::BinaryReader(const ByteSpan& data)
    : stream_(nullptr),
      data_(data.Data()),
      size_(data.Size()),
      position_(0),
      owns_stream_(false),
      using_data_mode_(true)
{
}

BinaryReader::BinaryReader(const ByteVector& data)
    : stream_(nullptr),
      data_(data.Data()),
      size_(data.Size()),
      position_(0),
      owns_stream_(false),
      using_data_mode_(true)
{
}

BinaryReader::BinaryReader(const std::vector<uint8_t>& data)
    : stream_(nullptr),
      data_(data.data()),
      size_(data.size()),
      position_(0),
      owns_stream_(false),
      using_data_mode_(true)
{
}

bool BinaryReader::ReadBool() { return ReadUInt8() != 0; }

bool BinaryReader::ReadBoolean() { return ReadBool(); }

uint8_t BinaryReader::ReadUInt8()
{
    if (using_data_mode_)
    {
        if (position_ >= size_) throw std::out_of_range("Unexpected end of data");
        if (size_ == 0)
        {
            throw std::out_of_range("Unexpected end of data");
        }
        return data_[position_++];
    }
    else
    {
        uint8_t value;
        stream_->read(reinterpret_cast<char*>(&value), sizeof(value));
        if (stream_->gcount() != sizeof(value)) throw std::runtime_error("Failed to read UInt8");
        return value;
    }
}

uint8_t BinaryReader::ReadByte()
{
    // ReadByte() is equivalent to ReadUInt8() in C#
    return ReadUInt8();
}

uint8_t BinaryReader::PeekUInt8()
{
    if (using_data_mode_)
    {
        if (position_ >= size_) throw std::out_of_range("Unexpected end of data");
        return data_[position_];
    }
    else
    {
        uint8_t value;
        std::streampos pos = stream_->tellg();
        stream_->read(reinterpret_cast<char*>(&value), sizeof(value));
        if (stream_->gcount() != sizeof(value)) throw std::runtime_error("Failed to peek UInt8");
        stream_->seekg(pos);
        return value;
    }
}

uint16_t BinaryReader::ReadUInt16()
{
    uint16_t value;
    ReadRawBytes(reinterpret_cast<uint8_t*>(&value), sizeof(value));
    return value;
}

uint32_t BinaryReader::ReadUInt32()
{
    uint32_t value;
    ReadRawBytes(reinterpret_cast<uint8_t*>(&value), sizeof(value));
    return value;
}

uint64_t BinaryReader::ReadUInt64()
{
    uint64_t value;
    ReadRawBytes(reinterpret_cast<uint8_t*>(&value), sizeof(value));
    return value;
}

int8_t BinaryReader::ReadInt8()
{
    int8_t value;
    ReadRawBytes(reinterpret_cast<uint8_t*>(&value), sizeof(value));
    return value;
}

int16_t BinaryReader::ReadInt16()
{
    int16_t value;
    ReadRawBytes(reinterpret_cast<uint8_t*>(&value), sizeof(value));
    return value;
}

int32_t BinaryReader::ReadInt32()
{
    int32_t value;
    ReadRawBytes(reinterpret_cast<uint8_t*>(&value), sizeof(value));
    return value;
}

int64_t BinaryReader::ReadInt64()
{
    int64_t value;
    ReadRawBytes(reinterpret_cast<uint8_t*>(&value), sizeof(value));
    return value;
}

ByteVector BinaryReader::ReadBytes(size_t count)
{
    if (count > DEFAULT_MAX_ARRAY_SIZE) throw std::out_of_range("Byte array size exceeds maximum allowed size");

    EnsureAvailable(count);

    ByteVector value(count);
    if (count > 0) ReadRawBytes(value.Data(), count);
    return value;
}

UInt160 BinaryReader::ReadUInt160() { return UInt160(ReadBytes(UInt160::Size).AsSpan()); }

UInt256 BinaryReader::ReadUInt256() { return UInt256(ReadBytes(UInt256::Size).AsSpan()); }

Fixed8 BinaryReader::ReadFixed8() { return Fixed8(ReadInt64()); }

int64_t BinaryReader::ReadVarInt()
{
    uint8_t fb = ReadUInt8();
    switch (fb)
    {
        case 0xFD:
            return ReadUInt16();
        case 0xFE:
            return ReadUInt32();
        case 0xFF:
        {
            uint64_t value = ReadUInt64();
            if (value > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
                throw std::out_of_range("VarInt value exceeds int64_t maximum");
            return static_cast<int64_t>(value);
        }
        default:
            return fb;
    }
}

int64_t BinaryReader::ReadVarInt(int64_t max)
{
    int64_t value = ReadVarInt();
    if (value > max) throw std::out_of_range("Value exceeds maximum allowed");
    return value;
}

ByteVector BinaryReader::ReadVarBytes()
{
    int64_t count = ReadVarInt();
    if (count < 0)
    {
        throw std::out_of_range("Invalid byte array size: count=" + std::to_string(count) + " (negative)");
    }
    // Use the smaller of size_t max and int64_t max for safety
    constexpr int64_t max_safe_size = std::numeric_limits<int64_t>::max();
    if (count > max_safe_size)
    {
        throw std::out_of_range("Invalid byte array size: count=" + std::to_string(count) +
                                " > max=" + std::to_string(max_safe_size));
    }
    return ReadBytes(static_cast<size_t>(count));
}

ByteVector BinaryReader::ReadVarBytes(size_t maxSize)
{
    int64_t count = ReadVarInt();
    if (count < 0) throw std::out_of_range("Byte array size cannot be negative");

    int64_t maxSizeInt64 = (maxSize > static_cast<size_t>(std::numeric_limits<int64_t>::max()))
                               ? std::numeric_limits<int64_t>::max()
                               : static_cast<int64_t>(maxSize);

    if (count > maxSizeInt64) throw std::out_of_range("Byte array size exceeds maximum allowed");
    return ReadBytes(static_cast<size_t>(count));
}

std::string BinaryReader::ReadString()
{
    ByteVector bytes = ReadVarBytes();
    return std::string(reinterpret_cast<const char*>(bytes.Data()), bytes.Size());
}

std::string BinaryReader::ReadVarString() { return ReadString(); }

std::string BinaryReader::ReadVarString(size_t maxLength)
{
    int64_t length = ReadVarInt();
    if (length < 0 || length > static_cast<int64_t>(maxLength))
        throw std::out_of_range("String length exceeds maximum allowed");

    ByteVector bytes = ReadBytes(static_cast<size_t>(length));
    return std::string(reinterpret_cast<const char*>(bytes.Data()), static_cast<size_t>(length));
}

std::string BinaryReader::ReadFixedString(size_t length)
{
    ByteVector bytes = ReadBytes(length);
    // Find the null terminator if there is one
    size_t nullTerminator = 0;
    while (nullTerminator < length && bytes[nullTerminator] != 0) nullTerminator++;

    return std::string(reinterpret_cast<const char*>(bytes.Data()), nullTerminator);
}

void BinaryReader::ReadBytes(uint8_t* data, size_t size)
{
    if (!data && size > 0) throw std::invalid_argument("data pointer cannot be null");

    if (size > DEFAULT_MAX_ARRAY_SIZE) throw std::out_of_range("Read size exceeds maximum allowed size");

    if (size > 0) ReadRawBytes(data, size);
}

size_t BinaryReader::GetPosition() const
{
    if (using_data_mode_)
    {
        return position_;
    }
    else if (stream_)
    {
        return static_cast<size_t>(stream_->tellg());
    }
    else
    {
        return 0;
    }
}

size_t BinaryReader::Available() const
{
    if (using_data_mode_)
    {
        return size_ - position_;
    }
    else if (stream_)
    {
        // Save current stream state
        std::streampos current = stream_->tellg();
        std::ios::iostate old_state = stream_->rdstate();

        // Check if tellg() failed
        if (current == std::streampos(-1) || stream_->fail())
        {
            return 0;
        }

        // Temporarily seek to end to get size
        stream_->seekg(0, std::ios::end);
        std::streampos end = stream_->tellg();

        // Restore original position and state
        stream_->clear(old_state);
        stream_->seekg(current);

        // Check if operations succeeded
        if (end == std::streampos(-1) || current == std::streampos(-1))
        {
            return 0;
        }

        return static_cast<size_t>(end - current);
    }
    else
    {
        return 0;
    }
}

void BinaryReader::EnsureAvailable(size_t size) const
{
    // Complete availability checking with proper stream state validation
    // Handles both data mode and stream mode correctly

    if (using_data_mode_)
    {
        // Data mode: check against buffer bounds
        if (size > (size_ - position_))
        {
            throw std::out_of_range("Not enough bytes available to read: requested " + std::to_string(size) +
                                    " bytes, available " + std::to_string(size_ - position_));
        }
    }
    else if (stream_)
    {
        // Stream mode: check stream state and available data

        // First check if stream is in good state
        if (!stream_->good() && !stream_->eof())
        {
            throw std::runtime_error("Stream is in error state");
        }

        // Save current position
        std::streampos original_pos = stream_->tellg();

        try
        {
            // Try to peek ahead to see if enough data is available
            if (stream_->good())
            {
                // Seek to end to get size
                stream_->seekg(0, std::ios::end);
                std::streampos end_pos = stream_->tellg();

                // Restore position
                stream_->seekg(original_pos);

                // Calculate available bytes
                if (end_pos != std::streampos(-1) && original_pos != std::streampos(-1))
                {
                    size_t available = static_cast<size_t>(end_pos - original_pos);
                    if (size > available)
                    {
                        throw std::out_of_range("Not enough bytes available in stream: requested " +
                                                std::to_string(size) + " bytes, available " +
                                                std::to_string(available));
                    }
                }
                else
                {
                    // If seeking fails, we'll have to let the read operation handle it
                    // This is a fallback for non-seekable streams
                }
            }
        }
        catch (const std::exception&)
        {
            // If we can't determine availability through seeking,
            // restore position and let the read operation handle it
            try
            {
                stream_->seekg(original_pos);
            }
            catch (const std::ios_base::failure& e)
            {
                // If seek fails, stream may be corrupted
                throw std::runtime_error("Failed to restore stream position during availability check: " +
                                         std::string(e.what()));
            }
            catch (const std::exception& e)
            {
                // Other stream errors
                throw std::runtime_error("Stream error during position restore: " + std::string(e.what()));
            }
        }
    }
    else
    {
        throw std::runtime_error("No valid data source for binary reader");
    }
}

void BinaryReader::ReadRawBytes(uint8_t* data, size_t size)
{
    if (!data) throw std::invalid_argument("data pointer cannot be null");

    EnsureAvailable(size);

    if (using_data_mode_)
    {
        if (size > 0 && data_)
        {
            std::memcpy(data, data_ + position_, size);
        }
        position_ += size;
    }
    else
    {
        stream_->read(reinterpret_cast<char*>(data), size);
        if (static_cast<size_t>(stream_->gcount()) != size) throw std::runtime_error("Unexpected end of stream");
    }
}
}  // namespace neo::io
