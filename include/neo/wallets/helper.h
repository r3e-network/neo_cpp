#pragma once

#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/io/byte_span.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <string>
#include <vector>

namespace neo::wallets
{
    /**
     * @brief Helper functions for wallet operations.
     * 
     * ## Overview
     * The Helper class provides essential wallet operations including address conversion,
     * script creation, cryptographic operations, and encoding/decoding functions.
     * 
     * ## API Reference
     * - **Address Operations**: ToAddress, ToScriptHash, IsValidAddress
     * - **Script Creation**: CreateSignatureScript, CreateMultiSigScript
     * - **Cryptographic Operations**: Sign, VerifySignature, GeneratePrivateKey, GetPublicKey
     * - **Encoding/Decoding**: ToHexString, FromHexString, Base58Encode, Base58Decode
     * - **Checksum Operations**: CalculateChecksum, ValidateChecksum
     * 
     * ## Usage Examples
     * ```cpp
     * // Convert script hash to address
     * auto address = Helper::ToAddress(scriptHash);
     * 
     * // Create signature script
     * auto script = Helper::CreateSignatureScript(publicKey);
     * 
     * // Sign a message
     * auto signature = Helper::Sign(message, privateKey);
     * ```
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
        static std::vector<uint8_t> CreateSignatureScript(const neo::cryptography::ecc::ECPoint& public_key);

        /**
         * @brief Creates a multi-signature script.
         * @param m The minimum number of signatures required.
         * @param public_keys The public keys.
         * @return The multi-signature script.
         */
        static std::vector<uint8_t> CreateMultiSigScript(int m, const std::vector<neo::cryptography::ecc::ECPoint>& public_keys);

        /**
         * @brief Creates a script hash from a script.
         * @param script_data The script data.
         * @param script_size The script size.
         * @return The script hash.
         */
        static io::UInt160 ToScriptHash(const uint8_t* script_data, size_t script_size);

        /**
         * @brief Creates a script hash from a script (ByteSpan overload).
         * @param script The script.
         * @return The script hash.
         */
        static io::UInt160 ToScriptHash(io::ByteSpan script);

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
         * @param message_size The message size.
         * @param private_key The private key.
         * @param private_key_size The private key size.
         * @return The signature.
         */
        static std::vector<uint8_t> Sign(const uint8_t* message, size_t message_size, const uint8_t* private_key, size_t private_key_size);

        /**
         * @brief Signs a message with a private key (ByteSpan overload).
         * @param message The message to sign.
         * @param private_key The private key.
         * @return The signature.
         */
        static std::vector<uint8_t> Sign(io::ByteSpan message, io::ByteSpan private_key);

        /**
         * @brief Verifies a signature.
         * @param message The original message.
         * @param message_size The message size.
         * @param signature The signature.
         * @param signature_size The signature size.
         * @param public_key The public key.
         * @return True if signature is valid, false otherwise.
         */
        static bool VerifySignature(const uint8_t* message, size_t message_size,
                                   const uint8_t* signature, size_t signature_size,
                                   const neo::cryptography::ecc::ECPoint& public_key);

        /**
         * @brief Verifies a signature (ByteSpan overload).
         * @param message The original message.
         * @param signature The signature.
         * @param public_key The public key.
         * @return True if signature is valid, false otherwise.
         */
        static bool VerifySignature(io::ByteSpan message, io::ByteSpan signature, const neo::cryptography::ecc::ECPoint& public_key);

        /**
         * @brief Generates a new private key.
         * @return A new private key.
         */
        static std::vector<uint8_t> GeneratePrivateKey();

        /**
         * @brief Gets the public key from a private key.
         * @param private_key The private key.
         * @param private_key_size The private key size.
         * @return The public key.
         */
        static neo::cryptography::ecc::ECPoint GetPublicKey(const uint8_t* private_key, size_t private_key_size);

        /**
         * @brief Gets the public key from a private key (ByteSpan overload).
         * @param private_key The private key.
         * @return The public key.
         */
        static neo::cryptography::ecc::ECPoint GetPublicKey(io::ByteSpan private_key);

        /**
         * @brief Calculates the script hash for a public key.
         * @param public_key The public key.
         * @return The script hash.
         */
        static io::UInt160 GetScriptHash(const neo::cryptography::ecc::ECPoint& public_key);

        /**
         * @brief Converts bytes to a hex string.
         * @param data The byte data.
         * @param data_size The data size.
         * @param reverse Whether to reverse the bytes.
         * @return The hex string.
         */
        static std::string ToHexString(const uint8_t* data, size_t data_size, bool reverse = false);

        /**
         * @brief Converts bytes to a hex string (ByteSpan overload).
         * @param data The byte data.
         * @param reverse Whether to reverse the bytes.
         * @return The hex string.
         */
        static std::string ToHexString(io::ByteSpan data, bool reverse = false);

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
         * @param data_size The data size.
         * @return The checksum bytes.
         */
        static std::vector<uint8_t> CalculateChecksum(const uint8_t* data, size_t data_size);

        /**
         * @brief Calculates the checksum for address generation (ByteSpan overload).
         * @param data The data to checksum.
         * @return The checksum bytes.
         */
        static std::vector<uint8_t> CalculateChecksum(io::ByteSpan data);

        /**
         * @brief Base58 encodes data.
         * @param data The data to encode.
         * @param data_size The data size.
         * @return The base58 encoded string.
         */
        static std::string Base58Encode(const uint8_t* data, size_t data_size);

        /**
         * @brief Base58 encodes data (ByteSpan overload).
         * @param data The data to encode.
         * @return The base58 encoded string.
         */
        static std::string Base58Encode(io::ByteSpan data);

        /**
         * @brief Base58 decodes a string.
         * @param encoded The base58 encoded string.
         * @return The decoded data.
         */
        static std::vector<uint8_t> Base58Decode(const std::string& encoded);

        /**
         * @brief Base58Check encodes data.
         * @param data The data to encode.
         * @param data_size The data size.
         * @return The base58check encoded string.
         */
        static std::string Base58CheckEncode(const uint8_t* data, size_t data_size);

        /**
         * @brief Base58Check encodes data (ByteSpan overload).
         * @param data The data to encode.
         * @return The base58check encoded string.
         */
        static std::string Base58CheckEncode(io::ByteSpan data);

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
         * @param data_size The data size.
         * @return True if checksum is valid, false otherwise.
         */
        static bool ValidateChecksum(const uint8_t* data, size_t data_size);

        /**
         * @brief Validates the checksum in base58check decoding (ByteSpan overload).
         * @param data The data with checksum.
         * @return True if checksum is valid, false otherwise.
         */
        static bool ValidateChecksum(io::ByteSpan data);
    };
}
