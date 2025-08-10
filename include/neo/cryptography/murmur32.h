#pragma once

#include <neo/io/byte_span.h>

#include <cstdint>

namespace neo::cryptography
{
/**
 * @brief Murmur32 hash algorithm implementation.
 *
 * This class provides MurmurHash32 implementation compatible with
 * the Neo C# reference implementation.
 */
class Murmur32
{
   public:
    /**
     * @brief Computes Murmur32 hash of the given data.
     * @param data The data to hash.
     * @param seed The seed value (default: 0).
     * @return The 32-bit hash value.
     */
    static uint32_t Hash(const io::ByteSpan& data, uint32_t seed = 0);

    /**
     * @brief Computes Murmur32 hash of the given data.
     * @param data Pointer to the data.
     * @param len Length of the data in bytes.
     * @param seed The seed value (default: 0).
     * @return The 32-bit hash value.
     */
    static uint32_t Hash(const uint8_t* data, size_t len, uint32_t seed = 0);

   private:
    static constexpr uint32_t c1 = 0xcc9e2d51;
    static constexpr uint32_t c2 = 0x1b873593;
    static constexpr uint32_t r1 = 15;
    static constexpr uint32_t r2 = 13;
    static constexpr uint32_t m = 5;
    static constexpr uint32_t n = 0xe6546b64;

    static uint32_t RotateLeft(uint32_t value, int shift);
    static uint32_t FinalizeHash(uint32_t hash, size_t len);
};
}  // namespace neo::cryptography