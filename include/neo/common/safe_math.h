#pragma once

#include <limits>
#include <stdexcept>
#include <type_traits>

namespace neo::common
{
    /**
     * @brief Safe arithmetic operations that check for overflow
     */
    class SafeMath
    {
    public:
        /**
         * @brief Safe addition with overflow check
         * @tparam T Numeric type
         * @param a First operand
         * @param b Second operand
         * @return Result of a + b
         * @throws std::overflow_error if operation would overflow
         */
        template<typename T>
        static T Add(T a, T b)
        {
            static_assert(std::is_integral_v<T>, "SafeMath only works with integral types");
            
            if constexpr (std::is_signed_v<T>) {
                if (b > 0 && a > std::numeric_limits<T>::max() - b) {
                    throw std::overflow_error("Integer overflow in addition");
                }
                if (b < 0 && a < std::numeric_limits<T>::min() - b) {
                    throw std::overflow_error("Integer underflow in addition");
                }
            } else {
                if (a > std::numeric_limits<T>::max() - b) {
                    throw std::overflow_error("Integer overflow in addition");
                }
            }
            
            return a + b;
        }
        
        /**
         * @brief Safe subtraction with overflow check
         * @tparam T Numeric type
         * @param a First operand
         * @param b Second operand
         * @return Result of a - b
         * @throws std::overflow_error if operation would overflow
         */
        template<typename T>
        static T Subtract(T a, T b)
        {
            static_assert(std::is_integral_v<T>, "SafeMath only works with integral types");
            
            if constexpr (std::is_signed_v<T>) {
                if (b < 0 && a > std::numeric_limits<T>::max() + b) {
                    throw std::overflow_error("Integer overflow in subtraction");
                }
                if (b > 0 && a < std::numeric_limits<T>::min() + b) {
                    throw std::overflow_error("Integer underflow in subtraction");
                }
            } else {
                if (a < b) {
                    throw std::overflow_error("Integer underflow in subtraction");
                }
            }
            
            return a - b;
        }
        
        /**
         * @brief Safe multiplication with overflow check
         * @tparam T Numeric type
         * @param a First operand
         * @param b Second operand
         * @return Result of a * b
         * @throws std::overflow_error if operation would overflow
         */
        template<typename T>
        static T Multiply(T a, T b)
        {
            static_assert(std::is_integral_v<T>, "SafeMath only works with integral types");
            
            if (a == 0 || b == 0) {
                return 0;
            }
            
            if constexpr (std::is_signed_v<T>) {
                if (a > 0) {
                    if (b > 0) {
                        if (a > std::numeric_limits<T>::max() / b) {
                            throw std::overflow_error("Integer overflow in multiplication");
                        }
                    } else {
                        if (b < std::numeric_limits<T>::min() / a) {
                            throw std::overflow_error("Integer underflow in multiplication");
                        }
                    }
                } else {
                    if (b > 0) {
                        if (a < std::numeric_limits<T>::min() / b) {
                            throw std::overflow_error("Integer underflow in multiplication");
                        }
                    } else {
                        if (a != 0 && b < std::numeric_limits<T>::max() / a) {
                            throw std::overflow_error("Integer overflow in multiplication");
                        }
                    }
                }
            } else {
                if (a > std::numeric_limits<T>::max() / b) {
                    throw std::overflow_error("Integer overflow in multiplication");
                }
            }
            
            return a * b;
        }
        
        /**
         * @brief Safe division with divide-by-zero check
         * @tparam T Numeric type
         * @param a Dividend
         * @param b Divisor
         * @return Result of a / b
         * @throws std::domain_error if b is zero
         * @throws std::overflow_error if operation would overflow (for signed min / -1)
         */
        template<typename T>
        static T Divide(T a, T b)
        {
            static_assert(std::is_integral_v<T>, "SafeMath only works with integral types");
            
            if (b == 0) {
                throw std::domain_error("Division by zero");
            }
            
            if constexpr (std::is_signed_v<T>) {
                if (a == std::numeric_limits<T>::min() && b == -1) {
                    throw std::overflow_error("Integer overflow in division");
                }
            }
            
            return a / b;
        }
        
        /**
         * @brief Check if adding two values would overflow
         * @tparam T Numeric type
         * @param a First operand
         * @param b Second operand
         * @return true if operation would overflow, false otherwise
         */
        template<typename T>
        static bool WouldAddOverflow(T a, T b) noexcept
        {
            static_assert(std::is_integral_v<T>, "SafeMath only works with integral types");
            
            if constexpr (std::is_signed_v<T>) {
                return (b > 0 && a > std::numeric_limits<T>::max() - b) ||
                       (b < 0 && a < std::numeric_limits<T>::min() - b);
            } else {
                return a > std::numeric_limits<T>::max() - b;
            }
        }
        
        /**
         * @brief Check if multiplying two values would overflow
         * @tparam T Numeric type
         * @param a First operand
         * @param b Second operand
         * @return true if operation would overflow, false otherwise
         */
        template<typename T>
        static bool WouldMultiplyOverflow(T a, T b) noexcept
        {
            static_assert(std::is_integral_v<T>, "SafeMath only works with integral types");
            
            if (a == 0 || b == 0) {
                return false;
            }
            
            if constexpr (std::is_signed_v<T>) {
                if (a > 0) {
                    if (b > 0) {
                        return a > std::numeric_limits<T>::max() / b;
                    } else {
                        return b < std::numeric_limits<T>::min() / a;
                    }
                } else {
                    if (b > 0) {
                        return a < std::numeric_limits<T>::min() / b;
                    } else {
                        return a != 0 && b < std::numeric_limits<T>::max() / a;
                    }
                }
            } else {
                return a > std::numeric_limits<T>::max() / b;
            }
        }
    };
}