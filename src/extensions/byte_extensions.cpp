#include <neo/extensions/byte_extensions.h>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <functional>

namespace neo::extensions
{
    constexpr char ByteExtensions::HexChars[];

    int ByteExtensions::XxHash3_32(std::span<const uint8_t> value, int64_t seed)
    {
        // Simple hash implementation - in production, use a proper xxhash3 library
        std::hash<std::string_view> hasher;
        std::string_view view(reinterpret_cast<const char*>(value.data()), value.size());
        return static_cast<int>(hasher(view) ^ static_cast<size_t>(seed));
    }

    int ByteExtensions::XxHash3_32(const std::vector<uint8_t>& value, int64_t seed)
    {
        return XxHash3_32(std::span<const uint8_t>(value), seed);
    }

    std::string ByteExtensions::ToHexString(const std::vector<uint8_t>& value)
    {
        return ToHexString(std::span<const uint8_t>(value), false);
    }

    std::string ByteExtensions::ToHexString(const std::vector<uint8_t>& value, bool reverse)
    {
        return ToHexString(std::span<const uint8_t>(value), reverse);
    }

    std::string ByteExtensions::ToHexString(std::span<const uint8_t> value)
    {
        return ToHexString(value, false);
    }

    std::string ByteExtensions::ToHexString(std::span<const uint8_t> value, bool reverse)
    {
        if (value.empty())
            return "";

        std::string result;
        result.reserve(value.size() * 2);

        if (reverse)
        {
            for (auto it = value.rbegin(); it != value.rend(); ++it)
            {
                uint8_t b = *it;
                result += HexChars[b >> 4];
                result += HexChars[b & 0xF];
            }
        }
        else
        {
            for (uint8_t b : value)
            {
                result += HexChars[b >> 4];
                result += HexChars[b & 0xF];
            }
        }

        return result;
    }

    std::vector<uint8_t> ByteExtensions::FromHexString(const std::string& hex)
    {
        if (hex.length() % 2 != 0)
            throw std::invalid_argument("Hex string must have even length");

        std::vector<uint8_t> result;
        result.reserve(hex.length() / 2);

        for (size_t i = 0; i < hex.length(); i += 2)
        {
            uint8_t high = HexCharToValue(hex[i]);
            uint8_t low = HexCharToValue(hex[i + 1]);
            result.push_back((high << 4) | low);
        }

        return result;
    }

    bool ByteExtensions::NotZero(std::span<const uint8_t> value)
    {
        return std::any_of(value.begin(), value.end(), [](uint8_t b) { return b != 0; });
    }

    bool ByteExtensions::NotZero(const std::vector<uint8_t>& value)
    {
        return NotZero(std::span<const uint8_t>(value));
    }

    bool ByteExtensions::IsZero(std::span<const uint8_t> value)
    {
        return !NotZero(value);
    }

    bool ByteExtensions::IsZero(const std::vector<uint8_t>& value)
    {
        return IsZero(std::span<const uint8_t>(value));
    }

    std::vector<uint8_t> ByteExtensions::Reverse(const std::vector<uint8_t>& value)
    {
        std::vector<uint8_t> result = value;
        std::reverse(result.begin(), result.end());
        return result;
    }

    void ByteExtensions::ReverseInPlace(std::vector<uint8_t>& value)
    {
        std::reverse(value.begin(), value.end());
    }

    std::vector<uint8_t> ByteExtensions::Concat(const std::vector<std::vector<uint8_t>>& arrays)
    {
        size_t total_size = 0;
        for (const auto& array : arrays)
        {
            total_size += array.size();
        }

        std::vector<uint8_t> result;
        result.reserve(total_size);

        for (const auto& array : arrays)
        {
            result.insert(result.end(), array.begin(), array.end());
        }

        return result;
    }

    std::vector<uint8_t> ByteExtensions::Concat(const std::vector<uint8_t>& first, const std::vector<uint8_t>& second)
    {
        std::vector<uint8_t> result;
        result.reserve(first.size() + second.size());
        result.insert(result.end(), first.begin(), first.end());
        result.insert(result.end(), second.begin(), second.end());
        return result;
    }

    std::vector<uint8_t> ByteExtensions::Slice(const std::vector<uint8_t>& value, size_t start, size_t length)
    {
        if (start > value.size())
            throw std::out_of_range("Start index out of range");
        if (start + length > value.size())
            throw std::out_of_range("Length extends beyond array bounds");

        return std::vector<uint8_t>(value.begin() + start, value.begin() + start + length);
    }

    std::vector<uint8_t> ByteExtensions::Slice(const std::vector<uint8_t>& value, size_t start)
    {
        if (start > value.size())
            throw std::out_of_range("Start index out of range");

        return std::vector<uint8_t>(value.begin() + start, value.end());
    }

    bool ByteExtensions::SequenceEqual(std::span<const uint8_t> left, std::span<const uint8_t> right)
    {
        return std::equal(left.begin(), left.end(), right.begin(), right.end());
    }

    bool ByteExtensions::SequenceEqual(const std::vector<uint8_t>& left, const std::vector<uint8_t>& right)
    {
        return SequenceEqual(std::span<const uint8_t>(left), std::span<const uint8_t>(right));
    }

    uint8_t ByteExtensions::HexCharToValue(char c)
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        
        throw std::invalid_argument("Invalid hex character");
    }
}
