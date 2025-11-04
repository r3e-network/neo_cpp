#include "neo/rpc/error_codes.h"
#include <nlohmann/json.hpp>

namespace neo::rpc {

const std::unordered_map<ErrorCode, std::string> ErrorCodes::error_messages_ = {
    // Standard JSON-RPC 2.0 errors
    {ErrorCode::ParseError, "Parse error"},
    {ErrorCode::InvalidRequest, "Invalid Request"},
    {ErrorCode::MethodNotFound, "Method not found"},
    {ErrorCode::InvalidParams, "Invalid params"},
    {ErrorCode::InternalError, "Internal error"},
    
    // Neo-specific error codes
    {ErrorCode::UnknownBlock, "Unknown block"},
    {ErrorCode::UnknownTransaction, "Unknown transaction"},
    {ErrorCode::UnknownContract, "Unknown contract"},
    {ErrorCode::UnknownStorageItem, "Unknown storage item"},
    {ErrorCode::UnknownScriptContainer, "Unknown script container"},
    {ErrorCode::UnknownService, "Unknown service"},
    {ErrorCode::UnknownValidator, "Unknown validator"},
    {ErrorCode::UnknownCommittee, "Unknown committee"},
    {ErrorCode::UnknownSession, "Unknown session"},
    {ErrorCode::UnknownIterator, "Unknown iterator"},
    
    // Blockchain errors
    {ErrorCode::BlockchainNotAvailable, "Blockchain not available"},
    {ErrorCode::MemoryPoolNotAvailable, "Memory pool not available"},
    {ErrorCode::InvalidBlockIndex, "Invalid block index"},
    {ErrorCode::InvalidTransactionHash, "Invalid transaction hash"},
    {ErrorCode::InvalidContractHash, "Invalid contract hash"},
    {ErrorCode::InvalidAddress, "Invalid address"},
    {ErrorCode::InvalidScript, "Invalid script"},
    {ErrorCode::InvalidSignature, "Invalid signature"},
    {ErrorCode::InvalidWitness, "Invalid witness"},
    {ErrorCode::InvalidAttribute, "Invalid attribute"},
    
    // Transaction errors
    {ErrorCode::InsufficientFunds, "Insufficient funds"},
    {ErrorCode::TransactionExpired, "Transaction expired"},
    {ErrorCode::TransactionTooLarge, "Transaction too large"},
    {ErrorCode::TransactionAttributesTooLarge, "Transaction attributes too large"},
    {ErrorCode::TransactionScriptTooLarge, "Transaction script too large"},
    {ErrorCode::TransactionAlreadyExists, "Transaction already exists"},
    {ErrorCode::TransactionVerificationFailed, "Transaction verification failed"},
    {ErrorCode::TransactionPolicyViolation, "Transaction policy violation"},
    {ErrorCode::TransactionConflict, "Transaction conflict"},
    {ErrorCode::RpcVerificationFailed, "Inventory verification failed"},
    {ErrorCode::RpcAlreadyExists, "Inventory already exists"},
    {ErrorCode::RpcMempoolCapReached, "Memory pool capacity reached"},
    {ErrorCode::RpcAlreadyInPool, "Inventory already in pool"},
    {ErrorCode::RpcInsufficientNetworkFee, "Insufficient network fee"},
    {ErrorCode::RpcPolicyFailed, "Policy check failed"},
    {ErrorCode::RpcInvalidTransactionScript, "Invalid transaction script"},
    {ErrorCode::RpcInvalidTransactionAttribute, "Invalid transaction attribute"},
    {ErrorCode::RpcInvalidSignature, "Invalid signature"},
    {ErrorCode::RpcInvalidInventorySize, "Invalid inventory size"},
    {ErrorCode::RpcExpiredTransaction, "Expired transaction"},
    {ErrorCode::RpcInsufficientFunds, "Insufficient funds for fee"},
    {ErrorCode::RpcInvalidContractVerification, "Invalid contract verification"},
    
    // Contract errors
    {ErrorCode::ContractNotFound, "Contract not found"},
    {ErrorCode::ContractInvocationFailed, "Contract invocation failed"},
    {ErrorCode::ContractExecutionFailed, "Contract execution failed"},
    {ErrorCode::InsufficientGas, "Insufficient GAS"},
    {ErrorCode::StackOverflow, "Stack overflow"},
    {ErrorCode::StackUnderflow, "Stack underflow"},
    {ErrorCode::InvalidOperation, "Invalid operation"},
    {ErrorCode::OutOfGas, "Out of gas"},
    
    // Network errors
    {ErrorCode::NetworkError, "Network error"},
    {ErrorCode::PeerDisconnected, "Peer disconnected"},
    {ErrorCode::InvalidNetworkMagic, "Invalid network magic"},
    {ErrorCode::InvalidProtocolVersion, "Invalid protocol version"},
    {ErrorCode::InvalidMessage, "Invalid message"},
    {ErrorCode::ConsensusError, "Consensus error"},
    
    // Wallet errors
    {ErrorCode::WalletNotFound, "Wallet not found"},
    {ErrorCode::WalletLocked, "Wallet is locked"},
    {ErrorCode::WalletUnlockFailed, "Wallet unlock failed"},
    {ErrorCode::InvalidPassword, "Invalid password"},
    {ErrorCode::InsufficientPrivileges, "Insufficient privileges"},
    {ErrorCode::KeyNotFound, "Key not found"},
    {ErrorCode::AddressNotInWallet, "Address not in wallet"},
    
    // Plugin errors
    {ErrorCode::PluginNotFound, "Plugin not found"},
    {ErrorCode::PluginDisabled, "Plugin disabled"},
    {ErrorCode::PluginError, "Plugin error"},
    
    // Storage errors
    {ErrorCode::StorageError, "Storage error"},
    {ErrorCode::DatabaseError, "Database error"},
    {ErrorCode::InvalidKey, "Invalid key"},
    {ErrorCode::StorageKeyNotFound, "Key not found"},
    
    // Security errors
    {ErrorCode::AccessDenied, "Access denied"},
    {ErrorCode::AuthenticationFailed, "Authentication failed"},
    {ErrorCode::AuthorizationFailed, "Authorization failed"},
    {ErrorCode::RateLimitExceeded, "Rate limit exceeded"},
    
    // Oracle errors
    {ErrorCode::OracleNotEnabled, "Oracle service not enabled"},
    {ErrorCode::OracleRequestNotFound, "Oracle request not found"},
    {ErrorCode::OracleResponseTimeout, "Oracle response timeout"},
    
    // Application errors
    {ErrorCode::ApplicationNotFound, "Application not found"},
    {ErrorCode::ApplicationExecutionFailed, "Application execution failed"},
    {ErrorCode::ApplicationLogNotFound, "Application log not found"},
    
    // State service errors
    {ErrorCode::StateServiceNotEnabled, "State service not enabled"},
    {ErrorCode::StateNotFound, "State not found"},
    {ErrorCode::StateValidationFailed, "State validation failed"}
};

const std::unordered_map<ErrorCode, std::string> ErrorCodes::error_categories_ = {
    // Standard JSON-RPC errors
    {ErrorCode::ParseError, "JSON-RPC"},
    {ErrorCode::InvalidRequest, "JSON-RPC"},
    {ErrorCode::MethodNotFound, "JSON-RPC"},
    {ErrorCode::InvalidParams, "JSON-RPC"},
    {ErrorCode::InternalError, "JSON-RPC"},
    
    // Neo-specific errors by category
    {ErrorCode::UnknownBlock, "Unknown"},
    {ErrorCode::UnknownTransaction, "Unknown"},
    {ErrorCode::UnknownContract, "Unknown"},
    {ErrorCode::UnknownStorageItem, "Unknown"},
    {ErrorCode::UnknownScriptContainer, "Unknown"},
    {ErrorCode::UnknownService, "Unknown"},
    {ErrorCode::UnknownValidator, "Unknown"},
    {ErrorCode::UnknownCommittee, "Unknown"},
    {ErrorCode::UnknownSession, "Unknown"},
    {ErrorCode::UnknownIterator, "Unknown"},
    
    // Blockchain category
    {ErrorCode::BlockchainNotAvailable, "Blockchain"},
    {ErrorCode::MemoryPoolNotAvailable, "Blockchain"},
    {ErrorCode::InvalidBlockIndex, "Blockchain"},
    {ErrorCode::InvalidTransactionHash, "Blockchain"},
    {ErrorCode::InvalidContractHash, "Blockchain"},
    {ErrorCode::InvalidAddress, "Blockchain"},
    {ErrorCode::InvalidScript, "Blockchain"},
    {ErrorCode::InvalidSignature, "Blockchain"},
    {ErrorCode::InvalidWitness, "Blockchain"},
    {ErrorCode::InvalidAttribute, "Blockchain"},
    
    // Transaction category
    {ErrorCode::InsufficientFunds, "Transaction"},
    {ErrorCode::TransactionExpired, "Transaction"},
    {ErrorCode::TransactionTooLarge, "Transaction"},
    {ErrorCode::TransactionAttributesTooLarge, "Transaction"},
    {ErrorCode::TransactionScriptTooLarge, "Transaction"},
    {ErrorCode::TransactionAlreadyExists, "Transaction"},
    {ErrorCode::TransactionVerificationFailed, "Transaction"},
    {ErrorCode::TransactionPolicyViolation, "Transaction"},
    {ErrorCode::TransactionConflict, "Transaction"},
    {ErrorCode::RpcVerificationFailed, "Transaction"},
    {ErrorCode::RpcAlreadyExists, "Transaction"},
    {ErrorCode::RpcMempoolCapReached, "Transaction"},
    {ErrorCode::RpcAlreadyInPool, "Transaction"},
    {ErrorCode::RpcInsufficientNetworkFee, "Transaction"},
    {ErrorCode::RpcPolicyFailed, "Transaction"},
    {ErrorCode::RpcInvalidTransactionScript, "Transaction"},
    {ErrorCode::RpcInvalidTransactionAttribute, "Transaction"},
    {ErrorCode::RpcInvalidSignature, "Transaction"},
    {ErrorCode::RpcInvalidInventorySize, "Transaction"},
    {ErrorCode::RpcExpiredTransaction, "Transaction"},
    {ErrorCode::RpcInsufficientFunds, "Transaction"},
    {ErrorCode::RpcInvalidContractVerification, "Transaction"},
    
    // Contract category
    {ErrorCode::ContractNotFound, "Contract"},
    {ErrorCode::ContractInvocationFailed, "Contract"},
    {ErrorCode::ContractExecutionFailed, "Contract"},
    {ErrorCode::InsufficientGas, "Contract"},
    {ErrorCode::StackOverflow, "Contract"},
    {ErrorCode::StackUnderflow, "Contract"},
    {ErrorCode::InvalidOperation, "Contract"},
    {ErrorCode::OutOfGas, "Contract"},
    
    // Network category
    {ErrorCode::NetworkError, "Network"},
    {ErrorCode::PeerDisconnected, "Network"},
    {ErrorCode::InvalidNetworkMagic, "Network"},
    {ErrorCode::InvalidProtocolVersion, "Network"},
    {ErrorCode::InvalidMessage, "Network"},
    {ErrorCode::ConsensusError, "Network"},
    
    // Wallet category
    {ErrorCode::WalletNotFound, "Wallet"},
    {ErrorCode::WalletLocked, "Wallet"},
    {ErrorCode::WalletUnlockFailed, "Wallet"},
    {ErrorCode::InvalidPassword, "Wallet"},
    {ErrorCode::InsufficientPrivileges, "Wallet"},
    {ErrorCode::KeyNotFound, "Wallet"},
    {ErrorCode::AddressNotInWallet, "Wallet"},
    
    // Plugin category
    {ErrorCode::PluginNotFound, "Plugin"},
    {ErrorCode::PluginDisabled, "Plugin"},
    {ErrorCode::PluginError, "Plugin"},
    
    // Storage category
    {ErrorCode::StorageError, "Storage"},
    {ErrorCode::DatabaseError, "Storage"},
    {ErrorCode::InvalidKey, "Storage"},
    {ErrorCode::StorageKeyNotFound, "Storage"},
    
    // Security category
    {ErrorCode::AccessDenied, "Security"},
    {ErrorCode::AuthenticationFailed, "Security"},
    {ErrorCode::AuthorizationFailed, "Security"},
    {ErrorCode::RateLimitExceeded, "Security"},
    
    // Oracle category
    {ErrorCode::OracleNotEnabled, "Oracle"},
    {ErrorCode::OracleRequestNotFound, "Oracle"},
    {ErrorCode::OracleResponseTimeout, "Oracle"},
    
    // Application category
    {ErrorCode::ApplicationNotFound, "Application"},
    {ErrorCode::ApplicationExecutionFailed, "Application"},
    {ErrorCode::ApplicationLogNotFound, "Application"},
    
    // State service category
    {ErrorCode::StateServiceNotEnabled, "StateService"},
    {ErrorCode::StateNotFound, "StateService"},
    {ErrorCode::StateValidationFailed, "StateService"}
};

std::string ErrorCodes::GetErrorMessage(ErrorCode code) {
    auto it = error_messages_.find(code);
    if (it != error_messages_.end()) {
        return it->second;
    }
    return "Unknown error";
}

bool ErrorCodes::IsStandardError(ErrorCode code) {
    int32_t code_value = static_cast<int32_t>(code);
    return code_value >= -32768 && code_value <= -32000;
}

bool ErrorCodes::IsNeoError(ErrorCode code) {
    return !IsStandardError(code);
}

std::string ErrorCodes::GetErrorCategory(ErrorCode code) {
    auto it = error_categories_.find(code);
    if (it != error_categories_.end()) {
        return it->second;
    }
    return "Unknown";
}

nlohmann::json ErrorCodes::CreateErrorObject(ErrorCode code, 
                                            const std::string& message,
                                            const nlohmann::json& data) {
    nlohmann::json error;
    error["code"] = static_cast<int32_t>(code);
    error["message"] = message.empty() ? GetErrorMessage(code) : message;
    
    if (!data.is_null()) {
        error["data"] = data;
    }
    
    return error;
}

} // namespace neo::rpc
