#include <neo/io/binary_reader.h>
#include <neo/io/iserializable.h>
#include <stdexcept>

namespace neo::io
{
    BinaryReader::BinaryReader(std::istream& stream)
        : stream_(stream)
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
        uint8_t value;
        stream_.read(reinterpret_cast<char*>(&value), sizeof(value));
        if (stream_.gcount() != sizeof(value))
            throw std::runtime_error("Failed to read UInt8");
        return value;
    }

    uint8_t BinaryReader::ReadByte()
    {
        // ReadByte() is equivalent to ReadUInt8() in C#
        return ReadUInt8();
    }

    uint8_t BinaryReader::PeekUInt8()
    {
        uint8_t value;
        std::streampos pos = stream_.tellg();
        stream_.read(reinterpret_cast<char*>(&value), sizeof(value));
        if (stream_.gcount() != sizeof(value))
            throw std::runtime_error("Failed to peek UInt8");
        stream_.seekg(pos);
        return value;
    }

    uint16_t BinaryReader::ReadUInt16()
    {
        uint16_t value;
        stream_.read(reinterpret_cast<char*>(&value), sizeof(value));
        if (stream_.gcount() != sizeof(value))
            throw std::runtime_error("Failed to read UInt16");
        return value;
    }

    uint32_t BinaryReader::ReadUInt32()
    {
        uint32_t value;
        stream_.read(reinterpret_cast<char*>(&value), sizeof(value));
        if (stream_.gcount() != sizeof(value))
            throw std::runtime_error("Failed to read UInt32");
        return value;
    }

    uint64_t BinaryReader::ReadUInt64()
    {
        uint64_t value;
        stream_.read(reinterpret_cast<char*>(&value), sizeof(value));
        if (stream_.gcount() != sizeof(value))
            throw std::runtime_error("Failed to read UInt64");
        return value;
    }

    int8_t BinaryReader::ReadInt8()
    {
        int8_t value;
        stream_.read(reinterpret_cast<char*>(&value), sizeof(value));
        if (stream_.gcount() != sizeof(value))
            throw std::runtime_error("Failed to read Int8");
        return value;
    }

    int16_t BinaryReader::ReadInt16()
    {
        int16_t value;
        stream_.read(reinterpret_cast<char*>(&value), sizeof(value));
        if (stream_.gcount() != sizeof(value))
            throw std::runtime_error("Failed to read Int16");
        return value;
    }

    int32_t BinaryReader::ReadInt32()
    {
        int32_t value;
        stream_.read(reinterpret_cast<char*>(&value), sizeof(value));
        if (stream_.gcount() != sizeof(value))
            throw std::runtime_error("Failed to read Int32");
        return value;
    }

    int64_t BinaryReader::ReadInt64()
    {
        int64_t value;
        stream_.read(reinterpret_cast<char*>(&value), sizeof(value));
        if (stream_.gcount() != sizeof(value))
            throw std::runtime_error("Failed to read Int64");
        return value;
    }

    ByteVector BinaryReader::ReadBytes(size_t count)
    {
        ByteVector value(count);
        stream_.read(reinterpret_cast<char*>(value.Data()), count);
        if (static_cast<size_t>(stream_.gcount()) != count)
            throw std::runtime_error("Failed to read bytes");
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
        if (count < 0 || count > std::numeric_limits<size_t>::max())
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
        return std::string(reinterpret_cast<const char*>(ReadVarBytes().Data()));
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
        stream_.read(reinterpret_cast<char*>(data), size);
        if (stream_.gcount() != static_cast<std::streamsize>(size))
            throw std::runtime_error("Unexpected end of stream");
    }
}
