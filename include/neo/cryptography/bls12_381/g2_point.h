/**
 * @file g2_point.h
 * @brief BLS12-381 G2 group operations
 */

#pragma once

#include <neo/cryptography/bls12_381/field_element.h>
#include <neo/io/byte_vector.h>
#include <optional>
#include <vector>

namespace neo::cryptography::bls12_381 {

/**
 * @brief Point on the G2 elliptic curve (E'(Fp2))
 * @details Represents points on the BLS12-381 G2 twisted curve
 */
class G2Point {
public:
    FieldElement2 x;
    FieldElement2 y;
    FieldElement2 z;  // Jacobian coordinate
    bool is_infinity;
    
    // Constants
    static const G2Point& Generator();
    static const G2Point& Identity();
    
    // Constructors
    G2Point();
    G2Point(const FieldElement2& x, const FieldElement2& y);
    G2Point(const FieldElement2& x, const FieldElement2& y, const FieldElement2& z);
    
    // Point operations
    G2Point Add(const G2Point& other) const;
    G2Point Double() const;
    G2Point Negate() const;
    G2Point ScalarMultiply(const std::vector<uint8_t>& scalar) const;
    
    // Efficient operations
    G2Point MultiplyByEndomorphism(const std::vector<uint8_t>& scalar) const;
    G2Point FrobeniusMap(unsigned int power) const;
    
    // Validation
    bool IsOnCurve() const;
    bool IsInSubgroup() const;
    bool IsInfinity() const { return is_infinity; }
    
    // Coordinate conversion
    G2Point ToAffine() const;
    void Normalize();
    
    // Serialization
    std::vector<uint8_t> Serialize(bool compressed = true) const;
    static std::optional<G2Point> Deserialize(const std::vector<uint8_t>& data);
    
    // Comparison
    bool Equals(const G2Point& other) const;
    bool operator==(const G2Point& other) const { return Equals(other); }
    bool operator!=(const G2Point& other) const { return !Equals(other); }
    
    // Hash-to-curve for G2
    static G2Point HashToCurve(const std::vector<uint8_t>& message,
                               const std::vector<uint8_t>& domain_separator);
    
    // Pairing preparation
    std::vector<FieldElement2> PreparePairing() const;
    
private:
    // Internal helpers
    static G2Point AddAffine(const G2Point& p1, const G2Point& p2);
    static G2Point AddJacobian(const G2Point& p1, const G2Point& p2);
    static G2Point DoubleJacobian(const G2Point& p);
    
    // Twist isomorphism
    static FieldElement2 ApplyTwist(const FieldElement2& point);
    static FieldElement2 ApplyUntwist(const FieldElement2& point);
};

/**
 * @brief Multi-scalar multiplication for G2 points
 */
class G2MultiScalarMul {
public:
    static G2Point Compute(const std::vector<G2Point>& points,
                           const std::vector<std::vector<uint8_t>>& scalars);
    
private:
    static std::vector<int8_t> ComputeNAF(const std::vector<uint8_t>& scalar,
                                          unsigned int width);
    static std::vector<G2Point> PrecomputeTable(const G2Point& point,
                                                unsigned int width);
};

/**
 * @brief G2 signature aggregation
 */
class G2Aggregation {
public:
    // Aggregate multiple G2 points (e.g., public keys)
    static G2Point Aggregate(const std::vector<G2Point>& points);
    
    // Batch verification optimization
    static bool BatchVerify(const std::vector<G2Point>& public_keys,
                            const std::vector<std::vector<uint8_t>>& messages,
                            const std::vector<G2Point>& signatures);
};

/**
 * @brief Precomputed G2 points for pairing
 */
class G2Prepared {
public:
    std::vector<FieldElement2> coefficients;
    G2Point point;
    
    explicit G2Prepared(const G2Point& p);
    
    // Get precomputed line evaluations
    const std::vector<FieldElement2>& GetCoefficients() const { return coefficients; }
    
private:
    void Precompute();
    std::vector<FieldElement2> ComputeLineEvaluations(const G2Point& p) const;
};

} // namespace neo::cryptography::bls12_381