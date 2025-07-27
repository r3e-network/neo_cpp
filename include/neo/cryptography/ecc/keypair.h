#pragma once

#include <memory>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>

namespace neo::cryptography::ecc
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
     * @brief Generate a new random key pair
     */
    static std::unique_ptr<KeyPair> Generate();

    /**
     * @brief Create key pair from WIF (Wallet Import Format)
     */
    static std::unique_ptr<KeyPair> FromWIF(const std::string& wif);

    /**
     * @brief Get the private key
     */
    const io::ByteVector& GetPrivateKey() const;

    /**
     * @brief Get the private key (alias for C# compatibility)
     */
    const io::ByteVector& PrivateKey() const
    {
        return GetPrivateKey();
    }

    /**
     * @brief Get the public key
     */
    const ECPoint& GetPublicKey() const;

    /**
     * @brief Get the public key (alias for C# compatibility)
     */
    const ECPoint& PublicKey() const
    {
        return GetPublicKey();
    }

    /**
     * @brief Get the script hash for this key pair
     */
    io::UInt160 GetScriptHash() const;

    /**
     * @brief Export private key to WIF format
     */
    std::string ToWIF() const;

    /**
     * @brief Sign data with this key pair
     */
    io::ByteVector Sign(const io::ByteVector& data) const;

    /**
     * @brief Verify signature with this key pair's public key
     */
    bool Verify(const io::ByteVector& data, const io::ByteVector& signature) const;

    /**
     * @brief Destructor
     */
    ~KeyPair() = default;

    // Delete copy operations for security (cryptographic keys should not be copied)
    KeyPair(const KeyPair&) = delete;
    KeyPair& operator=(const KeyPair&) = delete;

    // Allow move operations
    KeyPair(KeyPair&&) noexcept = default;
    KeyPair& operator=(KeyPair&&) noexcept = default;

  private:
    io::ByteVector privateKey_;
    mutable std::unique_ptr<ECPoint> publicKey_;

    void ComputePublicKey() const;
};
}  // namespace neo::cryptography::ecc