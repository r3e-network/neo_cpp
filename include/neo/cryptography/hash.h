#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <cstdint>

namespace neo::cryptography
{
    /**
     * @brief Provides cryptographic hash functions.
     */
    class Hash
    {
    public:
        /**
         * @brief Computes the SHA-256 hash of the input data.
         * @param data The input data.
         * @return The SHA-256 hash.
         */
        static io::UInt256 Sha256(const io::ByteSpan& data);

        /**
         * @brief Computes the RIPEMD-160 hash of the input data.
         * @param data The input data.
         * @return The RIPEMD-160 hash.
         */
        static io::UInt160 Ripemd160(const io::ByteSpan& data);

        /**
         * @brief Computes the SHA-256 hash of the SHA-256 hash of the input data.
         * @param data The input data.
         * @return The double SHA-256 hash.
         */
        static io::UInt256 Hash256(const io::ByteSpan& data);

        /**
         * @brief Computes the RIPEMD-160 hash of the SHA-256 hash of the input data.
         * @param data The input data.
         * @return The RIPEMD-160 hash of the SHA-256 hash.
         */
        static io::UInt160 Hash160(const io::ByteSpan& data);

        /**
         * @brief Computes the Keccak-256 hash of the input data.
         * @param data The input data.
         * @return The Keccak-256 hash.
         */
        static io::UInt256 Keccak256(const io::ByteSpan& data);

        /**
         * @brief Computes the Murmur32 hash of the input data.
         * @param data The input data.
         * @param seed The seed value.
         * @return The Murmur32 hash.
         */
        static uint32_t Murmur32(const io::ByteSpan& data, uint32_t seed);
    };
}
