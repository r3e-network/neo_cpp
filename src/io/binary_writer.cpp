#include <neo/io/binary_writer.h>
#include <neo/io/iserializable.h>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstddef>
#include <algorithm>

namespace neo::io
{
    BinaryWriter::BinaryWriter(std::ostream& stream)
        : stream_(&stream), buffer_(nullptr), owns_stream_(false)
    {
    }

    BinaryWriter::BinaryWriter(ByteVector& buffer)
        : stream_(nullptr), buffer_(&buffer), owns_stream_(false)
    {
    }

    void BinaryWriter::Write(bool value)
    {
        Write(static_cast<uint8_t>(value ? 1 : 0));
    }

    void BinaryWriter::WriteBoolean(bool value)
    {
        Write(value);
    }

    void BinaryWriter::WriteBool(bool value)
    {
        Write(value);
    }

    void BinaryWriter::Write(uint8_t value)
    {
        if (buffer_) {
            buffer_->Push(value);
        } else {
            stream_->write(reinterpret_cast<const char*>(&value), sizeof(value));
        }
    }

    void BinaryWriter::WriteByte(uint8_t value)
    {
        Write(value);
    }

    void BinaryWriter::Write(uint16_t value)
    {
        WriteRawBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
    }

    void BinaryWriter::WriteUInt16(uint16_t value)
    {
        Write(value);
    }

    void BinaryWriter::Write(uint32_t value)
    {
        WriteRawBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
    }

    void BinaryWriter::Write(uint64_t value)
    {
        WriteRawBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
    }

    void BinaryWriter::WriteUInt64(uint64_t value)
    {
        Write(value);
    }

    void BinaryWriter::Write(int8_t value)
    {
        WriteRawBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
    }

    void BinaryWriter::Write(int16_t value)
    {
        WriteRawBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
    }

    void BinaryWriter::Write(int32_t value)
    {
        WriteRawBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
    }

    void BinaryWriter::Write(int64_t value)
    {
        WriteRawBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(value));
    }

    void BinaryWriter::WriteInt64(int64_t value)
    {
        Write(value);
    }

    void BinaryWriter::Write(const ByteSpan& value)
    {
        WriteRawBytes(value.Data(), value.Size());
    }

    void BinaryWriter::Write(const std::string& value)
    {
        WriteString(value);
    }

    void BinaryWriter::Write(const UInt160& value)
    {
        Write(value.AsSpan());
    }

    void BinaryWriter::Write(const UInt256& value)
    {
        Write(value.AsSpan());
    }

    void BinaryWriter::Write(const Fixed8& value)
    {
        Write(value.ToString());
    }

    void BinaryWriter::Write(const ISerializable& value)
    {
        value.Serialize(*this);
    }

    void BinaryWriter::WriteVarInt(int64_t value)
    {
        if (value < 0)
            throw std::invalid_argument("Value must be non-negative");

        if (value < 0xFD)
        {
            Write(static_cast<uint8_t>(value));
        }
        else if (value <= 0xFFFF)
        {
            Write(static_cast<uint8_t>(0xFD));
            Write(static_cast<uint16_t>(value));
        }
        else if (value <= 0xFFFFFFFF)
        {
            Write(static_cast<uint8_t>(0xFE));
            Write(static_cast<uint32_t>(value));
        }
        else
        {
            Write(static_cast<uint8_t>(0xFF));
            Write(static_cast<uint64_t>(value));
        }
    }

    void BinaryWriter::WriteVarBytes(const ByteSpan& value)
    {
        WriteVarInt(value.Size());
        Write(value);
    }

    void BinaryWriter::WriteVarBytes(const std::vector<uint8_t>& value)
    {
        WriteVarInt(value.size());
        WriteRawBytes(value.data(), value.size());
    }

    void BinaryWriter::WriteString(const std::string& value)
    {
        WriteVarBytes(ByteSpan(reinterpret_cast<const uint8_t*>(value.data()), value.size()));
    }

    void BinaryWriter::WriteVarString(const std::string& value)
    {
        WriteString(value);
    }

    void BinaryWriter::WriteFixedString(const std::string& value, size_t length)
    {
        // Create a buffer of the exact size
        std::vector<uint8_t> buffer(length, 0);

        // Copy the string to the buffer, truncating if necessary
        size_t copyLength = std::min(value.size(), length);
        std::memcpy(buffer.data(), value.data(), copyLength);

        // Write the buffer
        Write(ByteSpan(buffer.data(), length));
    }

    void BinaryWriter::WriteBytes(const uint8_t* data, size_t size)
    {
        WriteRawBytes(data, size);
    }

    void BinaryWriter::WriteBytes(const ByteVector& data)
    {
        WriteBytes(data.Data(), data.Size());
    }

    void BinaryWriter::WriteRawBytes(const uint8_t* data, size_t size)
    {
        if (buffer_) {
            for (size_t i = 0; i < size; ++i) {
                buffer_->Push(data[i]);
            }
        } else {
            stream_->write(reinterpret_cast<const char*>(data), size);
        }
    }
}
