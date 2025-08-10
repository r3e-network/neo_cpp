#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace neo::extensions
{
/**
 * @brief Utility class for comparing byte arrays.
 *
 * ## Overview
 * Provides comprehensive comparison functions for byte arrays, vectors, and spans.
 * Supports lexicographic ordering, equality checking, and custom comparison operations.
 *
 * ## API Reference
 * - **Equality**: Compare for exact byte-by-byte equality
 * - **Ordering**: Lexicographic comparison for sorting
 * - **Utilities**: Hash code generation, prefix matching
 *
 * ## Usage Examples
 * ```cpp
 * // Compare two byte vectors
 * auto result = ByteArrayComparer::Compare(vec1, vec2);
 *
 * // Check equality
 * bool equal = ByteArrayComparer::Equals(span1, span2);
 *
 * // Generate hash code
 * auto hash = ByteArrayComparer::GetHashCode(data);
 * ```
 */
class ByteArrayComparer
{
   public:
    /**
     * @brief Compare two byte spans lexicographically
     * @param left First byte span
     * @param right Second byte span
     * @return -1 if left < right, 0 if equal, 1 if left > right
     */
    static int Compare(const io::ByteSpan& left, const io::ByteSpan& right);

    /**
     * @brief Compare two byte vectors lexicographically
     * @param left First byte vector
     * @param right Second byte vector
     * @return -1 if left < right, 0 if equal, 1 if left > right
     */
    static int Compare(const io::ByteVector& left, const io::ByteVector& right);

    /**
     * @brief Compare two std::vector<uint8_t> lexicographically
     * @param left First vector
     * @param right Second vector
     * @return -1 if left < right, 0 if equal, 1 if left > right
     */
    static int Compare(const std::vector<uint8_t>& left, const std::vector<uint8_t>& right);

    /**
     * @brief Check if two byte spans are equal
     * @param left First byte span
     * @param right Second byte span
     * @return True if equal, false otherwise
     */
    static bool Equals(const io::ByteSpan& left, const io::ByteSpan& right);

    /**
     * @brief Check if two byte vectors are equal
     * @param left First byte vector
     * @param right Second byte vector
     * @return True if equal, false otherwise
     */
    static bool Equals(const io::ByteVector& left, const io::ByteVector& right);

    /**
     * @brief Check if two std::vector<uint8_t> are equal
     * @param left First vector
     * @param right Second vector
     * @return True if equal, false otherwise
     */
    static bool Equals(const std::vector<uint8_t>& left, const std::vector<uint8_t>& right);

    /**
     * @brief Check if left array starts with right array
     * @param left Array to check
     * @param right Prefix to look for
     * @return True if left starts with right
     */
    static bool StartsWith(const io::ByteSpan& left, const io::ByteSpan& right);

    /**
     * @brief Check if left array ends with right array
     * @param left Array to check
     * @param right Suffix to look for
     * @return True if left ends with right
     */
    static bool EndsWith(const io::ByteSpan& left, const io::ByteSpan& right);

    /**
     * @brief Generate hash code for byte span
     * @param data Byte span to hash
     * @return Hash code
     */
    static size_t GetHashCode(const io::ByteSpan& data);

    /**
     * @brief Generate hash code for byte vector
     * @param data Byte vector to hash
     * @return Hash code
     */
    static size_t GetHashCode(const io::ByteVector& data);

    /**
     * @brief Find first occurrence of pattern in data
     * @param data Data to search in
     * @param pattern Pattern to find
     * @return Index of first occurrence, or SIZE_MAX if not found
     */
    static size_t FindFirst(const io::ByteSpan& data, const io::ByteSpan& pattern);

    /**
     * @brief Find last occurrence of pattern in data
     * @param data Data to search in
     * @param pattern Pattern to find
     * @return Index of last occurrence, or SIZE_MAX if not found
     */
    static size_t FindLast(const io::ByteSpan& data, const io::ByteSpan& pattern);

    /**
     * @brief Get minimum of two byte arrays
     * @param left First array
     * @param right Second array
     * @return Reference to the smaller array
     */
    static const io::ByteVector& Min(const io::ByteVector& left, const io::ByteVector& right);

    /**
     * @brief Get maximum of two byte arrays
     * @param left First array
     * @param right Second array
     * @return Reference to the larger array
     */
    static const io::ByteVector& Max(const io::ByteVector& left, const io::ByteVector& right);

    /**
     * @brief Functor for using as std::map or std::set comparator
     */
    struct Less
    {
        bool operator()(const io::ByteVector& left, const io::ByteVector& right) const
        {
            return Compare(left, right) < 0;
        }
    };

    /**
     * @brief Functor for equality comparison
     */
    struct Equal
    {
        bool operator()(const io::ByteVector& left, const io::ByteVector& right) const { return Equals(left, right); }
    };

    /**
     * @brief Functor for hash generation
     */
    struct Hash
    {
        size_t operator()(const io::ByteVector& data) const { return GetHashCode(data); }
    };
};
}  // namespace neo::extensions
