/**
 * @file pairing.h
 * @brief BLS12-381 pairing operations
 */

#pragma once

#include <neo/cryptography/bls12_381/field_element.h>
#include <neo/cryptography/bls12_381/g1_point.h>
#include <neo/cryptography/bls12_381/g2_point.h>
#include <vector>

namespace neo::cryptography::bls12_381 {

/**
 * @brief BLS12-381 optimal ate pairing implementation
 */
class Pairing {
public:
    /**
     * @brief Compute the pairing e(P, Q) for P in G1 and Q in G2
     * @param p Point in G1
     * @param q Point in G2
     * @return Result in GT (multiplicative subgroup of Fp12)
     */
    static FieldElement12 Compute(const G1Point& p, const G2Point& q);
    
    /**
     * @brief Multi-pairing: compute product of e(P[i], Q[i])
     * @param g1_points Points in G1
     * @param g2_points Points in G2
     * @return Product of pairings
     */
    static FieldElement12 MultiPairing(const std::vector<G1Point>& g1_points,
                                       const std::vector<G2Point>& g2_points);
    
    /**
     * @brief Verify pairing equation e(P1, Q1) * e(P2, Q2) = 1
     * @details Commonly used for signature verification
     */
    static bool VerifyPairingEquation(const G1Point& p1, const G2Point& q1,
                                      const G1Point& p2, const G2Point& q2);
    
    /**
     * @brief Batch pairing verification
     * @details More efficient than individual verifications
     */
    static bool BatchVerify(const std::vector<std::pair<G1Point, G2Point>>& pairs);
    
private:
    // Miller loop computation
    static FieldElement12 MillerLoop(const G1Point& p, const G2Point& q);
    
    // Line evaluation functions
    static FieldElement12 LineDouble(const G2Point& r, const G1Point& p);
    static FieldElement12 LineAdd(const G2Point& r, const G2Point& q, const G1Point& p);
    
    // Optimal ate pairing parameters
    static const std::vector<uint8_t>& GetLoopParameter();
    static bool IsLoopParameterNegative();
};

/**
 * @brief Precomputed pairing for efficiency
 */
class PrecomputedPairing {
public:
    PrecomputedPairing(const G1Point& p, const G2Point& q);
    
    // Get precomputed result
    FieldElement12 GetResult() const { return result_; }
    
    // Combine with another pairing
    FieldElement12 Multiply(const PrecomputedPairing& other) const;
    
private:
    FieldElement12 result_;
    std::vector<FieldElement12> miller_lines_;
    
    void Precompute(const G1Point& p, const G2Point& q);
};

/**
 * @brief BLS signature scheme using BLS12-381
 */
class BLSSignature {
public:
    // Key generation
    struct KeyPair {
        std::vector<uint8_t> private_key;
        G2Point public_key;
    };
    
    static KeyPair GenerateKeyPair();
    
    // Signing
    static G1Point Sign(const std::vector<uint8_t>& message,
                       const std::vector<uint8_t>& private_key,
                       const std::vector<uint8_t>& domain_separator = {});
    
    // Verification
    static bool Verify(const G1Point& signature,
                       const std::vector<uint8_t>& message,
                       const G2Point& public_key,
                       const std::vector<uint8_t>& domain_separator = {});
    
    // Aggregated signatures
    static G1Point AggregateSignatures(const std::vector<G1Point>& signatures);
    
    static bool VerifyAggregated(const G1Point& aggregated_signature,
                                 const std::vector<std::vector<uint8_t>>& messages,
                                 const std::vector<G2Point>& public_keys,
                                 const std::vector<uint8_t>& domain_separator = {});
    
    // Threshold signatures
    struct ThresholdShare {
        size_t index;
        G1Point signature_share;
    };
    
    static G1Point CombineThresholdShares(const std::vector<ThresholdShare>& shares,
                                          size_t threshold);
};

/**
 * @brief Zero-knowledge proof utilities
 */
class ZKProof {
public:
    // Proof of possession for BLS keys
    struct ProofOfPossession {
        G1Point proof;
        
        static ProofOfPossession Generate(const std::vector<uint8_t>& private_key,
                                         const G2Point& public_key);
        
        bool Verify(const G2Point& public_key) const;
    };
    
    // Groth16 verifier for BLS12-381
    struct Groth16Proof {
        G1Point a;
        G2Point b;
        G1Point c;
    };
    
    static bool VerifyGroth16(const Groth16Proof& proof,
                              const std::vector<FieldElement>& public_inputs,
                              const std::vector<G1Point>& vk_alpha_g1,
                              const G2Point& vk_beta_g2,
                              const G2Point& vk_gamma_g2,
                              const G2Point& vk_delta_g2);
};

} // namespace neo::cryptography::bls12_381