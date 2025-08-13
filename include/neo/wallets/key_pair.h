/**
 * @file key_pair.h
 * @brief Key Pair
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>

#include <memory>
#include <string>

namespace neo::wallets
{
/**
 * @brief Represents a cryptographic key pair (private key + public key)
 */
class KeyPair
{
   public:
    /**
     * @brief Constructor with private key
     */
    explicit KeyPair(const io::ByteVector& privateKey);

    /**
     * @brief Destructor
     */
    ~KeyPair();

    /**
     * @brief Copy constructor
     */
    KeyPair(const KeyPair& other);

    /**
     * @brief Move constructor
     */
    KeyPair(KeyPair&& other) noexcept;

    /**
     * @brief Copy assignment operator
     */
    KeyPair& operator=(const KeyPair& other);

    /**
     * @brief Move assignment operator
     */
    KeyPair& operator=(KeyPair&& other) noexcept;

    /**
     * @brief Generate a new random key pair
     */
    static std::unique_ptr<KeyPair> Generate();

    /**
     * @brief Create key pair from WIF (Wallet Import Format)
     */
    static std::unique_ptr<KeyPair> FromWIF(const std::string& wif);

    /**
     * @brief Create key pair from hex string
     */
    static KeyPair FromHex(const std::string& hex);

    /**
     * @brief Get the private key
     */
    const io::ByteVector& GetPrivateKey() const;

    /**
     * @brief Get the public key
     */
    const cryptography::ecc::ECPoint& GetPublicKey() const;

    /**
     * @brief Get the script hash
     */
    io::UInt160 GetScriptHash() const;

    /**
     * @brief Get the address
     */
    std::string GetAddress(uint8_t address_version = 0x17) const;

    /**
     * @brief Export private key to WIF format
     */
    std::string ToWIF() const;

    /**
     * @brief Export private key to hex format
     */
    std::string ToHex() const;

    /**
     * @brief Check if the key pair is valid
     */
    bool IsValid() const;

    /**
     * @brief Sign data with this key pair
     */
    io::ByteVector Sign(const io::ByteVector& data) const;

    /**
     * @brief Verify signature with this key pair's public key
     */
    bool Verify(const io::ByteVector& data, const io::ByteVector& signature) const;

    /**
     * @brief Equality operator
     */
    bool operator==(const KeyPair& other) const;

    /**
     * @brief Inequality operator
     */
    bool operator!=(const KeyPair& other) const;

   private:
    io::ByteVector privateKey_;
    mutable std::unique_ptr<cryptography::ecc::ECPoint> publicKey_;
    mutable std::unique_ptr<io::UInt160> scriptHash_;

    void ComputePublicKey() const;
    void ComputeScriptHash() const;
    void Clear();

    /**
     * @brief Validate if private key is valid for secp256r1 curve
     */
    static bool IsValidPrivateKey(const io::ByteVector& privateKey);

    /**
     * @brief Validate private key (alias)
     */
    static bool ValidatePrivateKey(const io::ByteVector& privateKey);

    /**
     * @brief Base58 encode data
     */
    static std::string Base58Encode(const io::ByteVector& data);
};
}  // namespace neo::wallets
