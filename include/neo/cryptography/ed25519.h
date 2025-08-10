#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>

#include <array>
#include <string>

namespace neo::cryptography
{

// Forward declarations
class Ed25519;

/**
 * @brief Ed25519 signature scheme implementation.
 *
 * This class provides Ed25519 digital signature functionality following RFC 8032.
 * Ed25519 is a high-speed, high-security signature scheme that is widely used
 * in blockchain and cryptographic applications.
 */
class Ed25519
{
   public:
    static constexpr size_t PRIVATE_KEY_SIZE = 32;
    static constexpr size_t PUBLIC_KEY_SIZE = 32;
    static constexpr size_t SIGNATURE_SIZE = 64;
    static constexpr size_t SEED_SIZE = 32;

    // Forward declaration
    class PublicKey;

    /**
     * @brief Represents an Ed25519 private key.
     */
    class PrivateKey
    {
       public:
        /**
         * @brief Constructs a private key from raw bytes.
         * @param key_data The 32-byte private key data.
         */
        explicit PrivateKey(const io::ByteSpan& key_data);

        /**
         * @brief Generates a random private key.
         * @return A new random private key.
         */
        static PrivateKey Generate();

        /**
         * @brief Gets the raw private key bytes.
         * @return The private key as a byte vector.
         */
        io::ByteVector GetBytes() const;

        /**
         * @brief Derives the public key from this private key.
         * @return The corresponding public key.
         */
        PublicKey GetPublicKey() const;

        /**
         * @brief Signs a message with this private key.
         * @param message The message to sign.
         * @return The 64-byte signature.
         */
        io::ByteVector Sign(const io::ByteSpan& message) const;

       private:
        std::array<uint8_t, PRIVATE_KEY_SIZE> key_data_;
    };

    /**
     * @brief Represents an Ed25519 public key.
     */
    class PublicKey
    {
       public:
        /**
         * @brief Constructs a public key from raw bytes.
         * @param key_data The 32-byte public key data.
         */
        explicit PublicKey(const io::ByteSpan& key_data);

        /**
         * @brief Gets the raw public key bytes.
         * @return The public key as a byte vector.
         */
        io::ByteVector GetBytes() const;

        /**
         * @brief Verifies a signature against a message.
         * @param message The original message.
         * @param signature The signature to verify.
         * @return True if the signature is valid, false otherwise.
         */
        bool Verify(const io::ByteSpan& message, const io::ByteSpan& signature) const;

        /**
         * @brief Converts the public key to hex string.
         * @return Hex representation of the public key.
         */
        std::string ToHex() const;

        /**
         * @brief Creates a public key from hex string.
         * @param hex The hex string representation.
         * @return The public key.
         */
        static PublicKey FromHex(const std::string& hex);

       private:
        std::array<uint8_t, PUBLIC_KEY_SIZE> key_data_;
    };

    /**
     * @brief Generates a key pair from a seed.
     * @param seed The 32-byte seed.
     * @return A pair of (private_key, public_key).
     */
    static std::pair<PrivateKey, PublicKey> GenerateKeyPair(const io::ByteSpan& seed);

    /**
     * @brief Generates a random key pair.
     * @return A pair of (private_key, public_key).
     */
    static std::pair<PrivateKey, PublicKey> GenerateKeyPair();

    /**
     * @brief Verifies a signature without requiring a PublicKey object.
     * @param message The original message.
     * @param signature The signature to verify.
     * @param public_key The public key bytes.
     * @return True if the signature is valid, false otherwise.
     */
    static bool Verify(const io::ByteSpan& message, const io::ByteSpan& signature, const io::ByteSpan& public_key);
};

}  // namespace neo::cryptography