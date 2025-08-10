// Copyright (C) 2015-2025 The Neo Project.
//
// big_decimal.h file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef NEO_CORE_BIG_DECIMAL_H
#define NEO_CORE_BIG_DECIMAL_H

#include <boost/multiprecision/cpp_int.hpp>
#include <compare>
#include <cstdint>
#include <optional>
#include <string>

namespace neo
{

/**
 * @brief Represents a fixed-point number of arbitrary precision.
 *
 * This class provides a decimal number representation with arbitrary precision
 * using a BigInteger value and a fixed number of decimal places.
 */
class BigDecimal
{
   public:
    using BigInteger = boost::multiprecision::cpp_int;

   private:
    BigInteger value_;
    uint8_t decimals_;

   public:
    /**
     * @brief Default constructor - creates a BigDecimal with value 0 and 0 decimals.
     */
    BigDecimal() : value_(0), decimals_(0) {}

    /**
     * @brief Constructs a BigDecimal with the specified value and decimals.
     * @param value The BigInteger value of the number
     * @param decimals The number of decimal places for this number
     */
    BigDecimal(const BigInteger& value, uint8_t decimals);

    /**
     * @brief Constructs a BigDecimal from a double value.
     * @param value The double value to convert
     */
    explicit BigDecimal(double value);

    /**
     * @brief Constructs a BigDecimal from a double value with specified decimals.
     * @param value The double value to convert
     * @param decimals The number of decimal places for this number
     */
    BigDecimal(double value, uint8_t decimals);

    /**
     * @brief Constructs a BigDecimal from an integer value.
     * @param value The integer value to convert
     * @param decimals The number of decimal places for this number (default: 0)
     */
    template <typename T>
    BigDecimal(T value, uint8_t decimals = 0)
        requires std::is_integral_v<T>
        : value_(value), decimals_(decimals)
    {
    }

    // Accessors
    /**
     * @brief Gets the BigInteger value of the number.
     * @return The BigInteger value
     */
    const BigInteger& value() const { return value_; }

    /**
     * @brief Gets the number of decimal places for this number.
     * @return The number of decimal places
     */
    uint8_t decimals() const { return decimals_; }

    /**
     * @brief Gets the sign of the number.
     * @return 1 if positive, -1 if negative, 0 if zero
     */
    int sign() const;

    // Operations
    /**
     * @brief Changes the number of decimal places for this BigDecimal.
     * @param decimals The new number of decimal places
     * @return A new BigDecimal with the specified decimal places
     * @throws std::invalid_argument if the conversion would lose precision
     */
    BigDecimal change_decimals(uint8_t decimals) const;

    /**
     * @brief Converts the BigDecimal to a string representation.
     * @return String representation of the number
     */
    std::string to_string() const;

    /**
     * @brief Converts the BigDecimal to a double value.
     * @return Double representation of the number
     */
    double ToDouble() const;

    // Static parsing methods
    /**
     * @brief Parses a BigDecimal from a string.
     * @param s The string to parse
     * @param decimals The number of decimal places for the result
     * @return The parsed BigDecimal
     * @throws std::invalid_argument if the string is not in the correct format
     */
    static BigDecimal parse(const std::string& s, uint8_t decimals);

    /**
     * @brief Attempts to parse a BigDecimal from a string.
     * @param s The string to parse
     * @param decimals The number of decimal places for the result
     * @return The parsed BigDecimal if successful, std::nullopt otherwise
     */
    static std::optional<BigDecimal> try_parse(const std::string& s, uint8_t decimals);

    // Arithmetic operators
    BigDecimal operator+(const BigDecimal& other) const;
    BigDecimal operator-(const BigDecimal& other) const;
    BigDecimal operator*(const BigDecimal& other) const;
    BigDecimal operator/(const BigDecimal& other) const;
    BigDecimal operator%(const BigDecimal& other) const;

    BigDecimal& operator+=(const BigDecimal& other);
    BigDecimal& operator-=(const BigDecimal& other);
    BigDecimal& operator*=(const BigDecimal& other);
    BigDecimal& operator/=(const BigDecimal& other);
    BigDecimal& operator%=(const BigDecimal& other);

    BigDecimal operator-() const;

    // Comparison operators
    std::strong_ordering operator<=>(const BigDecimal& other) const;
    bool operator==(const BigDecimal& other) const;

    // Hash support
    std::size_t hash() const;

   private:
    /**
     * @brief Normalizes two BigDecimals to have the same number of decimal places.
     * @param left The first BigDecimal
     * @param right The second BigDecimal
     * @return A pair of BigIntegers with the same scale
     */
    static std::pair<BigInteger, BigInteger> normalize(const BigDecimal& left, const BigDecimal& right);

    /**
     * @brief Calculates 10^exponent using BigInteger.
     * @param exponent The exponent
     * @return 10^exponent as BigInteger
     */
    static BigInteger pow10(int exponent);
};

}  // namespace neo

// Hash specialization for std::hash
namespace std
{
template <>
struct hash<neo::BigDecimal>
{
    std::size_t operator()(const neo::BigDecimal& bd) const { return bd.hash(); }
};
}  // namespace std

#endif  // NEO_CORE_BIG_DECIMAL_H