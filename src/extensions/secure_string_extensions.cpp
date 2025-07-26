#include <neo/extensions/secure_string_extensions.h>
#include <neo/cryptography/hash.h>
#include <neo/io/byte_vector.h>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <cctype>
#include <random>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdlib>
#endif

namespace neo::extensions
{
    // SecureString implementation
    SecureStringExtensions::SecureString::SecureString(const std::string& str)
        : length_(str.length())
    {
        if (length_ > 0)
        {
            data_ = std::make_unique<char[]>(length_ + 1);
            std::memcpy(data_.get(), str.c_str(), length_);
            data_[length_] = '\0';
        }
    }

    SecureStringExtensions::SecureString::SecureString(const char* str)
        : length_(str ? std::strlen(str) : 0)
    {
        if (length_ > 0)
        {
            data_ = std::make_unique<char[]>(length_ + 1);
            std::memcpy(data_.get(), str, length_);
            data_[length_] = '\0';
        }
    }

    SecureStringExtensions::SecureString::SecureString(const char* data, size_t length)
        : length_(length)
    {
        if (length_ > 0 && data)
        {
            data_ = std::make_unique<char[]>(length_ + 1);
            std::memcpy(data_.get(), data, length_);
            data_[length_] = '\0';
        }
    }

    SecureStringExtensions::SecureString::SecureString(SecureString&& other) noexcept
        : data_(std::move(other.data_)), length_(other.length_)
    {
        other.length_ = 0;
    }

    SecureStringExtensions::SecureString& SecureStringExtensions::SecureString::operator=(SecureString&& other) noexcept
    {
        if (this != &other)
        {
            // Securely clear current data
            if (data_ && length_ > 0)
            {
                SecureStringExtensions::SecureClear(data_.get(), length_);
            }
            
            data_ = std::move(other.data_);
            length_ = other.length_;
            other.length_ = 0;
        }
        return *this;
    }

    SecureStringExtensions::SecureString::~SecureString()
    {
        if (data_ && length_ > 0)
        {
            SecureStringExtensions::SecureClear(data_.get(), length_);
        }
    }

    std::string SecureStringExtensions::SecureString::to_string() const
    {
        if (!data_ || length_ == 0)
            return "";
        return std::string(data_.get(), length_);
    }

    bool SecureStringExtensions::SecureString::secure_equals(const SecureString& other) const
    {
        if (length_ != other.length_)
            return false;
            
        if (length_ == 0)
            return true;
            
        return SecureStringExtensions::SecureEquals(data_.get(), other.data_.get(), length_);
    }

    bool SecureStringExtensions::SecureString::secure_equals(const std::string& other) const
    {
        if (length_ != other.length())
            return false;
            
        if (length_ == 0)
            return true;
            
        return SecureStringExtensions::SecureEquals(data_.get(), other.c_str(), length_);
    }

    char SecureStringExtensions::SecureString::at(size_t index) const
    {
        if (index >= length_)
            throw std::out_of_range("Index out of range");
        return data_[index];
    }

    SecureStringExtensions::SecureString SecureStringExtensions::SecureString::substr(size_t start, size_t count) const
    {
        if (start >= length_)
            return SecureString("");
            
        size_t actualCount = std::min(count, length_ - start);
        return SecureString(data_.get() + start, actualCount);
    }

    // Static methods implementation
    SecureStringExtensions::SecureString SecureStringExtensions::CreateSecureString(const std::string& str)
    {
        return SecureString(str);
    }

    SecureStringExtensions::SecureString SecureStringExtensions::CreateSecureString(const char* str)
    {
        return SecureString(str);
    }

    void SecureStringExtensions::SecureClear(void* data, size_t size)
    {
        if (!data || size == 0)
            return;
            
        PlatformSecureClear(data, size);
    }

    void SecureStringExtensions::SecureClear(std::string& str)
    {
        if (!str.empty())
        {
            SecureClear(&str[0], str.length());
            str.clear();
        }
    }

    void SecureStringExtensions::SecureClear(std::vector<char>& vec)
    {
        if (!vec.empty())
        {
            SecureClear(vec.data(), vec.size());
            vec.clear();
        }
    }

    bool SecureStringExtensions::SecureEquals(const std::string& left, const std::string& right)
    {
        if (left.length() != right.length())
            return false;
            
        return SecureEquals(left.c_str(), right.c_str(), left.length());
    }

    bool SecureStringExtensions::SecureEquals(const void* left, const void* right, size_t size)
    {
        if (!left || !right)
            return left == right;
            
        if (size == 0)
            return true;
            
        // Constant-time comparison to prevent timing attacks
        const unsigned char* l = static_cast<const unsigned char*>(left);
        const unsigned char* r = static_cast<const unsigned char*>(right);
        
        unsigned char result = 0;
        for (size_t i = 0; i < size; ++i)
        {
            result |= l[i] ^ r[i];
        }
        
        return result == 0;
    }

    SecureStringExtensions::SecureString SecureStringExtensions::GenerateSecureRandom(size_t length, const std::string& charset)
    {
        if (length == 0 || charset.empty())
            return SecureString("");
            
        // Use cryptographically secure random number generation
        auto result = std::make_unique<char[]>(length + 1);
        
        // Cryptographically secure random generation using system entropy
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<> dis(0, charset.length() - 1);
        
        for (size_t i = 0; i < length; ++i)
        {
            result[i] = charset[dis(gen)];
        }
        result[length] = '\0';
        
        return SecureString(result.get(), length);
    }

    bool SecureStringExtensions::ValidatePasswordStrength(const SecureString& password, 
                                                         size_t minLength,
                                                         bool requireUppercase,
                                                         bool requireLowercase,
                                                         bool requireDigits,
                                                         bool requireSpecial)
    {
        if (password.length() < minLength)
            return false;
            
        bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;
        
        for (size_t i = 0; i < password.length(); ++i)
        {
            char c = password.at(i);
            
            if (std::isupper(c))
                hasUpper = true;
            else if (std::islower(c))
                hasLower = true;
            else if (std::isdigit(c))
                hasDigit = true;
            else if (std::ispunct(c))
                hasSpecial = true;
        }
        
        return (!requireUppercase || hasUpper) &&
               (!requireLowercase || hasLower) &&
               (!requireDigits || hasDigit) &&
               (!requireSpecial || hasSpecial);
    }

    SecureStringExtensions::SecureString SecureStringExtensions::SecureConcat(const SecureString& left, const SecureString& right)
    {
        size_t totalLength = left.length() + right.length();
        if (totalLength == 0)
            return SecureString("");
            
        auto result = std::make_unique<char[]>(totalLength + 1);
        
        if (left.length() > 0)
            std::memcpy(result.get(), left.data(), left.length());
        if (right.length() > 0)
            std::memcpy(result.get() + left.length(), right.data(), right.length());
        result[totalLength] = '\0';
        
        return SecureString(result.get(), totalLength);
    }

    size_t SecureStringExtensions::SecureFind(const SecureString& haystack, const SecureString& needle)
    {
        if (needle.length() == 0)
            return 0;
        if (needle.length() > haystack.length())
            return std::string::npos;
            
        for (size_t i = 0; i <= haystack.length() - needle.length(); ++i)
        {
            if (SecureEquals(haystack.data() + i, needle.data(), needle.length()))
                return i;
        }
        
        return std::string::npos;
    }

    bool SecureStringExtensions::SecureContains(const SecureString& haystack, const SecureString& needle)
    {
        return SecureFind(haystack, needle) != std::string::npos;
    }

    SecureStringExtensions::SecureString SecureStringExtensions::SecureHash(const SecureString& input, const SecureString& salt)
    {
        // Use SHA-256 for secure hashing
        // Combine input and salt
        io::ByteVector dataVector;
        dataVector.Reserve(input.length() + salt.length());
        
        // Add input bytes
        for (size_t i = 0; i < input.length(); ++i)
        {
            dataVector.Push(static_cast<uint8_t>(input.at(i)));
        }
        
        // Add salt bytes
        for (size_t i = 0; i < salt.length(); ++i)
        {
            dataVector.Push(static_cast<uint8_t>(salt.at(i)));
        }
        
        // Use SHA-256 from Neo's cryptography module
        auto hash = neo::cryptography::Hash::Sha256(dataVector.AsSpan());
        
        // Convert hash to hex string for SecureString
        std::string hexResult;
        hexResult.reserve(hash.Size() * 2);
        const char* hexChars = "0123456789abcdef";
        
        for (size_t i = 0; i < hash.Size(); ++i)
        {
            uint8_t byte = hash.Data()[i];
            hexResult.push_back(hexChars[byte >> 4]);
            hexResult.push_back(hexChars[byte & 0x0F]);
        }
        
        return SecureString(hexResult);
    }

    void SecureStringExtensions::PlatformSecureClear(void* data, size_t size)
    {
#ifdef _WIN32
        SecureZeroMemory(data, size);
#else
        // Use explicit_bzero if available, otherwise volatile memset
        #if defined(explicit_bzero)
            explicit_bzero(data, size);
        #else
            volatile unsigned char* p = static_cast<volatile unsigned char*>(data);
            for (size_t i = 0; i < size; ++i)
            {
                p[i] = 0;
            }
        #endif
#endif
    }
}
