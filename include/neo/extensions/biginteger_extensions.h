/**
 * @file biginteger_extensions.h
 * @brief Biginteger Extensions
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace neo::extensions
{
/**
 * @brief Extensions for BigInteger operations.
 *
 * ## Overview
 * Provides utilities for arbitrary precision integer arithmetic,
 * conversions, and mathematical operations needed for blockchain operations.
 *
 * ## API Reference
 * - **Arithmetic**: Add, subtract, multiply, divide operations
 * - **Conversion**: To/from strings, byte arrays, standard integers
 * - **Utilities**: Comparison, bit operations, modular arithmetic
 *
 * ## Usage Examples
 * ```cpp
 * // Parse from string
 * auto big = BigIntegerExtensions::FromString("123456789012345678901234567890");
 *
 * // Convert to hex
 * std::string hex = BigIntegerExtensions::ToHexString(big);
 *
 * // Arithmetic operations
 * auto result = BigIntegerExtensions::Add(big1, big2);
 * ```
 *
 * ## Design Notes
 * - Uses vector<uint64_t> for internal representation
 * - Little-endian storage (least significant digit first)
 * - Supports both positive and negative numbers
 */
class BigIntegerExtensions
{
   public:
    /**
     * @brief BigInteger representation using vector of 64-bit words
     */
    struct BigInteger
    {
        std::vector<uint64_t> words;  ///< Little-endian words
        bool isNegative;              ///< Sign flag

        BigInteger() : isNegative(false) {}
        explicit BigInteger(int64_t value);
        explicit BigInteger(uint64_t value);
        explicit BigInteger(const std::string& value);

        // Basic operators
        BigInteger operator+(const BigInteger& other) const;
        BigInteger operator-(const BigInteger& other) const;
        BigInteger operator*(const BigInteger& other) const;
        BigInteger operator/(const BigInteger& other) const;
        BigInteger operator%(const BigInteger& other) const;

        bool operator==(const BigInteger& other) const;
        bool operator!=(const BigInteger& other) const;
        bool operator<(const BigInteger& other) const;
        bool operator>(const BigInteger& other) const;
        bool operator<=(const BigInteger& other) const;
        bool operator>=(const BigInteger& other) const;

        BigInteger& operator+=(const BigInteger& other);
        BigInteger& operator-=(const BigInteger& other);
        BigInteger& operator*=(const BigInteger& other);
        BigInteger& operator/=(const BigInteger& other);
        BigInteger& operator%=(const BigInteger& other);

        // Conversion methods
        std::string ToString() const;
        std::string ToHexString() const;
        std::vector<uint8_t> ToByteArray() const;
        int64_t ToInt64() const;
        uint64_t ToUInt64() const;

        // Utility methods
        bool IsZero() const;
        bool IsOne() const;
        bool IsEven() const;
        bool IsOdd() const;
        BigInteger Abs() const;
        BigInteger Negate() const;

       private:
        void Normalize();
    };

    /**
     * @brief Create BigInteger from decimal string
     * @param value Decimal string representation
     * @return BigInteger instance
     */
    static BigInteger FromString(const std::string& value);

    /**
     * @brief Create BigInteger from hexadecimal string
     * @param hex Hexadecimal string (with or without 0x prefix)
     * @return BigInteger instance
     */
    static BigInteger FromHexString(const std::string& hex);

    /**
     * @brief Create BigInteger from byte array
     * @param bytes Byte array (big-endian)
     * @param isNegative Whether the number is negative
     * @return BigInteger instance
     */
    static BigInteger FromByteArray(const std::vector<uint8_t>& bytes, bool isNegative = false);

    /**
     * @brief Create BigInteger from 64-bit integer
     * @param value Integer value
     * @return BigInteger instance
     */
    static BigInteger FromInt64(int64_t value);

    /**
     * @brief Create BigInteger from 64-bit unsigned integer
     * @param value Unsigned integer value
     * @return BigInteger instance
     */
    static BigInteger FromUInt64(uint64_t value);

    /**
     * @brief Convert BigInteger to decimal string
     * @param value BigInteger to convert
     * @return Decimal string representation
     */
    static std::string ToString(const BigInteger& value);

    /**
     * @brief Convert BigInteger to hexadecimal string
     * @param value BigInteger to convert
     * @param prefix Whether to include "0x" prefix
     * @return Hexadecimal string representation
     */
    static std::string ToHexString(const BigInteger& value, bool prefix = false);

    /**
     * @brief Convert BigInteger to byte array
     * @param value BigInteger to convert
     * @return Byte array (big-endian)
     */
    static std::vector<uint8_t> ToByteArray(const BigInteger& value);

    /**
     * @brief Add two BigIntegers
     * @param left First operand
     * @param right Second operand
     * @return Sum result
     */
    static BigInteger Add(const BigInteger& left, const BigInteger& right);

    /**
     * @brief Subtract two BigIntegers
     * @param left First operand
     * @param right Second operand
     * @return Difference result
     */
    static BigInteger Subtract(const BigInteger& left, const BigInteger& right);

    /**
     * @brief Multiply two BigIntegers
     * @param left First operand
     * @param right Second operand
     * @return Product result
     */
    static BigInteger Multiply(const BigInteger& left, const BigInteger& right);

    /**
     * @brief Divide two BigIntegers
     * @param dividend Dividend
     * @param divisor Divisor
     * @return Quotient result
     */
    static BigInteger Divide(const BigInteger& dividend, const BigInteger& divisor);

    /**
     * @brief Modulo operation
     * @param dividend Dividend
     * @param divisor Divisor
     * @return Remainder result
     */
    static BigInteger Modulo(const BigInteger& dividend, const BigInteger& divisor);

    /**
     * @brief Power operation
     * @param baseValue Base value
     * @param exponent Exponent
     * @return Result of base^exponent
     */
    static BigInteger Power(const BigInteger& baseValue, uint32_t exponent);

    /**
     * @brief Modular power operation (base^exponent mod modulus)
     * @param baseValue Base value
     * @param exponent Exponent
     * @param modulus Modulus
     * @return Result of (base^exponent) mod modulus
     */
    static BigInteger ModularPower(const BigInteger& baseValue, const BigInteger& exponent, const BigInteger& modulus);

    /**
     * @brief Greatest Common Divisor
     * @param left First number
     * @param right Second number
     * @return GCD of left and right
     */
    static BigInteger GreatestCommonDivisor(const BigInteger& left, const BigInteger& right);

    /**
     * @brief Compare two BigIntegers
     * @param left First operand
     * @param right Second operand
     * @return -1 if left < right, 0 if equal, 1 if left > right
     */
    static int Compare(const BigInteger& left, const BigInteger& right);

    /**
     * @brief Get absolute value
     * @param value BigInteger value
     * @return Absolute value
     */
    static BigInteger Abs(const BigInteger& value);

    /**
     * @brief Negate BigInteger
     * @param value BigInteger value
     * @return Negated value
     */
    static BigInteger Negate(const BigInteger& value);

    /**
     * @brief Check if BigInteger is zero
     * @param value BigInteger value
     * @return True if zero
     */
    static bool IsZero(const BigInteger& value);

    /**
     * @brief Check if BigInteger is one
     * @param value BigInteger value
     * @return True if one
     */
    static bool IsOne(const BigInteger& value);

    /**
     * @brief Check if BigInteger is even
     * @param value BigInteger value
     * @return True if even
     */
    static bool IsEven(const BigInteger& value);

    /**
     * @brief Bit shift left
     * @param value BigInteger value
     * @param shift Number of bits to shift
     * @return Shifted result
     */
    static BigInteger LeftShift(const BigInteger& value, uint32_t shift);

    /**
     * @brief Bit shift right
     * @param value BigInteger value
     * @param shift Number of bits to shift
     * @return Shifted result
     */
    static BigInteger RightShift(const BigInteger& value, uint32_t shift);

    // Constants
    static const BigInteger Zero;
    static const BigInteger One;
    static const BigInteger MinusOne;
};

// Stream operators
std::ostream& operator<<(std::ostream& os, const BigIntegerExtensions::BigInteger& value);
std::istream& operator>>(std::istream& is, BigIntegerExtensions::BigInteger& value);
}  // namespace neo::extensions
