#pragma once

#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/cryptography/ecc/ec_point.h>
#include <string>
#include <vector>
#include <span>

namespace neo::wallets
{
    /**
     * @brief Helper functions for wallet operations.
     */
    class Helper
    {
    public:
        /**
         * @brief Converts a script hash to a Neo address.
         * @param script_hash The script hash.
         * @param address_version The address version byte.
         * @return The Neo address string.
         */
        static std::string ToAddress(const io::UInt160& script_hash, uint8_t address_version = 0x35);

        /**
         * @brief Converts a Neo address to a script hash.
         * @param address The Neo address string.
         * @param address_version The address version byte.
         * @return The script hash.
         */
        static io::UInt160 ToScriptHash(const std::string& address, uint8_t address_version = 0x35);

        /**
         * @brief Creates a signature script for a public key.
         * @param public_key The public key.
         * @return The signature script.
         */
        static std::vector<uint8_t> CreateSignatureScript(const cryptography::ecc::ECPoint& public_key);

        /**
         * @brief Creates a multi-signature script.
         * @param m The minimum number of signatures required.
         * @param public_keys The public keys.
         * @return The multi-signature script.
         */
        static std::vector<uint8_t> CreateMultiSigScript(int m, const std::vector<cryptography::ecc::ECPoint>& public_keys);

        /**
         * @brief Creates a script hash from a script.
         * @param script The script.
         * @return The script hash.
         */
        static io::UInt160 ToScriptHash(std::span<const uint8_t> script);

        /**
         * @brief Checks if an address is valid.
         * @param address The address to validate.
         * @param address_version The address version byte.
         * @return True if valid, false otherwise.
         */
        static bool IsValidAddress(const std::string& address, uint8_t address_version = 0x35);

        /**
         * @brief Signs a message with a private key.
         * @param message The message to sign.
         * @param private_key The private key.
         * @return The signature.
         */
        static std::vector<uint8_t> Sign(std::span<const uint8_t> message, std::span<const uint8_t> private_key);

        /**
         * @brief Verifies a signature.
         * @param message The original message.
         * @param signature The signature.
         * @param public_key The public key.
         * @return True if signature is valid, false otherwise.
         */
        static bool VerifySignature(std::span<const uint8_t> message, 
                                   std::span<const uint8_t> signature, 
                                   const cryptography::ecc::ECPoint& public_key);

        /**
         * @brief Generates a new private key.
         * @return A new private key.
         */
        static std::vector<uint8_t> GeneratePrivateKey();

        /**
         * @brief Gets the public key from a private key.
         * @param private_key The private key.
         * @return The public key.
         */
        static cryptography::ecc::ECPoint GetPublicKey(std::span<const uint8_t> private_key);

        /**
         * @brief Calculates the script hash for a public key.
         * @param public_key The public key.
         * @return The script hash.
         */
        static io::UInt160 GetScriptHash(const cryptography::ecc::ECPoint& public_key);

        /**
         * @brief Converts bytes to a hex string.
         * @param data The byte data.
         * @param reverse Whether to reverse the bytes.
         * @return The hex string.
         */
        static std::string ToHexString(std::span<const uint8_t> data, bool reverse = false);

        /**
         * @brief Converts a hex string to bytes.
         * @param hex The hex string.
         * @param reverse Whether to reverse the result.
         * @return The byte data.
         */
        static std::vector<uint8_t> FromHexString(const std::string& hex, bool reverse = false);

        /**
         * @brief Calculates the checksum for address generation.
         * @param data The data to checksum.
         * @return The checksum bytes.
         */
        static std::vector<uint8_t> CalculateChecksum(std::span<const uint8_t> data);

        /**
         * @brief Base58 encodes data.
         * @param data The data to encode.
         * @return The base58 encoded string.
         */
        static std::string Base58Encode(std::span<const uint8_t> data);

        /**
         * @brief Base58 decodes a string.
         * @param encoded The base58 encoded string.
         * @return The decoded data.
         */
        static std::vector<uint8_t> Base58Decode(const std::string& encoded);

        /**
         * @brief Base58Check encodes data.
         * @param data The data to encode.
         * @return The base58check encoded string.
         */
        static std::string Base58CheckEncode(std::span<const uint8_t> data);

        /**
         * @brief Base58Check decodes a string.
         * @param encoded The base58check encoded string.
         * @return The decoded data.
         */
        static std::vector<uint8_t> Base58CheckDecode(const std::string& encoded);

    private:
        /**
         * @brief Validates the checksum in base58check decoding.
         * @param data The data with checksum.
         * @return True if checksum is valid, false otherwise.
         */
        static bool ValidateChecksum(std::span<const uint8_t> data);
    };
}
