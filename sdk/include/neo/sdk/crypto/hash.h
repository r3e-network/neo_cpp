#pragma once

/**
 * @file hash.h
 * @brief Cryptographic hash functions for Neo blockchain
 * @author Neo C++ Team
 * @date 2025
 */

#include <neo/sdk/core/types.h>
#include <array>
#include <string>
#include <vector>

namespace neo::sdk::crypto {

/**
 * @brief Hash algorithms supported by Neo
 */
class Hash {
public:
    /**
     * @brief SHA256 hash function
     * @param data Input data
     * @return 32-byte hash
     */
    static std::array<uint8_t, 32> SHA256(const std::vector<uint8_t>& data);
    static std::array<uint8_t, 32> SHA256(const std::string& data);
    static core::UInt256 SHA256ToUInt256(const std::vector<uint8_t>& data);

    /**
     * @brief Double SHA256 (SHA256(SHA256(data)))
     * @param data Input data
     * @return 32-byte hash
     */
    static std::array<uint8_t, 32> DoubleSHA256(const std::vector<uint8_t>& data);
    static core::UInt256 DoubleSHA256ToUInt256(const std::vector<uint8_t>& data);

    /**
     * @brief RIPEMD160 hash function
     * @param data Input data
     * @return 20-byte hash
     */
    static std::array<uint8_t, 20> RIPEMD160(const std::vector<uint8_t>& data);
    static core::UInt160 RIPEMD160ToUInt160(const std::vector<uint8_t>& data);

    /**
     * @brief Hash160 (RIPEMD160(SHA256(data)))
     * @param data Input data
     * @return 20-byte hash
     */
    static std::array<uint8_t, 20> Hash160(const std::vector<uint8_t>& data);
    static core::UInt160 Hash160ToUInt160(const std::vector<uint8_t>& data);

    /**
     * @brief Hash256 (SHA256(SHA256(data)))
     * @param data Input data
     * @return 32-byte hash
     */
    static std::array<uint8_t, 32> Hash256(const std::vector<uint8_t>& data);
    static core::UInt256 Hash256ToUInt256(const std::vector<uint8_t>& data);

    /**
     * @brief Keccak256 hash (used in Ethereum compatibility)
     * @param data Input data
     * @return 32-byte hash
     */
    static std::array<uint8_t, 32> Keccak256(const std::vector<uint8_t>& data);

    /**
     * @brief Murmur3 hash (32-bit)
     * @param data Input data
     * @param seed Optional seed
     * @return 32-bit hash
     */
    static uint32_t Murmur32(const std::vector<uint8_t>& data, uint32_t seed = 0);

    /**
     * @brief Calculate Merkle root
     * @param hashes List of hashes
     * @return Merkle root hash
     */
    static core::UInt256 CalculateMerkleRoot(const std::vector<core::UInt256>& hashes);

    /**
     * @brief Verify Merkle proof
     * @param root Merkle root
     * @param leaf Leaf hash
     * @param proof Merkle proof path
     * @return true if proof is valid
     */
    static bool VerifyMerkleProof(
        const core::UInt256& root,
        const core::UInt256& leaf,
        const std::vector<core::UInt256>& proof
    );
};

/**
 * @brief Base58 encoding/decoding with checksum
 */
class Base58Check {
public:
    /**
     * @brief Encode data to Base58Check
     * @param data Raw data
     * @return Base58Check encoded string
     */
    static std::string Encode(const std::vector<uint8_t>& data);

    /**
     * @brief Decode Base58Check string
     * @param encoded Base58Check string
     * @return Decoded data (empty if invalid)
     */
    static std::vector<uint8_t> Decode(const std::string& encoded);

    /**
     * @brief Check if string is valid Base58Check
     * @param encoded String to check
     * @return true if valid
     */
    static bool IsValid(const std::string& encoded);
};

/**
 * @brief Address utilities
 */
class Address {
public:
    /**
     * @brief Convert script hash to address
     * @param scriptHash Script hash
     * @param version Address version (0x35 for Neo N3)
     * @return Neo address
     */
    static std::string FromScriptHash(const core::UInt160& scriptHash, uint8_t version = 0x35);

    /**
     * @brief Convert address to script hash
     * @param address Neo address
     * @return Script hash (null if invalid)
     */
    static std::optional<core::UInt160> ToScriptHash(const std::string& address);

    /**
     * @brief Validate Neo address
     * @param address Address to validate
     * @return true if valid
     */
    static bool IsValid(const std::string& address);

    /**
     * @brief Get address version
     * @param address Neo address
     * @return Version byte
     */
    static uint8_t GetVersion(const std::string& address);
};

} // namespace neo::sdk::crypto