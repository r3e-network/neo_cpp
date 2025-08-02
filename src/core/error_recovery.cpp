#include <neo/core/error_recovery.h>
#include <neo/core/exceptions.h>

namespace neo::core
{

bool ErrorRecovery::IsRetriableException(const std::exception& e)
{
    // Check if this is a Neo exception with a retriable error code
    if (const auto* neo_ex = dynamic_cast<const NeoException*>(&e)) {
        switch (neo_ex->GetErrorCode()) {
            // Network errors are usually retriable
            case NeoException::ErrorCode::NETWORK_ERROR:
            case NeoException::ErrorCode::CONNECTION_FAILED:
            case NeoException::ErrorCode::PEER_DISCONNECTED:
            case NeoException::ErrorCode::TIMEOUT:
            
            // Some storage errors might be retriable
            case NeoException::ErrorCode::STORAGE_ERROR:
            case NeoException::ErrorCode::ACCESS_DENIED:
            
            // Temporary VM issues
            case NeoException::ErrorCode::EXECUTION_TIMEOUT:
            
            // Blockchain temporary issues
            case NeoException::ErrorCode::CONSENSUS_FAILURE:
                return true;
                
            // These are generally not retriable
            case NeoException::ErrorCode::INVALID_ARGUMENT:
            case NeoException::ErrorCode::INVALID_FORMAT:
            case NeoException::ErrorCode::INVALID_SIGNATURE:
            case NeoException::ErrorCode::INVALID_KEY:
            case NeoException::ErrorCode::INVALID_BLOCK:
            case NeoException::ErrorCode::INVALID_TRANSACTION:
            case NeoException::ErrorCode::SERIALIZATION_ERROR:
            case NeoException::ErrorCode::DESERIALIZATION_ERROR:
                return false;
                
            default:
                return false;
        }
    }
    
    // For standard exceptions, be conservative
    return false;
}

bool ErrorRecovery::IsTransientException(const std::exception& e)
{
    if (const auto* neo_ex = dynamic_cast<const NeoException*>(&e)) {
        switch (neo_ex->GetErrorCode()) {
            // These are typically transient
            case NeoException::ErrorCode::TIMEOUT:
            case NeoException::ErrorCode::CONNECTION_FAILED:
            case NeoException::ErrorCode::PEER_DISCONNECTED:
            case NeoException::ErrorCode::NETWORK_ERROR:
            case NeoException::ErrorCode::INSUFFICIENT_SPACE:
            case NeoException::ErrorCode::ACCESS_DENIED:
            case NeoException::ErrorCode::EXECUTION_TIMEOUT:
                return true;
                
            default:
                return false;
        }
    }
    
    return false;
}

bool ErrorRecovery::IsFatalException(const std::exception& e)
{
    if (const auto* neo_ex = dynamic_cast<const NeoException*>(&e)) {
        switch (neo_ex->GetErrorCode()) {
            // These indicate fundamental problems that won't resolve
            case NeoException::ErrorCode::STORAGE_CORRUPTION:
            case NeoException::ErrorCode::INVALID_CONTRACT_STATE:
            case NeoException::ErrorCode::BUFFER_OVERFLOW:
            case NeoException::ErrorCode::BUFFER_UNDERFLOW:
            case NeoException::ErrorCode::OUT_OF_RANGE:
                return true;
                
            default:
                return false;
        }
    }
    
    // Unknown exceptions are considered potentially fatal
    return true;
}

ErrorRecovery::RetryConfig ErrorRecovery::NetworkRetryConfig()
{
    RetryConfig config;
    config.max_attempts = 5;
    config.base_delay = std::chrono::milliseconds(200);
    config.backoff_multiplier = 2.0;
    config.max_delay = std::chrono::milliseconds(10000);
    config.exponential_backoff = true;
    
    config.should_retry = [](const std::exception& e) {
        return IsRetriableException(e) || IsTransientException(e);
    };
    
    return config;
}

ErrorRecovery::RetryConfig ErrorRecovery::DatabaseRetryConfig()
{
    RetryConfig config;
    config.max_attempts = 3;
    config.base_delay = std::chrono::milliseconds(100);
    config.backoff_multiplier = 1.5;
    config.max_delay = std::chrono::milliseconds(2000);
    config.exponential_backoff = true;
    
    config.should_retry = [](const std::exception& e) {
        if (IsFatalException(e)) {
            return false;
        }
        return IsRetriableException(e) || IsTransientException(e);
    };
    
    return config;
}

ErrorRecovery::RetryConfig ErrorRecovery::FileOperationRetryConfig()
{
    RetryConfig config;
    config.max_attempts = 3;
    config.base_delay = std::chrono::milliseconds(50);
    config.backoff_multiplier = 2.0;
    config.max_delay = std::chrono::milliseconds(1000);
    config.exponential_backoff = true;
    
    config.should_retry = [](const std::exception& e) {
        if (const auto* neo_ex = dynamic_cast<const NeoException*>(&e)) {
            // File operations might fail due to temporary access issues
            switch (neo_ex->GetErrorCode()) {
                case NeoException::ErrorCode::ACCESS_DENIED:
                case NeoException::ErrorCode::STORAGE_ERROR:
                    return true;
                default:
                    return false;
            }
        }
        return false;
    };
    
    return config;
}

ErrorRecovery::RetryConfig ErrorRecovery::CryptographyRetryConfig()
{
    RetryConfig config;
    config.max_attempts = 2; // Crypto operations usually succeed or fail deterministically
    config.base_delay = std::chrono::milliseconds(10);
    config.backoff_multiplier = 1.0; // No backoff for crypto
    config.max_delay = std::chrono::milliseconds(10);
    config.exponential_backoff = false;
    
    config.should_retry = [](const std::exception& e) {
        if (const auto* neo_ex = dynamic_cast<const NeoException*>(&e)) {
            // Only retry on very specific crypto errors that might be transient
            switch (neo_ex->GetErrorCode()) {
                case NeoException::ErrorCode::HASH_CALCULATION_FAILED:
                    return true; // Might be due to temporary resource issues
                default:
                    return false; // Most crypto errors are deterministic
            }
        }
        return false;
    };
    
    return config;
}

} // namespace neo::core