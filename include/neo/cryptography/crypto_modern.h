/**
 * @file crypto_modern.h
 * @brief Modern OpenSSL 3.0+ compatible cryptography functions
 */

#pragma once

#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/io/byte_vector.h>
#include <neo/io/byte_span.h>

namespace neo {
namespace cryptography {

/**
 * @brief Modern implementation of Hash160 using EVP API
 * @param data Input data to hash
 * @return 160-bit hash (SHA256 then RIPEMD160)
 */
io::UInt160 Hash160Modern(const io::ByteSpan& data);

/**
 * @brief Modern implementation of HMAC-SHA256
 * @param data Data to authenticate
 * @param key Secret key
 * @return HMAC-SHA256 result
 */
io::ByteVector HmacSha256(const io::ByteSpan& data, const io::ByteSpan& key);

/**
 * @brief Generate cryptographically secure random bytes
 * @param length Number of bytes to generate
 * @return Random bytes
 */
io::ByteVector GenerateRandomBytes(size_t length);

/**
 * @brief Modern SHA256 implementation
 * @param data Input data
 * @return 256-bit hash
 */
io::UInt256 Sha256(const io::ByteSpan& data);

/**
 * @brief Modern double SHA256 implementation
 * @param data Input data
 * @return 256-bit hash (SHA256 of SHA256)
 */
io::UInt256 Hash256(const io::ByteSpan& data);

/**
 * @brief Verify ECDSA signature
 * @param message Message that was signed
 * @param signature Signature to verify
 * @param publicKey Public key to verify with
 * @return true if signature is valid
 */
bool VerifySignature(const io::ByteSpan& message, const io::ByteSpan& signature, const io::ByteSpan& publicKey);

/**
 * @brief Create ECDSA signature
 * @param message Message to sign
 * @param privateKey Private key to sign with
 * @return Signature
 */
io::ByteVector SignData(const io::ByteSpan& message, const io::ByteSpan& privateKey);

} // namespace cryptography
} // namespace neo