#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <vector>

namespace neo::cryptography::neo_signatures
{
/**
 * @brief Neo-specific cryptographic signature operations header.
 *
 * This header provides the exact cryptographic signature operations
 * used by the Neo blockchain, ensuring 100% compatibility with
 * the C# Neo node implementation.
 */

/**
 * @brief Creates a Neo-style signature redeem script for a single public key.
 *
 * This function creates a redeem script that can be used to verify
 * a single signature. The format matches Neo's standard signature
 * verification script.
 *
 * @param publicKey The 33-byte compressed public key
 * @return The redeem script bytes (35 bytes total)
 * @throws std::invalid_argument if publicKey is not 33 bytes
 */
io::ByteVector CreateSingleSignatureRedeemScript(const io::ByteSpan& publicKey);

/**
 * @brief Creates a Neo-style multi-signature redeem script.
 *
 * Creates a multi-signature redeem script that requires m signatures
 * out of n public keys. This follows Neo's multi-signature format.
 *
 * @param m Minimum number of signatures required (1-16)
 * @param publicKeys Vector of public keys (each 33 bytes compressed, max 16 keys)
 * @return The multi-signature redeem script bytes
 * @throws std::invalid_argument if parameters are invalid
 */
io::ByteVector CreateMultiSignatureRedeemScript(int m, const std::vector<io::ByteSpan>& publicKeys);

/**
 * @brief Signs a message hash using ECDSA with secp256r1 curve.
 *
 * This function performs ECDSA signing using Neo's standard curve (secp256r1)
 * and returns the signature in Neo's expected format (64 bytes: r + s).
 *
 * @param messageHash The 32-byte message hash to sign
 * @param privateKey The 32-byte private key
 * @return The 64-byte signature (32 bytes r + 32 bytes s)
 * @throws std::invalid_argument if key size is wrong
 * @throws std::runtime_error if signing fails
 */
io::ByteVector SignMessageHash(const io::UInt256& messageHash, const io::ByteSpan& privateKey);

/**
 * @brief Verifies an ECDSA signature using secp256r1 curve.
 *
 * Verifies a signature created with SignMessageHash using the corresponding
 * public key. Uses Neo's standard secp256r1 curve.
 *
 * @param messageHash The 32-byte message hash that was signed
 * @param signature The 64-byte signature (r + s format)
 * @param publicKey The 33-byte compressed public key
 * @return true if signature is valid, false otherwise
 */
bool VerifyMessageHash(const io::UInt256& messageHash, const io::ByteSpan& signature, const io::ByteSpan& publicKey);

/**
 * @brief Computes the script hash (Hash160) for a redeem script.
 *
 * Computes Hash160 (SHA256 followed by RIPEMD160) of the redeem script.
 * This is used to create script hash addresses in Neo.
 *
 * @param redeemScript The redeem script bytes
 * @return The 20-byte script hash
 */
io::UInt160 ComputeScriptHash(const io::ByteSpan& redeemScript);

/**
 * @brief Creates a verification script for a given script hash.
 *
 * Creates a verification script that pushes the script hash and calls CHECKSIG.
 * This is used in certain Neo transaction verification scenarios.
 *
 * @param scriptHash The 20-byte script hash
 * @return The verification script bytes
 */
io::ByteVector CreateVerificationScript(const io::UInt160& scriptHash);

/**
 * @brief Derives a public key from a private key using secp256r1.
 *
 * Computes the corresponding public key for a given private key
 * using Neo's standard secp256r1 curve. Returns compressed format.
 *
 * @param privateKey The 32-byte private key
 * @return The 33-byte compressed public key
 * @throws std::invalid_argument if private key size is wrong
 * @throws std::runtime_error if key derivation fails
 */
io::ByteVector DerivePublicKey(const io::ByteSpan& privateKey);

/**
 * @brief Validates that a public key is valid on the secp256r1 curve.
 *
 * Checks if the given public key is valid:
 * - Correct size (33 bytes compressed)
 * - Point is on the secp256r1 curve
 * - Point is not at infinity
 *
 * @param publicKey The 33-byte compressed public key to validate
 * @return true if the public key is valid, false otherwise
 */
bool ValidatePublicKey(const io::ByteSpan& publicKey);
}  // namespace neo::cryptography::neo_signatures