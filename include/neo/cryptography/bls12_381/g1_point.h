/**
 * @file g1_point.h
 * @brief BLS12-381 G1 group operations
 */

#pragma once

#include <neo/cryptography/bls12_381/field_element.h>
#include <neo/io/byte_vector.h>
#include <optional>
#include <vector>

namespace neo::cryptography::bls12_381 {

/**
 * @brief Point on the G1 elliptic curve (E(Fp))
 * @details Represents points on the BLS12-381 G1 curve y² = x³ + 4
 */
class G1Point {
public:
    FieldElement x;
    FieldElement y;
    FieldElement z;  // Jacobian coordinate for efficiency
    bool is_infinity;
    
    // Constants
    static const G1Point& Generator();
    static const G1Point& Identity();
    
    // Constructors
    G1Point();
    G1Point(const FieldElement& x, const FieldElement& y);
    G1Point(const FieldElement& x, const FieldElement& y, const FieldElement& z);
    
    // Point operations
    G1Point Add(const G1Point& other) const;
    G1Point Double() const;
    G1Point Negate() const;
    G1Point ScalarMultiply(const std::vector<uint8_t>& scalar) const;
    
    // Efficient scalar multiplication
    G1Point MultiplyByEndomorphism(const std::vector<uint8_t>& scalar) const;
    
    // Validation
    bool IsOnCurve() const;
    bool IsInSubgroup() const;
    bool IsInfinity() const { return is_infinity; }
    
    // Coordinate conversion
    G1Point ToAffine() const;
    void Normalize();
    
    // Serialization
    std::vector<uint8_t> Serialize(bool compressed = true) const;
    static std::optional<G1Point> Deserialize(const std::vector<uint8_t>& data);
    
    // Comparison
    bool Equals(const G1Point& other) const;
    bool operator==(const G1Point& other) const { return Equals(other); }
    bool operator!=(const G1Point& other) const { return !Equals(other); }
    
    // Hash-to-curve
    static G1Point HashToCurve(const std::vector<uint8_t>& message, 
                               const std::vector<uint8_t>& domain_separator);
    
private:
    // Internal helpers for point arithmetic
    static G1Point AddAffine(const G1Point& p1, const G1Point& p2);
    static G1Point AddJacobian(const G1Point& p1, const G1Point& p2);
    static G1Point DoubleJacobian(const G1Point& p);
};

/**
 * @brief Multi-scalar multiplication for G1 points
 * @details Efficiently computes sum(scalars[i] * points[i])
 */
class G1MultiScalarMul {
public:
    static G1Point Compute(const std::vector<G1Point>& points,
                           const std::vector<std::vector<uint8_t>>& scalars);
    
private:
    // Windowed NAF representation for efficiency
    static std::vector<int8_t> ComputeNAF(const std::vector<uint8_t>& scalar, 
                                          unsigned int width);
    
    // Precomputation tables
    static std::vector<G1Point> PrecomputeTable(const G1Point& point, 
                                                unsigned int width);
};

/**
 * @brief G1 signature aggregation
 */
class G1Aggregation {
public:
    // Aggregate multiple G1 points (e.g., signatures)
    static G1Point Aggregate(const std::vector<G1Point>& points);
    
    // Verify aggregated signature
    static bool VerifyAggregated(const std::vector<G1Point>& public_keys,
                                 const std::vector<std::vector<uint8_t>>& messages,
                                 const G1Point& signature);
};

} // namespace neo::cryptography::bls12_381