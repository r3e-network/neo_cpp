#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>

#include <cstdint>
#include <string>

namespace neo::cryptography
{

/**
 * @brief MurmurHash3 implementation for Neo blockchain
 *
 * MurmurHash3 is a non-cryptographic hash function suitable for general hash-based lookup.
 * Used in Neo for various internal hashing operations.
 */
class MurmurHash3
{
   public:
    /**
     * @brief Compute MurmurHash3 32-bit hash
     * @param data Input data to hash
     * @param len Length of input data
     * @param seed Hash seed value
     * @return 32-bit hash value
     */
    static uint32_t Hash32(const uint8_t* data, size_t len, uint32_t seed = 0);

    /**
     * @brief Compute MurmurHash3 32-bit hash from ByteSpan
     * @param data Input data to hash
     * @param seed Hash seed value
     * @return 32-bit hash value
     */
    static uint32_t Hash32(const io::ByteSpan& data, uint32_t seed = 0);

    /**
     * @brief Compute MurmurHash3 32-bit hash from ByteVector
     * @param data Input data to hash
     * @param seed Hash seed value
     * @return 32-bit hash value
     */
    static uint32_t Hash32(const io::ByteVector& data, uint32_t seed = 0);

    /**
     * @brief Compute MurmurHash3 32-bit hash from string
     * @param data Input string to hash
     * @param seed Hash seed value
     * @return 32-bit hash value
     */
    static uint32_t Hash32(const std::string& data, uint32_t seed = 0);
};

}  // namespace neo::cryptography