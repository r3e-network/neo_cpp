#pragma once

#include <neo/core/exceptions.h>
#include <functional>
#include <vector>
#include <chrono>
#include <thread>
#include <optional>
#include <future>
#include <mutex>

namespace neo::core
{
/**
 * @brief Comprehensive error recovery framework for Neo C++ implementation
 * 
 * Provides robust error handling strategies including retry logic,
 * circuit breakers, fallback mechanisms, and graceful degradation
 * to ensure production-ready reliability and fault tolerance.
 */
class ErrorRecovery
{
public:
    /**
     * @brief Result of an error recovery operation
     */
    template<typename T>
    struct RecoveryResult
    {
        bool success;
        std::optional<T> value;
        std::string error_message;
        NeoException::ErrorCode error_code;
        int attempts_made;
        
        RecoveryResult(bool succeeded = false)
            : success(succeeded), error_code(NeoException::ErrorCode::UNKNOWN_ERROR), attempts_made(0) {}
            
        RecoveryResult(const T& val, int attempts = 1)
            : success(true), value(val), attempts_made(attempts) {}
            
        RecoveryResult(const std::string& error, NeoException::ErrorCode code, int attempts)
            : success(false), error_message(error), error_code(code), attempts_made(attempts) {}
            
        operator bool() const { return success; }
    };

    /**
     * @brief Configuration for retry operations
     */
    struct RetryConfig
    {
        int max_attempts;
        std::chrono::milliseconds base_delay;
        double backoff_multiplier;
        std::chrono::milliseconds max_delay;
        bool exponential_backoff;
        
        // Function to determine if an exception should trigger a retry
        std::function<bool(const std::exception&)> should_retry;
        
        RetryConfig() : max_attempts(3),
                       base_delay(100),
                       backoff_multiplier(2.0),
                       max_delay(5000),
                       exponential_backoff(true),
                       should_retry([](const std::exception&) { return true; }) {}
    };

    /**
     * @brief Execute a function with retry logic
     * @param operation Function to execute
     * @param config Retry configuration
     * @return Recovery result with success/failure information
     */
    template<typename T>
    static RecoveryResult<T> Retry(std::function<T()> operation, const RetryConfig& config = RetryConfig())
    {
        std::string last_error;
        NeoException::ErrorCode last_error_code = NeoException::ErrorCode::UNKNOWN_ERROR;
        
        for (int attempt = 1; attempt <= config.max_attempts; ++attempt) {
            try {
                T result = operation();
                return RecoveryResult<T>(result, attempt);
            } catch (const NeoException& e) {
                last_error = e.what();
                last_error_code = e.GetErrorCode();
                
                if (!config.should_retry(e) || attempt == config.max_attempts) {
                    break;
                }
            } catch (const std::exception& e) {
                last_error = e.what();
                
                if (!config.should_retry(e) || attempt == config.max_attempts) {
                    break;
                }
            }
            
            // Calculate delay for next attempt
            if (attempt < config.max_attempts) {
                auto delay = config.base_delay;
                if (config.exponential_backoff) {
                    for (int i = 1; i < attempt; ++i) {
                        delay = std::chrono::milliseconds(
                            static_cast<long long>(delay.count() * config.backoff_multiplier));
                    }
                }
                delay = std::min(delay, config.max_delay);
                std::this_thread::sleep_for(delay);
            }
        }
        
        return RecoveryResult<T>(last_error, last_error_code, config.max_attempts);
    }

    /**
     * @brief Execute a function with fallback on failure
     * @param primary_operation Primary function to try
     * @param fallback_operation Fallback function if primary fails
     * @return Recovery result
     */
    template<typename T>
    static RecoveryResult<T> WithFallback(std::function<T()> primary_operation, 
                                         std::function<T()> fallback_operation)
    {
        try {
            T result = primary_operation();
            return RecoveryResult<T>(result);
        } catch (const NeoException& e) {
            try {
                T fallback_result = fallback_operation();
                return RecoveryResult<T>(fallback_result);
            } catch (const std::exception& fallback_error) {
                return RecoveryResult<T>("Primary failed: " + std::string(e.what()) + 
                                       ", Fallback failed: " + fallback_error.what(), 
                                       e.GetErrorCode(), 2);
            }
        } catch (const std::exception& e) {
            try {
                T fallback_result = fallback_operation();
                return RecoveryResult<T>(fallback_result);
            } catch (const std::exception& fallback_error) {
                return RecoveryResult<T>("Primary failed: " + std::string(e.what()) + 
                                       ", Fallback failed: " + fallback_error.what(),
                                       NeoException::ErrorCode::UNKNOWN_ERROR, 2);
            }
        }
    }

    /**
     * @brief Circuit breaker for preventing cascade failures
     */
    class CircuitBreaker
    {
    public:
        enum class State
        {
            CLOSED,    // Normal operation
            OPEN,      // Circuit is open, calls are failing fast
            HALF_OPEN  // Testing if service has recovered
        };

        struct Config
        {
            int failure_threshold = 5;           // Failures before opening circuit
            std::chrono::seconds timeout{30};    // Time before trying half-open
            int success_threshold = 2;           // Successes needed to close circuit
        };

        explicit CircuitBreaker(const Config& config) : config_(config) {}
        CircuitBreaker() : CircuitBreaker(Config{}) {}

        template<typename T>
        RecoveryResult<T> Execute(std::function<T()> operation)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            if (state_ == State::OPEN) {
                if (std::chrono::steady_clock::now() - last_failure_time_ > config_.timeout) {
                    state_ = State::HALF_OPEN;
                    consecutive_successes_ = 0;
                } else {
                    return RecoveryResult<T>("Circuit breaker is OPEN", 
                        NeoException::ErrorCode::TIMEOUT, 0);
                }
            }
            
            try {
                T result = operation();
                OnSuccess();
                return RecoveryResult<T>(result);
            } catch (const NeoException& e) {
                OnFailure();
                return RecoveryResult<T>(e.what(), e.GetErrorCode(), 1);
            } catch (const std::exception& e) {
                OnFailure();
                return RecoveryResult<T>(e.what(), NeoException::ErrorCode::UNKNOWN_ERROR, 1);
            }
        }

        State GetState() const { return state_; }
        int GetFailureCount() const { return failure_count_; }

    private:
        Config config_;
        State state_ = State::CLOSED;
        int failure_count_ = 0;
        int consecutive_successes_ = 0;
        std::chrono::steady_clock::time_point last_failure_time_;
        mutable std::mutex mutex_;

        void OnSuccess()
        {
            failure_count_ = 0;
            if (state_ == State::HALF_OPEN) {
                consecutive_successes_++;
                if (consecutive_successes_ >= config_.success_threshold) {
                    state_ = State::CLOSED;
                }
            }
        }

        void OnFailure()
        {
            failure_count_++;
            last_failure_time_ = std::chrono::steady_clock::now();
            consecutive_successes_ = 0;
            
            if (failure_count_ >= config_.failure_threshold) {
                state_ = State::OPEN;
            }
        }
    };

    /**
     * @brief Bulkhead pattern for resource isolation
     */
    class Bulkhead
    {
    public:
        struct Config
        {
            int max_concurrent_calls = 10;
            std::chrono::milliseconds timeout{5000};
        };

        explicit Bulkhead(const Config& config) : config_(config) {}
        Bulkhead() : Bulkhead(Config{}) {}

        template<typename T>
        RecoveryResult<T> Execute(std::function<T()> operation)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            
            if (current_calls_ >= config_.max_concurrent_calls) {
                return RecoveryResult<T>("Bulkhead capacity exceeded", 
                    NeoException::ErrorCode::TIMEOUT, 0);
            }
            
            current_calls_++;
            lock.unlock();
            
            try {
                T result = operation();
                
                lock.lock();
                current_calls_--;
                return RecoveryResult<T>(result);
            } catch (const NeoException& e) {
                lock.lock();
                current_calls_--;
                return RecoveryResult<T>(e.what(), e.GetErrorCode(), 1);
            } catch (const std::exception& e) {
                lock.lock();
                current_calls_--;
                return RecoveryResult<T>(e.what(), NeoException::ErrorCode::UNKNOWN_ERROR, 1);
            }
        }

        int GetCurrentCalls() const { return current_calls_; }

    private:
        Config config_;
        int current_calls_ = 0;
        mutable std::mutex mutex_;
    };

    /**
     * @brief Timeout wrapper for operations
     */
    template<typename T>
    static RecoveryResult<T> WithTimeout(std::function<T()> operation, 
                                        std::chrono::milliseconds timeout)
    {
        std::promise<RecoveryResult<T>> promise;
        auto future = promise.get_future();
        
        std::thread worker([&promise, operation]() {
            try {
                T result = operation();
                promise.set_value(RecoveryResult<T>(result));
            } catch (const NeoException& e) {
                promise.set_value(RecoveryResult<T>(e.what(), e.GetErrorCode(), 1));
            } catch (const std::exception& e) {
                promise.set_value(RecoveryResult<T>(e.what(), NeoException::ErrorCode::UNKNOWN_ERROR, 1));
            }
        });
        
        if (future.wait_for(timeout) == std::future_status::timeout) {
            // Note: In production, we should properly terminate the worker thread
            worker.detach();
            return RecoveryResult<T>("Operation timed out", NeoException::ErrorCode::TIMEOUT, 1);
        }
        
        worker.join();
        return future.get();
    }

    /**
     * @brief Safe execution wrapper that never throws
     */
    template<typename T>
    static RecoveryResult<T> SafeExecute(std::function<T()> operation, 
                                        const std::string& operation_name = "operation")
    {
        try {
            T result = operation();
            return RecoveryResult<T>(result);
        } catch (const NeoException& e) {
            return RecoveryResult<T>(operation_name + " failed: " + e.what(), e.GetErrorCode(), 1);
        } catch (const std::exception& e) {
            return RecoveryResult<T>(operation_name + " failed: " + e.what(), 
                NeoException::ErrorCode::UNKNOWN_ERROR, 1);
        } catch (...) {
            return RecoveryResult<T>(operation_name + " failed with unknown exception", 
                NeoException::ErrorCode::UNKNOWN_ERROR, 1);
        }
    }

    /**
     * @brief Exception categorization for better error handling
     */
    static bool IsRetriableException(const std::exception& e);
    static bool IsTransientException(const std::exception& e);
    static bool IsFatalException(const std::exception& e);
    
    /**
     * @brief Create standard retry configurations for different scenarios
     */
    static RetryConfig NetworkRetryConfig();
    static RetryConfig DatabaseRetryConfig();
    static RetryConfig FileOperationRetryConfig();
    static RetryConfig CryptographyRetryConfig();
};

// Utility macros for error recovery
#define SAFE_EXECUTE(operation) \
    neo::core::ErrorRecovery::SafeExecute<decltype(operation())>([&]() { return operation(); }, #operation)

#define RETRY_OPERATION(operation, config) \
    neo::core::ErrorRecovery::Retry<decltype(operation())>([&]() { return operation(); }, config)

#define WITH_FALLBACK(primary, fallback) \
    neo::core::ErrorRecovery::WithFallback<decltype(primary())>([&]() { return primary(); }, [&]() { return fallback(); })

} // namespace neo::core