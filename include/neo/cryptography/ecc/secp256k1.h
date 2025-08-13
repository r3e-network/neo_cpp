/**
 * @file secp256k1.h
 * @brief Secp256k1
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_vector.h>

#include <cstdint>
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
 * @brief secp256k1 elliptic curve operations (for Ethereum compatibility)
 */
class Secp256k1
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
};

/**
 * @brief ECCurve enumeration for curve selection
 */
enum class ECCurve
{
    Secp256r1,
    Secp256k1
};

}  // namespace neo::cryptography::ecc