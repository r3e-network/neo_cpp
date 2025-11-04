#include <neo/extensions/base64.h>
#include <neo/extensions/integer_extensions.h>

#include <algorithm>
#include <string>
#include <vector>

namespace neo::extensions
{
namespace
{
uint8_t CalcVarSize(uint64_t value)
{
    if (value < 0xFD)
    {
        return 1;
    }
    if (value <= 0xFFFF)
    {
        return 3;  // prefix + 2 bytes
    }
    if (value <= 0xFFFFFFFF)
    {
        return 5;  // prefix + 4 bytes
    }
    return 9;  // prefix + 8 bytes
}

template <typename T>
std::vector<uint8_t> ToLittleEndian(T value)
{
    std::vector<uint8_t> bytes(sizeof(T));
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        bytes[i] = static_cast<uint8_t>((static_cast<uint64_t>(value) >> (8 * i)) & 0xFF);
    }
    return bytes;
}

template <typename T>
T FromLittleEndian(const std::vector<uint8_t>& bytes, size_t offset)
{
    if (offset + sizeof(T) > bytes.size())
    {
        throw std::out_of_range("Insufficient bytes for conversion");
    }

    T value = 0;
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        value |= static_cast<T>(static_cast<uint64_t>(bytes[offset + i]) << (8 * i));
    }
    return value;
}
}  // namespace

uint8_t IntegerExtensions::GetVarSize(int32_t value) { return CalcVarSize(static_cast<uint64_t>(value)); }
uint8_t IntegerExtensions::GetVarSize(uint16_t value) { return CalcVarSize(value); }
uint8_t IntegerExtensions::GetVarSize(uint32_t value) { return CalcVarSize(value); }
uint8_t IntegerExtensions::GetVarSize(int64_t value) { return CalcVarSize(static_cast<uint64_t>(value)); }
uint8_t IntegerExtensions::GetVarSize(uint64_t value) { return CalcVarSize(value); }

std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytes(int16_t value) { return ToLittleEndian(value); }
std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytes(uint16_t value) { return ToLittleEndian(value); }
std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytes(int32_t value) { return ToLittleEndian(value); }
std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytes(uint32_t value) { return ToLittleEndian(value); }
std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytes(int64_t value) { return ToLittleEndian(value); }
std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytes(uint64_t value) { return ToLittleEndian(value); }

int16_t IntegerExtensions::FromLittleEndianBytes16(const std::vector<uint8_t>& bytes, size_t offset)
{
    return FromLittleEndian<int16_t>(bytes, offset);
}

uint16_t IntegerExtensions::FromLittleEndianBytesU16(const std::vector<uint8_t>& bytes, size_t offset)
{
    return FromLittleEndian<uint16_t>(bytes, offset);
}

int32_t IntegerExtensions::FromLittleEndianBytes32(const std::vector<uint8_t>& bytes, size_t offset)
{
    return FromLittleEndian<int32_t>(bytes, offset);
}

uint32_t IntegerExtensions::FromLittleEndianBytesU32(const std::vector<uint8_t>& bytes, size_t offset)
{
    return FromLittleEndian<uint32_t>(bytes, offset);
}

int64_t IntegerExtensions::FromLittleEndianBytes64(const std::vector<uint8_t>& bytes, size_t offset)
{
    return FromLittleEndian<int64_t>(bytes, offset);
}

uint64_t IntegerExtensions::FromLittleEndianBytesU64(const std::vector<uint8_t>& bytes, size_t offset)
{
    return FromLittleEndian<uint64_t>(bytes, offset);
}

bool IntegerExtensions::IsLittleEndian()
{
    const uint16_t value = 0x1;
    return *reinterpret_cast<const uint8_t*>(&value) == 0x1;
}

std::string Base64::Encode(const io::ByteSpan& data)
{
    return std::string(reinterpret_cast<const char*>(data.Data()), data.Size());
}

io::ByteVector Base64::Decode(const std::string& base64)
{
    return io::ByteVector(reinterpret_cast<const uint8_t*>(base64.data()), base64.size());
}
}  // namespace neo::extensions
