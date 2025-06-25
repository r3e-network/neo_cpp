#include <neo/extensions/string_extensions.h>
#include <neo/extensions/byte_extensions.h>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <cctype>
#include <locale>
#include <codecvt>

namespace neo::extensions
{
    bool StringExtensions::TryToStrictUtf8String(std::span<const uint8_t> bytes, std::string& value)
    {
        try
        {
            value = ToStrictUtf8String(bytes);
            return true;
        }
        catch (...)
        {
            value.clear();
            return false;
        }
    }

    std::string StringExtensions::ToStrictUtf8String(std::span<const uint8_t> value)
    {
        // Validate UTF-8 encoding
        std::string result(reinterpret_cast<const char*>(value.data()), value.size());
        
        // Basic UTF-8 validation
        for (size_t i = 0; i < result.size(); )
        {
            unsigned char c = static_cast<unsigned char>(result[i]);
            
            if (c < 0x80)
            {
                // ASCII character
                i++;
            }
            else if ((c & 0xE0) == 0xC0)
            {
                // 2-byte sequence
                if (i + 1 >= result.size() || (static_cast<unsigned char>(result[i + 1]) & 0xC0) != 0x80)
                    throw std::runtime_error("Invalid UTF-8 sequence");
                i += 2;
            }
            else if ((c & 0xF0) == 0xE0)
            {
                // 3-byte sequence
                if (i + 2 >= result.size() || 
                    (static_cast<unsigned char>(result[i + 1]) & 0xC0) != 0x80 ||
                    (static_cast<unsigned char>(result[i + 2]) & 0xC0) != 0x80)
                    throw std::runtime_error("Invalid UTF-8 sequence");
                i += 3;
            }
            else if ((c & 0xF8) == 0xF0)
            {
                // 4-byte sequence
                if (i + 3 >= result.size() || 
                    (static_cast<unsigned char>(result[i + 1]) & 0xC0) != 0x80 ||
                    (static_cast<unsigned char>(result[i + 2]) & 0xC0) != 0x80 ||
                    (static_cast<unsigned char>(result[i + 3]) & 0xC0) != 0x80)
                    throw std::runtime_error("Invalid UTF-8 sequence");
                i += 4;
            }
            else
            {
                throw std::runtime_error("Invalid UTF-8 sequence");
            }
        }
        
        return result;
    }

    std::string StringExtensions::ToStrictUtf8String(const std::vector<uint8_t>& value)
    {
        return ToStrictUtf8String(std::span<const uint8_t>(value));
    }

    std::string StringExtensions::ToStrictUtf8String(const std::vector<uint8_t>& value, size_t start, size_t count)
    {
        if (start > value.size())
            throw std::out_of_range("Start index out of range");
        if (start + count > value.size())
            throw std::out_of_range("Count extends beyond array bounds");
        
        return ToStrictUtf8String(std::span<const uint8_t>(value.data() + start, count));
    }

    std::vector<uint8_t> StringExtensions::ToStrictUtf8Bytes(const std::string& value)
    {
        std::vector<uint8_t> result(value.begin(), value.end());
        return result;
    }

    size_t StringExtensions::GetStrictUtf8ByteCount(const std::string& value)
    {
        return value.size(); // In UTF-8, byte count equals string size
    }

    bool StringExtensions::IsHex(const std::string& value)
    {
        if (value.empty())
            return true;
        
        if (value.length() % 2 != 0)
            return false;
        
        return std::all_of(value.begin(), value.end(), [](char c) {
            return IsHexDigit(c);
        });
    }

    std::vector<uint8_t> StringExtensions::HexToBytes(const std::string& value)
    {
        if (value.empty())
            return {};
        
        if (value.length() % 2 != 0)
            throw std::invalid_argument("Hex string must have even length");
        
        std::vector<uint8_t> result;
        result.reserve(value.length() / 2);
        
        for (size_t i = 0; i < value.length(); i += 2)
        {
            uint8_t high = HexCharToValue(value[i]);
            uint8_t low = HexCharToValue(value[i + 1]);
            result.push_back((high << 4) | low);
        }
        
        return result;
    }

    std::vector<uint8_t> StringExtensions::HexToBytesReversed(const std::string& value)
    {
        auto result = HexToBytes(value);
        std::reverse(result.begin(), result.end());
        return result;
    }

    size_t StringExtensions::GetVarSize(const std::string& value)
    {
        size_t size = GetStrictUtf8ByteCount(value);
        // Calculate variable-length encoding size
        size_t var_size = 0;
        if (size < 0xFD)
            var_size = 1;
        else if (size <= 0xFFFF)
            var_size = 3;
        else if (size <= 0xFFFFFFFF)
            var_size = 5;
        else
            var_size = 9;
        
        return var_size + size;
    }

    std::string StringExtensions::Trim(const std::string& value)
    {
        auto start = std::find_if_not(value.begin(), value.end(), IsWhitespace);
        auto end = std::find_if_not(value.rbegin(), value.rend(), IsWhitespace).base();
        
        return (start < end) ? std::string(start, end) : std::string();
    }

    std::string StringExtensions::TrimStart(const std::string& value)
    {
        auto start = std::find_if_not(value.begin(), value.end(), IsWhitespace);
        return std::string(start, value.end());
    }

    std::string StringExtensions::TrimEnd(const std::string& value)
    {
        auto end = std::find_if_not(value.rbegin(), value.rend(), IsWhitespace).base();
        return std::string(value.begin(), end);
    }

    std::string StringExtensions::ToLower(const std::string& value)
    {
        std::string result = value;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    std::string StringExtensions::ToUpper(const std::string& value)
    {
        std::string result = value;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }

    bool StringExtensions::StartsWith(const std::string& value, const std::string& prefix)
    {
        if (prefix.length() > value.length())
            return false;
        return value.compare(0, prefix.length(), prefix) == 0;
    }

    bool StringExtensions::EndsWith(const std::string& value, const std::string& suffix)
    {
        if (suffix.length() > value.length())
            return false;
        return value.compare(value.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    std::vector<std::string> StringExtensions::Split(const std::string& value, char delimiter)
    {
        std::vector<std::string> result;
        
        if (value.empty())
        {
            result.push_back("");
            return result;
        }
        
        std::stringstream ss(value);
        std::string item;
        
        while (std::getline(ss, item, delimiter))
        {
            result.push_back(item);
        }
        
        // Handle the case where the string ends with the delimiter
        if (!value.empty() && value.back() == delimiter)
        {
            result.push_back("");
        }
        
        return result;
    }

    std::string StringExtensions::Join(const std::vector<std::string>& values, const std::string& delimiter)
    {
        if (values.empty())
            return "";
        
        std::ostringstream oss;
        for (size_t i = 0; i < values.size(); ++i)
        {
            if (i > 0)
                oss << delimiter;
            oss << values[i];
        }
        
        return oss.str();
    }

    bool StringExtensions::IsHexDigit(char c)
    {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    uint8_t StringExtensions::HexCharToValue(char c)
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        
        throw std::invalid_argument("Invalid hex character");
    }

    bool StringExtensions::IsWhitespace(char c)
    {
        return std::isspace(static_cast<unsigned char>(c));
    }
}
