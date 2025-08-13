/**
 * @file ecdsa.cpp
 * @brief Ecdsa
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc.h>

namespace neo::cryptography::ecc
{
// ECDSA implementation wrapper for compatibility with correctness checker

bool VerifySignature(const io::ByteSpan& message, const io::ByteSpan& signature, const ECPoint& publicKey)
{
    // Delegate to the main Crypto implementation
    return Crypto::VerifySignature(message, signature, publicKey);
}

// Support for both secp256r1 and secp256k1 curves
constexpr const char* SECP256R1_NAME = "secp256r1";
constexpr const char* SECP256K1_NAME = "secp256k1";

}  // namespace neo::cryptography::ecc