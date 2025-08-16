#pragma once

/**
 * @file unified_keypair.h
 * @brief Unified key pair management for Neo blockchain
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 * 
 * This file consolidates multiple duplicate KeyPair implementations
 * into a single, comprehensive key management class.
 */

#include <neo/cryptography/ecc/ec_point.h>
#include <neo/io/byte_span.h>
#include <neo/io/uint256.h>
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace neo::cryptography {

// Forward declarations
class PrivateKey;
class PublicKey;

/**
 * @brief Unified cryptographic key pair
 * 
 * This class consolidates all KeyPair implementations from:
 * - neo::cryptography::ecc::KeyPair
 * - neo::wallets::KeyPair
 * - neo::sdk::crypto::KeyPair
 * - BLS12_381 KeyPair structs
 */
class UnifiedKeyPair {
public:
    // Type aliases
    using PrivateKeyBytes = std::array<uint8_t, 32>;
    using PublicKeyBytes = std::vector<uint8_t>;  // Variable size for different curves
    using Signature = std::vector<uint8_t>;

    /**
     * @brief Supported elliptic curves
     */
    enum class CurveType {
        Secp256r1,  // Default for Neo
        Secp256k1,  // Bitcoin compatibility
        BLS12_381   // BLS signatures
    };

    // ============= Constructors =============
    
    /**
     * @brief Default constructor - generates new random key pair
     */
    UnifiedKeyPair(CurveType curve = CurveType::Secp256r1);
    
    /**
     * @brief Construct from private key bytes
     */
    explicit UnifiedKeyPair(const PrivateKeyBytes& privateKey, 
                           CurveType curve = CurveType::Secp256r1);
    
    /**
     * @brief Construct from WIF (Wallet Import Format) string
     */
    explicit UnifiedKeyPair(const std::string& wif);
    
    /**
     * @brief Copy constructor
     */
    UnifiedKeyPair(const UnifiedKeyPair& other);
    
    /**
     * @brief Move constructor
     */
    UnifiedKeyPair(UnifiedKeyPair&& other) noexcept;
    
    /**
     * @brief Destructor - securely wipes private key from memory
     */
    ~UnifiedKeyPair();

    // ============= Operators =============
    
    UnifiedKeyPair& operator=(const UnifiedKeyPair& other);
    UnifiedKeyPair& operator=(UnifiedKeyPair&& other) noexcept;
    bool operator==(const UnifiedKeyPair& other) const;
    bool operator!=(const UnifiedKeyPair& other) const;

    // ============= Key Generation =============
    
    /**
     * @brief Generate new random key pair
     */
    static UnifiedKeyPair Generate(CurveType curve = CurveType::Secp256r1);
    
    /**
     * @brief Generate deterministic key pair from seed
     */
    static UnifiedKeyPair FromSeed(const std::vector<uint8_t>& seed, 
                                   CurveType curve = CurveType::Secp256r1);
    
    /**
     * @brief Import from WIF (Wallet Import Format)
     */
    static UnifiedKeyPair FromWIF(const std::string& wif);
    
    /**
     * @brief Import from private key hex string
     */
    static UnifiedKeyPair FromPrivateKeyHex(const std::string& hex, 
                                           CurveType curve = CurveType::Secp256r1);

    // ============= Key Access =============
    
    /**
     * @brief Get private key bytes (handle with care!)
     */
    PrivateKeyBytes GetPrivateKeyBytes() const;
    
    /**
     * @brief Get public key bytes
     */
    PublicKeyBytes GetPublicKeyBytes(bool compressed = true) const;
    
    /**
     * @brief Get public key as EC point
     */
    ecc::ECPoint GetPublicKeyPoint() const;
    
    /**
     * @brief Get curve type
     */
    CurveType GetCurveType() const { return curveType_; }
    
    /**
     * @brief Get address (Neo format)
     */
    std::string GetAddress() const;
    
    /**
     * @brief Get script hash
     */
    io::UInt160 GetScriptHash() const;

    // ============= Export Functions =============
    
    /**
     * @brief Export as WIF (Wallet Import Format)
     */
    std::string ToWIF() const;
    
    /**
     * @brief Export private key as hex string
     */
    std::string ToPrivateKeyHex() const;
    
    /**
     * @brief Export public key as hex string
     */
    std::string ToPublicKeyHex(bool compressed = true) const;

    // ============= Cryptographic Operations =============
    
    /**
     * @brief Sign message
     */
    Signature Sign(const std::vector<uint8_t>& message) const;
    Signature Sign(const io::ByteSpan& message) const;
    Signature Sign(const io::UInt256& hash) const;
    
    /**
     * @brief Verify signature
     */
    bool Verify(const std::vector<uint8_t>& message, const Signature& signature) const;
    bool Verify(const io::ByteSpan& message, const Signature& signature) const;
    bool Verify(const io::UInt256& hash, const Signature& signature) const;
    
    /**
     * @brief Static signature verification
     */
    static bool VerifySignature(const PublicKeyBytes& publicKey,
                               const std::vector<uint8_t>& message,
                               const Signature& signature,
                               CurveType curve = CurveType::Secp256r1);

    // ============= Utility Functions =============
    
    /**
     * @brief Check if private key is valid
     */
    bool IsValid() const;
    
    /**
     * @brief Clear private key from memory
     */
    void Clear();
    
    /**
     * @brief Get key size in bits
     */
    size_t GetKeySize() const;
    
    /**
     * @brief Convert curve type to string
     */
    static std::string CurveTypeToString(CurveType curve);
    
    /**
     * @brief Parse curve type from string
     */
    static CurveType ParseCurveType(const std::string& str);

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
    CurveType curveType_;
    
    // Security: Zero out memory
    static void SecureZero(void* ptr, size_t len);
};

// ============= Compatibility Aliases =============

// Alias for backward compatibility
using KeyPair = UnifiedKeyPair;

// Namespace aliases for different modules
namespace ecc {
    using KeyPair = UnifiedKeyPair;
}

namespace wallets {
    using KeyPair = UnifiedKeyPair;
}

} // namespace neo::cryptography

// SDK compatibility
namespace neo::sdk::crypto {
    using KeyPair = neo::cryptography::UnifiedKeyPair;
}

namespace neo::wallets {
    using KeyPair = neo::cryptography::UnifiedKeyPair;
}