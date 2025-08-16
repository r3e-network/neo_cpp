#pragma once

/**
 * @file unified_hash.h
 * @brief Unified cryptographic hash functions for Neo blockchain
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 * 
 * This file consolidates multiple duplicate hash implementations
 * into a single, comprehensive hash utility class.
 */

#include <neo/io/byte_span.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <array>
#include <string>
#include <vector>
#include <span>

namespace neo::cryptography {

/**
 * @brief Unified cryptographic hash functions
 * 
 * This class consolidates all hash implementations from:
 * - neo::cryptography::Hash
 * - neo::sdk::crypto::Hash
 * - Other duplicate implementations
 */
class UnifiedHash {
public:
    // Type aliases for clarity
    using Bytes = std::vector<uint8_t>;
    using Hash160 = std::array<uint8_t, 20>;
    using Hash256 = std::array<uint8_t, 32>;

    // ============= SHA256 Functions =============
    
    /**
     * @brief Compute SHA256 hash
     */
    static Hash256 SHA256(const Bytes& data);
    static Hash256 SHA256(const std::string& data);
    static Hash256 SHA256(std::span<const uint8_t> data);
    static io::UInt256 SHA256ToUInt256(const Bytes& data);
    static io::UInt256 SHA256ToUInt256(const io::ByteSpan& data);
    
    // Legacy compatibility
    static io::UInt256 Sha256(const io::ByteSpan& data) {
        return SHA256ToUInt256(data);
    }

    // ============= Double SHA256 Functions =============
    
    /**
     * @brief Compute double SHA256 (SHA256(SHA256(data)))
     */
    static Hash256 DoubleSHA256(const Bytes& data);
    static Hash256 DoubleSHA256(std::span<const uint8_t> data);
    static io::UInt256 DoubleSHA256ToUInt256(const Bytes& data);
    static io::UInt256 DoubleSHA256ToUInt256(const io::ByteSpan& data);
    
    // Legacy compatibility
    static io::UInt256 Hash256(const io::ByteSpan& data) {
        return DoubleSHA256ToUInt256(data);
    }

    // ============= RIPEMD160 Functions =============
    
    /**
     * @brief Compute RIPEMD160 hash
     */
    static Hash160 RIPEMD160(const Bytes& data);
    static Hash160 RIPEMD160(std::span<const uint8_t> data);
    static io::UInt160 RIPEMD160ToUInt160(const Bytes& data);
    static io::UInt160 RIPEMD160ToUInt160(const io::ByteSpan& data);
    
    // Legacy compatibility
    static io::UInt160 Ripemd160(const io::ByteSpan& data) {
        return RIPEMD160ToUInt160(data);
    }

    // ============= Hash160 Functions =============
    
    /**
     * @brief Compute Hash160 (RIPEMD160(SHA256(data)))
     * Used for generating addresses from public keys
     */
    static Hash160 ComputeHash160(const Bytes& data);
    static Hash160 ComputeHash160(std::span<const uint8_t> data);
    static io::UInt160 Hash160ToUInt160(const Bytes& data);
    static io::UInt160 Hash160ToUInt160(const io::ByteSpan& data);
    
    // Legacy compatibility
    static io::UInt160 Hash160(const io::ByteSpan& data) {
        return Hash160ToUInt160(data);
    }

    // ============= Keccak256 Functions =============
    
    /**
     * @brief Compute Keccak256 hash (used in Ethereum compatibility)
     */
    static Hash256 Keccak256(const Bytes& data);
    static Hash256 Keccak256(std::span<const uint8_t> data);
    static io::UInt256 Keccak256ToUInt256(const Bytes& data);

    // ============= Utility Functions =============
    
    /**
     * @brief Verify a hash against data
     */
    static bool VerifySHA256(const Bytes& data, const Hash256& hash);
    static bool VerifyDoubleSHA256(const Bytes& data, const Hash256& hash);
    static bool VerifyHash160(const Bytes& data, const Hash160& hash);
    
    /**
     * @brief Convert between representations
     */
    static Bytes ToBytes(const Hash256& hash);
    static Bytes ToBytes(const Hash160& hash);
    static std::string ToHexString(const Hash256& hash);
    static std::string ToHexString(const Hash160& hash);
    
private:
    // Internal implementation helpers
    static Hash256 ComputeSHA256Internal(const uint8_t* data, size_t len);
    static Hash160 ComputeRIPEMD160Internal(const uint8_t* data, size_t len);
};

// ============= Compatibility Aliases =============

// Alias for backward compatibility with existing code
using Hash = UnifiedHash;

// Namespace aliases for SDK compatibility
namespace sdk_compat {
    using Hash = UnifiedHash;
}

} // namespace neo::cryptography

// Global namespace alias for SDK compatibility
namespace neo::sdk::crypto {
    using Hash = neo::cryptography::UnifiedHash;
}