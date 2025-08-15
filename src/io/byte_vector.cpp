/**
 * @file byte_vector.cpp
 * @brief Byte Vector
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/extensions/base64.h>
#include <neo/extensions/integer_extensions.h>
#include <neo/io/byte_vector.h>

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace neo::io
{
ByteVector ByteVector::Parse(const std::string& hex)
{
    if (hex.empty()) return ByteVector();

    // Remove '0x' prefix if present
    std::string hexStr = hex;
    if (hexStr.size() >= 2 && hexStr[0] == '0' && (hexStr[1] == 'x' || hexStr[1] == 'X')) hexStr = hexStr.substr(2);

    // Ensure even length
    if (hexStr.length() % 2 != 0) throw std::invalid_argument("Invalid hex string length");

    // Validate hex characters
    for (char c : hexStr)
    {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
            throw std::invalid_argument("Invalid hex character");
    }

    ByteVector result(hexStr.length() / 2);

    for (size_t i = 0; i < hexStr.length(); i += 2)
    {
        std::string byteStr = hexStr.substr(i, 2);
        try
        {
            result[i / 2] = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
        }
        catch (const std::exception&)
        {
            throw std::invalid_argument("Invalid hex string");
        }
    }

    return result;
}

ByteVector ByteVector::Concat(const ByteSpan& a, const ByteSpan& b)
{
    ByteVector result(a.Size() + b.Size());
    std::memcpy(result.Data(), a.Data(), a.Size());
    std::memcpy(result.Data() + a.Size(), b.Data(), b.Size());
    return result;
}

ByteVector ByteVector::FromUInt16(uint16_t value)
{
    ByteVector result(2);
    result[0] = static_cast<uint8_t>(value & 0xFF);
    result[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    return result;
}

ByteVector ByteVector::FromUInt32(uint32_t value)
{
    ByteVector result(4);
    result[0] = static_cast<uint8_t>(value & 0xFF);
    result[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    result[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    result[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
    return result;
}

std::string ByteVector::ToHexString(const ByteSpan& span)
{
    static const char* hexChars = "0123456789abcdef";  // Match C# Neo lowercase
    std::string result;
    result.reserve(span.Size() * 2);

    for (size_t i = 0; i < span.Size(); i++)
    {
        uint8_t value = span[i];
        result.push_back(hexChars[value >> 4]);
        result.push_back(hexChars[value & 0x0F]);
    }

    return result;
}

ByteVector ByteVector::FromHexString(const std::string& hex)
{
    if (hex.size() % 2 != 0) throw std::invalid_argument("Hex string must have an even number of characters");

    ByteVector result(hex.size() / 2);

    for (size_t i = 0; i < hex.size(); i += 2)
    {
        char highNibble = hex[i];
        char lowNibble = hex[i + 1];

        uint8_t highValue;
        if (highNibble >= '0' && highNibble <= '9')
            highValue = highNibble - '0';
        else if (highNibble >= 'a' && highNibble <= 'f')
            highValue = highNibble - 'a' + 10;
        else if (highNibble >= 'A' && highNibble <= 'F')
            highValue = highNibble - 'A' + 10;
        else
            throw std::invalid_argument("Invalid hex character");

        uint8_t lowValue;
        if (lowNibble >= '0' && lowNibble <= '9')
            lowValue = lowNibble - '0';
        else if (lowNibble >= 'a' && lowNibble <= 'f')
            lowValue = lowNibble - 'a' + 10;
        else if (lowNibble >= 'A' && lowNibble <= 'F')
            lowValue = lowNibble - 'A' + 10;
        else
            throw std::invalid_argument("Invalid hex character");

        result[i / 2] = (highValue << 4) | lowValue;
    }

    return result;
}

size_t ByteVector::GetVarSize() const
{
    return extensions::IntegerExtensions::GetVarSize(static_cast<uint64_t>(data_.size())) + data_.size();
}

std::string ByteVector::ToBase64String() const { return extensions::Base64::Encode(AsSpan()); }

ByteVector ByteVector::FromBase64String(const std::string& base64) { return extensions::Base64::Decode(base64); }
}  // namespace neo::io
