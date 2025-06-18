#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <optional>
#include <neo/io/byte_span.h>

namespace neo::cryptography
{
    /**
     * @brief Represents an elliptic curve point for cryptographic operations
     * 
     * This class handles elliptic curve points used in Neo's cryptographic
     * operations, particularly for public keys and digital signatures.
     */
    class ECPoint
    {
    public:
        /**
         * @brief Default constructor - creates point at infinity
         */
        ECPoint();

        /**
         * @brief Constructor from encoded point data
         * @param encodedPoint The encoded point data
         */
        explicit ECPoint(const std::vector<uint8_t>& encodedPoint);

        /**
         * @brief Constructor from hex string
         * @param hexString The hex string representation of the point
         */
        explicit ECPoint(const std::string& hexString);

        /**
         * @brief Copy constructor
         */
        ECPoint(const ECPoint& other);

        /**
         * @brief Assignment operator
         */
        ECPoint& operator=(const ECPoint& other);

        /**
         * @brief Destructor
         */
        ~ECPoint();

        /**
         * @brief Parse ECPoint from hex string
         * @param hexString The hex string to parse
         * @return Parsed ECPoint
         * @throws std::invalid_argument if string is invalid
         */
        static ECPoint Parse(const std::string& hexString);

        /**
         * @brief Create ECPoint from byte data
         * @param data The byte data representing the point
         * @return Optional ECPoint if parsing succeeds, nullopt if invalid
         */
        static std::optional<ECPoint> FromBytes(const std::vector<uint8_t>& data);

        /**
         * @brief Create ECPoint from byte span
         * @param span The byte span representing the point
         * @return Optional ECPoint if parsing succeeds, nullopt if invalid
         */
        static std::optional<ECPoint> FromBytes(const neo::io::ByteSpan& span);

        /**
         * @brief Get the encoded point data
         * @return Vector containing the encoded point
         */
        std::vector<uint8_t> GetEncoded() const;

        /**
         * @brief Convert to hex string representation
         * @return Hex string representation of the point
         */
        std::string ToString() const;

        /**
         * @brief Check if this is the point at infinity
         * @return True if point at infinity
         */
        bool IsInfinity() const;

        /**
         * @brief Check if the point is valid
         * @return True if the point is valid
         */
        bool IsValid() const;

        /**
         * @brief Equality operator
         */
        bool operator==(const ECPoint& other) const;

        /**
         * @brief Inequality operator
         */
        bool operator!=(const ECPoint& other) const;

        /**
         * @brief Less than operator for ordering
         */
        bool operator<(const ECPoint& other) const;

        /**
         * @brief Get the size of the encoded point
         * @return Size in bytes
         */
        size_t Size() const;

    private:
        class Impl;
        std::unique_ptr<Impl> pImpl_;
    };
}

// Hash function for ECPoint to use in unordered containers
namespace std
{
    template<>
    struct hash<neo::cryptography::ECPoint>
    {
        size_t operator()(const neo::cryptography::ECPoint& point) const;
    };
} 