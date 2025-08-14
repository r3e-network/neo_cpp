/**
 * @file error_handling.h
 * @brief Standardized error handling framework for Neo C++
 * @details Provides consistent error handling patterns across the codebase
 */

#pragma once

#include <exception>
#include <string>
#include <source_location>
#include <format>
#include <optional>
#include <variant>
#include <system_error>

namespace neo {
namespace error {

/**
 * @brief Error severity levels
 */
enum class Severity {
    Debug,      // Debugging information
    Info,       // Informational messages
    Warning,    // Warning conditions
    Error,      // Error conditions
    Critical    // Critical failures requiring immediate action
};

/**
 * @brief Error categories for Neo-specific errors
 */
enum class ErrorCategory {
    Network,      // Network-related errors
    Consensus,    // Consensus mechanism errors
    Storage,      // Storage/persistence errors
    Validation,   // Validation errors
    Cryptography, // Cryptographic operation errors
    SmartContract,// Smart contract execution errors
    Configuration,// Configuration errors
    System        // System-level errors
};

/**
 * @brief Neo-specific error codes
 */
enum class ErrorCode {
    // General errors
    Success = 0,
    UnknownError = 1,
    InvalidArgument = 2,
    OutOfRange = 3,
    NotImplemented = 4,
    
    // Network errors (1000-1999)
    NetworkTimeout = 1000,
    ConnectionFailed = 1001,
    PeerDisconnected = 1002,
    InvalidMessage = 1003,
    RateLimitExceeded = 1004,
    
    // Consensus errors (2000-2999)
    ConsensusTimeout = 2000,
    InvalidBlock = 2001,
    InvalidSignature = 2002,
    ForkDetected = 2003,
    
    // Storage errors (3000-3999)
    StorageCorrupted = 3000,
    StorageUnavailable = 3001,
    DiskFull = 3002,
    ReadError = 3003,
    WriteError = 3004,
    
    // Validation errors (4000-4999)
    ValidationFailed = 4000,
    InvalidTransaction = 4001,
    InsufficientFunds = 4002,
    DuplicateTransaction = 4003,
    
    // Cryptography errors (5000-5999)
    CryptoOperationFailed = 5000,
    InvalidKey = 5001,
    SignatureMismatch = 5002,
    HashMismatch = 5003,
    
    // Smart contract errors (6000-6999)
    ContractExecutionFailed = 6000,
    ContractNotFound = 6001,
    GasExhausted = 6002,
    StackOverflow = 6003,
    InvalidOpcode = 6004
};

/**
 * @brief Convert error code to string
 */
inline const char* ErrorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success: return "Success";
        case ErrorCode::UnknownError: return "Unknown error";
        case ErrorCode::InvalidArgument: return "Invalid argument";
        case ErrorCode::OutOfRange: return "Out of range";
        case ErrorCode::NotImplemented: return "Not implemented";
        
        case ErrorCode::NetworkTimeout: return "Network timeout";
        case ErrorCode::ConnectionFailed: return "Connection failed";
        case ErrorCode::PeerDisconnected: return "Peer disconnected";
        case ErrorCode::InvalidMessage: return "Invalid message";
        case ErrorCode::RateLimitExceeded: return "Rate limit exceeded";
        
        case ErrorCode::ConsensusTimeout: return "Consensus timeout";
        case ErrorCode::InvalidBlock: return "Invalid block";
        case ErrorCode::InvalidSignature: return "Invalid signature";
        case ErrorCode::ForkDetected: return "Fork detected";
        
        case ErrorCode::StorageCorrupted: return "Storage corrupted";
        case ErrorCode::StorageUnavailable: return "Storage unavailable";
        case ErrorCode::DiskFull: return "Disk full";
        case ErrorCode::ReadError: return "Read error";
        case ErrorCode::WriteError: return "Write error";
        
        case ErrorCode::ValidationFailed: return "Validation failed";
        case ErrorCode::InvalidTransaction: return "Invalid transaction";
        case ErrorCode::InsufficientFunds: return "Insufficient funds";
        case ErrorCode::DuplicateTransaction: return "Duplicate transaction";
        
        case ErrorCode::CryptoOperationFailed: return "Cryptographic operation failed";
        case ErrorCode::InvalidKey: return "Invalid key";
        case ErrorCode::SignatureMismatch: return "Signature mismatch";
        case ErrorCode::HashMismatch: return "Hash mismatch";
        
        case ErrorCode::ContractExecutionFailed: return "Contract execution failed";
        case ErrorCode::ContractNotFound: return "Contract not found";
        case ErrorCode::GasExhausted: return "Gas exhausted";
        case ErrorCode::StackOverflow: return "Stack overflow";
        case ErrorCode::InvalidOpcode: return "Invalid opcode";
        
        default: return "Unknown error code";
    }
}

/**
 * @brief Base exception class for Neo errors
 */
class NeoException : public std::exception {
public:
    NeoException(ErrorCode code, 
                 const std::string& message,
                 const std::source_location& location = std::source_location::current())
        : code_(code)
        , message_(message)
        , location_(location) {
        formatted_message_ = std::format("[{}:{}] Error {}: {} - {}",
            location_.file_name(),
            location_.line(),
            static_cast<int>(code_),
            ErrorCodeToString(code_),
            message_);
    }
    
    const char* what() const noexcept override {
        return formatted_message_.c_str();
    }
    
    ErrorCode code() const noexcept { return code_; }
    const std::string& message() const noexcept { return message_; }
    const std::source_location& location() const noexcept { return location_; }
    
private:
    ErrorCode code_;
    std::string message_;
    std::source_location location_;
    std::string formatted_message_;
};

/**
 * @brief Result type for operations that can fail
 * @tparam T Success value type
 * @tparam E Error type (defaults to NeoException)
 */
template<typename T, typename E = NeoException>
class Result {
public:
    // Success constructor
    Result(T&& value) : data_(std::forward<T>(value)) {}
    Result(const T& value) : data_(value) {}
    
    // Error constructor
    Result(E&& error) : data_(std::forward<E>(error)) {}
    Result(const E& error) : data_(error) {}
    
    // Check if result is success
    bool IsSuccess() const { return std::holds_alternative<T>(data_); }
    bool IsError() const { return std::holds_alternative<E>(data_); }
    
    // Get value (throws if error)
    T& Value() {
        if (auto* val = std::get_if<T>(&data_)) {
            return *val;
        }
        throw std::get<E>(data_);
    }
    
    const T& Value() const {
        if (auto* val = std::get_if<T>(&data_)) {
            return *val;
        }
        throw std::get<E>(data_);
    }
    
    // Get error (throws if success)
    E& Error() {
        if (auto* err = std::get_if<E>(&data_)) {
            return *err;
        }
        throw std::logic_error("Result is not an error");
    }
    
    const E& Error() const {
        if (auto* err = std::get_if<E>(&data_)) {
            return *err;
        }
        throw std::logic_error("Result is not an error");
    }
    
    // Get value or default
    T ValueOr(const T& default_value) const {
        if (auto* val = std::get_if<T>(&data_)) {
            return *val;
        }
        return default_value;
    }
    
    // Map success value
    template<typename F>
    auto Map(F&& func) -> Result<decltype(func(std::declval<T>())), E> {
        if (IsSuccess()) {
            return func(std::get<T>(data_));
        }
        return std::get<E>(data_);
    }
    
    // Map error
    template<typename F>
    auto MapError(F&& func) -> Result<T, decltype(func(std::declval<E>()))> {
        if (IsError()) {
            return func(std::get<E>(data_));
        }
        return std::get<T>(data_);
    }
    
    // Monadic bind
    template<typename F>
    auto AndThen(F&& func) -> decltype(func(std::declval<T>())) {
        if (IsSuccess()) {
            return func(std::get<T>(data_));
        }
        return std::get<E>(data_);
    }
    
private:
    std::variant<T, E> data_;
};

/**
 * @brief Helper function to create success result
 */
template<typename T>
Result<T> Ok(T&& value) {
    return Result<T>(std::forward<T>(value));
}

/**
 * @brief Helper function to create error result
 */
template<typename T, typename E>
Result<T, E> Err(E&& error) {
    return Result<T, E>(std::forward<E>(error));
}

/**
 * @brief Macro for early return on error
 */
#define TRY(expr) \
    do { \
        auto _result = (expr); \
        if (_result.IsError()) { \
            return _result.Error(); \
        } \
    } while(0)

/**
 * @brief Macro for early return with value extraction
 */
#define TRY_ASSIGN(var, expr) \
    auto _result_##var = (expr); \
    if (_result_##var.IsError()) { \
        return _result_##var.Error(); \
    } \
    auto var = _result_##var.Value()

/**
 * @brief Assert with custom message
 */
#define NEO_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            throw neo::error::NeoException( \
                neo::error::ErrorCode::UnknownError, \
                message, \
                std::source_location::current() \
            ); \
        } \
    } while(0)

/**
 * @brief Check precondition
 */
#define NEO_REQUIRE(condition, error_code, message) \
    do { \
        if (!(condition)) { \
            throw neo::error::NeoException( \
                error_code, \
                message, \
                std::source_location::current() \
            ); \
        } \
    } while(0)

/**
 * @brief Ensure postcondition
 */
#define NEO_ENSURE(condition, error_code, message) \
    NEO_REQUIRE(condition, error_code, message)

/**
 * @brief Mark unreachable code
 */
#define NEO_UNREACHABLE() \
    throw neo::error::NeoException( \
        neo::error::ErrorCode::UnknownError, \
        "Unreachable code reached", \
        std::source_location::current() \
    )

/**
 * @brief Error context for structured error handling
 */
class ErrorContext {
public:
    static ErrorContext& Instance() {
        static ErrorContext instance;
        return instance;
    }
    
    void LogError(const NeoException& error) {
        // Log to appropriate sink
        // This would integrate with the logging system
    }
    
    void SetErrorHandler(std::function<void(const NeoException&)> handler) {
        error_handler_ = handler;
    }
    
    void HandleError(const NeoException& error) {
        if (error_handler_) {
            error_handler_(error);
        } else {
            LogError(error);
        }
    }
    
private:
    ErrorContext() = default;
    std::function<void(const NeoException&)> error_handler_;
};

/**
 * @brief RAII guard for error recovery
 */
class ErrorGuard {
public:
    explicit ErrorGuard(std::function<void()> recovery_func)
        : recovery_func_(recovery_func)
        , should_recover_(true) {}
    
    ~ErrorGuard() {
        if (should_recover_ && std::uncaught_exceptions() > 0) {
            try {
                recovery_func_();
            } catch (...) {
                // Suppress exceptions in destructor
            }
        }
    }
    
    void Dismiss() { should_recover_ = false; }
    
private:
    std::function<void()> recovery_func_;
    bool should_recover_;
};

} // namespace error
} // namespace neo