#include <algorithm>
#include <iomanip>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <sstream>
#include <stdexcept>

namespace neo::io
{
UInt160 UInt160::Parse(const std::string& hex)
{
    UInt160 result;
    if (!TryParse(hex, result))
    {
        throw std::invalid_argument("Invalid UInt160 hex string: " + hex);
    }
    return result;
}

bool UInt160::TryParse(const std::string& hex, UInt160& result)
{
    std::string cleanHex = hex;

    // Remove 0x prefix if present
    if (cleanHex.size() >= 2 && cleanHex.substr(0, 2) == "0x")
    {
        cleanHex = cleanHex.substr(2);
    }

    // Check length - must be exactly 40 hex characters for UInt160
    if (cleanHex.size() != Size * 2)
    {
        return false;
    }

    // Parse hex string (left-to-right byte order)
    for (size_t i = 0; i < Size; ++i)
    {
        std::string byteStr = cleanHex.substr(i * 2, 2);
        char* endPtr;
        unsigned long byteVal = std::strtoul(byteStr.c_str(), &endPtr, 16);

        if (*endPtr != '\0' || byteVal > 255)
        {
            return false;
        }

        result.data_[i] = static_cast<uint8_t>(byteVal);
    }

    return true;
}

UInt160 UInt160::FromString(const std::string& hex_string)
{
    return Parse(hex_string);
}

bool UInt160::IsZero() const
{
    for (size_t i = 0; i < Size; ++i)
    {
        if (data_[i] != 0)
            return false;
    }
    return true;
}

void UInt160::Serialize(BinaryWriter& writer) const
{
    writer.WriteBytes(data_.data(), Size);
}

void UInt160::Deserialize(BinaryReader& reader)
{
    reader.ReadBytes(data_.data(), Size);
}
}  // namespace neo::io
