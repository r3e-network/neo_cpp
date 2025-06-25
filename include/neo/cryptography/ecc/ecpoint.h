#pragma once

#include <neo/io/byte_vector.h>
#include <neo/io/byte_span.h>
#include <neo/io/uint256.h>
#include <neo/io/iserializable.h>
#include <string>
#include <memory>

namespace neo::cryptography::ecc
{
    /**
     * @brief Represents a point on an elliptic curve.
     */
    class ECPoint : public io::ISerializable
    {
    public:
        /**
         * @brief The maximum size of an ECPoint in bytes (compressed format).
         */
        static constexpr size_t MaxSize = 33;
        /**
         * @brief Constructs an empty ECPoint.
         */
        ECPoint();

        /**
         * @brief Virtual destructor.
         */
        virtual ~ECPoint() = default;

        /**
         * @brief Constructs an ECPoint with the specified curve name.
         * @param curveName The curve name.
         */
        explicit ECPoint(const std::string& curveName);

        /**
         * @brief Gets the curve name.
         * @return The curve name.
         */
        const std::string& GetCurveName() const;

        /**
         * @brief Sets the curve name.
         * @param curveName The curve name.
         */
        void SetCurveName(const std::string& curveName);

        /**
         * @brief Checks if this point is at infinity.
         * @return True if this point is at infinity, false otherwise.
         */
        bool IsInfinity() const;

        /**
         * @brief Sets whether this point is at infinity.
         * @param isInfinity True if this point is at infinity, false otherwise.
         */
        void SetInfinity(bool isInfinity);

        /**
         * @brief Gets the X coordinate.
         * @return The X coordinate.
         */
        const io::UInt256& GetX() const;

        /**
         * @brief Sets the X coordinate.
         * @param x The X coordinate.
         */
        void SetX(const io::UInt256& x);

        /**
         * @brief Gets the Y coordinate.
         * @return The Y coordinate.
         */
        const io::UInt256& GetY() const;

        /**
         * @brief Sets the Y coordinate.
         * @param y The Y coordinate.
         */
        void SetY(const io::UInt256& y);

        /**
         * @brief Converts this point to a byte array.
         * @param compressed True to use compressed format, false otherwise.
         * @return The byte array.
         */
        io::ByteVector ToBytes(bool compressed = true) const;

        /**
         * @brief Converts this point to a byte array (compressed format).
         * This method matches the C# ECPoint.ToArray() method.
         * @return The byte array in compressed format.
         */
        io::ByteVector ToArray() const;

        /**
         * @brief Converts this point to a hex string.
         * @param compressed True to use compressed format, false otherwise.
         * @return The hex string.
         */
        std::string ToHex(bool compressed = true) const;

        /**
         * @brief Creates an ECPoint from a byte array.
         * @param data The byte array.
         * @param curveName The curve name.
         * @return The ECPoint.
         */
        static ECPoint FromBytes(const io::ByteSpan& data, const std::string& curveName);

        /**
         * @brief Creates an ECPoint from a byte array (with default curve).
         * @param data The byte array.
         * @return The ECPoint.
         */
        static ECPoint FromBytes(const io::ByteSpan& data) { return FromBytes(data, "secp256r1"); }

        /**
         * @brief Creates an ECPoint from a hex string.
         * @param hex The hex string.
         * @param curveName The curve name.
         * @return The ECPoint.
         */
        static ECPoint FromHex(const std::string& hex, const std::string& curveName);

        /**
         * @brief Creates an ECPoint from a hex string (with default curve).
         * @param hex The hex string.
         * @return The ECPoint.
         */
        static ECPoint FromHex(const std::string& hex) { return FromHex(hex, "secp256r1"); }

        /**
         * @brief Converts this point to a string (hex representation).
         * @param compressed True to use compressed format, false otherwise.
         * @return The hex string representation.
         */
        std::string ToString(bool compressed = true) const { return ToHex(compressed); }

        /**
         * @brief Parses an ECPoint from a hex string.
         * @param hex The hex string.
         * @param curveName The curve name.
         * @return The ECPoint.
         */
        static ECPoint Parse(const std::string& hex, const std::string& curveName = "secp256r1") { return FromHex(hex, curveName); }

        /**
         * @brief Gets the infinity point.
         * @param curveName The curve name.
         * @return The infinity point.
         */
        static ECPoint Infinity(const std::string& curveName = "secp256r1");

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
         * @brief Checks if this ECPoint is less than another ECPoint.
         * @param other The other ECPoint.
         * @return True if this ECPoint is less than the other ECPoint, false otherwise.
         */
        bool operator<(const ECPoint& other) const;

        /**
         * @brief Checks if this ECPoint is greater than another ECPoint.
         * @param other The other ECPoint.
         * @return True if this ECPoint is greater than the other ECPoint, false otherwise.
         */
        bool operator>(const ECPoint& other) const;

        /**
         * @brief Checks if this ECPoint is less than or equal to another ECPoint.
         * @param other The other ECPoint.
         * @return True if this ECPoint is less than or equal to the other ECPoint, false otherwise.
         */
        bool operator<=(const ECPoint& other) const;

        /**
         * @brief Checks if this ECPoint is greater than or equal to another ECPoint.
         * @param other The other ECPoint.
         * @return True if this ECPoint is greater than or equal to the other ECPoint, false otherwise.
         */
        bool operator>=(const ECPoint& other) const;

        // ISerializable implementation
        /**
         * @brief Serializes this ECPoint to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes this ECPoint from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

    private:
        std::string curveName_;
        bool isInfinity_;
        io::UInt256 x_;
        io::UInt256 y_;
    };
}
