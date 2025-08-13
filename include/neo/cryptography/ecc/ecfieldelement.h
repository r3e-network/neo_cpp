/**
 * @file ecfieldelement.h
 * @brief Ecfieldelement
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/extensions/biginteger_extensions.h>
#include <neo/io/byte_vector.h>

#include <string>

namespace neo::cryptography::ecc
{

/**
 * @brief Represents an element in an elliptic curve finite field
 *
 * This class implements arithmetic operations for elements in the finite field
 * used by elliptic curve cryptography, specifically for secp256r1 curve.
 */
class ECFieldElement
{
   private:
    extensions::BigIntegerExtensions::BigInteger value_;

    static const extensions::BigIntegerExtensions::BigInteger& GetFieldModulus();

   public:
    /**
     * @brief Construct field element from BigInteger
     * @param value The value (will be reduced modulo field modulus)
     */
    explicit ECFieldElement(const extensions::BigIntegerExtensions::BigInteger& value);

    /**
     * @brief Construct field element from byte array
     * @param bytes Byte representation of the value
     */
    explicit ECFieldElement(const io::ByteVector& bytes);

    /**
     * @brief Get the zero element
     * @return Zero field element
     */
    static ECFieldElement Zero();

    /**
     * @brief Get the one element
     * @return One field element
     */
    static ECFieldElement One();

    /**
     * @brief Add two field elements
     * @param other The other element to add
     * @return Sum of the two elements
     */
    ECFieldElement Add(const ECFieldElement& other) const;

    /**
     * @brief Subtract two field elements
     * @param other The element to subtract
     * @return Difference of the two elements
     */
    ECFieldElement Subtract(const ECFieldElement& other) const;

    /**
     * @brief Multiply two field elements
     * @param other The other element to multiply
     * @return Product of the two elements
     */
    ECFieldElement Multiply(const ECFieldElement& other) const;

    /**
     * @brief Divide two field elements
     * @param other The element to divide by
     * @return Quotient of the two elements
     * @throws std::invalid_argument if dividing by zero
     */
    ECFieldElement Divide(const ECFieldElement& other) const;

    /**
     * @brief Negate the field element
     * @return Negated element
     */
    ECFieldElement Negate() const;

    /**
     * @brief Square the field element
     * @return Squared element
     */
    ECFieldElement Square() const;

    /**
     * @brief Compute modular inverse of the element
     * @return Modular inverse
     * @throws std::invalid_argument if element is zero
     * @throws std::runtime_error if inverse doesn't exist
     */
    ECFieldElement ModularInverse() const;

    /**
     * @brief Compute square root of the element
     * @return Square root in the field
     */
    ECFieldElement Sqrt() const;

    /**
     * @brief Check if element is zero
     * @return true if element is zero
     */
    bool IsZero() const;

    /**
     * @brief Check if element is one
     * @return true if element is one
     */
    bool IsOne() const;

    /**
     * @brief Equality comparison
     * @param other The other element to compare
     * @return true if elements are equal
     */
    bool operator==(const ECFieldElement& other) const;

    /**
     * @brief Inequality comparison
     * @param other The other element to compare
     * @return true if elements are not equal
     */
    bool operator!=(const ECFieldElement& other) const;

    /**
     * @brief Get the underlying value
     * @return Reference to the BigInteger value
     */
    const extensions::BigIntegerExtensions::BigInteger& GetValue() const;

    /**
     * @brief Convert to byte array
     * @return Byte representation of the element
     */
    io::ByteVector ToByteArray() const;

    /**
     * @brief Convert to string representation
     * @return String representation of the element
     */
    std::string ToString() const;
};

}  // namespace neo::cryptography::ecc