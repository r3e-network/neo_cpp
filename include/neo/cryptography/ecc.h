#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint256.h>
#include <string>
#include <memory>

namespace neo::cryptography
{
    /**
     * @brief Represents a point on an elliptic curve.
     */
    class ECPoint
    {
    public:
        /**
         * @brief Constructs an ECPoint from a byte array.
         * @param data The byte array.
         * @param curve The curve name.
         * @throws std::invalid_argument if the data is invalid.
         */
        static ECPoint FromBytes(const io::ByteSpan& data, const std::string& curve = "secp256r1");

        /**
         * @brief Constructs an ECPoint from a hex string.
         * @param hex The hex string.
         * @param curve The curve name.
         * @throws std::invalid_argument if the hex string is invalid.
         */
        static ECPoint FromHex(const std::string& hex, const std::string& curve = "secp256r1");

        /**
         * @brief Converts the ECPoint to a byte array.
         * @param compressed Whether to use compressed format.
         * @return The byte array.
         */
        io::ByteVector ToBytes(bool compressed = true) const;

        /**
         * @brief Converts the ECPoint to a hex string.
         * @param compressed Whether to use compressed format.
         * @return The hex string.
         */
        std::string ToHex(bool compressed = true) const;

        /**
         * @brief Checks if this ECPoint is equal to another ECPoint.
         * @param other The other ECPoint.
         * @return True if the ECPoints are equal, false otherwise.
         */
        bool operator==(const ECPoint& other) const;

        /**
         * @brief Checks if this ECPoint is not equal to another ECPoint.
         * @param other The other ECPoint.
         * @return True if the ECPoints are not equal, false otherwise.
         */
        bool operator!=(const ECPoint& other) const;

        /**
         * @brief Gets the curve name.
         * @return The curve name.
         */
        std::string GetCurveName() const;

        /**
         * @brief Checks if this ECPoint is infinity.
         * @return True if this ECPoint is infinity, false otherwise.
         */
        bool IsInfinity() const;

        /**
         * @brief Destructor.
         */
        ~ECPoint();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;

        ECPoint(std::unique_ptr<Impl> impl);

    friend class Secp256r1;
    friend class Secp256k1;
    };

    /**
     * @brief Base class for elliptic curves.
     */
    class ECCurve
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~ECCurve() = default;

        /**
         * @brief Gets a curve by name.
         * @param name The curve name.
         * @return The curve.
         * @throws std::invalid_argument if the curve name is invalid.
         */
        static std::shared_ptr<ECCurve> GetCurve(const std::string& name);

        /**
         * @brief Gets the curve name.
         * @return The curve name.
         */
        virtual std::string GetName() const = 0;

        /**
         * @brief Generates a key pair.
         * @param privateKey The private key.
         * @return The public key.
         * @throws std::invalid_argument if the private key is invalid.
         */
        virtual ECPoint GenerateKeyPair(const io::ByteSpan& privateKey) const = 0;

        /**
         * @brief Signs a message.
         * @param message The message.
         * @param privateKey The private key.
         * @return The signature.
         * @throws std::invalid_argument if the private key is invalid.
         */
        virtual io::ByteVector Sign(const io::ByteSpan& message, const io::ByteSpan& privateKey) const = 0;

        /**
         * @brief Verifies a signature.
         * @param message The message.
         * @param signature The signature.
         * @param publicKey The public key.
         * @return True if the signature is valid, false otherwise.
         */
        virtual bool Verify(const io::ByteSpan& message, const io::ByteSpan& signature, const ECPoint& publicKey) const = 0;

        /**
         * @brief Gets the size of a private key in bytes.
         * @return The size of a private key in bytes.
         */
        virtual size_t GetPrivateKeySize() const = 0;

        /**
         * @brief Gets the size of a signature in bytes.
         * @return The size of a signature in bytes.
         */
        virtual size_t GetSignatureSize() const = 0;

        /**
         * @brief Gets the size of a public key in bytes (uncompressed).
         * @return The size of a public key in bytes.
         */
        virtual size_t GetPublicKeySize() const = 0;

        /**
         * @brief Gets the size of a public key in bytes (compressed).
         * @return The size of a public key in bytes.
         */
        virtual size_t GetCompressedPublicKeySize() const = 0;
    };

    /**
     * @brief Implementation of the secp256r1 curve.
     */
    class Secp256r1 : public ECCurve
    {
    public:
        /**
         * @brief Constructs a Secp256r1 curve.
         */
        Secp256r1();

        /**
         * @brief Gets the curve name.
         * @return The curve name.
         */
        std::string GetName() const override;

        /**
         * @brief Generates a key pair.
         * @param privateKey The private key.
         * @return The public key.
         * @throws std::invalid_argument if the private key is invalid.
         */
        ECPoint GenerateKeyPair(const io::ByteSpan& privateKey) const override;

        /**
         * @brief Signs a message.
         * @param message The message.
         * @param privateKey The private key.
         * @return The signature.
         * @throws std::invalid_argument if the private key is invalid.
         */
        io::ByteVector Sign(const io::ByteSpan& message, const io::ByteSpan& privateKey) const override;

        /**
         * @brief Verifies a signature.
         * @param message The message.
         * @param signature The signature.
         * @param publicKey The public key.
         * @return True if the signature is valid, false otherwise.
         */
        bool Verify(const io::ByteSpan& message, const io::ByteSpan& signature, const ECPoint& publicKey) const override;

        /**
         * @brief Gets the size of a private key in bytes.
         * @return The size of a private key in bytes.
         */
        size_t GetPrivateKeySize() const override;

        /**
         * @brief Gets the size of a signature in bytes.
         * @return The size of a signature in bytes.
         */
        size_t GetSignatureSize() const override;

        /**
         * @brief Gets the size of a public key in bytes (uncompressed).
         * @return The size of a public key in bytes.
         */
        size_t GetPublicKeySize() const override;

        /**
         * @brief Gets the size of a public key in bytes (compressed).
         * @return The size of a public key in bytes.
         */
        size_t GetCompressedPublicKeySize() const override;
    };

    /**
     * @brief Implementation of the secp256k1 curve.
     */
    class Secp256k1 : public ECCurve
    {
    public:
        /**
         * @brief Constructs a Secp256k1 curve.
         */
        Secp256k1();

        /**
         * @brief Gets the curve name.
         * @return The curve name.
         */
        std::string GetName() const override;

        /**
         * @brief Generates a key pair.
         * @param privateKey The private key.
         * @return The public key.
         * @throws std::invalid_argument if the private key is invalid.
         */
        ECPoint GenerateKeyPair(const io::ByteSpan& privateKey) const override;

        /**
         * @brief Signs a message.
         * @param message The message.
         * @param privateKey The private key.
         * @return The signature.
         * @throws std::invalid_argument if the private key is invalid.
         */
        io::ByteVector Sign(const io::ByteSpan& message, const io::ByteSpan& privateKey) const override;

        /**
         * @brief Verifies a signature.
         * @param message The message.
         * @param signature The signature.
         * @param publicKey The public key.
         * @return True if the signature is valid, false otherwise.
         */
        bool Verify(const io::ByteSpan& message, const io::ByteSpan& signature, const ECPoint& publicKey) const override;

        /**
         * @brief Gets the size of a private key in bytes.
         * @return The size of a private key in bytes.
         */
        size_t GetPrivateKeySize() const override;

        /**
         * @brief Gets the size of a signature in bytes.
         * @return The size of a signature in bytes.
         */
        size_t GetSignatureSize() const override;

        /**
         * @brief Gets the size of a public key in bytes (uncompressed).
         * @return The size of a public key in bytes.
         */
        size_t GetPublicKeySize() const override;

        /**
         * @brief Gets the size of a public key in bytes (compressed).
         * @return The size of a public key in bytes.
         */
        size_t GetCompressedPublicKeySize() const override;
    };
}
