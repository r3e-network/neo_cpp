#pragma once

#include <neo/cryptography/ecc/ec_point.h>
#include <neo/io/uint160.h>
#include <vector>
#include <string>
#include <span>
#include <memory>

namespace neo::wallets
{
    /**
     * @brief Represents a cryptographic key pair for wallet operations.
     */
    class KeyPair
    {
    public:
        /**
         * @brief Constructor from private key.
         * @param private_key The private key bytes.
         */
        explicit KeyPair(std::span<const uint8_t> private_key);

        /**
         * @brief Constructor from private key vector.
         * @param private_key The private key bytes.
         */
        explicit KeyPair(const std::vector<uint8_t>& private_key);

        /**
         * @brief Destructor.
         */
        ~KeyPair();

        /**
         * @brief Copy constructor.
         * @param other The other key pair.
         */
        KeyPair(const KeyPair& other);

        /**
         * @brief Move constructor.
         * @param other The other key pair.
         */
        KeyPair(KeyPair&& other) noexcept;

        /**
         * @brief Copy assignment operator.
         * @param other The other key pair.
         * @return Reference to this key pair.
         */
        KeyPair& operator=(const KeyPair& other);

        /**
         * @brief Move assignment operator.
         * @param other The other key pair.
         * @return Reference to this key pair.
         */
        KeyPair& operator=(KeyPair&& other) noexcept;

        /**
         * @brief Gets the private key.
         * @return The private key bytes.
         */
        std::vector<uint8_t> GetPrivateKey() const;

        /**
         * @brief Gets the public key.
         * @return The public key.
         */
        cryptography::ecc::ECPoint GetPublicKey() const;

        /**
         * @brief Gets the script hash for this key pair.
         * @return The script hash.
         */
        io::UInt160 GetScriptHash() const;

        /**
         * @brief Gets the Neo address for this key pair.
         * @param address_version The address version byte.
         * @return The Neo address.
         */
        std::string GetAddress(uint8_t address_version = 0x35) const;

        /**
         * @brief Signs a message with this key pair.
         * @param message The message to sign.
         * @return The signature.
         */
        std::vector<uint8_t> Sign(std::span<const uint8_t> message) const;

        /**
         * @brief Verifies a signature against this key pair's public key.
         * @param message The original message.
         * @param signature The signature to verify.
         * @return True if signature is valid, false otherwise.
         */
        bool VerifySignature(std::span<const uint8_t> message, std::span<const uint8_t> signature) const;

        /**
         * @brief Exports the key pair to WIF (Wallet Import Format).
         * @return The WIF string.
         */
        std::string ToWIF() const;

        /**
         * @brief Creates a key pair from WIF (Wallet Import Format).
         * @param wif The WIF string.
         * @return The key pair.
         */
        static KeyPair FromWIF(const std::string& wif);

        /**
         * @brief Generates a new random key pair.
         * @return A new key pair.
         */
        static KeyPair Generate();

        /**
         * @brief Creates a key pair from a hex string.
         * @param hex The hex string of the private key.
         * @return The key pair.
         */
        static KeyPair FromHex(const std::string& hex);

        /**
         * @brief Exports the private key to hex string.
         * @return The hex string.
         */
        std::string ToHex() const;

        /**
         * @brief Checks if this key pair is valid.
         * @return True if valid, false otherwise.
         */
        bool IsValid() const;

        /**
         * @brief Equality operator.
         * @param other The other key pair.
         * @return True if equal, false otherwise.
         */
        bool operator==(const KeyPair& other) const;

        /**
         * @brief Inequality operator.
         * @param other The other key pair.
         * @return True if not equal, false otherwise.
         */
        bool operator!=(const KeyPair& other) const;

    private:
        std::vector<uint8_t> private_key_;
        mutable std::unique_ptr<cryptography::ecc::ECPoint> public_key_;
        mutable std::unique_ptr<io::UInt160> script_hash_;

        /**
         * @brief Validates the private key.
         * @param private_key The private key to validate.
         * @return True if valid, false otherwise.
         */
        static bool ValidatePrivateKey(std::span<const uint8_t> private_key);

        /**
         * @brief Computes the public key from the private key.
         */
        void ComputePublicKey() const;

        /**
         * @brief Computes the script hash from the public key.
         */
        void ComputeScriptHash() const;

        /**
         * @brief Clears sensitive data.
         */
        void Clear();
    };
}
