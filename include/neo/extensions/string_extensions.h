/**
 * @file string_extensions.h
 * @brief String Extensions
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace neo::extensions
{
/**
 * @brief Extension methods for strings.
 */
class StringExtensions
{
   public:
    /**
     * @brief Converts a byte span to a strict UTF8 string.
     * @param bytes The byte span to convert.
     * @param value The converted string (output parameter).
     * @return True if the conversion is successful, otherwise false.
     */
    static bool TryToStrictUtf8String(std::span<const uint8_t> bytes, std::string& value);

    /**
     * @brief Converts a byte span to a strict UTF8 string.
     * @param value The byte span to convert.
     * @return The converted string.
     * @throws std::runtime_error if conversion fails.
     */
    static std::string ToStrictUtf8String(std::span<const uint8_t> value);

    /**
     * @brief Converts a byte array to a strict UTF8 string.
     * @param value The byte array to convert.
     * @return The converted string.
     * @throws std::runtime_error if conversion fails.
     */
    static std::string ToStrictUtf8String(const std::vector<uint8_t>& value);

    /**
     * @brief Converts a byte array to a strict UTF8 string.
     * @param value The byte array to convert.
     * @param start The start index of the byte array.
     * @param count The count of the byte array.
     * @return The converted string.
     * @throws std::runtime_error if conversion fails.
     */
    static std::string ToStrictUtf8String(const std::vector<uint8_t>& value, size_t start, size_t count);

    /**
     * @brief Converts a string to a strict UTF8 byte array.
     * @param value The string to convert.
     * @return The converted byte array.
     */
    static std::vector<uint8_t> ToStrictUtf8Bytes(const std::string& value);

    /**
     * @brief Gets the size of the specified string encoded in strict UTF8.
     * @param value The specified string.
     * @return The size of the string.
     */
    static size_t GetStrictUtf8ByteCount(const std::string& value);

    /**
     * @brief Determines if the specified string is a valid hex string.
     * @param value The specified string.
     * @return True if the string is a valid hex string (or empty); otherwise false.
     */
    static bool IsHex(const std::string& value);

    /**
     * @brief Converts a hex string to byte array.
     * @param value The hex string to convert.
     * @return The converted byte array.
     * @throws std::invalid_argument if the hex string is invalid.
     */
    static std::vector<uint8_t> HexToBytes(const std::string& value);

    /**
     * @brief Converts a hex string to byte array then reverses the order of the bytes.
     * @param value The hex string to convert.
     * @return The converted reversed byte array.
     * @throws std::invalid_argument if the hex string is invalid.
     */
    static std::vector<uint8_t> HexToBytesReversed(const std::string& value);

    /**
     * @brief Gets the size of the specified string encoded in variable-length encoding.
     * @param value The specified string.
     * @return The size of the string.
     */
    static size_t GetVarSize(const std::string& value);

    /**
     * @brief Trims whitespace from the beginning and end of a string.
     * @param value The string to trim.
     * @return The trimmed string.
     */
    static std::string Trim(const std::string& value);

    /**
     * @brief Trims whitespace from the beginning of a string.
     * @param value The string to trim.
     * @return The trimmed string.
     */
    static std::string TrimStart(const std::string& value);

    /**
     * @brief Trims whitespace from the end of a string.
     * @param value The string to trim.
     * @return The trimmed string.
     */
    static std::string TrimEnd(const std::string& value);

    /**
     * @brief Converts a string to lowercase.
     * @param value The string to convert.
     * @return The lowercase string.
     */
    static std::string ToLower(const std::string& value);

    /**
     * @brief Converts a string to uppercase.
     * @param value The string to convert.
     * @return The uppercase string.
     */
    static std::string ToUpper(const std::string& value);

    /**
     * @brief Checks if a string starts with a specified prefix.
     * @param value The string to check.
     * @param prefix The prefix to look for.
     * @return True if the string starts with the prefix, false otherwise.
     */
    static bool StartsWith(const std::string& value, const std::string& prefix);

    /**
     * @brief Checks if a string ends with a specified suffix.
     * @param value The string to check.
     * @param suffix The suffix to look for.
     * @return True if the string ends with the suffix, false otherwise.
     */
    static bool EndsWith(const std::string& value, const std::string& suffix);

    /**
     * @brief Splits a string by a delimiter.
     * @param value The string to split.
     * @param delimiter The delimiter character.
     * @return A vector of split strings.
     */
    static std::vector<std::string> Split(const std::string& value, char delimiter);

    /**
     * @brief Joins a vector of strings with a delimiter.
     * @param values The strings to join.
     * @param delimiter The delimiter string.
     * @return The joined string.
     */
    static std::string Join(const std::vector<std::string>& values, const std::string& delimiter);

   private:
    /**
     * @brief Checks if a character is a valid hex digit.
     * @param c The character to check.
     * @return True if the character is a valid hex digit, false otherwise.
     */
    static bool IsHexDigit(char c);

    /**
     * @brief Converts a hex character to its numeric value.
     * @param c The hex character.
     * @return The numeric value.
     * @throws std::invalid_argument if character is not a valid hex digit.
     */
    static uint8_t HexCharToValue(char c);

    /**
     * @brief Checks if a character is whitespace.
     * @param c The character to check.
     * @return True if the character is whitespace, false otherwise.
     */
    static bool IsWhitespace(char c);
};
}  // namespace neo::extensions
