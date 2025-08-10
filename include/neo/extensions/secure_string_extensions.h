#pragma once

#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace neo::extensions
{
/**
 * @brief Extensions for secure string operations.
 *
 * ## Overview
 * Provides secure handling of sensitive string data with automatic memory clearing,
 * secure comparison operations, and protection against timing attacks.
 *
 * ## API Reference
 * - **SecureString**: RAII class for automatic memory clearing
 * - **Secure Operations**: Constant-time comparison, secure clearing
 * - **Utilities**: Secure copying, validation, conversion
 *
 * ## Usage Examples
 * ```cpp
 * // Create secure string
 * auto securePassword = SecureStringExtensions::CreateSecureString("password123");
 *
 * // Secure comparison
 * bool equal = SecureStringExtensions::SecureEquals(str1, str2);
 *
 * // Secure clear
 * SecureStringExtensions::SecureClear(sensitiveData);
 * ```
 *
 * ## Design Notes
 * - Memory is automatically zeroed on destruction
 * - Constant-time operations prevent timing attacks
 * - Uses platform-specific secure memory APIs where available
 */
class SecureStringExtensions
{
   public:
    /**
     * @brief RAII class for secure string handling
     */
    class SecureString
    {
       private:
        std::unique_ptr<char[]> data_;
        size_t length_;

       public:
        /**
         * @brief Construct from regular string
         * @param str String to secure
         */
        explicit SecureString(const std::string& str);

        /**
         * @brief Construct from C-string
         * @param str C-string to secure
         */
        explicit SecureString(const char* str);

        /**
         * @brief Construct from buffer
         * @param data Buffer to copy
         * @param length Buffer length
         */
        SecureString(const char* data, size_t length);

        /**
         * @brief Move constructor
         */
        SecureString(SecureString&& other) noexcept;

        /**
         * @brief Move assignment
         */
        SecureString& operator=(SecureString&& other) noexcept;

        /**
         * @brief Destructor - securely clears memory
         */
        ~SecureString();

        // Disable copy operations for security
        SecureString(const SecureString&) = delete;
        SecureString& operator=(const SecureString&) = delete;

        /**
         * @brief Get pointer to data (use with caution)
         * @return Pointer to secure data
         */
        const char* data() const { return data_.get(); }

        /**
         * @brief Get length of string
         * @return String length
         */
        size_t length() const { return length_; }

        /**
         * @brief Check if empty
         * @return True if empty
         */
        bool empty() const { return length_ == 0; }

        /**
         * @brief Create regular string (use with caution)
         * @return Copy as std::string
         */
        std::string to_string() const;

        /**
         * @brief Secure comparison with another SecureString
         * @param other String to compare with
         * @return True if equal (constant-time)
         */
        bool secure_equals(const SecureString& other) const;

        /**
         * @brief Secure comparison with regular string
         * @param other String to compare with
         * @return True if equal (constant-time)
         */
        bool secure_equals(const std::string& other) const;

        /**
         * @brief Get character at position (bounds checked)
         * @param index Position
         * @return Character at position
         */
        char at(size_t index) const;

        /**
         * @brief Create substring
         * @param start Start position
         * @param count Number of characters
         * @return New SecureString with substring
         */
        SecureString substr(size_t start, size_t count = std::string::npos) const;
    };

    /**
     * @brief Create secure string from regular string
     * @param str String to secure
     * @return SecureString instance
     */
    static SecureString CreateSecureString(const std::string& str);

    /**
     * @brief Create secure string from C-string
     * @param str C-string to secure
     * @return SecureString instance
     */
    static SecureString CreateSecureString(const char* str);

    /**
     * @brief Securely clear memory buffer
     * @param data Pointer to memory
     * @param size Size of memory to clear
     */
    static void SecureClear(void* data, size_t size);

    /**
     * @brief Securely clear string
     * @param str String to clear
     */
    static void SecureClear(std::string& str);

    /**
     * @brief Securely clear vector of chars
     * @param vec Vector to clear
     */
    static void SecureClear(std::vector<char>& vec);

    /**
     * @brief Constant-time string comparison
     * @param left First string
     * @param right Second string
     * @return True if equal (constant-time)
     */
    static bool SecureEquals(const std::string& left, const std::string& right);

    /**
     * @brief Constant-time buffer comparison
     * @param left First buffer
     * @param right Second buffer
     * @param size Size to compare
     * @return True if equal (constant-time)
     */
    static bool SecureEquals(const void* left, const void* right, size_t size);

    /**
     * @brief Generate cryptographically secure random string
     * @param length Length of string to generate
     * @param charset Character set to use (default: alphanumeric)
     * @return SecureString with random data
     */
    static SecureString GenerateSecureRandom(
        size_t length, const std::string& charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    /**
     * @brief Validate password strength
     * @param password Password to validate
     * @param minLength Minimum required length
     * @param requireUppercase Require uppercase letters
     * @param requireLowercase Require lowercase letters
     * @param requireDigits Require digits
     * @param requireSpecial Require special characters
     * @return True if password meets requirements
     */
    static bool ValidatePasswordStrength(const SecureString& password, size_t minLength = 8,
                                         bool requireUppercase = true, bool requireLowercase = true,
                                         bool requireDigits = true, bool requireSpecial = true);

    /**
     * @brief Secure string concatenation
     * @param left First string
     * @param right Second string
     * @return Concatenated SecureString
     */
    static SecureString SecureConcat(const SecureString& left, const SecureString& right);

    /**
     * @brief Find substring in secure string
     * @param haystack String to search in
     * @param needle Substring to find
     * @return Position of first occurrence, or std::string::npos if not found
     */
    static size_t SecureFind(const SecureString& haystack, const SecureString& needle);

    /**
     * @brief Check if secure string contains substring
     * @param haystack String to search in
     * @param needle Substring to find
     * @return True if contains substring
     */
    static bool SecureContains(const SecureString& haystack, const SecureString& needle);

    /**
     * @brief Create secure hash of string (for storage/comparison)
     * @param input String to hash
     * @param salt Salt to use (optional)
     * @return Hash as SecureString
     */
    static SecureString SecureHash(const SecureString& input, const SecureString& salt = SecureString(""));

   private:
    /**
     * @brief Platform-specific secure memory clearing
     * @param data Pointer to memory
     * @param size Size to clear
     */
    static void PlatformSecureClear(void* data, size_t size);
};
}  // namespace neo::extensions
