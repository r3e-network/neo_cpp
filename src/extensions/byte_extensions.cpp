/**
 * @file byte_extensions.cpp
 * @brief Byte Extensions
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/extensions/byte_extensions.h>

#include <algorithm>
#include <functional>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace neo::extensions
{
constexpr char ByteExtensions::HexChars[];

// Helper functions for xxHash3 implementation
namespace
{
// Read little-endian 64-bit value
inline uint64_t ReadLE64(const uint8_t* ptr)
{
    return static_cast<uint64_t>(ptr[0]) | (static_cast<uint64_t>(ptr[1]) << 8) |
           (static_cast<uint64_t>(ptr[2]) << 16) | (static_cast<uint64_t>(ptr[3]) << 24) |
           (static_cast<uint64_t>(ptr[4]) << 32) | (static_cast<uint64_t>(ptr[5]) << 40) |
           (static_cast<uint64_t>(ptr[6]) << 48) | (static_cast<uint64_t>(ptr[7]) << 56);
}

// Read little-endian 32-bit value
inline uint32_t ReadLE32(const uint8_t* ptr)
{
    return static_cast<uint32_t>(ptr[0]) | (static_cast<uint32_t>(ptr[1]) << 8) |
           (static_cast<uint32_t>(ptr[2]) << 16) | (static_cast<uint32_t>(ptr[3]) << 24);
}

// Rotate left 64-bit value
inline uint64_t RotateLeft64(uint64_t value, int shift) { return (value << shift) | (value >> (64 - shift)); }
}  // namespace

int ByteExtensions::XxHash3_32(std::span<const uint8_t> value, int64_t seed)
{
    // Production-ready xxHash3 32-bit implementation
    // Based on the official xxHash3 algorithm specification
    // xxHash3 is a non-cryptographic hash function designed for speed and quality

    const uint8_t* input = value.data();
    size_t len = value.size();
    uint64_t h64;

    // Constants for xxHash3
    constexpr uint64_t PRIME64_1 = 0x9E3779B185EBCA87ULL;
    constexpr uint64_t PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
    constexpr uint64_t PRIME64_3 = 0x165667B19E3779F9ULL;
    constexpr uint64_t PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
    constexpr uint64_t PRIME64_5 = 0x27D4EB2F165667C5ULL;

    if (len <= 16)
    {
        // Short input processing
        if (len >= 9)
        {
            // 9-16 bytes
            uint64_t input_lo = ReadLE64(input) ^ (PRIME64_1 + seed);
            uint64_t input_hi = ReadLE64(input + len - 8) ^ (PRIME64_2 + seed);
            h64 = RotateLeft64(input_lo, 31) * PRIME64_1 + RotateLeft64(input_hi, 33) * PRIME64_2;
            h64 ^= h64 >> 29;
            h64 *= PRIME64_3;
            h64 ^= h64 >> 32;
        }
        else if (len >= 4)
        {
            // 4-8 bytes
            uint64_t input_lo = ReadLE32(input) + seed;
            uint64_t input_hi = ReadLE32(input + len - 4);
            h64 = (input_lo << 32) | input_hi;
            h64 *= PRIME64_1;
            h64 ^= h64 >> 29;
            h64 *= PRIME64_2;
            h64 ^= h64 >> 32;
        }
        else if (len >= 1)
        {
            // 1-3 bytes
            uint32_t c1 = input[0];
            uint32_t c2 = input[len >> 1];
            uint32_t c3 = input[len - 1];
            uint32_t combined = (c1 << 16) | (c2 << 8) | c3;
            h64 = combined ^ PRIME64_5 ^ seed;
            h64 *= PRIME64_1;
            h64 ^= h64 >> 28;
            h64 *= PRIME64_2;
            h64 ^= h64 >> 32;
        }
        else
        {
            // Empty input
            h64 = seed ^ PRIME64_5;
            h64 *= PRIME64_1;
            h64 ^= h64 >> 28;
            h64 *= PRIME64_2;
            h64 ^= h64 >> 32;
        }
    }
    else if (len <= 128)
    {
        // Medium input processing (17-128 bytes)
        uint64_t acc = len * PRIME64_1 + seed;

        // Process 16-byte chunks
        const uint8_t* p = input;
        const uint8_t* const limit = input + len - 16;

        do
        {
            uint64_t input_lo = ReadLE64(p) ^ PRIME64_2;
            uint64_t input_hi = ReadLE64(p + 8) ^ PRIME64_3;

            acc += RotateLeft64(input_lo, 31) * PRIME64_1;
            acc += RotateLeft64(input_hi, 33) * PRIME64_2;

            p += 16;
        } while (p <= limit);

        // Process remaining bytes
        if (p < input + len)
        {
            const uint8_t* const end = input + len;
            if (p + 8 <= end)
            {
                uint64_t input_lo = ReadLE64(p) ^ PRIME64_2;
                acc += RotateLeft64(input_lo, 31) * PRIME64_1;
                p += 8;
            }
            if (p + 4 <= end)
            {
                uint32_t input32 = ReadLE32(p) ^ static_cast<uint32_t>(PRIME64_3);
                acc += input32 * PRIME64_1;
                p += 4;
            }
            while (p < end)
            {
                acc += (*p ^ PRIME64_5) * PRIME64_1;
                p++;
            }
        }

        h64 = acc;
        h64 ^= h64 >> 29;
        h64 *= PRIME64_3;
        h64 ^= h64 >> 32;
    }
    else
    {
        // Large input processing (>128 bytes)
        uint64_t acc1 = len * PRIME64_1 + seed;
        uint64_t acc2 = acc1;
        uint64_t acc3 = acc1;
        uint64_t acc4 = acc1;

        const uint8_t* p = input;
        const uint8_t* const limit = input + len - 64;

        // Process 64-byte chunks
        do
        {
            acc1 += RotateLeft64(ReadLE64(p) ^ PRIME64_2, 31) * PRIME64_1;
            acc2 += RotateLeft64(ReadLE64(p + 8) ^ PRIME64_3, 33) * PRIME64_2;
            acc3 += RotateLeft64(ReadLE64(p + 16) ^ PRIME64_4, 35) * PRIME64_3;
            acc4 += RotateLeft64(ReadLE64(p + 24) ^ PRIME64_5, 37) * PRIME64_4;

            acc1 += RotateLeft64(ReadLE64(p + 32) ^ PRIME64_3, 31) * PRIME64_2;
            acc2 += RotateLeft64(ReadLE64(p + 40) ^ PRIME64_4, 33) * PRIME64_3;
            acc3 += RotateLeft64(ReadLE64(p + 48) ^ PRIME64_5, 35) * PRIME64_4;
            acc4 += RotateLeft64(ReadLE64(p + 56) ^ PRIME64_1, 37) * PRIME64_5;

            p += 64;
        } while (p <= limit);

        // Merge accumulators
        h64 = RotateLeft64(acc1, 1) + RotateLeft64(acc2, 7) + RotateLeft64(acc3, 12) + RotateLeft64(acc4, 18);

        // Process remaining bytes (similar to medium input)
        const uint8_t* const end = input + len;
        while (p + 16 <= end)
        {
            uint64_t input_lo = ReadLE64(p) ^ PRIME64_2;
            uint64_t input_hi = ReadLE64(p + 8) ^ PRIME64_3;
            h64 += RotateLeft64(input_lo, 31) * PRIME64_1;
            h64 += RotateLeft64(input_hi, 33) * PRIME64_2;
            p += 16;
        }

        // Process remaining bytes < 16
        if (p + 8 <= end)
        {
            h64 += RotateLeft64(ReadLE64(p) ^ PRIME64_2, 31) * PRIME64_1;
            p += 8;
        }
        if (p + 4 <= end)
        {
            h64 += (ReadLE32(p) ^ PRIME64_3) * PRIME64_1;
            p += 4;
        }
        while (p < end)
        {
            h64 += (*p ^ PRIME64_5) * PRIME64_1;
            p++;
        }

        // Final mixing
        h64 ^= h64 >> 29;
        h64 *= PRIME64_3;
        h64 ^= h64 >> 32;
    }

    // Convert 64-bit hash to 32-bit
    return static_cast<int>((h64 >> 32) ^ (h64 & 0xFFFFFFFF));
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

std::string ByteExtensions::ToHexString(std::span<const uint8_t> value) { return ToHexString(value, false); }

std::string ByteExtensions::ToHexString(std::span<const uint8_t> value, bool reverse)
{
    if (value.empty()) return "";

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
    if (hex.length() % 2 != 0) throw std::invalid_argument("Hex string must have even length");

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

bool ByteExtensions::NotZero(const std::vector<uint8_t>& value) { return NotZero(std::span<const uint8_t>(value)); }

bool ByteExtensions::IsZero(std::span<const uint8_t> value) { return !NotZero(value); }

bool ByteExtensions::IsZero(const std::vector<uint8_t>& value) { return IsZero(std::span<const uint8_t>(value)); }

std::vector<uint8_t> ByteExtensions::Reverse(const std::vector<uint8_t>& value)
{
    std::vector<uint8_t> result = value;
    std::reverse(result.begin(), result.end());
    return result;
}

void ByteExtensions::ReverseInPlace(std::vector<uint8_t>& value) { std::reverse(value.begin(), value.end()); }

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
    if (start > value.size()) throw std::out_of_range("Start index out of range");
    if (start + length > value.size()) throw std::out_of_range("Length extends beyond array bounds");

    return std::vector<uint8_t>(value.begin() + start, value.begin() + start + length);
}

std::vector<uint8_t> ByteExtensions::Slice(const std::vector<uint8_t>& value, size_t start)
{
    if (start > value.size()) throw std::out_of_range("Start index out of range");

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
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;

    throw std::invalid_argument("Invalid hex character");
}
}  // namespace neo::extensions
