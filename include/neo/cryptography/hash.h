#pragma once

#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/io/byte_span.h>
#include <optional>
#include <vector>

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
         * @brief Computes Keccak256 hash (WARNING: Currently using SHA3-256).
         * @param data The data to hash.
         * @return The Keccak256 hash.
         */
        static io::UInt256 Keccak256(const io::ByteSpan& data);
        
        /**
         * @brief Computes proper Keccak256 hash (Ethereum-compatible).
         * @param data The data to hash.
         * @return The true Keccak256 hash.
         */
        static io::UInt256 Keccak256Proper(const io::ByteSpan& data);

        /**
         * @brief Computes Murmur32 hash.
         * @param data The data to hash.
         * @param seed The seed value.
         * @return The Murmur32 hash.
         */
        static uint32_t Murmur32(const io::ByteSpan& data, uint32_t seed = 0);
    };
    
    // Forward declaration
    namespace ecc {
        class ECPoint;
    }
    
    /**
     * @brief Recovers public key from signature (Ethereum-style ECRecover)
     * @param hash The hash that was signed (32 bytes)
     * @param signature The signature (64 bytes: r + s)
     * @param recovery_id Recovery ID (0-3)
     * @return The recovered public key, or nullopt if recovery failed
     */
    std::optional<ecc::ECPoint> ECRecover(const io::ByteSpan& hash, 
                                         const io::ByteSpan& signature, 
                                         uint8_t recovery_id);
    
    /**
     * @brief Simplified ECRecover that tries all recovery IDs
     * @param hash The hash that was signed
     * @param signature The signature (64 bytes: r + s)
     * @return Vector of possible recovered public keys
     */
    std::vector<ecc::ECPoint> ECRecoverAll(const io::ByteSpan& hash, 
                                          const io::ByteSpan& signature);
}
