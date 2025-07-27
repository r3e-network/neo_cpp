#pragma once

#include <cstdint>
#include <neo/io/byte_vector.h>
#include <string>

// Forward declarations
namespace neo::cryptography::ecc
{
class KeyPair;
class ECPoint;
}  // namespace neo::cryptography::ecc

namespace neo::cryptography::ecc
{
/**
 * @brief secp256r1 elliptic curve operations
 */
class Secp256r1
{
  public:
    static constexpr size_t PRIVATE_KEY_SIZE = 32;
    static constexpr size_t PUBLIC_KEY_SIZE = 33;  // Compressed
    static constexpr size_t SIGNATURE_SIZE = 64;

    /**
     * @brief Generate a random private key
     */
    static io::ByteVector GeneratePrivateKey();

    /**
     * @brief Compute public key from private key
     */
    static io::ByteVector ComputePublicKey(const io::ByteVector& privateKey);

    /**
     * @brief Sign data with private key
     */
    static io::ByteVector Sign(const io::ByteVector& data, const io::ByteVector& privateKey);

    /**
     * @brief Verify signature with public key
     */
    static bool Verify(const io::ByteVector& data, const io::ByteVector& signature, const io::ByteVector& publicKey);

    /**
     * @brief Validate private key
     */
    static bool IsValidPrivateKey(const io::ByteVector& privateKey);

    /**
     * @brief Validate public key
     */
    static bool IsValidPublicKey(const io::ByteVector& publicKey);

    /**
     * @brief Check if byte vector is all zeros
     */
    static bool IsZero(const io::ByteVector& value);

    /**
     * @brief Verify public key is on secp256r1 curve
     */
    static bool IsOnCurve(const io::ByteVector& publicKey);

    // Methods that match C# Neo implementation

    /**
     * @brief Generate a key pair
     */
    static KeyPair GenerateKeyPair();

    /**
     * @brief Create key pair from private key
     */
    static KeyPair FromPrivateKey(const io::ByteVector& privateKey);

    /**
     * @brief Create key pair from WIF
     */
    static KeyPair FromWIF(const std::string& wif);

    /**
     * @brief Convert private key to WIF format
     */
    static std::string ToWIF(const io::ByteVector& privateKey, bool compressed = true);

    /**
     * @brief Convert private key to NEP2 format (encrypted)
     */
    static std::string ToNEP2(const io::ByteVector& privateKey, const std::string& passphrase, int scryptN = 16384,
                              int scryptR = 8, int scryptP = 8);

    /**
     * @brief Create private key from NEP2 format (decrypt)
     */
    static io::ByteVector FromNEP2(const std::string& nep2, const std::string& passphrase);

    /**
     * @brief Create private key from NEP2 format (decrypt) with custom scrypt parameters
     */
    static io::ByteVector FromNEP2(const std::string& nep2, const std::string& passphrase, int scryptN, int scryptR,
                                   int scryptP);
};
}  // namespace neo::cryptography::ecc