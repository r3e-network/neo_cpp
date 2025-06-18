#pragma once

#include <neo/io/byte_vector.h>
#include <neo/io/byte_span.h>
#include <cstdint>
#include <vector>
#include <functional>

namespace neo::extensions
{
    /**
     * @brief Equality comparer for byte arrays.
     * 
     * ## Overview
     * Provides equality comparison and hash generation for byte arrays.
     * Optimized for use in hash tables and equality-based operations.
     * 
     * ## API Reference
     * - **Equality**: Fast byte-by-byte equality checking
     * - **Hashing**: Consistent hash generation for containers
     * - **Utilities**: Comparison operators and functors
     * 
     * ## Usage Examples
     * ```cpp
     * // Check equality
     * bool equal = ByteArrayEqualityComparer::Equals(vec1, vec2);
     * 
     * // Generate hash
     * auto hash = ByteArrayEqualityComparer::GetHashCode(data);
     * 
     * // Use as unordered_map comparator
     * std::unordered_map<ByteVector, Value, ByteArrayEqualityComparer::Hash, ByteArrayEqualityComparer::Equal> map;
     * ```
     */
    class ByteArrayEqualityComparer
    {
    public:
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
         * @brief Generate hash code for std::vector<uint8_t>
         * @param data Vector to hash
         * @return Hash code
         */
        static size_t GetHashCode(const std::vector<uint8_t>& data);

        /**
         * @brief Equality functor for use with standard containers
         */
        struct Equal
        {
            bool operator()(const io::ByteVector& left, const io::ByteVector& right) const
            {
                return Equals(left, right);
            }
            
            bool operator()(const io::ByteSpan& left, const io::ByteSpan& right) const
            {
                return Equals(left, right);
            }
            
            bool operator()(const std::vector<uint8_t>& left, const std::vector<uint8_t>& right) const
            {
                return Equals(left, right);
            }
        };

        /**
         * @brief Hash functor for use with standard containers
         */
        struct Hash
        {
            size_t operator()(const io::ByteVector& data) const
            {
                return GetHashCode(data);
            }
            
            size_t operator()(const io::ByteSpan& data) const
            {
                return GetHashCode(data);
            }
            
            size_t operator()(const std::vector<uint8_t>& data) const
            {
                return GetHashCode(data);
            }
        };

        /**
         * @brief Combined hash and equality for unordered containers
         */
        struct HashEqual
        {
            size_t operator()(const io::ByteVector& data) const
            {
                return GetHashCode(data);
            }
            
            bool operator()(const io::ByteVector& left, const io::ByteVector& right) const
            {
                return Equals(left, right);
            }
        };
    };
}
