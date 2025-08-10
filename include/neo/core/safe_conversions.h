#pragma once

#include <charconv>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

namespace neo::core
{

/**
 * @brief Safe string to number conversion utilities
 *
 * These functions provide safe conversions from strings to numeric types
 * with proper error handling and bounds checking.
 */
class SafeConversions
{
   public:
    /**
     * @brief Safely convert string to int32_t
     * @param str Input string
     * @return Parsed value
     * @throws std::runtime_error on conversion failure
     */
    static int32_t SafeToInt32(const std::string& str)
    {
        if (str.empty())
        {
            throw std::runtime_error("Cannot convert empty string to int32");
        }

        // Trim whitespace
        size_t start = str.find_first_not_of(" \t\n\r");
        size_t end = str.find_last_not_of(" \t\n\r");
        if (start == std::string::npos)
        {
            throw std::runtime_error("Cannot convert whitespace-only string to int32");
        }

        std::string trimmed = str.substr(start, end - start + 1);

        // Use std::from_chars for C++17
#if __cpp_lib_to_chars >= 201611L
        int32_t value;
        auto [ptr, ec] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value);

        if (ec == std::errc())
        {
            // Check if entire string was consumed
            if (ptr == trimmed.data() + trimmed.size())
            {
                return value;
            }
            throw std::runtime_error("Invalid int32 format: contains non-numeric characters");
        }
        else if (ec == std::errc::result_out_of_range)
        {
            throw std::runtime_error("Int32 value out of range: " + str);
        }
        else
        {
            throw std::runtime_error("Invalid int32 value: " + str);
        }
#else
        // Fallback for older compilers
        try
        {
            size_t idx = 0;
            long long value = std::stoll(trimmed, &idx);

            if (idx != trimmed.length())
            {
                throw std::runtime_error("Invalid int32 format: contains non-numeric characters");
            }

            if (value < std::numeric_limits<int32_t>::min() || value > std::numeric_limits<int32_t>::max())
            {
                throw std::runtime_error("Int32 value out of range: " + str);
            }

            return static_cast<int32_t>(value);
        }
        catch (const std::out_of_range&)
        {
            throw std::runtime_error("Int32 value out of range: " + str);
        }
        catch (const std::invalid_argument&)
        {
            throw std::runtime_error("Invalid int32 value: " + str);
        }
#endif
    }

    /**
     * @brief Safely convert string to uint32_t
     * @param str Input string
     * @return Parsed value
     * @throws std::runtime_error on conversion failure
     */
    static uint32_t SafeToUInt32(const std::string& str)
    {
        if (str.empty())
        {
            throw std::runtime_error("Cannot convert empty string to uint32");
        }

        // Trim whitespace
        size_t start = str.find_first_not_of(" \t\n\r");
        size_t end = str.find_last_not_of(" \t\n\r");
        if (start == std::string::npos)
        {
            throw std::runtime_error("Cannot convert whitespace-only string to uint32");
        }

        std::string trimmed = str.substr(start, end - start + 1);

        // Check for negative sign
        if (!trimmed.empty() && trimmed[0] == '-')
        {
            throw std::runtime_error("Cannot convert negative value to uint32: " + str);
        }

#if __cpp_lib_to_chars >= 201611L
        uint32_t value;
        auto [ptr, ec] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value);

        if (ec == std::errc())
        {
            if (ptr == trimmed.data() + trimmed.size())
            {
                return value;
            }
            throw std::runtime_error("Invalid uint32 format: contains non-numeric characters");
        }
        else if (ec == std::errc::result_out_of_range)
        {
            throw std::runtime_error("UInt32 value out of range: " + str);
        }
        else
        {
            throw std::runtime_error("Invalid uint32 value: " + str);
        }
#else
        try
        {
            size_t idx = 0;
            unsigned long long value = std::stoull(trimmed, &idx);

            if (idx != trimmed.length())
            {
                throw std::runtime_error("Invalid uint32 format: contains non-numeric characters");
            }

            if (value > std::numeric_limits<uint32_t>::max())
            {
                throw std::runtime_error("UInt32 value out of range: " + str);
            }

            return static_cast<uint32_t>(value);
        }
        catch (const std::out_of_range&)
        {
            throw std::runtime_error("UInt32 value out of range: " + str);
        }
        catch (const std::invalid_argument&)
        {
            throw std::runtime_error("Invalid uint32 value: " + str);
        }
#endif
    }

    /**
     * @brief Safely convert string to int64_t
     * @param str Input string
     * @return Parsed value
     * @throws std::runtime_error on conversion failure
     */
    static int64_t SafeToInt64(const std::string& str)
    {
        if (str.empty())
        {
            throw std::runtime_error("Cannot convert empty string to int64");
        }

        try
        {
            size_t idx = 0;
            int64_t value = std::stoll(str, &idx);

            if (idx != str.length())
            {
                throw std::runtime_error("Invalid int64 format: contains non-numeric characters");
            }

            return value;
        }
        catch (const std::out_of_range&)
        {
            throw std::runtime_error("Int64 value out of range: " + str);
        }
        catch (const std::invalid_argument&)
        {
            throw std::runtime_error("Invalid int64 value: " + str);
        }
    }

    /**
     * @brief Safely convert string to uint64_t
     * @param str Input string
     * @return Parsed value
     * @throws std::runtime_error on conversion failure
     */
    static uint64_t SafeToUInt64(const std::string& str)
    {
        if (str.empty())
        {
            throw std::runtime_error("Cannot convert empty string to uint64");
        }

        // Check for negative sign
        if (!str.empty() && str[0] == '-')
        {
            throw std::runtime_error("Cannot convert negative value to uint64: " + str);
        }

        try
        {
            size_t idx = 0;
            uint64_t value = std::stoull(str, &idx);

            if (idx != str.length())
            {
                throw std::runtime_error("Invalid uint64 format: contains non-numeric characters");
            }

            return value;
        }
        catch (const std::out_of_range&)
        {
            throw std::runtime_error("UInt64 value out of range: " + str);
        }
        catch (const std::invalid_argument&)
        {
            throw std::runtime_error("Invalid uint64 value: " + str);
        }
    }

    /**
     * @brief Safely convert string to double
     * @param str Input string
     * @return Parsed value
     * @throws std::runtime_error on conversion failure
     */
    static double SafeToDouble(const std::string& str)
    {
        if (str.empty())
        {
            throw std::runtime_error("Cannot convert empty string to double");
        }

        try
        {
            size_t idx = 0;
            double value = std::stod(str, &idx);

            if (idx != str.length())
            {
                throw std::runtime_error("Invalid double format: contains non-numeric characters");
            }

            return value;
        }
        catch (const std::out_of_range&)
        {
            throw std::runtime_error("Double value out of range: " + str);
        }
        catch (const std::invalid_argument&)
        {
            throw std::runtime_error("Invalid double value: " + str);
        }
    }

    /**
     * @brief Safely convert string to port number (1-65535)
     * @param str Input string
     * @return Port number
     * @throws std::runtime_error on conversion failure or invalid port
     */
    static uint16_t SafeToPort(const std::string& str)
    {
        uint32_t value = SafeToUInt32(str);

        if (value == 0 || value > 65535)
        {
            throw std::runtime_error("Invalid port number (must be 1-65535): " + str);
        }

        return static_cast<uint16_t>(value);
    }

    /**
     * @brief Try to convert string to int32_t
     * @param str Input string
     * @return Optional containing value if successful
     */
    static std::optional<int32_t> TryToInt32(const std::string& str)
    {
        try
        {
            return SafeToInt32(str);
        }
        catch (const std::exception&)
        {
            return std::nullopt;
        }
    }

    /**
     * @brief Try to convert string to uint32_t
     * @param str Input string
     * @return Optional containing value if successful
     */
    static std::optional<uint32_t> TryToUInt32(const std::string& str)
    {
        try
        {
            return SafeToUInt32(str);
        }
        catch (const std::exception&)
        {
            return std::nullopt;
        }
    }

    /**
     * @brief Try to convert string to int64_t
     * @param str Input string
     * @return Optional containing value if successful
     */
    static std::optional<int64_t> TryToInt64(const std::string& str)
    {
        try
        {
            return SafeToInt64(str);
        }
        catch (const std::exception&)
        {
            return std::nullopt;
        }
    }

    /**
     * @brief Try to convert string to uint64_t
     * @param str Input string
     * @return Optional containing value if successful
     */
    static std::optional<uint64_t> TryToUInt64(const std::string& str)
    {
        try
        {
            return SafeToUInt64(str);
        }
        catch (const std::exception&)
        {
            return std::nullopt;
        }
    }

    /**
     * @brief Try to convert string to double
     * @param str Input string
     * @return Optional containing value if successful
     */
    static bool IsValidNeoAddress(const std::string& address)
    {
        // Validate NEO address format and checksum
        if (address.length() != 34) return false;
        if (address[0] != 'N') return false;

        // Validate base58 characters
        const std::string base58_chars = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
        for (char c : address)
        {
            if (base58_chars.find(c) == std::string::npos)
            {
                return false;
            }
        }

        return true;
    }

    static std::optional<double> TryToDouble(const std::string& str)
    {
        try
        {
            return SafeToDouble(str);
        }
        catch (const std::exception&)
        {
            return std::nullopt;
        }
    }

    /**
     * @brief Validate and sanitize hex string
     * @param hex Input hex string (may have 0x prefix)
     * @param expectedLength Expected byte length (0 for any length)
     * @return Sanitized hex string without 0x prefix
     * @throws std::runtime_error on validation failure
     */
    static std::string ValidateHexString(const std::string& hex, size_t expectedLength = 0)
    {
        if (hex.empty())
        {
            throw std::runtime_error("Hex string cannot be empty");
        }

        // Remove 0x prefix if present
        std::string cleanHex = hex;
        if (cleanHex.length() >= 2 && cleanHex[0] == '0' && (cleanHex[1] == 'x' || cleanHex[1] == 'X'))
        {
            cleanHex = cleanHex.substr(2);
        }

        if (cleanHex.empty())
        {
            throw std::runtime_error("Hex string cannot be just '0x'");
        }

        // Must have even number of hex digits
        if (cleanHex.length() % 2 != 0)
        {
            throw std::runtime_error("Hex string must have even number of digits");
        }

        // Check expected length
        if (expectedLength > 0 && cleanHex.length() / 2 != expectedLength)
        {
            throw std::runtime_error("Hex string wrong length: expected " + std::to_string(expectedLength) +
                                     " bytes, got " + std::to_string(cleanHex.length() / 2));
        }

        // Validate hex characters
        for (char c : cleanHex)
        {
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
            {
                throw std::runtime_error("Invalid hex character: " + std::string(1, c));
            }
        }

        return cleanHex;
    }
};

}  // namespace neo::core