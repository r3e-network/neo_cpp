/**
 * @file eccurve.h
 * @brief Elliptic curve cryptography
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/cryptography/ecc/ec_point.h>
#include <neo/io/byte_vector.h>

#include <memory>
#include <vector>

namespace neo::cryptography::ecc
{
/**
 * @brief Elliptic curve parameters and operations
 */
class ECCurve
{
   public:
    /**
     * @brief Curve parameters
     */
    struct CurveParams
    {
        io::ByteVector p;  // Prime modulus
        io::ByteVector a;  // Curve parameter a
        io::ByteVector b;  // Curve parameter b
        io::ByteVector g;  // Generator point
        io::ByteVector n;  // Order of generator
        io::ByteVector h;  // Cofactor
    };

   private:
    CurveParams params_;
    std::shared_ptr<ECPoint> generator_;

   public:
    /**
     * @brief Get the secp256r1 curve (NIST P-256)
     * @return Secp256r1 curve instance
     */
    static const ECCurve& Secp256r1();

    /**
     * @brief Get the secp256k1 curve
     * @return Secp256k1 curve instance
     */
    static const ECCurve& Secp256k1();

    /**
     * @brief Constructor with curve parameters
     * @param params Curve parameters
     */
    explicit ECCurve(const CurveParams& params);

    /**
     * @brief Get curve parameters
     * @return Curve parameters
     */
    const CurveParams& GetParams() const { return params_; }

    /**
     * @brief Get generator point
     * @return Generator point
     */
    std::shared_ptr<ECPoint> GetGenerator() const { return generator_; }

    /**
     * @brief Create point from coordinates
     * @param x X coordinate
     * @param y Y coordinate
     * @return EC point
     */
    std::shared_ptr<ECPoint> CreatePoint(const io::ByteVector& x, const io::ByteVector& y) const;

    /**
     * @brief Decode point from bytes
     * @param data Encoded point data
     * @return Decoded EC point
     */
    std::shared_ptr<ECPoint> DecodePoint(const io::ByteVector& data) const;

    /**
     * @brief Verify point is on curve
     * @param point Point to verify
     * @return True if point is on curve
     */
    bool IsOnCurve(const ECPoint& point) const;

    /**
     * @brief Get curve name
     * @return Curve name string
     */
    std::string GetName() const;

    /**
     * @brief Get field size in bytes
     * @return Field size in bytes
     */
    size_t GetFieldSize() const;
};
}  // namespace neo::cryptography::ecc