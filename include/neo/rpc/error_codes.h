#pragma once

#include <string>
#include <unordered_map>

namespace neo::rpc {

/**
 * @brief Standard JSON-RPC 2.0 error codes compatible with Neo C# implementation
 */
enum class ErrorCode : int32_t {
    // Standard JSON-RPC 2.0 errors
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    
    // Neo-specific error codes (compatible with C# implementation)
    UnknownBlock = -100,
    UnknownTransaction = -101,
    UnknownContract = -102,
    UnknownStorageItem = -103,
    UnknownScriptContainer = -104,
    UnknownService = -105,
    UnknownValidator = -106,
    UnknownCommittee = -107,
    
    // Blockchain errors
    BlockchainNotAvailable = -200,
    MemoryPoolNotAvailable = -201,
    InvalidBlockIndex = -202,
    InvalidTransactionHash = -203,
    InvalidContractHash = -204,
    InvalidAddress = -205,
    InvalidScript = -206,
    InvalidSignature = -207,
    InvalidWitness = -208,
    InvalidAttribute = -209,
    
    // Transaction errors
    InsufficientFunds = -300,
    TransactionExpired = -301,
    TransactionTooLarge = -302,
    TransactionAttributesTooLarge = -303,
    TransactionScriptTooLarge = -304,
    TransactionAlreadyExists = -305,
    TransactionVerificationFailed = -306,
    TransactionPolicyViolation = -307,
    TransactionConflict = -308,
    
    // Contract errors
    ContractNotFound = -400,
    ContractInvocationFailed = -401,
    ContractExecutionFailed = -402,
    InsufficientGas = -403,
    StackOverflow = -404,
    StackUnderflow = -405,
    InvalidOperation = -406,
    OutOfGas = -407,
    
    // Network errors
    NetworkError = -500,
    PeerDisconnected = -501,
    InvalidNetworkMagic = -502,
    InvalidProtocolVersion = -503,
    InvalidMessage = -504,
    ConsensusError = -505,
    
    // Wallet errors
    WalletNotFound = -600,
    WalletLocked = -601,
    WalletUnlockFailed = -602,
    InvalidPassword = -603,
    InsufficientPrivileges = -604,
    KeyNotFound = -605,
    AddressNotInWallet = -606,
    
    // Plugin errors
    PluginNotFound = -700,
    PluginDisabled = -701,
    PluginError = -702,
    
    // Storage errors
    StorageError = -800,
    DatabaseError = -801,
    InvalidKey = -802,
    KeyNotFound = -803,
    
    // Security errors
    AccessDenied = -900,
    AuthenticationFailed = -901,
    AuthorizationFailed = -902,
    RateLimitExceeded = -903,
    
    // Oracle errors
    OracleNotEnabled = -1000,
    OracleRequestNotFound = -1001,
    OracleResponseTimeout = -1002,
    
    // Application errors
    ApplicationNotFound = -1100,
    ApplicationExecutionFailed = -1101,
    ApplicationLogNotFound = -1102,
    
    // State service errors
    StateServiceNotEnabled = -1200,
    StateNotFound = -1201,
    StateValidationFailed = -1202
};

/**
 * @brief Error code utilities for consistent error handling
 */
class ErrorCodes {
public:
    /**
     * @brief Get error message for error code
     * @param code Error code
     * @return Error message string
     */
    static std::string GetErrorMessage(ErrorCode code);
    
    /**
     * @brief Check if error code is a standard JSON-RPC error
     * @param code Error code
     * @return true if standard JSON-RPC error
     */
    static bool IsStandardError(ErrorCode code);
    
    /**
     * @brief Check if error code is a Neo-specific error
     * @param code Error code
     * @return true if Neo-specific error
     */
    static bool IsNeoError(ErrorCode code);
    
    /**
     * @brief Get error category name
     * @param code Error code
     * @return Category name
     */
    static std::string GetErrorCategory(ErrorCode code);
    
    /**
     * @brief Create JSON-RPC error object
     * @param code Error code
     * @param message Optional custom message
     * @param data Optional error data
     * @return JSON error object
     */
    static nlohmann::json CreateErrorObject(ErrorCode code, 
                                          const std::string& message = "",
                                          const nlohmann::json& data = nlohmann::json());

private:
    static const std::unordered_map<ErrorCode, std::string> error_messages_;
    static const std::unordered_map<ErrorCode, std::string> error_categories_;
};

/**
 * @brief RPC exception with error code
 */
class RpcException : public std::exception {
public:
    RpcException(ErrorCode code, const std::string& message = "")
        : code_(code), message_(message.empty() ? ErrorCodes::GetErrorMessage(code) : message) {}
    
    const char* what() const noexcept override {
        return message_.c_str();
    }
    
    ErrorCode GetCode() const { return code_; }
    const std::string& GetMessage() const { return message_; }
    
    nlohmann::json ToJson() const {
        return ErrorCodes::CreateErrorObject(code_, message_);
    }

private:
    ErrorCode code_;
    std::string message_;
};

} // namespace neo::rpc