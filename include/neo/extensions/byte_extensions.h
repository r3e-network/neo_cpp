#pragma once

#include <string>
#include <vector>
#include <span>
#include <cstdint>

namespace neo::extensions
{
    /**
     * @brief Extension methods for byte arrays and spans.
     */
    class ByteExtensions
    {
    public:
        /**
         * @brief Computes the 32-bit hash value for the specified byte array using the xxhash3 algorithm.
         * @param value The input to compute the hash code for.
         * @param seed The seed used by the xxhash3 algorithm.
         * @return The computed hash code.
         */
        static int XxHash3_32(std::span<const uint8_t> value, int64_t seed = DefaultXxHash3Seed);

        /**
         * @brief Computes the 32-bit hash value for the specified byte array using the xxhash3 algorithm.
         * @param value The input to compute the hash code for.
         * @param seed The seed used by the xxhash3 algorithm.
         * @return The computed hash code.
         */
        static int XxHash3_32(const std::vector<uint8_t>& value, int64_t seed = DefaultXxHash3Seed);

        /**
         * @brief Converts a byte array to hex string.
         * @param value The byte array to convert.
         * @return The converted hex string.
         * @throws std::invalid_argument if value is empty when not allowed.
         */
        static std::string ToHexString(const std::vector<uint8_t>& value);

        /**
         * @brief Converts a byte array to hex string.
         * @param value The byte array to convert.
         * @param reverse Indicates whether it should be converted in the reversed byte order.
         * @return The converted hex string.
         */
        static std::string ToHexString(const std::vector<uint8_t>& value, bool reverse);

        /**
         * @brief Converts a byte span to hex string.
         * @param value The byte span to convert.
         * @return The converted hex string.
         */
        static std::string ToHexString(std::span<const uint8_t> value);

        /**
         * @brief Converts a byte span to hex string.
         * @param value The byte span to convert.
         * @param reverse Indicates whether it should be converted in the reversed byte order.
         * @return The converted hex string.
         */
        static std::string ToHexString(std::span<const uint8_t> value, bool reverse);

        /**
         * @brief Converts a hex string to byte array.
         * @param hex The hex string to convert.
         * @return The converted byte array.
         * @throws std::invalid_argument if hex string is invalid.
         */
        static std::vector<uint8_t> FromHexString(const std::string& hex);

        /**
         * @brief Checks if all bytes are zero in a byte array.
         * @param value The byte array to check.
         * @return false if all bytes are zero, true otherwise.
         */
        static bool NotZero(std::span<const uint8_t> value);

        /**
         * @brief Checks if all bytes are zero in a byte array.
         * @param value The byte array to check.
         * @return false if all bytes are zero, true otherwise.
         */
        static bool NotZero(const std::vector<uint8_t>& value);

        /**
         * @brief Checks if all bytes are zero in a byte array.
         * @param value The byte array to check.
         * @return true if all bytes are zero, false otherwise.
         */
        static bool IsZero(std::span<const uint8_t> value);

        /**
         * @brief Checks if all bytes are zero in a byte array.
         * @param value The byte array to check.
         * @return true if all bytes are zero, false otherwise.
         */
        static bool IsZero(const std::vector<uint8_t>& value);

        /**
         * @brief Reverses the byte order of a byte array.
         * @param value The byte array to reverse.
         * @return The reversed byte array.
         */
        static std::vector<uint8_t> Reverse(const std::vector<uint8_t>& value);

        /**
         * @brief Reverses the byte order of a byte array in place.
         * @param value The byte array to reverse.
         */
        static void ReverseInPlace(std::vector<uint8_t>& value);

        /**
         * @brief Concatenates multiple byte arrays.
         * @param arrays The byte arrays to concatenate.
         * @return The concatenated byte array.
         */
        static std::vector<uint8_t> Concat(const std::vector<std::vector<uint8_t>>& arrays);

        /**
         * @brief Concatenates two byte arrays.
         * @param first The first byte array.
         * @param second The second byte array.
         * @return The concatenated byte array.
         */
        static std::vector<uint8_t> Concat(const std::vector<uint8_t>& first, const std::vector<uint8_t>& second);

        /**
         * @brief Gets a slice of a byte array.
         * @param value The source byte array.
         * @param start The start index.
         * @param length The length of the slice.
         * @return The sliced byte array.
         * @throws std::out_of_range if indices are invalid.
         */
        static std::vector<uint8_t> Slice(const std::vector<uint8_t>& value, size_t start, size_t length);

        /**
         * @brief Gets a slice of a byte array from start to end.
         * @param value The source byte array.
         * @param start The start index.
         * @return The sliced byte array.
         * @throws std::out_of_range if indices are invalid.
         */
        static std::vector<uint8_t> Slice(const std::vector<uint8_t>& value, size_t start);

        /**
         * @brief Compares two byte arrays for equality.
         * @param left The first byte array.
         * @param right The second byte array.
         * @return True if arrays are equal, false otherwise.
         */
        static bool SequenceEqual(std::span<const uint8_t> left, std::span<const uint8_t> right);

        /**
         * @brief Compares two byte arrays for equality.
         * @param left The first byte array.
         * @param right The second byte array.
         * @return True if arrays are equal, false otherwise.
         */
        static bool SequenceEqual(const std::vector<uint8_t>& left, const std::vector<uint8_t>& right);

    private:
        static constexpr int64_t DefaultXxHash3Seed = 40343;
        static constexpr char HexChars[] = "0123456789abcdef";

        /**
         * @brief Converts a single hex character to its numeric value.
         * @param c The hex character.
         * @return The numeric value.
         * @throws std::invalid_argument if character is not a valid hex digit.
         */
        static uint8_t HexCharToValue(char c);
    };
}
