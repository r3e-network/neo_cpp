#pragma once

#include <cstdint>
#include <string>
#include <limits>
#include <stdexcept>

// Forward declaration
namespace neo::core
{
    class BigDecimal;
}

namespace neo::core
{
    /**
     * @brief Fixed8 compatibility class for Neo 2.x tests.
     * This uses a simple int64_t representation with 8 decimal places.
     */
    class Fixed8
    {
    private:
        int64_t value_; // Value stored as int64_t with 8 decimal places (e.g., 1.0 = 100000000)
        static constexpr int64_t SCALE_FACTOR = 100000000; // 10^8

    public:
        // Constants
        static constexpr int64_t Decimals = SCALE_FACTOR;
        /**
         * @brief Constructs a Fixed8 with value 0.
         */
        Fixed8() : value_(0) {}

        /**
         * @brief Constructs a Fixed8 with the specified raw int value.
         * @param value The raw internal value (not scaled).
         */
        explicit Fixed8(int value) : value_(static_cast<int64_t>(value)) {}

        /**
         * @brief Constructs a Fixed8 with the specified raw int64_t value.
         * @param value The raw internal value (not scaled).
         */
        explicit Fixed8(int64_t value) : value_(value) {}

        /**
         * @brief Constructs a Fixed8 with the specified double value.
         * @param value The double value (will be scaled).
         */
        explicit Fixed8(double value) : value_(static_cast<int64_t>(value * SCALE_FACTOR)) {}

        /**
         * @brief Constructs a Fixed8 from a BigDecimal.
         * @param value The BigDecimal value.
         */
        explicit Fixed8(const BigDecimal& value);

        /**
         * @brief Gets the raw int64_t value.
         * @return The raw value.
         */
        int64_t GetRawValue() const { return value_; }

        /**
         * @brief Converts to int64_t.
         * @return The int64_t value.
         */
        int64_t ToInt64() const { return value_ / SCALE_FACTOR; }

        /**
         * @brief Converts to double.
         * @return The double value.
         */
        double ToDouble() const { return static_cast<double>(value_) / SCALE_FACTOR; }

        /**
         * @brief Gets the underlying value (compatibility method).
         * @return The raw value
         */
        int64_t Value() const { return value_; }

        /**
         * @brief Converts the Fixed8 to a string representation.
         * @return String representation of the number
         */
        std::string ToString() const;

        /**
         * @brief Gets a Fixed8 representing zero.
         * @return Fixed8 with value 0
         */
        static Fixed8 Zero() { return Fixed8(); }

        /**
         * @brief Gets a Fixed8 representing one.
         * @return Fixed8 with value 1
         */
        static Fixed8 One() {
            Fixed8 result;
            result.value_ = SCALE_FACTOR;
            return result;
        }

        /**
         * @brief Gets the maximum Fixed8 value.
         * @return Maximum Fixed8 value
         */
        static Fixed8 MaxValue() {
            Fixed8 result;
            result.value_ = std::numeric_limits<int64_t>::max();
            return result;
        }

        /**
         * @brief Gets the minimum Fixed8 value.
         * @return Minimum Fixed8 value
         */
        static Fixed8 MinValue() {
            Fixed8 result;
            result.value_ = std::numeric_limits<int64_t>::min();
            return result;
        }

        /**
         * @brief Creates a Fixed8 from a double value.
         * @param value The double value
         * @return Fixed8 representation
         */
        static Fixed8 FromDouble(double value) { return Fixed8(value); }

        /**
         * @brief Creates a Fixed8 from a decimal value.
         * @param value The decimal value
         * @return Fixed8 representation
         * @throws std::overflow_error if value is too large
         */
        static Fixed8 FromDecimal(double value) {
            if (value > static_cast<double>(std::numeric_limits<int64_t>::max()) / SCALE_FACTOR ||
                value < static_cast<double>(std::numeric_limits<int64_t>::min()) / SCALE_FACTOR) {
                throw std::overflow_error("Value too large for Fixed8");
            }
            // Use rounding to handle floating point precision issues
            Fixed8 result;
            result.value_ = static_cast<int64_t>(value * SCALE_FACTOR + (value >= 0 ? 0.5 : -0.5));
            return result;
        }

        /**
         * @brief Parses a Fixed8 from a string.
         * @param str The string to parse
         * @return Fixed8 representation
         */
        static Fixed8 Parse(const std::string& str);

        /**
         * @brief Equality operator.
         * @param other The other Fixed8.
         * @return True if equal.
         */
        bool operator==(const Fixed8& other) const { return value_ == other.value_; }

        /**
         * @brief Inequality operator.
         * @param other The other Fixed8.
         * @return True if not equal.
         */
        bool operator!=(const Fixed8& other) const { return value_ != other.value_; }

        /**
         * @brief Less than operator.
         * @param other The other Fixed8.
         * @return True if less than.
         */
        bool operator<(const Fixed8& other) const { return value_ < other.value_; }

        /**
         * @brief Greater than operator.
         * @param other The other Fixed8.
         * @return True if greater than.
         */
        bool operator>(const Fixed8& other) const { return value_ > other.value_; }

        /**
         * @brief Less than or equal operator.
         * @param other The other Fixed8.
         * @return True if less than or equal.
         */
        bool operator<=(const Fixed8& other) const { return value_ <= other.value_; }

        /**
         * @brief Greater than or equal operator.
         * @param other The other Fixed8.
         * @return True if greater than or equal.
         */
        bool operator>=(const Fixed8& other) const { return value_ >= other.value_; }

        /**
         * @brief Addition operator.
         * @param other The other Fixed8.
         * @return The sum.
         * @throws std::overflow_error if overflow occurs.
         */
        Fixed8 operator+(const Fixed8& other) const {
            Fixed8 result;
            // Check for overflow
            if ((other.value_ > 0 && value_ > std::numeric_limits<int64_t>::max() - other.value_) ||
                (other.value_ < 0 && value_ < std::numeric_limits<int64_t>::min() - other.value_)) {
                throw std::overflow_error("Fixed8 addition overflow");
            }
            result.value_ = value_ + other.value_;
            return result;
        }

        /**
         * @brief Subtraction operator.
         * @param other The other Fixed8.
         * @return The difference.
         * @throws std::overflow_error if overflow occurs.
         */
        Fixed8 operator-(const Fixed8& other) const {
            Fixed8 result;
            // Check for overflow
            if ((other.value_ < 0 && value_ > std::numeric_limits<int64_t>::max() + other.value_) ||
                (other.value_ > 0 && value_ < std::numeric_limits<int64_t>::min() + other.value_)) {
                throw std::overflow_error("Fixed8 subtraction overflow");
            }
            result.value_ = value_ - other.value_;
            return result;
        }

        /**
         * @brief Multiplication operator.
         * @param other The other Fixed8.
         * @return The product.
         * @throws std::overflow_error if overflow occurs.
         */
        Fixed8 operator*(const Fixed8& other) const {
            Fixed8 result;
            // Check for overflow in multiplication
            if (value_ != 0 && other.value_ != 0) {
                int64_t max_val = std::numeric_limits<int64_t>::max() / SCALE_FACTOR;
                int64_t min_val = std::numeric_limits<int64_t>::min() / SCALE_FACTOR;
                if ((value_ > max_val || value_ < min_val) ||
                    (other.value_ > max_val || other.value_ < min_val)) {
                    throw std::overflow_error("Fixed8 multiplication overflow");
                }
            }
            result.value_ = (value_ * other.value_) / SCALE_FACTOR;
            return result;
        }

        /**
         * @brief Division operator.
         * @param other The other Fixed8.
         * @return The quotient.
         * @throws std::invalid_argument if dividing by zero.
         * @throws std::overflow_error if overflow occurs.
         */
        Fixed8 operator/(const Fixed8& other) const {
            if (other.value_ == 0) {
                throw std::invalid_argument("Division by zero");
            }
            Fixed8 result;
            // Check for overflow in division
            if (value_ == std::numeric_limits<int64_t>::min() && other.value_ == -1) {
                throw std::overflow_error("Fixed8 division overflow");
            }
            result.value_ = (value_ * SCALE_FACTOR) / other.value_;
            return result;
        }
    };
}