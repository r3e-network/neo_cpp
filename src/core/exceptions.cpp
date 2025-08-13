/**
 * @file exceptions.cpp
 * @brief Exception types
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/exceptions.h>

#include <sstream>

namespace neo::core
{

std::string NeoException::ErrorCodeToString(ErrorCode code)
{
    static const std::unordered_map<ErrorCode, std::string> error_code_names = {
        // General errors
        {ErrorCode::UNKNOWN_ERROR, "UNKNOWN_ERROR"},
        {ErrorCode::INVALID_ARGUMENT, "INVALID_ARGUMENT"},
        {ErrorCode::INVALID_STATE, "INVALID_STATE"},
        {ErrorCode::NOT_AVAILABLE, "NOT_AVAILABLE"},
        {ErrorCode::OUT_OF_RANGE, "OUT_OF_RANGE"},
        {ErrorCode::TIMEOUT, "TIMEOUT"},

        // Serialization errors
        {ErrorCode::SERIALIZATION_ERROR, "SERIALIZATION_ERROR"},
        {ErrorCode::DESERIALIZATION_ERROR, "DESERIALIZATION_ERROR"},
        {ErrorCode::INVALID_FORMAT, "INVALID_FORMAT"},
        {ErrorCode::BUFFER_OVERFLOW, "BUFFER_OVERFLOW"},
        {ErrorCode::BUFFER_UNDERFLOW, "BUFFER_UNDERFLOW"},

        // Cryptography errors
        {ErrorCode::CRYPTO_ERROR, "CRYPTO_ERROR"},
        {ErrorCode::INVALID_KEY, "INVALID_KEY"},
        {ErrorCode::INVALID_SIGNATURE, "INVALID_SIGNATURE"},
        {ErrorCode::HASH_CALCULATION_FAILED, "HASH_CALCULATION_FAILED"},
        {ErrorCode::ENCRYPTION_FAILED, "ENCRYPTION_FAILED"},
        {ErrorCode::DECRYPTION_FAILED, "DECRYPTION_FAILED"},

        // Network errors
        {ErrorCode::NETWORK_ERROR, "NETWORK_ERROR"},
        {ErrorCode::CONNECTION_FAILED, "CONNECTION_FAILED"},
        {ErrorCode::INVALID_MESSAGE, "INVALID_MESSAGE"},
        {ErrorCode::PROTOCOL_VIOLATION, "PROTOCOL_VIOLATION"},
        {ErrorCode::PEER_DISCONNECTED, "PEER_DISCONNECTED"},

        // Blockchain errors
        {ErrorCode::BLOCKCHAIN_ERROR, "BLOCKCHAIN_ERROR"},
        {ErrorCode::INVALID_BLOCK, "INVALID_BLOCK"},
        {ErrorCode::INVALID_TRANSACTION, "INVALID_TRANSACTION"},
        {ErrorCode::CONSENSUS_FAILURE, "CONSENSUS_FAILURE"},
        {ErrorCode::VERIFICATION_FAILED, "VERIFICATION_FAILED"},
        {ErrorCode::INVALID_CONTRACT, "INVALID_CONTRACT"},

        // Storage errors
        {ErrorCode::STORAGE_ERROR, "STORAGE_ERROR"},
        {ErrorCode::KEY_NOT_FOUND, "KEY_NOT_FOUND"},
        {ErrorCode::STORAGE_CORRUPTION, "STORAGE_CORRUPTION"},
        {ErrorCode::INSUFFICIENT_SPACE, "INSUFFICIENT_SPACE"},
        {ErrorCode::ACCESS_DENIED, "ACCESS_DENIED"},

        // VM errors
        {ErrorCode::VM_ERROR, "VM_ERROR"},
        {ErrorCode::STACK_OVERFLOW, "STACK_OVERFLOW"},
        {ErrorCode::STACK_UNDERFLOW, "STACK_UNDERFLOW"},
        {ErrorCode::INVALID_OPCODE, "INVALID_OPCODE"},
        {ErrorCode::EXECUTION_TIMEOUT, "EXECUTION_TIMEOUT"},
        {ErrorCode::OUT_OF_GAS, "OUT_OF_GAS"},

        // Smart contract errors
        {ErrorCode::CONTRACT_ERROR, "CONTRACT_ERROR"},
        {ErrorCode::CONTRACT_NOT_FOUND, "CONTRACT_NOT_FOUND"},
        {ErrorCode::CONTRACT_EXECUTION_FAILED, "CONTRACT_EXECUTION_FAILED"},
        {ErrorCode::INVALID_CONTRACT_STATE, "INVALID_CONTRACT_STATE"},

        // Wallet errors
        {ErrorCode::WALLET_ERROR, "WALLET_ERROR"},
        {ErrorCode::WALLET_NOT_FOUND, "WALLET_NOT_FOUND"},
        {ErrorCode::WALLET_LOCKED, "WALLET_LOCKED"},
        {ErrorCode::INVALID_PASSWORD, "INVALID_PASSWORD"},
        {ErrorCode::INSUFFICIENT_FUNDS, "INSUFFICIENT_FUNDS"}};

    auto it = error_code_names.find(code);
    if (it != error_code_names.end())
    {
        return it->second;
    }

    return "UNKNOWN_ERROR_CODE(" + std::to_string(static_cast<uint32_t>(code)) + ")";
}

std::string NeoException::FormatMessage(ErrorCode code, const std::string& message, const std::string& context)
{
    std::ostringstream oss;
    oss << "[" << ErrorCodeToString(code) << ":" << static_cast<uint32_t>(code) << "] " << message;

    if (!context.empty())
    {
        oss << " (at " << context << ")";
    }

    return oss.str();
}

}  // namespace neo::core