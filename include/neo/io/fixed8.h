#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>
#include <cmath>

namespace neo::io
{
    /**
     * @brief Represents a fixed-point decimal with 8 decimal places.
     */
    class Fixed8
    {
    public:
        /**
         * @brief The number of decimal places.
         */
        static constexpr int64_t Decimals = 100000000;

        /**
         * @brief Constructs a Fixed8 initialized to zero.
         */
        Fixed8() : value_(0) {}

        /**
         * @brief Constructs a Fixed8 from a raw value.
         * @param value The raw value.
         */
        explicit Fixed8(int64_t value) : value_(value) {}

        /**
         * @brief Gets the raw value.
         * @return The raw value.
         */
        int64_t Value() const noexcept { return value_; }

        /**
         * @brief Converts the Fixed8 to a double.
         * @return The double representation of the Fixed8.
         */
        double ToDouble() const { return static_cast<double>(value_) / Decimals; }

        /**
         * @brief Converts the Fixed8 to a string.
         * @return The string representation of the Fixed8.
         */
        std::string ToString() const;

        /**
         * @brief Adds another Fixed8 to this Fixed8.
         * @param other The other Fixed8.
         * @return The result of the addition.
         * @throws std::overflow_error if the addition would overflow.
         */
        Fixed8 operator+(const Fixed8& other) const
        {
            int64_t result = value_ + other.value_;
            if ((value_ > 0 && other.value_ > 0 && result < 0) ||
                (value_ < 0 && other.value_ < 0 && result > 0))
                throw std::overflow_error("Fixed8 addition overflow");
            return Fixed8(result);
        }

        /**
         * @brief Subtracts another Fixed8 from this Fixed8.
         * @param other The other Fixed8.
         * @return The result of the subtraction.
         * @throws std::overflow_error if the subtraction would overflow.
         */
        Fixed8 operator-(const Fixed8& other) const
        {
            int64_t result = value_ - other.value_;
            if ((value_ > 0 && other.value_ < 0 && result < 0) ||
                (value_ < 0 && other.value_ > 0 && result > 0))
                throw std::overflow_error("Fixed8 subtraction overflow");
            return Fixed8(result);
        }

        /**
         * @brief Multiplies this Fixed8 by another Fixed8.
         * @param other The other Fixed8.
         * @return The result of the multiplication.
         * @throws std::overflow_error if the multiplication would overflow.
         */
        Fixed8 operator*(const Fixed8& other) const
        {
            // Check for potential overflow before multiplication
            if (value_ != 0 && other.value_ != 0)
            {
                if (value_ > INT64_MAX / std::abs(other.value_) ||
                    value_ < INT64_MIN / std::abs(other.value_))
                {
                    throw std::overflow_error("Fixed8 multiplication overflow");
                }
            }

            // Perform the multiplication
            int64_t result = (value_ * other.value_) / Decimals;
            return Fixed8(result);
        }

        /**
         * @brief Divides this Fixed8 by another Fixed8.
         * @param other The other Fixed8.
         * @return The result of the division.
         * @throws std::overflow_error if the division would overflow.
         * @throws std::invalid_argument if the divisor is zero.
         */
        Fixed8 operator/(const Fixed8& other) const
        {
            if (other.value_ == 0)
                throw std::invalid_argument("Division by zero");

            // Check for potential overflow before division
            if (value_ != 0)
            {
                if (Decimals > INT64_MAX / std::abs(value_))
                {
                    throw std::overflow_error("Fixed8 division overflow");
                }
            }

            // Perform the division
            int64_t result = (value_ * Decimals) / other.value_;
            return Fixed8(result);
        }

        /**
         * @brief Checks if this Fixed8 is equal to another Fixed8.
         * @param other The other Fixed8.
         * @return True if the Fixed8s are equal, false otherwise.
         */
        bool operator==(const Fixed8& other) const { return value_ == other.value_; }

        /**
         * @brief Checks if this Fixed8 is not equal to another Fixed8.
         * @param other The other Fixed8.
         * @return True if the Fixed8s are not equal, false otherwise.
         */
        bool operator!=(const Fixed8& other) const { return value_ != other.value_; }

        /**
         * @brief Checks if this Fixed8 is less than another Fixed8.
         * @param other The other Fixed8.
         * @return True if this Fixed8 is less than the other Fixed8, false otherwise.
         */
        bool operator<(const Fixed8& other) const { return value_ < other.value_; }

        /**
         * @brief Checks if this Fixed8 is less than or equal to another Fixed8.
         * @param other The other Fixed8.
         * @return True if this Fixed8 is less than or equal to the other Fixed8, false otherwise.
         */
        bool operator<=(const Fixed8& other) const { return value_ <= other.value_; }

        /**
         * @brief Checks if this Fixed8 is greater than another Fixed8.
         * @param other The other Fixed8.
         * @return True if this Fixed8 is greater than the other Fixed8, false otherwise.
         */
        bool operator>(const Fixed8& other) const { return value_ > other.value_; }

        /**
         * @brief Checks if this Fixed8 is greater than or equal to another Fixed8.
         * @param other The other Fixed8.
         * @return True if this Fixed8 is greater than or equal to the other Fixed8, false otherwise.
         */
        bool operator>=(const Fixed8& other) const { return value_ >= other.value_; }

        /**
         * @brief Adds another Fixed8 to this Fixed8 and assigns the result to this Fixed8.
         * @param other The other Fixed8.
         * @return A reference to this Fixed8.
         * @throws std::overflow_error if the addition would overflow.
         */
        Fixed8& operator+=(const Fixed8& other)
        {
            int64_t result = value_ + other.value_;
            if ((value_ > 0 && other.value_ > 0 && result < 0) ||
                (value_ < 0 && other.value_ < 0 && result > 0))
                throw std::overflow_error("Fixed8 addition overflow");
            value_ = result;
            return *this;
        }

        /**
         * @brief Subtracts another Fixed8 from this Fixed8 and assigns the result to this Fixed8.
         * @param other The other Fixed8.
         * @return A reference to this Fixed8.
         * @throws std::overflow_error if the subtraction would overflow.
         */
        Fixed8& operator-=(const Fixed8& other)
        {
            int64_t result = value_ - other.value_;
            if ((value_ > 0 && other.value_ < 0 && result < 0) ||
                (value_ < 0 && other.value_ > 0 && result > 0))
                throw std::overflow_error("Fixed8 subtraction overflow");
            value_ = result;
            return *this;
        }

        /**
         * @brief Multiplies this Fixed8 by another Fixed8 and assigns the result to this Fixed8.
         * @param other The other Fixed8.
         * @return A reference to this Fixed8.
         * @throws std::overflow_error if the multiplication would overflow.
         */
        Fixed8& operator*=(const Fixed8& other)
        {
            // Check for potential overflow before multiplication
            if (value_ != 0 && other.value_ != 0)
            {
                if (value_ > INT64_MAX / std::abs(other.value_) ||
                    value_ < INT64_MIN / std::abs(other.value_))
                {
                    throw std::overflow_error("Fixed8 multiplication overflow");
                }
            }

            // Perform the multiplication
            value_ = (value_ * other.value_) / Decimals;
            return *this;
        }

        /**
         * @brief Divides this Fixed8 by another Fixed8 and assigns the result to this Fixed8.
         * @param other The other Fixed8.
         * @return A reference to this Fixed8.
         * @throws std::overflow_error if the division would overflow.
         * @throws std::invalid_argument if the divisor is zero.
         */
        Fixed8& operator/=(const Fixed8& other)
        {
            if (other.value_ == 0)
                throw std::invalid_argument("Division by zero");

            // Check for potential overflow before division
            if (value_ != 0)
            {
                if (Decimals > INT64_MAX / std::abs(value_))
                {
                    throw std::overflow_error("Fixed8 division overflow");
                }
            }

            // Perform the division
            value_ = (value_ * Decimals) / other.value_;
            return *this;
        }

        /**
         * @brief Creates a Fixed8 from a decimal value.
         * @param value The decimal value.
         * @return The Fixed8 representation of the decimal value.
         * @throws std::overflow_error if the conversion would overflow.
         */
        static Fixed8 FromDecimal(double value)
        {
            double result = value * Decimals;
            if (result > INT64_MAX || result < INT64_MIN)
                throw std::overflow_error("Fixed8 conversion overflow");
            return Fixed8(static_cast<int64_t>(result));
        }

        /**
         * @brief Creates a Fixed8 from a double value.
         * @param value The double value.
         * @return The Fixed8 representation of the double value.
         * @throws std::overflow_error if the conversion would overflow.
         */
        static Fixed8 FromDouble(double value)
        {
            return FromDecimal(value);
        }

        /**
         * @brief Parses a string into a Fixed8.
         * @param s The string to parse.
         * @return The parsed Fixed8.
         * @throws std::invalid_argument if the string is not a valid Fixed8.
         */
        static Fixed8 Parse(const std::string& s)
        {
            try
            {
                size_t pos = 0;
                double value = std::stod(s, &pos);
                if (pos != s.length())
                    throw std::invalid_argument("Invalid Fixed8 format");
                return FromDecimal(value);
            }
            catch (const std::exception& ex)
            {
                throw std::invalid_argument("Invalid Fixed8 format: " + std::string(ex.what()));
            }
        }

        /**
         * @brief Gets a Fixed8 with value zero.
         * @return A Fixed8 with value zero.
         */
        static Fixed8 Zero() { return Fixed8(); }

        /**
         * @brief Gets a Fixed8 with value one.
         * @return A Fixed8 with value one.
         */
        static Fixed8 One() { return Fixed8(Decimals); }

        /**
         * @brief Gets the maximum value of Fixed8.
         * @return The maximum value of Fixed8.
         */
        static Fixed8 MaxValue() { return Fixed8(INT64_MAX); }

        /**
         * @brief Gets the minimum value of Fixed8.
         * @return The minimum value of Fixed8.
         */
        static Fixed8 MinValue() { return Fixed8(INT64_MIN); }

    private:
        int64_t value_;
    };
}
