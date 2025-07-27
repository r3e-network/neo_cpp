#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <memory>
#include <string>

namespace neo::cryptography::bls12_381
{
    /**
     * @brief Represents a point on the G1 curve of BLS12-381.
     */
    class G1Point
    {
    public:
        /**
         * @brief Constructs a G1Point at infinity.
         */
        G1Point();

        /**
         * @brief Destructor.
         */
        ~G1Point();

        /**
         * @brief Copy constructor.
         */
        G1Point(const G1Point& other);

        /**
         * @brief Assignment operator.
         */
        G1Point& operator=(const G1Point& other);

        /**
         * @brief Constructs a G1Point from a byte array.
         * @param data The byte array.
         * @throws std::invalid_argument if the data is invalid.
         */
        explicit G1Point(const io::ByteSpan& data);

        /**
         * @brief Constructs a G1Point from a hex string.
         * @param hex The hex string.
         * @throws std::invalid_argument if the hex string is invalid.
         */
        static G1Point FromHex(const std::string& hex);

        /**
         * @brief Gets the generator point of the G1 curve.
         * @return The generator point.
         */
        static G1Point Generator();

        /**
         * @brief Converts the G1Point to a byte array.
         * @param compressed Whether to use compressed format.
         * @return The byte array.
         */
        io::ByteVector ToBytes(bool compressed = true) const;

        /**
         * @brief Converts the G1Point to a hex string.
         * @param compressed Whether to use compressed format.
         * @return The hex string.
         */
        std::string ToHex(bool compressed = true) const;

        /**
         * @brief Adds another G1Point to this G1Point.
         * @param other The other G1Point.
         * @return The result of the addition.
         */
        G1Point Add(const G1Point& other) const;

        /**
         * @brief Multiplies this G1Point by a scalar.
         * @param scalar The scalar.
         * @return The result of the multiplication.
         */
        G1Point Multiply(const io::ByteSpan& scalar) const;

        /**
         * @brief Checks if this G1Point is equal to another G1Point.
         * @param other The other G1Point.
         * @return True if the G1Points are equal, false otherwise.
         */
        bool operator==(const G1Point& other) const;

        /**
         * @brief Checks if this G1Point is not equal to another G1Point.
         * @param other The other G1Point.
         * @return True if the G1Points are not equal, false otherwise.
         */
        bool operator!=(const G1Point& other) const;

        /**
         * @brief Checks if this G1Point is infinity.
         * @return True if this G1Point is infinity, false otherwise.
         */
        bool IsInfinity() const;

        /**
         * @brief Gets the size of a G1Point in bytes (compressed).
         * @return The size of a G1Point in bytes.
         */
        static constexpr size_t CompressedSize = 48;

        /**
         * @brief Gets the size of a G1Point in bytes (uncompressed).
         * @return The size of a G1Point in bytes.
         */
        static constexpr size_t UncompressedSize = 96;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;

        G1Point(std::unique_ptr<Impl> impl);
    };

    /**
     * @brief Represents a point on the G2 curve of BLS12-381.
     */
    class G2Point
    {
    public:
        /**
         * @brief Constructs a G2Point at infinity.
         */
        G2Point();

        /**
         * @brief Destructor.
         */
        ~G2Point();

        /**
         * @brief Copy constructor.
         */
        G2Point(const G2Point& other);

        /**
         * @brief Assignment operator.
         */
        G2Point& operator=(const G2Point& other);

        /**
         * @brief Constructs a G2Point from a byte array.
         * @param data The byte array.
         * @throws std::invalid_argument if the data is invalid.
         */
        explicit G2Point(const io::ByteSpan& data);

        /**
         * @brief Constructs a G2Point from a hex string.
         * @param hex The hex string.
         * @throws std::invalid_argument if the hex string is invalid.
         */
        static G2Point FromHex(const std::string& hex);

        /**
         * @brief Gets the generator point of the G2 curve.
         * @return The generator point.
         */
        static G2Point Generator();

        /**
         * @brief Converts the G2Point to a byte array.
         * @param compressed Whether to use compressed format.
         * @return The byte array.
         */
        io::ByteVector ToBytes(bool compressed = true) const;

        /**
         * @brief Converts the G2Point to a hex string.
         * @param compressed Whether to use compressed format.
         * @return The hex string.
         */
        std::string ToHex(bool compressed = true) const;

        /**
         * @brief Adds another G2Point to this G2Point.
         * @param other The other G2Point.
         * @return The result of the addition.
         */
        G2Point Add(const G2Point& other) const;

        /**
         * @brief Multiplies this G2Point by a scalar.
         * @param scalar The scalar.
         * @return The result of the multiplication.
         */
        G2Point Multiply(const io::ByteSpan& scalar) const;

        /**
         * @brief Checks if this G2Point is equal to another G2Point.
         * @param other The other G2Point.
         * @return True if the G2Points are equal, false otherwise.
         */
        bool operator==(const G2Point& other) const;

        /**
         * @brief Checks if this G2Point is not equal to another G2Point.
         * @param other The other G2Point.
         * @return True if the G2Points are not equal, false otherwise.
         */
        bool operator!=(const G2Point& other) const;

        /**
         * @brief Checks if this G2Point is infinity.
         * @return True if this G2Point is infinity, false otherwise.
         */
        bool IsInfinity() const;

        /**
         * @brief Gets the size of a G2Point in bytes (compressed).
         * @return The size of a G2Point in bytes.
         */
        static constexpr size_t CompressedSize = 96;

        /**
         * @brief Gets the size of a G2Point in bytes (uncompressed).
         * @return The size of a G2Point in bytes.
         */
        static constexpr size_t UncompressedSize = 192;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;

        G2Point(std::unique_ptr<Impl> impl);
    };

    /**
     * @brief Represents a point in the target group GT of BLS12-381.
     */
    class GTPoint
    {
    public:
        /**
         * @brief Constructs a GTPoint at identity.
         */
        GTPoint();

        /**
         * @brief Destructor.
         */
        ~GTPoint();

        /**
         * @brief Copy constructor.
         */
        GTPoint(const GTPoint& other);

        /**
         * @brief Assignment operator.
         */
        GTPoint& operator=(const GTPoint& other);

        /**
         * @brief Constructs a GTPoint from a byte array.
         * @param data The byte array.
         * @throws std::invalid_argument if the data is invalid.
         */
        explicit GTPoint(const io::ByteSpan& data);

        /**
         * @brief Constructs a GTPoint from a hex string.
         * @param hex The hex string.
         * @throws std::invalid_argument if the hex string is invalid.
         */
        static GTPoint FromHex(const std::string& hex);

        /**
         * @brief Converts the GTPoint to a byte array.
         * @return The byte array.
         */
        io::ByteVector ToBytes() const;

        /**
         * @brief Converts the GTPoint to a hex string.
         * @return The hex string.
         */
        std::string ToHex() const;

        /**
         * @brief Multiplies this GTPoint by another GTPoint.
         * @param other The other GTPoint.
         * @return The result of the multiplication.
         */
        GTPoint Multiply(const GTPoint& other) const;

        /**
         * @brief Raises this GTPoint to a power.
         * @param scalar The scalar.
         * @return The result of the exponentiation.
         */
        GTPoint Pow(const io::ByteSpan& scalar) const;

        /**
         * @brief Checks if this GTPoint is equal to another GTPoint.
         * @param other The other GTPoint.
         * @return True if the GTPoints are equal, false otherwise.
         */
        bool operator==(const GTPoint& other) const;

        /**
         * @brief Checks if this GTPoint is not equal to another GTPoint.
         * @param other The other GTPoint.
         * @return True if the GTPoints are not equal, false otherwise.
         */
        bool operator!=(const GTPoint& other) const;

        /**
         * @brief Checks if this GTPoint is identity.
         * @return True if this GTPoint is identity, false otherwise.
         */
        bool IsIdentity() const;

        /**
         * @brief Gets the size of a GTPoint in bytes.
         * @return The size of a GTPoint in bytes.
         */
        static constexpr size_t Size = 576;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;

        GTPoint(std::unique_ptr<Impl> impl);

        // Friend functions that need access to Impl
        friend GTPoint Pairing(const G1Point& p, const G2Point& q);
        friend GTPoint MultiPairing(const std::vector<G1Point>& ps, const std::vector<G2Point>& qs);
    };

    /**
     * @brief Computes the pairing of a G1Point and a G2Point.
     * @param p The G1Point.
     * @param q The G2Point.
     * @return The result of the pairing.
     */
    GTPoint Pairing(const G1Point& p, const G2Point& q);

    /**
     * @brief Computes the multi-pairing of G1Points and G2Points.
     * @param ps The G1Points.
     * @param qs The G2Points.
     * @return The result of the multi-pairing.
     * @throws std::invalid_argument if the number of G1Points and G2Points is not equal.
     */
    GTPoint MultiPairing(const std::vector<G1Point>& ps, const std::vector<G2Point>& qs);

    /**
     * @brief Computes the Miller loop for a pair of points.
     * @param p The G1Point.
     * @param q The G2Point.
     * @return The result of the Miller loop computation.
     */
    GTPoint MillerLoop(const G1Point& p, const G2Point& q);

    /**
     * @brief Performs the final exponentiation step of GT.
     * @param f The result from Miller loop.
     * @return The final GT result.
     */
    GTPoint Gt(const GTPoint& f);

    /**
     * @brief Verifies a BLS signature.
     * @param publicKey The public key (G2Point).
     * @param message The message.
     * @param signature The signature (G1Point).
     * @return True if the signature is valid, false otherwise.
     */
    bool VerifySignature(const G2Point& publicKey, const io::ByteSpan& message, const G1Point& signature);

    /**
     * @brief Signs a message using BLS.
     * @param privateKey The private key.
     * @param message The message.
     * @return The signature (G1Point).
     */
    G1Point Sign(const io::ByteSpan& privateKey, const io::ByteSpan& message);

    /**
     * @brief Generates a public key from a private key.
     * @param privateKey The private key.
     * @return The public key (G2Point).
     */
    G2Point GeneratePublicKey(const io::ByteSpan& privateKey);

    /**
     * @brief Aggregates multiple signatures into a single signature.
     * @param signatures The signatures.
     * @return The aggregated signature.
     */
    G1Point AggregateSignatures(const std::vector<G1Point>& signatures);

    /**
     * @brief Verifies an aggregated signature.
     * @param publicKeys The public keys.
     * @param messages The messages.
     * @param signature The aggregated signature.
     * @return True if the signature is valid, false otherwise.
     * @throws std::invalid_argument if the number of public keys and messages is not equal.
     */
    bool VerifyAggregateSignature(const std::vector<G2Point>& publicKeys, const std::vector<io::ByteSpan>& messages, const G1Point& signature);

    /**
     * @brief Deserializes a G1Point from bytes.
     * @param data The byte data.
     * @param out The output G1Point.
     * @return True if deserialization was successful, false otherwise.
     */
    bool DeserializeG1Point(const io::ByteSpan& data, G1Point& out);

    /**
     * @brief Deserializes a G2Point from bytes.
     * @param data The byte data.
     * @param out The output G2Point.
     * @return True if deserialization was successful, false otherwise.
     */
    bool DeserializeG2Point(const io::ByteSpan& data, G2Point& out);

    /**
     * @brief Gets the G2 generator point.
     * @return The G2 generator point.
     */
    G2Point GetG2Generator();

    /**
     * @brief Negates a G2 point.
     * @param point The G2 point to negate.
     * @return The negated G2 point.
     */
    G2Point NegateG2(const G2Point& point);

    /**
     * @brief Multiplies two GT points.
     * @param a The first GT point.
     * @param b The second GT point.
     * @return The result of the multiplication.
     */
    GTPoint MultiplyGT(const GTPoint& a, const GTPoint& b);

    /**
     * @brief Checks if a GT point is the identity element.
     * @param point The GT point to check.
     * @return True if the point is identity, false otherwise.
     */
    bool IsIdentityGT(const GTPoint& point);

    /**
     * @brief Hashes a message to a G1 point.
     * @param message The message to hash.
     * @return The resulting G1 point.
     */
    G1Point HashToG1(const io::ByteSpan& message);

    /**
     * @brief Doubles a G1 point.
     * @param point The G1 point to double.
     * @return The doubled G1 point.
     */
    G1Point G1PointDouble(const G1Point& point);

    /**
     * @brief Negates a G1 point.
     * @param point The G1 point to negate.
     * @return The negated G1 point.
     */
    G1Point G1PointNegate(const G1Point& point);

    /**
     * @brief Gets the identity element for GT.
     * @return The GT identity element.
     */
    GTPoint GTPointIdentity();
}
