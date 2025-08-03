#pragma once

#include <neo/core/exceptions.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/io/byte_span.h>
#include <string>
#include <vector>
#include <functional>
#include <limits>

namespace neo::core
{
/**
 * @brief Comprehensive validation framework for Neo C++ implementation
 * 
 * Provides robust input validation, bounds checking, and data integrity
 * verification to ensure production-ready security and reliability.
 */
class Validator
{
public:
    /**
     * @brief Validation result containing success status and error details
     */
    struct ValidationResult
    {
        bool is_valid;
        std::string error_message;
        NeoException::ErrorCode error_code;
        
        ValidationResult(bool valid = true) 
            : is_valid(valid), error_code(NeoException::ErrorCode::UNKNOWN_ERROR) {}
            
        ValidationResult(bool valid, const std::string& message)
            : is_valid(valid), error_message(message), error_code(NeoException::ErrorCode::INVALID_ARGUMENT) {}
            
        ValidationResult(bool valid, const std::string& message, NeoException::ErrorCode code)
            : is_valid(valid), error_message(message), error_code(code) {}
            
        operator bool() const { return is_valid; }
    };

    // Basic type validation
    static ValidationResult ValidateNotNull(const void* ptr, const std::string& name = "pointer");
    static ValidationResult ValidateNotEmpty(const std::string& str, const std::string& name = "string");
    static ValidationResult ValidateNotEmpty(const std::vector<uint8_t>& data, const std::string& name = "data");
    static ValidationResult ValidateNotEmpty(const io::ByteSpan& span, const std::string& name = "span");
    
    // Numeric validation
    template<typename T>
    static ValidationResult ValidateRange(T value, T min_val, T max_val, const std::string& name = "value")
    {
        if (value < min_val || value > max_val) {
            return ValidationResult(false, 
                name + " (" + std::to_string(value) + ") is out of range [" + 
                std::to_string(min_val) + ", " + std::to_string(max_val) + "]",
                NeoException::ErrorCode::OUT_OF_RANGE);
        }
        return ValidationResult(true);
    }
    
    template<typename T>
    static ValidationResult ValidatePositive(T value, const std::string& name = "value")
    {
        if (value <= 0) {
            return ValidationResult(false, name + " must be positive", 
                NeoException::ErrorCode::INVALID_ARGUMENT);
        }
        return ValidationResult(true);
    }
    
    template<typename T>
    static ValidationResult ValidateNonNegative(T value, const std::string& name = "value")
    {
        if (value < 0) {
            return ValidationResult(false, name + " must be non-negative", 
                NeoException::ErrorCode::INVALID_ARGUMENT);
        }
        return ValidationResult(true);
    }
    
    // Size validation
    static ValidationResult ValidateSize(size_t actual_size, size_t expected_size, const std::string& name = "data");
    static ValidationResult ValidateMinSize(size_t actual_size, size_t min_size, const std::string& name = "data");
    static ValidationResult ValidateMaxSize(size_t actual_size, size_t max_size, const std::string& name = "data");
    static ValidationResult ValidateSizeRange(size_t actual_size, size_t min_size, size_t max_size, const std::string& name = "data");
    
    // String validation
    static ValidationResult ValidateHexString(const std::string& hex, const std::string& name = "hex string");
    static ValidationResult ValidateHexString(const std::string& hex, size_t expected_length, const std::string& name = "hex string");
    static ValidationResult ValidateBase58String(const std::string& base58, const std::string& name = "base58 string");
    static ValidationResult ValidateAddress(const std::string& address, const std::string& name = "address");
    
    // Neo-specific validation
    static ValidationResult ValidateUInt160(const io::UInt160& value, const std::string& name = "UInt160");
    static ValidationResult ValidateUInt256(const io::UInt256& value, const std::string& name = "UInt256");
    static ValidationResult ValidateByteSpan(const io::ByteSpan& span, const std::string& name = "ByteSpan");
    static ValidationResult ValidateByteSpan(const io::ByteSpan& span, size_t expected_size, const std::string& name = "ByteSpan");
    
    // Container validation
    template<typename T>
    static ValidationResult ValidateContainer(const std::vector<T>& container, size_t max_size, const std::string& name = "container")
    {
        if (container.size() > max_size) {
            return ValidationResult(false, 
                name + " size (" + std::to_string(container.size()) + 
                ") exceeds maximum (" + std::to_string(max_size) + ")",
                NeoException::ErrorCode::OUT_OF_RANGE);
        }
        return ValidationResult(true);
    }
    
    template<typename T>
    static ValidationResult ValidateContainerNotEmpty(const std::vector<T>& container, const std::string& name = "container")
    {
        if (container.empty()) {
            return ValidationResult(false, name + " cannot be empty", 
                NeoException::ErrorCode::INVALID_ARGUMENT);
        }
        return ValidationResult(true);
    }
    
    // Custom validation functions
    template<typename T>
    static ValidationResult ValidateCustom(const T& value, std::function<bool(const T&)> validator, 
                                          const std::string& error_message, const std::string& name = "value")
    {
        if (!validator(value)) {
            return ValidationResult(false, name + ": " + error_message, 
                NeoException::ErrorCode::INVALID_ARGUMENT);
        }
        return ValidationResult(true);
    }
    
    // Compound validation - validate multiple conditions
    static ValidationResult ValidateAll(const std::vector<ValidationResult>& results);
    static ValidationResult ValidateAny(const std::vector<ValidationResult>& results);
    
    // Throwing validators - throw exceptions on validation failure
    static void RequireNotNull(const void* ptr, const std::string& name = "pointer");
    static void RequireNotEmpty(const std::string& str, const std::string& name = "string");
    static void RequireNotEmpty(const std::vector<uint8_t>& data, const std::string& name = "data");
    
    template<typename T>
    static void RequireRange(T value, T min_val, T max_val, const std::string& name = "value")
    {
        auto result = ValidateRange(value, min_val, max_val, name);
        if (!result) {
            throw NeoException(result.error_code, result.error_message);
        }
    }
    
    template<typename T>
    static void RequirePositive(T value, const std::string& name = "value")
    {
        auto result = ValidatePositive(value, name);
        if (!result) {
            throw NeoException(result.error_code, result.error_message);
        }
    }
    
    static void RequireSize(size_t actual_size, size_t expected_size, const std::string& name = "data");
    static void RequireHexString(const std::string& hex, const std::string& name = "hex string");
    static void RequireHexString(const std::string& hex, size_t expected_length, const std::string& name = "hex string");
    
    // Security validation
    static ValidationResult ValidateNoScriptInjection(const std::string& input, const std::string& name = "input");
    static ValidationResult ValidateNoSQLInjection(const std::string& input, const std::string& name = "input");
    static ValidationResult ValidateFileName(const std::string& filename, const std::string& name = "filename");
    static ValidationResult ValidateFilePath(const std::string& filepath, const std::string& name = "filepath");
    
    // Network validation
    static ValidationResult ValidateIPAddress(const std::string& ip, const std::string& name = "IP address");
    static ValidationResult ValidatePort(uint16_t port, const std::string& name = "port");
    static ValidationResult ValidateURL(const std::string& url, const std::string& name = "URL");
    
    // Blockchain-specific validation
    static ValidationResult ValidateBlockHeight(uint32_t height, uint32_t max_height = std::numeric_limits<uint32_t>::max());
    static ValidationResult ValidateTransactionFee(int64_t fee, int64_t max_fee = std::numeric_limits<int64_t>::max());
    static ValidationResult ValidateGasAmount(int64_t gas, int64_t max_gas = std::numeric_limits<int64_t>::max());
    static ValidationResult ValidateNonce(uint32_t nonce);
    static ValidationResult ValidateTimestamp(uint64_t timestamp);
};

// Utility macros for common validations
#define VALIDATE_NOT_NULL(ptr) \
    do { \
        auto __result = neo::core::Validator::ValidateNotNull(ptr, #ptr); \
        if (!__result) throw neo::core::NeoException(__result.error_code, __result.error_message); \
    } while(0)

#define VALIDATE_NOT_EMPTY(value) \
    do { \
        auto __result = neo::core::Validator::ValidateNotEmpty(value, #value); \
        if (!__result) throw neo::core::NeoException(__result.error_code, __result.error_message); \
    } while(0)

#define VALIDATE_RANGE(value, min_val, max_val) \
    do { \
        auto __result = neo::core::Validator::ValidateRange(value, min_val, max_val, #value); \
        if (!__result) throw neo::core::NeoException(__result.error_code, __result.error_message); \
    } while(0)

#define VALIDATE_POSITIVE(value) \
    do { \
        auto __result = neo::core::Validator::ValidatePositive(value, #value); \
        if (!__result) throw neo::core::NeoException(__result.error_code, __result.error_message); \
    } while(0)

#define VALIDATE_SIZE(actual, expected) \
    do { \
        auto __result = neo::core::Validator::ValidateSize(actual, expected, #actual); \
        if (!__result) throw neo::core::NeoException(__result.error_code, __result.error_message); \
    } while(0)

#define VALIDATE_HEX_STRING(hex) \
    do { \
        auto __result = neo::core::Validator::ValidateHexString(hex, #hex); \
        if (!__result) throw neo::core::NeoException(__result.error_code, __result.error_message); \
    } while(0)

} // namespace neo::core