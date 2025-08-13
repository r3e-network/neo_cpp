/**
 * @file exceptions.h
 * @brief Exception types
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>

namespace neo::core
{
/**
 * @brief Base exception class for all Neo-specific exceptions
 *
 * Provides a consistent error handling framework with error codes,
 * detailed messages, and context information for debugging and logging.
 */
class NeoException : public std::runtime_error
{
   public:
    enum class ErrorCode : uint32_t
    {
        // General errors (1000-1999)
        UNKNOWN_ERROR = 1000,
        INVALID_ARGUMENT = 1001,
        INVALID_STATE = 1002,
        NOT_AVAILABLE = 1003,
        OUT_OF_RANGE = 1004,
        TIMEOUT = 1005,

        // Serialization errors (2000-2999)
        SERIALIZATION_ERROR = 2000,
        DESERIALIZATION_ERROR = 2001,
        INVALID_FORMAT = 2002,
        BUFFER_OVERFLOW = 2003,
        BUFFER_UNDERFLOW = 2004,

        // Cryptography errors (3000-3999)
        CRYPTO_ERROR = 3000,
        INVALID_KEY = 3001,
        INVALID_SIGNATURE = 3002,
        HASH_CALCULATION_FAILED = 3003,
        ENCRYPTION_FAILED = 3004,
        DECRYPTION_FAILED = 3005,

        // Network errors (4000-4999)
        NETWORK_ERROR = 4000,
        CONNECTION_FAILED = 4001,
        INVALID_MESSAGE = 4002,
        PROTOCOL_VIOLATION = 4003,
        PEER_DISCONNECTED = 4004,

        // Blockchain errors (5000-5999)
        BLOCKCHAIN_ERROR = 5000,
        INVALID_BLOCK = 5001,
        INVALID_TRANSACTION = 5002,
        CONSENSUS_FAILURE = 5003,
        VERIFICATION_FAILED = 5004,
        INVALID_CONTRACT = 5005,

        // Storage errors (6000-6999)
        STORAGE_ERROR = 6000,
        KEY_NOT_FOUND = 6001,
        STORAGE_CORRUPTION = 6002,
        INSUFFICIENT_SPACE = 6003,
        ACCESS_DENIED = 6004,

        // VM errors (7000-7999)
        VM_ERROR = 7000,
        STACK_OVERFLOW = 7001,
        STACK_UNDERFLOW = 7002,
        INVALID_OPCODE = 7003,
        EXECUTION_TIMEOUT = 7004,
        OUT_OF_GAS = 7005,

        // Smart contract errors (8000-8999)
        CONTRACT_ERROR = 8000,
        CONTRACT_NOT_FOUND = 8001,
        CONTRACT_EXECUTION_FAILED = 8002,
        INVALID_CONTRACT_STATE = 8003,

        // Wallet errors (9000-9999)
        WALLET_ERROR = 9000,
        WALLET_NOT_FOUND = 9001,
        WALLET_LOCKED = 9002,
        INVALID_PASSWORD = 9003,
        INSUFFICIENT_FUNDS = 9004
    };

    /**
     * @brief Construct a new Neo Exception
     * @param code Error code
     * @param message Descriptive error message
     * @param context Additional context information
     */
    NeoException(ErrorCode code, const std::string& message, const std::string& context = "")
        : std::runtime_error(FormatMessage(code, message, context)),
          error_code_(code),
          original_message_(message),
          context_(context)
    {
    }

    /**
     * @brief Get the error code
     * @return Error code enum value
     */
    ErrorCode GetErrorCode() const noexcept { return error_code_; }

    /**
     * @brief Get the original message without formatting
     * @return Original error message
     */
    const std::string& GetOriginalMessage() const noexcept { return original_message_; }

    /**
     * @brief Get the context information
     * @return Context string
     */
    const std::string& GetContext() const noexcept { return context_; }

    /**
     * @brief Convert error code to string
     * @param code Error code to convert
     * @return String representation of error code
     */
    static std::string ErrorCodeToString(ErrorCode code);

   private:
    ErrorCode error_code_;
    std::string original_message_;
    std::string context_;

    static std::string FormatMessage(ErrorCode code, const std::string& message, const std::string& context);
};

// Specific exception classes for different categories

/**
 * @brief Exception for serialization/deserialization errors
 */
class SerializationException : public NeoException
{
   public:
    SerializationException(const std::string& message, const std::string& context = "")
        : NeoException(ErrorCode::SERIALIZATION_ERROR, message, context)
    {
    }

    SerializationException(ErrorCode code, const std::string& message, const std::string& context = "")
        : NeoException(code, message, context)
    {
    }
};

/**
 * @brief Exception for cryptographic operations
 */
class CryptographyException : public NeoException
{
   public:
    CryptographyException(const std::string& message, const std::string& context = "")
        : NeoException(ErrorCode::CRYPTO_ERROR, message, context)
    {
    }

    CryptographyException(ErrorCode code, const std::string& message, const std::string& context = "")
        : NeoException(code, message, context)
    {
    }
};

/**
 * @brief Exception for network operations
 */
class NetworkException : public NeoException
{
   public:
    NetworkException(const std::string& message, const std::string& context = "")
        : NeoException(ErrorCode::NETWORK_ERROR, message, context)
    {
    }

    NetworkException(ErrorCode code, const std::string& message, const std::string& context = "")
        : NeoException(code, message, context)
    {
    }
};

/**
 * @brief Exception for blockchain operations
 */
class BlockchainException : public NeoException
{
   public:
    BlockchainException(const std::string& message, const std::string& context = "")
        : NeoException(ErrorCode::BLOCKCHAIN_ERROR, message, context)
    {
    }

    BlockchainException(ErrorCode code, const std::string& message, const std::string& context = "")
        : NeoException(code, message, context)
    {
    }
};

/**
 * @brief Exception for storage operations
 */
class StorageException : public NeoException
{
   public:
    StorageException(const std::string& message, const std::string& context = "")
        : NeoException(ErrorCode::STORAGE_ERROR, message, context)
    {
    }

    StorageException(ErrorCode code, const std::string& message, const std::string& context = "")
        : NeoException(code, message, context)
    {
    }
};

/**
 * @brief Exception for VM operations
 */
class VMException : public NeoException
{
   public:
    VMException(const std::string& message, const std::string& context = "")
        : NeoException(ErrorCode::VM_ERROR, message, context)
    {
    }

    VMException(ErrorCode code, const std::string& message, const std::string& context = "")
        : NeoException(code, message, context)
    {
    }
};

/**
 * @brief Exception for smart contract operations
 */
class ContractException : public NeoException
{
   public:
    ContractException(const std::string& message, const std::string& context = "")
        : NeoException(ErrorCode::CONTRACT_ERROR, message, context)
    {
    }

    ContractException(ErrorCode code, const std::string& message, const std::string& context = "")
        : NeoException(code, message, context)
    {
    }
};

/**
 * @brief Exception for wallet operations
 */
class WalletException : public NeoException
{
   public:
    WalletException(const std::string& message, const std::string& context = "")
        : NeoException(ErrorCode::WALLET_ERROR, message, context)
    {
    }

    WalletException(ErrorCode code, const std::string& message, const std::string& context = "")
        : NeoException(code, message, context)
    {
    }
};

// Utility macros for throwing exceptions with context
#define THROW_NEO_EXCEPTION(code, message) \
    throw neo::core::NeoException(code, message, __FILE__ ":" + std::to_string(__LINE__))

#define THROW_SERIALIZATION_EXCEPTION(message) \
    throw neo::core::SerializationException(message, __FILE__ ":" + std::to_string(__LINE__))

#define THROW_CRYPTO_EXCEPTION(message) \
    throw neo::core::CryptographyException(message, __FILE__ ":" + std::to_string(__LINE__))

#define THROW_NETWORK_EXCEPTION(message) \
    throw neo::core::NetworkException(message, __FILE__ ":" + std::to_string(__LINE__))

#define THROW_BLOCKCHAIN_EXCEPTION(message) \
    throw neo::core::BlockchainException(message, __FILE__ ":" + std::to_string(__LINE__))

#define THROW_STORAGE_EXCEPTION(message) \
    throw neo::core::StorageException(message, __FILE__ ":" + std::to_string(__LINE__))

#define THROW_VM_EXCEPTION(message) throw neo::core::VMException(message, __FILE__ ":" + std::to_string(__LINE__))

#define THROW_CONTRACT_EXCEPTION(message) \
    throw neo::core::ContractException(message, __FILE__ ":" + std::to_string(__LINE__))

#define THROW_WALLET_EXCEPTION(message) \
    throw neo::core::WalletException(message, __FILE__ ":" + std::to_string(__LINE__))

}  // namespace neo::core