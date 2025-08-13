/**
 * @file utility.h
 * @brief Utility
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace neo::extensions
{
/**
 * @brief RAII guard helper class
 */
template <typename Func>
class ScopeGuard
{
   public:
    explicit ScopeGuard(Func&& func) : func_(std::forward<Func>(func)), active_(true) {}
    ~ScopeGuard()
    {
        if (active_) func_();
    }

    ScopeGuard(ScopeGuard&& other) noexcept : func_(std::move(other.func_)), active_(other.active_)
    {
        other.active_ = false;
    }

    void Release() { active_ = false; }

   private:
    Func func_;
    bool active_;

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;
};

/**
 * @brief General utility functions.
 *
 * ## Overview
 * Provides commonly used utility functions for string manipulation,
 * memory operations, type conversions, and general helper methods.
 *
 * ## API Reference
 * - **String Operations**: Split, join, trim, case conversion
 * - **Memory Operations**: Safe memory operations, RAII helpers
 * - **Type Conversion**: Safe casting, parsing utilities
 * - **Validation**: Input validation, range checking
 *
 * ## Usage Examples
 * ```cpp
 * // String operations
 * auto parts = Utility::Split("a,b,c", ",");
 * auto joined = Utility::Join(parts, ";");
 *
 * // Memory operations
 * Utility::SecureZeroMemory(data, size);
 *
 * // Type conversion
 * auto value = Utility::TryParse<int>("123");
 * ```
 */
class Utility
{
   public:
    /**
     * @brief Split string by delimiter
     * @param str String to split
     * @param delimiter Delimiter string
     * @param removeEmpty Whether to remove empty parts
     * @return Vector of string parts
     */
    static std::vector<std::string> Split(const std::string& str, const std::string& delimiter,
                                          bool removeEmpty = true);

    /**
     * @brief Join string vector with delimiter
     * @param parts Vector of strings to join
     * @param delimiter Delimiter string
     * @return Joined string
     */
    static std::string Join(const std::vector<std::string>& parts, const std::string& delimiter);

    /**
     * @brief Trim whitespace from both ends of string
     * @param str String to trim
     * @return Trimmed string
     */
    static std::string Trim(const std::string& str);

    /**
     * @brief Trim whitespace from left end of string
     * @param str String to trim
     * @return Trimmed string
     */
    static std::string TrimLeft(const std::string& str);

    /**
     * @brief Trim whitespace from right end of string
     * @param str String to trim
     * @return Trimmed string
     */
    static std::string TrimRight(const std::string& str);

    /**
     * @brief Convert string to lowercase
     * @param str String to convert
     * @return Lowercase string
     */
    static std::string ToLower(const std::string& str);

    /**
     * @brief Convert string to uppercase
     * @param str String to convert
     * @return Uppercase string
     */
    static std::string ToUpper(const std::string& str);

    /**
     * @brief Check if string starts with prefix
     * @param str String to check
     * @param prefix Prefix to look for
     * @param ignoreCase Whether to ignore case
     * @return True if string starts with prefix
     */
    static bool StartsWith(const std::string& str, const std::string& prefix, bool ignoreCase = false);

    /**
     * @brief Check if string ends with suffix
     * @param str String to check
     * @param suffix Suffix to look for
     * @param ignoreCase Whether to ignore case
     * @return True if string ends with suffix
     */
    static bool EndsWith(const std::string& str, const std::string& suffix, bool ignoreCase = false);

    /**
     * @brief Check if string contains substring
     * @param str String to check
     * @param substring Substring to look for
     * @param ignoreCase Whether to ignore case
     * @return True if string contains substring
     */
    static bool Contains(const std::string& str, const std::string& substring, bool ignoreCase = false);

    /**
     * @brief Replace all occurrences of substring
     * @param str String to modify
     * @param from Substring to replace
     * @param to Replacement string
     * @return Modified string
     */
    static std::string Replace(const std::string& str, const std::string& from, const std::string& to);

    /**
     * @brief Safely zero memory (prevents compiler optimization)
     * @param ptr Pointer to memory
     * @param size Size in bytes
     */
    static void SecureZeroMemory(void* ptr, size_t size);

    /**
     * @brief Secure compare memory (constant time)
     * @param a First memory block
     * @param b Second memory block
     * @param size Size in bytes
     * @return True if memory blocks are equal
     */
    static bool SecureCompareMemory(const void* a, const void* b, size_t size);

    /**
     * @brief Get size of container
     * @tparam Container Container type
     * @param container Container instance
     * @return Size of container
     */
    template <typename Container>
    static size_t GetSize(const Container& container)
    {
        return container.size();
    }

    /**
     * @brief Check if container is empty
     * @tparam Container Container type
     * @param container Container instance
     * @return True if container is empty
     */
    template <typename Container>
    static bool IsEmpty(const Container& container)
    {
        return container.empty();
    }

    /**
     * @brief Try to parse string to type T
     * @tparam T Type to parse to
     * @param str String to parse
     * @param result Output parameter for result
     * @return True if parsing succeeded
     */
    template <typename T>
    static bool TryParse(const std::string& str, T& result);

    /**
     * @brief Parse string to type T with exception on failure
     * @tparam T Type to parse to
     * @param str String to parse
     * @return Parsed value
     */
    template <typename T>
    static T Parse(const std::string& str);

    /**
     * @brief Convert value to string
     * @tparam T Type to convert
     * @param value Value to convert
     * @return String representation
     */
    template <typename T>
    static std::string ToString(const T& value);

    /**
     * @brief Clamp value between min and max
     * @tparam T Type of value
     * @param value Value to clamp
     * @param min Minimum value
     * @param max Maximum value
     * @return Clamped value
     */
    template <typename T>
    static T Clamp(const T& value, const T& min, const T& max)
    {
        return std::max(min, std::min(value, max));
    }

    /**
     * @brief Check if value is in range [min, max]
     * @tparam T Type of value
     * @param value Value to check
     * @param min Minimum value (inclusive)
     * @param max Maximum value (inclusive)
     * @return True if value is in range
     */
    template <typename T>
    static bool IsInRange(const T& value, const T& min, const T& max)
    {
        return value >= min && value <= max;
    }

    /**
     * @brief Safe cast with range checking
     * @tparam To Target type
     * @tparam From Source type
     * @param value Value to cast
     * @return Casted value
     */
    template <typename To, typename From>
    static To SafeCast(const From& value);

    /**
     * @brief Get minimum of two values
     * @tparam T Type of values
     * @param a First value
     * @param b Second value
     * @return Minimum value
     */
    template <typename T>
    static const T& Min(const T& a, const T& b)
    {
        return std::min(a, b);
    }

    /**
     * @brief Get maximum of two values
     * @tparam T Type of values
     * @param a First value
     * @param b Second value
     * @return Maximum value
     */
    template <typename T>
    static const T& Max(const T& a, const T& b)
    {
        return std::max(a, b);
    }

    /**
     * @brief Swap two values
     * @tparam T Type of values
     * @param a First value
     * @param b Second value
     */
    template <typename T>
    static void Swap(T& a, T& b)
    {
        std::swap(a, b);
    }

    /**
     * @brief Format string with arguments (printf-style)
     * @param format Format string
     * @param args Arguments
     * @return Formatted string
     */
    template <typename... Args>
    static std::string Format(const std::string& format, Args&&... args);

    /**
     * @brief Check if pointer is valid (not null)
     * @tparam T Pointer type
     * @param ptr Pointer to check
     * @return True if pointer is not null
     */
    template <typename T>
    static bool IsValidPtr(const T* ptr)
    {
        return ptr != nullptr;
    }

    /**
     * @brief Check if shared_ptr is valid
     * @tparam T Type of shared_ptr
     * @param ptr Shared pointer to check
     * @return True if shared_ptr is not null
     */
    template <typename T>
    static bool IsValidPtr(const std::shared_ptr<T>& ptr)
    {
        return ptr != nullptr;
    }

    /**
     * @brief Create RAII guard for cleanup
     * @tparam Func Function type
     * @param func Cleanup function
     * @return RAII guard object
     */
    template <typename Func>
    static auto MakeGuard(Func&& func) -> ScopeGuard<Func>
    {
        return ScopeGuard<Func>(std::forward<Func>(func));
    }

    /**
     * @brief Hash combine utility (for custom hash functions)
     * @tparam T Type of value
     * @param seed Seed value to combine with
     * @param value Value to hash
     */
    template <typename T>
    static void HashCombine(size_t& seed, const T& value)
    {
        std::hash<T> hasher;
        seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    /**
     * @brief Calculate next power of 2
     * @param value Input value
     * @return Next power of 2 >= value
     */
    static uint32_t NextPowerOf2(uint32_t value);

    /**
     * @brief Check if value is power of 2
     * @param value Value to check
     * @return True if value is power of 2
     */
    static bool IsPowerOf2(uint32_t value);

    /**
     * @brief Reverse bytes in memory
     * @param data Pointer to data
     * @param size Size in bytes
     */
    static void ReverseBytes(uint8_t* data, size_t size);

    /**
     * @brief Convert bytes to hex string
     * @param data Byte data
     * @param uppercase Whether to use uppercase hex
     * @return Hex string
     */
    static std::string BytesToHex(const io::ByteSpan& data, bool uppercase = false);

    /**
     * @brief Convert hex string to bytes
     * @param hex Hex string
     * @return Byte vector
     */
    static io::ByteVector HexToBytes(const std::string& hex);
};
}  // namespace neo::extensions
