/**
 * @file crypto_wrappers.cpp
 * @brief Wrapper functions for modern cryptography implementations
 */

#include <neo/cryptography/crypto_modern.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/hash.h>
#include <openssl/rand.h>

namespace neo::cryptography {

// Generate random bytes
io::ByteVector GenerateRandomBytes(size_t length) {
    io::ByteVector result(length);
    if (RAND_bytes(result.Data(), length) != 1) {
        throw std::runtime_error("Failed to generate random bytes");
    }
    return result;
}

// HMAC SHA256
io::ByteVector HmacSha256(const io::ByteSpan& data, const io::ByteSpan& key) {
    return Crypto::HmacSha256(key, data);
}

// SHA256
io::UInt256 Sha256(const io::ByteSpan& data) {
    return Hash::Sha256(data);
}

} // namespace neo::cryptography