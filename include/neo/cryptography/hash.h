#pragma once

#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/io/byte_span.h>

namespace neo::cryptography
{
    /**
     * @brief Provides cryptographic hash functions.
     */
    class Hash
    {
    public:
        /**
         * @brief Computes SHA256 hash.
         * @param data The data to hash.
         * @return The SHA256 hash.
         */
        static io::UInt256 Sha256(const io::ByteSpan& data);

        /**
         * @brief Computes RIPEMD160 hash.
         * @param data The data to hash.
         * @return The RIPEMD160 hash.
         */
        static io::UInt160 Ripemd160(const io::ByteSpan& data);

        /**
         * @brief Computes Hash160 (RIPEMD160 of SHA256).
         * @param data The data to hash.
         * @return The Hash160.
         */
        static io::UInt160 Hash160(const io::ByteSpan& data);

        /**
         * @brief Computes Hash256 (double SHA256).
         * @param data The data to hash.
         * @return The Hash256.
         */
        static io::UInt256 Hash256(const io::ByteSpan& data);

        /**
         * @brief Computes Keccak256 hash.
         * @param data The data to hash.
         * @return The Keccak256 hash.
         */
        static io::UInt256 Keccak256(const io::ByteSpan& data);

        /**
         * @brief Computes Murmur32 hash.
         * @param data The data to hash.
         * @param seed The seed value.
         * @return The Murmur32 hash.
         */
        static uint32_t Murmur32(const io::ByteSpan& data, uint32_t seed = 0);
    };
}
