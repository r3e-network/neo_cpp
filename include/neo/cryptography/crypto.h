#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <cstdint>
#include <string>

// Forward declarations
namespace neo::cryptography::ecc
{
    class ECPoint;
}

namespace neo::cryptography
{
    /**
     * @brief Provides cryptographic utility functions.
     */
    class Crypto
    {
    public:
        /**
         * @brief Generates random bytes.
         * @param length The number of bytes to generate.
         * @return The random bytes.
         */
        static io::ByteVector GenerateRandomBytes(size_t length);

        /**
         * @brief Computes the SHA256 hash of the input data.
         * @param data The input data.
         * @param length The length of the input data.
         * @return The SHA256 hash.
         */
        static io::UInt256 Hash256(const uint8_t* data, size_t length);

        /**
         * @brief Computes the SHA256 hash of the input data.
         * @param data The input data.
         * @return The SHA256 hash.
         */
        static io::UInt256 Hash256(const io::ByteSpan& data);

        /**
         * @brief Computes the double SHA256 hash (SHA256 of SHA256) of the input data.
         * @param data The input data.
         * @return The double SHA256 hash.
         */
        static io::UInt256 Hash256(const io::ByteVector& data);

        /**
         * @brief Computes the SHA160 hash (RIPEMD160 of SHA256) of the input data.
         * @param data The input data.
         * @return The SHA160 hash.
         */
        static io::UInt160 Hash160(const io::ByteSpan& data);

        /**
         * @brief Encrypts data using AES-256-CBC.
         * @param data The data to encrypt.
         * @param key The encryption key (must be 32 bytes).
         * @param iv The initialization vector (must be 16 bytes).
         * @return The encrypted data.
         */
        static io::ByteVector AesEncrypt(const io::ByteSpan& data, const io::ByteSpan& key, const io::ByteSpan& iv);

        /**
         * @brief Decrypts data using AES-256-CBC.
         * @param data The data to decrypt.
         * @param key The decryption key (must be 32 bytes).
         * @param iv The initialization vector (must be 16 bytes).
         * @return The decrypted data.
         */
        static io::ByteVector AesDecrypt(const io::ByteSpan& data, const io::ByteSpan& key, const io::ByteSpan& iv);

        /**
         * @brief Derives a key using PBKDF2-HMAC-SHA256.
         * @param password The password.
         * @param salt The salt.
         * @param iterations The number of iterations.
         * @param keyLength The desired key length in bytes.
         * @return The derived key.
         */
        static io::ByteVector PBKDF2(const io::ByteSpan& password, const io::ByteSpan& salt, int iterations, int keyLength);

        /**
         * @brief Computes the HMAC-SHA256 of the input data.
         * @param key The key.
         * @param data The input data.
         * @return The HMAC-SHA256.
         */
        static io::ByteVector HmacSha256(const io::ByteSpan& key, const io::ByteSpan& data);

        /**
         * @brief Computes the Base64 encoding of the input data.
         * @param data The input data.
         * @return The Base64 encoded string.
         */
        static std::string Base64Encode(const io::ByteSpan& data);

        /**
         * @brief Decodes a Base64 encoded string.
         * @param base64 The Base64 encoded string.
         * @return The decoded data.
         */
        static io::ByteVector Base64Decode(const std::string& base64);

        /**
         * @brief Creates a signature redeem script for a public key.
         * @param publicKey The public key.
         * @return The signature redeem script.
         */
        static io::ByteVector CreateSignatureRedeemScript(const ecc::ECPoint& publicKey);

        /**
         * @brief Verifies a signature.
         * @param message The message that was signed.
         * @param signature The signature to verify.
         * @param publicKey The public key to verify against.
         * @return True if the signature is valid, false otherwise.
         */
        static bool VerifySignature(const io::ByteSpan& message, const io::ByteSpan& signature, const ecc::ECPoint& publicKey);
    };


}
