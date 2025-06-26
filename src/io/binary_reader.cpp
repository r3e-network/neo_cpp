#include <neo/io/binary_reader.h>
#include <neo/io/iserializable.h>
#include <stdexcept>
#include <cstring>

namespace neo::io
{
    BinaryReader::BinaryReader(std::istream& stream)
        : stream_(&stream), data_(nullptr), size_(0), position_(0), owns_stream_(false)
    {
    }

    BinaryReader::BinaryReader(const ByteSpan& data)
        : stream_(nullptr), data_(data.Data()), size_(data.Size()), position_(0), owns_stream_(false)
    {
    }

    BinaryReader::BinaryReader(const std::vector<uint8_t>& data)
        : stream_(nullptr), data_(data.data()), size_(data.size()), position_(0), owns_stream_(false)
    {
    }

    bool BinaryReader::ReadBool()
    {
        return ReadUInt8() != 0;
    }

    bool BinaryReader::ReadBoolean()
    {
        return ReadBool();
    }

    uint8_t BinaryReader::ReadUInt8()
    {
        if (data_) {
            if (position_ >= size_)
                throw std::out_of_range("Unexpected end of data");
            return data_[position_++];
        } else {
            uint8_t value;
            stream_->read(reinterpret_cast<char*>(&value), sizeof(value));
            if (stream_->gcount() != sizeof(value))
                throw std::runtime_error("Failed to read UInt8");
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
        if (data_) {
            if (position_ >= size_)
                throw std::out_of_range("Unexpected end of data");
            return data_[position_];
        } else {
            uint8_t value;
            std::streampos pos = stream_->tellg();
            stream_->read(reinterpret_cast<char*>(&value), sizeof(value));
            if (stream_->gcount() != sizeof(value))
                throw std::runtime_error("Failed to peek UInt8");
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
        if (count > DEFAULT_MAX_ARRAY_SIZE)
            throw std::out_of_range("Byte array size exceeds maximum allowed size");
            
        EnsureAvailable(count);
        
        ByteVector value(count);
        if (count > 0)
            ReadRawBytes(value.Data(), count);
        return value;
    }

    UInt160 BinaryReader::ReadUInt160()
    {
        return UInt160(ReadBytes(UInt160::Size).AsSpan());
    }

    UInt256 BinaryReader::ReadUInt256()
    {
        return UInt256(ReadBytes(UInt256::Size).AsSpan());
    }

    Fixed8 BinaryReader::ReadFixed8()
    {
        return Fixed8(ReadInt64());
    }

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
                return ReadUInt64();
            default:
                return fb;
        }
    }

    int64_t BinaryReader::ReadVarInt(int64_t max)
    {
        int64_t value = ReadVarInt();
        if (value > max)
            throw std::out_of_range("Value exceeds maximum allowed");
        return value;
    }

    ByteVector BinaryReader::ReadVarBytes()
    {
        int64_t count = ReadVarInt();
        if (count < 0 || count > static_cast<int64_t>(std::numeric_limits<size_t>::max()))
            throw std::out_of_range("Invalid byte array size");
        return ReadBytes(static_cast<size_t>(count));
    }

    ByteVector BinaryReader::ReadVarBytes(size_t maxSize)
    {
        int64_t count = ReadVarInt();
        if (count < 0 || count > static_cast<int64_t>(maxSize))
            throw std::out_of_range("Byte array size exceeds maximum allowed");
        return ReadBytes(static_cast<size_t>(count));
    }

    std::string BinaryReader::ReadString()
    {
        ByteVector bytes = ReadVarBytes();
        return std::string(reinterpret_cast<const char*>(bytes.Data()), bytes.Size());
    }

    std::string BinaryReader::ReadVarString()
    {
        return ReadString();
    }

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
        while (nullTerminator < length && bytes[nullTerminator] != 0)
            nullTerminator++;

        return std::string(reinterpret_cast<const char*>(bytes.Data()), nullTerminator);
    }

    void BinaryReader::ReadBytes(uint8_t* data, size_t size)
    {
        if (!data && size > 0)
            throw std::invalid_argument("data pointer cannot be null");
            
        if (size > DEFAULT_MAX_ARRAY_SIZE)
            throw std::out_of_range("Read size exceeds maximum allowed size");
            
        if (size > 0)
            ReadRawBytes(data, size);
    }

    size_t BinaryReader::GetPosition() const
    {
        if (data_) {
            return position_;
        } else if (stream_) {
            return static_cast<size_t>(stream_->tellg());
        } else {
            return 0;
        }
    }

    size_t BinaryReader::Available() const
    {
        if (data_) {
            return size_ - position_;
        } else if (stream_) {
            std::streampos current = stream_->tellg();
            stream_->seekg(0, std::ios::end);
            std::streampos end = stream_->tellg();
            stream_->seekg(current);
            return static_cast<size_t>(end - current);
        } else {
            return 0;
        }
    }

    void BinaryReader::EnsureAvailable(size_t size) const
    {
        if (size > Available())
            throw std::out_of_range("Not enough bytes available to read");
    }

    void BinaryReader::ReadRawBytes(uint8_t* data, size_t size)
    {
        if (!data)
            throw std::invalid_argument("data pointer cannot be null");
            
        EnsureAvailable(size);
        
        if (data_) {
            std::memcpy(data, data_ + position_, size);
            position_ += size;
        } else {
            stream_->read(reinterpret_cast<char*>(data), size);
            if (static_cast<size_t>(stream_->gcount()) != size)
                throw std::runtime_error("Unexpected end of stream");
        }
    }
}
