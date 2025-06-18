#pragma once

#include <neo/io/uint256.h>
#include <neo/io/byte_vector.h>
#include <vector>
#include <cstdint>

namespace neo::cryptography
{
    /**
     * @brief ECDSA cryptographic operations for Neo.
     */
    class ECDSA
    {
    public:
        /**
         * @brief Verifies an ECDSA signature.
         * @param message The message that was signed.
         * @param signature The signature to verify.
         * @param publicKey The public key to verify against.
         * @return True if the signature is valid, false otherwise.
         */
        static bool Verify(const std::vector<uint8_t>& message,
                          const std::vector<uint8_t>& signature,
                          const std::vector<uint8_t>& publicKey);

        /**
         * @brief Verifies an ECDSA signature with a hash.
         * @param hash The hash that was signed.
         * @param signature The signature to verify.
         * @param publicKey The public key to verify against.
         * @return True if the signature is valid, false otherwise.
         */
        static bool VerifyHash(const io::UInt256& hash,
                              const std::vector<uint8_t>& signature,
                              const std::vector<uint8_t>& publicKey);

        /**
         * @brief Creates an ECDSA signature.
         * @param message The message to sign.
         * @param privateKey The private key to sign with.
         * @return The signature.
         */
        static std::vector<uint8_t> Sign(const std::vector<uint8_t>& message,
                                        const std::vector<uint8_t>& privateKey);

        /**
         * @brief Creates an ECDSA signature for a hash.
         * @param hash The hash to sign.
         * @param privateKey The private key to sign with.
         * @return The signature.
         */
        static std::vector<uint8_t> SignHash(const io::UInt256& hash,
                                            const std::vector<uint8_t>& privateKey);

        /**
         * @brief Generates a new ECDSA key pair.
         * @param privateKey Output parameter for the private key.
         * @param publicKey Output parameter for the public key.
         */
        static void GenerateKeyPair(std::vector<uint8_t>& privateKey,
                                   std::vector<uint8_t>& publicKey);
    };

} // namespace neo::cryptography 