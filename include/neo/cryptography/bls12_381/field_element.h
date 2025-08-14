/**
 * @file field_element.h
 * @brief BLS12-381 field element operations
 */

#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace neo::cryptography::bls12_381 {

/**
 * @brief Finite field element for BLS12-381
 */
class FieldElement {
public:
    static constexpr size_t SIZE = 48;
    std::array<uint8_t, SIZE> data;
    
    // Field modulus
    static const FieldElement& Modulus();
    
    // Constructors
    FieldElement();
    explicit FieldElement(const std::array<uint8_t, SIZE>& bytes);
    explicit FieldElement(uint64_t value);
    
    // Arithmetic operations
    FieldElement Add(const FieldElement& other) const;
    FieldElement Subtract(const FieldElement& other) const;
    FieldElement Multiply(const FieldElement& other) const;
    FieldElement Negate() const;
    FieldElement Invert() const;
    FieldElement Square() const;
    FieldElement Power(const std::vector<uint8_t>& exponent) const;
    
    // Comparison
    bool Equals(const FieldElement& other) const;
    bool IsZero() const;
    bool IsOne() const;
    
    // Modular operations
    void ModReduce();
    
    // Conversion
    std::vector<uint8_t> ToBytes() const;
    static FieldElement FromBytes(const std::vector<uint8_t>& bytes);
    
    // Operators
    bool operator==(const FieldElement& other) const { return Equals(other); }
    bool operator!=(const FieldElement& other) const { return !Equals(other); }
};

/**
 * @brief Extended field element (Fp2) for BLS12-381
 */
class FieldElement2 {
public:
    FieldElement c0;  // Real part
    FieldElement c1;  // Imaginary part
    
    // Constructors
    FieldElement2();
    FieldElement2(const FieldElement& real, const FieldElement& imag);
    
    // Arithmetic operations
    FieldElement2 Add(const FieldElement2& other) const;
    FieldElement2 Subtract(const FieldElement2& other) const;
    FieldElement2 Multiply(const FieldElement2& other) const;
    FieldElement2 Square() const;
    FieldElement2 Invert() const;
    FieldElement2 Negate() const;
    FieldElement2 Conjugate() const;
    
    // Special operations
    FieldElement2 FrobeniusMap(unsigned int power) const;
    FieldElement2 MultiplyByNonresidue() const;
    
    // Comparison
    bool IsZero() const;
    bool IsOne() const;
    bool Equals(const FieldElement2& other) const;
};

/**
 * @brief Field element Fp12 for pairing operations
 */
class FieldElement12 {
public:
    std::array<FieldElement2, 6> coefficients;
    
    // Constructors
    FieldElement12();
    explicit FieldElement12(const FieldElement& value);
    
    // Arithmetic operations
    FieldElement12 Add(const FieldElement12& other) const;
    FieldElement12 Subtract(const FieldElement12& other) const;
    FieldElement12 Multiply(const FieldElement12& other) const;
    FieldElement12 Square() const;
    FieldElement12 Invert() const;
    FieldElement12 Conjugate() const;
    
    // Special operations for pairing
    FieldElement12 FrobeniusMap(unsigned int power) const;
    FieldElement12 CyclotomicSquare() const;
    FieldElement12 CyclotomicExponentiation(const std::vector<uint8_t>& exponent) const;
    
    // Final exponentiation for pairing
    FieldElement12 FinalExponentiation() const;
    
    // Comparison
    bool IsZero() const;
    bool IsOne() const;
    bool Equals(const FieldElement12& other) const;
};

} // namespace neo::cryptography::bls12_381