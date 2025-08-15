#pragma once

/**
 * @file keypair.h
 * @brief Cryptographic key pair management for Neo blockchain
 * @author Neo C++ Team
 * @date 2025
 */

#include <neo/sdk/core/types.h>
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace neo::sdk::crypto {

/**
 * @brief Elliptic curve types
 */
enum class CurveType {
    Secp256r1,  // Default for Neo
    Secp256k1   // Bitcoin curve
};

/**
 * @brief Private key wrapper
 */
class PrivateKey {
public:
    static constexpr size_t SIZE = 32;

    /**
     * @brief Create from byte array
     */
    explicit PrivateKey(const std::array<uint8_t, SIZE>& bytes);

    /**
     * @brief Create from hex string
     */
    explicit PrivateKey(const std::string& hex);

    /**
     * @brief Generate random private key
     */
    static PrivateKey Random();

    /**
     * @brief Get bytes
     */
    const std::array<uint8_t, SIZE>& GetBytes() const { return bytes_; }

    /**
     * @brief Convert to hex string
     */
    std::string ToHex() const;

    /**
     * @brief Convert to WIF (Wallet Import Format)
     */
    std::string ToWIF() const;

    /**
     * @brief Create from WIF string
     */
    static PrivateKey FromWIF(const std::string& wif);

    /**
     * @brief Sign data
     * @param data Data to sign
     * @return Signature bytes
     */
    std::vector<uint8_t> Sign(const std::vector<uint8_t>& data) const;

    /**
     * @brief Clear private key from memory
     */
    void Clear();

    /**
     * @brief Destructor (clears memory)
     */
    ~PrivateKey();

private:
    std::array<uint8_t, SIZE> bytes_;
};

/**
 * @brief Public key wrapper
 */
class PublicKey {
public:
    static constexpr size_t COMPRESSED_SIZE = 33;
    static constexpr size_t UNCOMPRESSED_SIZE = 65;

    /**
     * @brief Create from compressed bytes
     */
    explicit PublicKey(const std::array<uint8_t, COMPRESSED_SIZE>& bytes);

    /**
     * @brief Create from hex string
     */
    explicit PublicKey(const std::string& hex);

    /**
     * @brief Get compressed bytes
     */
    std::array<uint8_t, COMPRESSED_SIZE> GetCompressedBytes() const;

    /**
     * @brief Get uncompressed bytes
     */
    std::array<uint8_t, UNCOMPRESSED_SIZE> GetUncompressedBytes() const;

    /**
     * @brief Convert to hex string (compressed)
     */
    std::string ToHex() const;

    /**
     * @brief Get script hash
     */
    core::UInt160 GetScriptHash() const;

    /**
     * @brief Get Neo address
     */
    std::string GetAddress() const;

    /**
     * @brief Verify signature
     * @param data Original data
     * @param signature Signature to verify
     * @return true if signature is valid
     */
    bool Verify(const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature) const;

    /**
     * @brief Check if point is valid on curve
     */
    bool IsValid() const;

private:
    std::array<uint8_t, COMPRESSED_SIZE> compressed_;
};

/**
 * @brief Cryptographic key pair
 */
class KeyPair {
public:
    /**
     * @brief Generate new random key pair
     * @param curve Elliptic curve type
     */
    static KeyPair Generate(CurveType curve = CurveType::Secp256r1);

    /**
     * @brief Create from private key
     * @param privateKey Private key
     * @param curve Curve type
     */
    explicit KeyPair(const PrivateKey& privateKey, CurveType curve = CurveType::Secp256r1);

    /**
     * @brief Create from WIF
     * @param wif WIF string
     */
    static KeyPair FromWIF(const std::string& wif);

    /**
     * @brief Get private key
     */
    const PrivateKey& GetPrivateKey() const { return privateKey_; }

    /**
     * @brief Get public key
     */
    const PublicKey& GetPublicKey() const { return publicKey_; }

    /**
     * @brief Get script hash
     */
    core::UInt160 GetScriptHash() const;

    /**
     * @brief Get Neo address
     */
    std::string GetAddress() const;

    /**
     * @brief Sign data
     * @param data Data to sign
     * @return Signature
     */
    std::vector<uint8_t> Sign(const std::vector<uint8_t>& data) const;

    /**
     * @brief Sign message (adds Neo message prefix)
     * @param message Message to sign
     * @return Signature
     */
    std::vector<uint8_t> SignMessage(const std::string& message) const;

    /**
     * @brief Export to WIF
     */
    std::string ExportWIF() const;

    /**
     * @brief Export to NEP-2 encrypted format
     * @param password Password for encryption
     * @return NEP-2 encrypted key
     */
    std::string ExportNEP2(const std::string& password) const;

    /**
     * @brief Import from NEP-2 encrypted format
     * @param nep2 NEP-2 encrypted key
     * @param password Password for decryption
     * @return Key pair
     */
    static KeyPair FromNEP2(const std::string& nep2, const std::string& password);

private:
    PrivateKey privateKey_;
    PublicKey publicKey_;
    CurveType curve_;
};

/**
 * @brief Multi-signature address creator
 */
class MultiSig {
public:
    /**
     * @brief Create multi-signature address
     * @param m Minimum number of signatures required
     * @param publicKeys Public keys
     * @return Multi-sig script hash
     */
    static core::UInt160 CreateMultiSigScriptHash(
        uint8_t m,
        const std::vector<PublicKey>& publicKeys
    );

    /**
     * @brief Create multi-signature redeem script
     * @param m Minimum signatures
     * @param publicKeys Public keys
     * @return Redeem script
     */
    static std::vector<uint8_t> CreateMultiSigRedeemScript(
        uint8_t m,
        const std::vector<PublicKey>& publicKeys
    );

    /**
     * @brief Get multi-sig address
     * @param m Minimum signatures
     * @param publicKeys Public keys
     * @return Neo address
     */
    static std::string GetMultiSigAddress(
        uint8_t m,
        const std::vector<PublicKey>& publicKeys
    );
};

} // namespace neo::sdk::crypto