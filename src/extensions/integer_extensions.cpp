#include <cstring>
#include <limits>
#include <neo/extensions/integer_extensions.h>
#include <stdexcept>

namespace neo::extensions
{
uint8_t IntegerExtensions::GetVarSize(int32_t value)
{
    return GetVarSize(static_cast<int64_t>(value));
}

uint8_t IntegerExtensions::GetVarSize(uint16_t value)
{
    return GetVarSize(static_cast<int64_t>(value));
}

uint8_t IntegerExtensions::GetVarSize(uint32_t value)
{
    return GetVarSize(static_cast<int64_t>(value));
}

uint8_t IntegerExtensions::GetVarSize(int64_t value)
{
    if (value < 0xFD)
        return sizeof(uint8_t);
    else if (value <= std::numeric_limits<uint16_t>::max())
        return sizeof(uint8_t) + sizeof(uint16_t);
    else if (value <= std::numeric_limits<uint32_t>::max())
        return sizeof(uint8_t) + sizeof(uint32_t);
    else
        return sizeof(uint8_t) + sizeof(uint64_t);
}

uint8_t IntegerExtensions::GetVarSize(uint64_t value)
{
    if (value < 0xFD)
        return sizeof(uint8_t);
    else if (value <= std::numeric_limits<uint16_t>::max())
        return sizeof(uint8_t) + sizeof(uint16_t);
    else if (value <= std::numeric_limits<uint32_t>::max())
        return sizeof(uint8_t) + sizeof(uint32_t);
    else
        return sizeof(uint8_t) + sizeof(uint64_t);
}

std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytes(int16_t value)
{
    return ToLittleEndianBytesImpl(value);
}

std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytes(uint16_t value)
{
    return ToLittleEndianBytesImpl(value);
}

std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytes(int32_t value)
{
    return ToLittleEndianBytesImpl(value);
}

std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytes(uint32_t value)
{
    return ToLittleEndianBytesImpl(value);
}

std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytes(int64_t value)
{
    return ToLittleEndianBytesImpl(value);
}

std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytes(uint64_t value)
{
    return ToLittleEndianBytesImpl(value);
}

int16_t IntegerExtensions::FromLittleEndianBytes16(const std::vector<uint8_t>& bytes, size_t offset)
{
    return FromLittleEndianBytesImpl<int16_t>(bytes, offset);
}

uint16_t IntegerExtensions::FromLittleEndianBytesU16(const std::vector<uint8_t>& bytes, size_t offset)
{
    return FromLittleEndianBytesImpl<uint16_t>(bytes, offset);
}

int32_t IntegerExtensions::FromLittleEndianBytes32(const std::vector<uint8_t>& bytes, size_t offset)
{
    return FromLittleEndianBytesImpl<int32_t>(bytes, offset);
}

uint32_t IntegerExtensions::FromLittleEndianBytesU32(const std::vector<uint8_t>& bytes, size_t offset)
{
    return FromLittleEndianBytesImpl<uint32_t>(bytes, offset);
}

int64_t IntegerExtensions::FromLittleEndianBytes64(const std::vector<uint8_t>& bytes, size_t offset)
{
    return FromLittleEndianBytesImpl<int64_t>(bytes, offset);
}

uint64_t IntegerExtensions::FromLittleEndianBytesU64(const std::vector<uint8_t>& bytes, size_t offset)
{
    return FromLittleEndianBytesImpl<uint64_t>(bytes, offset);
}

bool IntegerExtensions::IsLittleEndian()
{
    uint16_t test = 0x0001;
    return *reinterpret_cast<uint8_t*>(&test) == 0x01;
}

template <typename T>
std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytesImpl(T value)
{
    std::vector<uint8_t> result(sizeof(T));

    if (IsLittleEndian())
    {
        std::memcpy(result.data(), &value, sizeof(T));
    }
    else
    {
        // Convert from big-endian to little-endian
        for (size_t i = 0; i < sizeof(T); ++i)
        {
            result[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
        }
    }

    return result;
}

template <typename T>
T IntegerExtensions::FromLittleEndianBytesImpl(const std::vector<uint8_t>& bytes, size_t offset)
{
    if (offset + sizeof(T) > bytes.size())
        throw std::out_of_range("Insufficient bytes for conversion");

    T result = 0;

    if (IsLittleEndian())
    {
        std::memcpy(&result, bytes.data() + offset, sizeof(T));
    }
    else
    {
        // Convert from little-endian to big-endian
        for (size_t i = 0; i < sizeof(T); ++i)
        {
            result |= static_cast<T>(bytes[offset + i]) << (i * 8);
        }
    }

    return result;
}

// Explicit template instantiations
template std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytesImpl<int16_t>(int16_t);
template std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytesImpl<uint16_t>(uint16_t);
template std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytesImpl<int32_t>(int32_t);
template std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytesImpl<uint32_t>(uint32_t);
template std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytesImpl<int64_t>(int64_t);
template std::vector<uint8_t> IntegerExtensions::ToLittleEndianBytesImpl<uint64_t>(uint64_t);

template int16_t IntegerExtensions::FromLittleEndianBytesImpl<int16_t>(const std::vector<uint8_t>&, size_t);
template uint16_t IntegerExtensions::FromLittleEndianBytesImpl<uint16_t>(const std::vector<uint8_t>&, size_t);
template int32_t IntegerExtensions::FromLittleEndianBytesImpl<int32_t>(const std::vector<uint8_t>&, size_t);
template uint32_t IntegerExtensions::FromLittleEndianBytesImpl<uint32_t>(const std::vector<uint8_t>&, size_t);
template int64_t IntegerExtensions::FromLittleEndianBytesImpl<int64_t>(const std::vector<uint8_t>&, size_t);
template uint64_t IntegerExtensions::FromLittleEndianBytesImpl<uint64_t>(const std::vector<uint8_t>&, size_t);
}  // namespace neo::extensions
