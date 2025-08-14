/**
 * @file bls12_381_refactored.h
 * @brief Refactored BLS12-381 main header
 * @details This header provides the complete BLS12-381 elliptic curve implementation
 *          split into modular components for better maintainability.
 */

#pragma once

// Core field operations
#include <neo/cryptography/bls12_381/field_element.h>

// Elliptic curve groups
#include <neo/cryptography/bls12_381/g1_point.h>
#include <neo/cryptography/bls12_381/g2_point.h>

// Pairing operations
#include <neo/cryptography/bls12_381/pairing.h>

namespace neo::cryptography::bls12_381 {

/**
 * @brief Main interface for BLS12-381 operations
 * @details Provides high-level API for common BLS12-381 operations
 */
class BLS12_381 {
public:
    // Version information
    static constexpr const char* VERSION = "1.2.0";
    static constexpr const char* CURVE_NAME = "BLS12-381";
    
    // Curve parameters
    static constexpr size_t FIELD_SIZE = 48;      // 381 bits ≈ 48 bytes
    static constexpr size_t G1_SIZE = 48;         // Compressed G1 point
    static constexpr size_t G2_SIZE = 96;         // Compressed G2 point
    static constexpr size_t SCALAR_SIZE = 32;     // Scalar field size
    
    /**
     * @brief Initialize BLS12-381 library
     * @details Sets up precomputed tables and constants
     */
    static void Initialize();
    
    /**
     * @brief Self-test to verify implementation correctness
     * @return true if all tests pass
     */
    static bool SelfTest();
    
    // Convenience functions for common operations
    
    /**
     * @brief Generate a random scalar in the correct field
     */
    static std::vector<uint8_t> GenerateRandomScalar();
    
    /**
     * @brief Hash arbitrary data to a G1 point
     */
    static G1Point HashToG1(const std::vector<uint8_t>& data,
                           const std::string& domain = "BLS_SIG_BLS12381G1_XMD:SHA-256_SSWU_RO_NUL_");
    
    /**
     * @brief Hash arbitrary data to a G2 point
     */
    static G2Point HashToG2(const std::vector<uint8_t>& data,
                           const std::string& domain = "BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_NUL_");
    
    /**
     * @brief Simple BLS signature creation
     */
    static G1Point Sign(const std::vector<uint8_t>& message,
                       const std::vector<uint8_t>& private_key);
    
    /**
     * @brief Simple BLS signature verification
     */
    static bool Verify(const G1Point& signature,
                      const std::vector<uint8_t>& message,
                      const G2Point& public_key);
    
    /**
     * @brief Aggregate multiple signatures
     */
    static G1Point AggregateSignatures(const std::vector<G1Point>& signatures);
    
    /**
     * @brief Verify an aggregated signature
     */
    static bool VerifyAggregated(const G1Point& aggregated_sig,
                                 const std::vector<std::vector<uint8_t>>& messages,
                                 const std::vector<G2Point>& public_keys);
    
    // Performance benchmarks
    struct Benchmarks {
        double g1_add_ms;
        double g1_mul_ms;
        double g2_add_ms;
        double g2_mul_ms;
        double pairing_ms;
        double multi_pairing_ms;
        double signature_ms;
        double verification_ms;
    };
    
    /**
     * @brief Run performance benchmarks
     */
    static Benchmarks RunBenchmarks(int iterations = 100);
    
    // Security validation
    
    /**
     * @brief Validate that a point is in the correct subgroup
     * @details Essential for preventing invalid curve attacks
     */
    static bool ValidateG1Point(const G1Point& point);
    static bool ValidateG2Point(const G2Point& point);
    
    /**
     * @brief Clear sensitive data from memory
     */
    static void SecureClear(std::vector<uint8_t>& data);
    
private:
    static bool initialized_;
    
    // Precomputed values for efficiency
    static void PrecomputeTables();
    static void PrecomputePairingConstants();
};

/**
 * @brief Utility functions for BLS12-381
 */
namespace utils {
    
    /**
     * @brief Convert between different point representations
     */
    std::vector<uint8_t> G1PointToBytes(const G1Point& point, bool compressed = true);
    G1Point G1PointFromBytes(const std::vector<uint8_t>& bytes);
    
    std::vector<uint8_t> G2PointToBytes(const G2Point& point, bool compressed = true);
    G2Point G2PointFromBytes(const std::vector<uint8_t>& bytes);
    
    /**
     * @brief Scalar field operations
     */
    std::vector<uint8_t> ScalarAdd(const std::vector<uint8_t>& a,
                                   const std::vector<uint8_t>& b);
    std::vector<uint8_t> ScalarMultiply(const std::vector<uint8_t>& a,
                                        const std::vector<uint8_t>& b);
    std::vector<uint8_t> ScalarInverse(const std::vector<uint8_t>& scalar);
    
    /**
     * @brief Lagrange interpolation for threshold schemes
     */
    std::vector<uint8_t> LagrangeCoefficient(size_t index,
                                             const std::vector<size_t>& indices);
}

/**
 * @brief Constants used in BLS12-381
 */
namespace constants {
    // Generator points
    extern const G1Point G1_GENERATOR;
    extern const G2Point G2_GENERATOR;
    
    // Field modulus
    extern const std::vector<uint8_t> FIELD_MODULUS;
    extern const std::vector<uint8_t> SCALAR_FIELD_MODULUS;
    
    // Curve equation parameters
    extern const FieldElement CURVE_B;  // y^2 = x^3 + B
    extern const FieldElement2 TWIST_B; // Twisted curve parameter
    
    // Pairing parameters
    extern const std::vector<uint8_t> ATE_LOOP_COUNT;
    extern const bool ATE_LOOP_IS_NEGATIVE;
}

} // namespace neo::cryptography::bls12_381