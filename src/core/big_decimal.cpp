// Copyright (C) 2015-2025 The Neo Project.
//
// big_decimal.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#include "neo/core/big_decimal.h"
#include <algorithm>
#include <cmath>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace neo
{

BigDecimal::BigDecimal(const BigInteger& value, uint8_t decimals) : value_(value), decimals_(decimals) {}

BigDecimal::BigDecimal(double value)
{
    // Convert double to string to preserve precision
    std::ostringstream oss;
    oss.precision(15);
    oss << std::fixed << value;
    std::string str = oss.str();

    // Remove trailing zeros
    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
    if (str.back() == '.')
    {
        str.pop_back();
    }

    // Find decimal point
    size_t decimal_pos = str.find('.');
    if (decimal_pos == std::string::npos)
    {
        value_ = BigInteger(str);
        decimals_ = 0;
    }
    else
    {
        decimals_ = static_cast<uint8_t>(str.length() - decimal_pos - 1);
        str.erase(decimal_pos, 1);
        value_ = BigInteger(str);
    }
}

BigDecimal::BigDecimal(double value, uint8_t decimals)
{
    // Scale the value by 10^decimals
    double scaled = value * std::pow(10.0, decimals);
    value_ = BigInteger(static_cast<long long>(std::round(scaled)));
    decimals_ = decimals;
}

int BigDecimal::sign() const
{
    if (value_ > 0)
        return 1;
    if (value_ < 0)
        return -1;
    return 0;
}

BigDecimal BigDecimal::change_decimals(uint8_t decimals) const
{
    if (decimals_ == decimals)
    {
        return *this;
    }

    BigInteger new_value;
    if (decimals_ < decimals)
    {
        // Increase decimal places - multiply by 10^(decimals - decimals_)
        new_value = value_ * pow10(decimals - decimals_);
    }
    else
    {
        // Decrease decimal places - divide by 10^(decimals_ - decimals)
        BigInteger divisor = pow10(decimals_ - decimals);
        BigInteger remainder;
        boost::multiprecision::divide_qr(value_, divisor, new_value, remainder);

        if (remainder != 0)
        {
            throw std::invalid_argument("Cannot change decimals without losing precision");
        }
    }

    return BigDecimal(new_value, decimals);
}

std::string BigDecimal::to_string() const
{
    if (decimals_ == 0)
    {
        return value_.str();
    }

    BigInteger divisor = pow10(decimals_);
    BigInteger quotient, remainder;
    boost::multiprecision::divide_qr(value_, divisor, quotient, remainder);

    if (remainder == 0)
    {
        return quotient.str();
    }

    // Handle negative numbers
    std::string result;
    if (value_ < 0 && quotient == 0)
    {
        result = "-0";
    }
    else
    {
        result = quotient.str();
    }

    result += ".";

    // Convert remainder to string with leading zeros
    BigInteger abs_remainder = boost::multiprecision::abs(remainder);
    std::string remainder_str = abs_remainder.str();

    // Pad with leading zeros if necessary
    while (remainder_str.length() < decimals_)
    {
        remainder_str = "0" + remainder_str;
    }

    result += remainder_str;

    // Remove trailing zeros
    while (result.back() == '0' && result[result.length() - 2] != '.')
    {
        result.pop_back();
    }

    return result;
}

double BigDecimal::ToDouble() const
{
    if (decimals_ == 0)
    {
        return static_cast<double>(value_);
    }

    // Convert to double by dividing by 10^decimals
    double divisor = std::pow(10.0, decimals_);
    return static_cast<double>(value_) / divisor;
}

BigDecimal BigDecimal::parse(const std::string& s, uint8_t decimals)
{
    auto result = try_parse(s, decimals);
    if (!result)
    {
        throw std::invalid_argument("Invalid BigDecimal format: " + s);
    }
    return *result;
}

std::optional<BigDecimal> BigDecimal::try_parse(const std::string& s, uint8_t decimals)
{
    if (s.empty())
    {
        return std::nullopt;
    }

    std::string input = s;
    int exponent = 0;

    // Handle scientific notation
    std::regex sci_regex(R"(^(.+)[eE]([-+]?\d+)$)");
    std::smatch match;
    if (std::regex_match(input, match, sci_regex))
    {
        try
        {
            exponent = std::stoi(match[2].str());
            input = match[1].str();
        }
        catch (const std::invalid_argument&)
        {
            return std::nullopt;
        }
        catch (const std::out_of_range&)
        {
            return std::nullopt;
        }
    }

    // Find decimal point
    size_t decimal_pos = input.find('.');
    if (decimal_pos != std::string::npos)
    {
        // Remove trailing zeros after decimal point
        size_t end = input.find_last_not_of('0');
        if (end != std::string::npos && end > decimal_pos)
        {
            input = input.substr(0, end + 1);
        }

        // Calculate effective exponent
        exponent -= static_cast<int>(input.length() - decimal_pos - 1);

        // Remove decimal point
        input.erase(decimal_pos, 1);
    }

    // Calculate final decimal places
    int final_decimals = exponent + static_cast<int>(decimals);
    if (final_decimals < 0)
    {
        return std::nullopt;
    }

    // Add zeros if needed
    if (final_decimals > 0)
    {
        input += std::string(final_decimals, '0');
    }

    // Parse the BigInteger
    try
    {
        BigInteger value(input);
        return BigDecimal(value, decimals);
    }
    catch (const std::invalid_argument&)
    {
        return std::nullopt;
    }
    catch (const std::runtime_error&)
    {
        return std::nullopt;
    }
}

std::pair<BigDecimal::BigInteger, BigDecimal::BigInteger> BigDecimal::normalize(const BigDecimal& left,
                                                                                const BigDecimal& right)
{
    BigInteger left_value = left.value_;
    BigInteger right_value = right.value_;

    if (left.decimals_ < right.decimals_)
    {
        left_value *= pow10(right.decimals_ - left.decimals_);
    }
    else if (left.decimals_ > right.decimals_)
    {
        right_value *= pow10(left.decimals_ - right.decimals_);
    }

    return {left_value, right_value};
}

BigDecimal::BigInteger BigDecimal::pow10(int exponent)
{
    if (exponent < 0)
    {
        throw std::invalid_argument("Negative exponent not supported");
    }

    BigInteger result = 1;
    BigInteger base = 10;

    while (exponent > 0)
    {
        if (exponent & 1)
        {
            result *= base;
        }
        base *= base;
        exponent >>= 1;
    }

    return result;
}

// Arithmetic operators
BigDecimal BigDecimal::operator+(const BigDecimal& other) const
{
    auto [left_val, right_val] = normalize(*this, other);
    uint8_t result_decimals = std::max(decimals_, other.decimals_);
    return BigDecimal(left_val + right_val, result_decimals);
}

BigDecimal BigDecimal::operator-(const BigDecimal& other) const
{
    auto [left_val, right_val] = normalize(*this, other);
    uint8_t result_decimals = std::max(decimals_, other.decimals_);
    return BigDecimal(left_val - right_val, result_decimals);
}

BigDecimal BigDecimal::operator*(const BigDecimal& other) const
{
    BigInteger result_value = value_ * other.value_;
    uint8_t result_decimals = decimals_ + other.decimals_;
    return BigDecimal(result_value, result_decimals);
}

BigDecimal BigDecimal::operator/(const BigDecimal& other) const
{
    if (other.value_ == 0)
    {
        throw std::runtime_error("Division by zero");
    }

    // To maintain precision, we scale the dividend
    uint8_t result_decimals = std::max(decimals_, other.decimals_);
    BigInteger scaled_dividend = value_ * pow10(result_decimals);
    BigInteger result_value = scaled_dividend / other.value_;

    return BigDecimal(result_value, result_decimals);
}

BigDecimal BigDecimal::operator%(const BigDecimal& other) const
{
    if (other.value_ == 0)
    {
        throw std::runtime_error("Division by zero");
    }

    auto [left_val, right_val] = normalize(*this, other);
    uint8_t result_decimals = std::max(decimals_, other.decimals_);
    return BigDecimal(left_val % right_val, result_decimals);
}

BigDecimal& BigDecimal::operator+=(const BigDecimal& other)
{
    *this = *this + other;
    return *this;
}

BigDecimal& BigDecimal::operator-=(const BigDecimal& other)
{
    *this = *this - other;
    return *this;
}

BigDecimal& BigDecimal::operator*=(const BigDecimal& other)
{
    *this = *this * other;
    return *this;
}

BigDecimal& BigDecimal::operator/=(const BigDecimal& other)
{
    *this = *this / other;
    return *this;
}

BigDecimal& BigDecimal::operator%=(const BigDecimal& other)
{
    *this = *this % other;
    return *this;
}

BigDecimal BigDecimal::operator-() const
{
    return BigDecimal(-value_, decimals_);
}

// Comparison operators
std::strong_ordering BigDecimal::operator<=>(const BigDecimal& other) const
{
    auto [left_val, right_val] = normalize(*this, other);

    if (left_val < right_val)
        return std::strong_ordering::less;
    if (left_val > right_val)
        return std::strong_ordering::greater;
    return std::strong_ordering::equal;
}

bool BigDecimal::operator==(const BigDecimal& other) const
{
    return (*this <=> other) == std::strong_ordering::equal;
}

std::size_t BigDecimal::hash() const
{
    // Normalize to remove trailing zeros for consistent hashing
    BigInteger divisor = pow10(decimals_);
    BigInteger quotient, remainder;
    boost::multiprecision::divide_qr(value_, divisor, quotient, remainder);

    std::hash<std::string> hasher;
    return hasher(quotient.str()) ^ (hasher(remainder.str()) << 1);
}

}  // namespace neo