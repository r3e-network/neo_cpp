#include <gtest/gtest.h>
#include <neo/core/exceptions.h>
#include <neo/core/validation.h>
#include <neo/core/error_recovery.h>
#include <thread>
#include <chrono>

using namespace neo::core;

class ErrorHandlingComprehensiveTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Reset any global state if needed
    }
    
    void TearDown() override
    {
        // Clean up
    }
};

// Exception Framework Tests
TEST_F(ErrorHandlingComprehensiveTest, NeoExceptionBasics)
{
    NeoException ex(NeoException::ErrorCode::INVALID_ARGUMENT, "Test message", "test context");
    
    EXPECT_EQ(ex.GetErrorCode(), NeoException::ErrorCode::INVALID_ARGUMENT);
    EXPECT_EQ(ex.GetOriginalMessage(), "Test message");
    EXPECT_EQ(ex.GetContext(), "test context");
    
    std::string what_str = ex.what();
    EXPECT_TRUE(what_str.find("INVALID_ARGUMENT") != std::string::npos);
    EXPECT_TRUE(what_str.find("Test message") != std::string::npos);
    EXPECT_TRUE(what_str.find("test context") != std::string::npos);
}

TEST_F(ErrorHandlingComprehensiveTest, SpecificExceptionTypes)
{
    // Test SerializationException
    SerializationException ser_ex("Serialization failed", "test.cpp:100");
    EXPECT_EQ(ser_ex.GetErrorCode(), NeoException::ErrorCode::SERIALIZATION_ERROR);
    
    // Test CryptographyException
    CryptographyException crypto_ex(NeoException::ErrorCode::INVALID_SIGNATURE, 
                                  "Invalid signature", "crypto.cpp:200");
    EXPECT_EQ(crypto_ex.GetErrorCode(), NeoException::ErrorCode::INVALID_SIGNATURE);
    
    // Test NetworkException
    NetworkException net_ex("Connection failed", "network.cpp:300");
    EXPECT_EQ(net_ex.GetErrorCode(), NeoException::ErrorCode::NETWORK_ERROR);
    
    // Test BlockchainException
    BlockchainException bc_ex(NeoException::ErrorCode::INVALID_BLOCK, 
                             "Invalid block", "blockchain.cpp:400");
    EXPECT_EQ(bc_ex.GetErrorCode(), NeoException::ErrorCode::INVALID_BLOCK);
}

TEST_F(ErrorHandlingComprehensiveTest, ErrorCodeConversion)
{
    std::string code_str = NeoException::ErrorCodeToString(NeoException::ErrorCode::INVALID_ARGUMENT);
    EXPECT_EQ(code_str, "INVALID_ARGUMENT");
    
    code_str = NeoException::ErrorCodeToString(NeoException::ErrorCode::CRYPTO_ERROR);
    EXPECT_EQ(code_str, "CRYPTO_ERROR");
    
    // Test unknown error code
    code_str = NeoException::ErrorCodeToString(static_cast<NeoException::ErrorCode>(99999));
    EXPECT_TRUE(code_str.find("UNKNOWN_ERROR_CODE") != std::string::npos);
}

// Validation Framework Tests
TEST_F(ErrorHandlingComprehensiveTest, BasicValidation)
{
    // Test ValidateNotNull
    int value = 42;
    auto result = Validator::ValidateNotNull(&value, "test_value");
    EXPECT_TRUE(result.is_valid);
    
    result = Validator::ValidateNotNull(nullptr, "null_value");
    EXPECT_FALSE(result.is_valid);
    EXPECT_EQ(result.error_code, NeoException::ErrorCode::INVALID_ARGUMENT);
    
    // Test ValidateNotEmpty for string
    result = Validator::ValidateNotEmpty(std::string("hello"), "test_string");
    EXPECT_TRUE(result.is_valid);
    
    result = Validator::ValidateNotEmpty(std::string(""), "empty_string");
    EXPECT_FALSE(result.is_valid);
    
    // Test ValidateRange
    result = Validator::ValidateRange(50, 0, 100, "test_range");
    EXPECT_TRUE(result.is_valid);
    
    result = Validator::ValidateRange(150, 0, 100, "out_of_range");
    EXPECT_FALSE(result.is_valid);
    EXPECT_EQ(result.error_code, NeoException::ErrorCode::OUT_OF_RANGE);
}

TEST_F(ErrorHandlingComprehensiveTest, StringValidation)
{
    // Test ValidateHexString
    auto result = Validator::ValidateHexString("1234abcd", "hex_string");
    EXPECT_TRUE(result.is_valid);
    
    result = Validator::ValidateHexString("0x1234abcd", "hex_with_prefix");
    EXPECT_TRUE(result.is_valid);
    
    result = Validator::ValidateHexString("123g", "invalid_hex");
    EXPECT_FALSE(result.is_valid);
    EXPECT_EQ(result.error_code, NeoException::ErrorCode::INVALID_FORMAT);
    
    result = Validator::ValidateHexString("123", "odd_length");
    EXPECT_FALSE(result.is_valid);
    
    // Test ValidateHexString with expected length
    result = Validator::ValidateHexString("1234abcd", 4, "hex_4_bytes");
    EXPECT_TRUE(result.is_valid);
    
    result = Validator::ValidateHexString("1234abcd", 8, "hex_wrong_length");
    EXPECT_FALSE(result.is_valid);
}

TEST_F(ErrorHandlingComprehensiveTest, SecurityValidation)
{
    // Test ValidateNoScriptInjection
    auto result = Validator::ValidateNoScriptInjection("normal text", "safe_input");
    EXPECT_TRUE(result.is_valid);
    
    result = Validator::ValidateNoScriptInjection("<script>alert('xss')</script>", "malicious_input");
    EXPECT_FALSE(result.is_valid);
    
    result = Validator::ValidateNoScriptInjection("javascript:alert('xss')", "js_injection");
    EXPECT_FALSE(result.is_valid);
    
    // Test ValidateNoSQLInjection
    result = Validator::ValidateNoSQLInjection("normal query", "safe_query");
    EXPECT_TRUE(result.is_valid);
    
    result = Validator::ValidateNoSQLInjection("'; DROP TABLE users; --", "sql_injection");
    EXPECT_FALSE(result.is_valid);
}

TEST_F(ErrorHandlingComprehensiveTest, NetworkValidation)
{
    // Test ValidateIPAddress
    auto result = Validator::ValidateIPAddress("192.168.1.1", "valid_ip");
    EXPECT_TRUE(result.is_valid);
    
    result = Validator::ValidateIPAddress("256.256.256.256", "invalid_ip");
    EXPECT_FALSE(result.is_valid);
    
    result = Validator::ValidateIPAddress("not.an.ip", "malformed_ip");
    EXPECT_FALSE(result.is_valid);
    
    // Test ValidatePort
    result = Validator::ValidatePort(8080, "valid_port");
    EXPECT_TRUE(result.is_valid);
    
    result = Validator::ValidatePort(0, "invalid_port");
    EXPECT_FALSE(result.is_valid);
    
    // Test ValidateURL
    result = Validator::ValidateURL("https://example.com", "valid_url");
    EXPECT_TRUE(result.is_valid);
    
    result = Validator::ValidateURL("ftp://example.com", "invalid_protocol");
    EXPECT_FALSE(result.is_valid);
}

TEST_F(ErrorHandlingComprehensiveTest, BlockchainValidation)
{
    // Test ValidateTransactionFee
    auto result = Validator::ValidateTransactionFee(1000, 10000);
    EXPECT_TRUE(result.is_valid);
    
    result = Validator::ValidateTransactionFee(-100, 10000);
    EXPECT_FALSE(result.is_valid);
    
    result = Validator::ValidateTransactionFee(20000, 10000);
    EXPECT_FALSE(result.is_valid);
    
    // Test ValidateGasAmount
    result = Validator::ValidateGasAmount(5000, 100000);
    EXPECT_TRUE(result.is_valid);
    
    result = Validator::ValidateGasAmount(-1, 100000);
    EXPECT_FALSE(result.is_valid);
    
    // Test ValidateTimestamp
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    result = Validator::ValidateTimestamp(now);
    EXPECT_TRUE(result.is_valid);
    
    result = Validator::ValidateTimestamp(now + 7200); // 2 hours in future
    EXPECT_FALSE(result.is_valid);
}

TEST_F(ErrorHandlingComprehensiveTest, ThrowingValidators)
{
    // Test RequireNotNull
    int value = 42;
    EXPECT_NO_THROW(Validator::RequireNotNull(&value, "test_value"));
    EXPECT_THROW(Validator::RequireNotNull(nullptr, "null_value"), NeoException);
    
    // Test RequireNotEmpty
    EXPECT_NO_THROW(Validator::RequireNotEmpty(std::string("hello"), "test_string"));
    EXPECT_THROW(Validator::RequireNotEmpty(std::string(""), "empty_string"), NeoException);
    
    // Test RequireRange
    EXPECT_NO_THROW(Validator::RequireRange(50, 0, 100, "test_range"));
    EXPECT_THROW(Validator::RequireRange(150, 0, 100, "out_of_range"), NeoException);
}

// Error Recovery Framework Tests
TEST_F(ErrorHandlingComprehensiveTest, RetryMechanism)
{
    int attempt_count = 0;
    
    // Test successful operation
    auto success_op = [&]() -> int {
        attempt_count++;
        return 42;
    };
    
    attempt_count = 0;
    auto result = ErrorRecovery::Retry<int>(success_op);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.value.value(), 42);
    EXPECT_EQ(result.attempts_made, 1);
    EXPECT_EQ(attempt_count, 1);
    
    // Test operation that fails then succeeds
    auto flaky_op = [&]() -> int {
        attempt_count++;
        if (attempt_count < 3) {
            throw std::runtime_error("Temporary failure");
        }
        return 42;
    };
    
    attempt_count = 0;
    ErrorRecovery::RetryConfig config;
    config.max_attempts = 5;
    config.base_delay = std::chrono::milliseconds(1); // Fast for testing
    
    result = ErrorRecovery::Retry<int>(flaky_op, config);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.value.value(), 42);
    EXPECT_EQ(result.attempts_made, 3);
    EXPECT_EQ(attempt_count, 3);
    
    // Test operation that always fails
    auto fail_op = [&]() -> int {
        attempt_count++;
        throw std::runtime_error("Always fails");
    };
    
    attempt_count = 0;
    result = ErrorRecovery::Retry<int>(fail_op, config);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.attempts_made, 5);
    EXPECT_EQ(attempt_count, 5);
}

TEST_F(ErrorHandlingComprehensiveTest, FallbackMechanism)
{
    int primary_calls = 0;
    int fallback_calls = 0;
    
    // Test primary succeeds
    auto success_primary = [&]() -> int {
        primary_calls++;
        return 42;
    };
    
    auto fallback = [&]() -> int {
        fallback_calls++;
        return 99;
    };
    
    primary_calls = fallback_calls = 0;
    auto result = ErrorRecovery::WithFallback<int>(success_primary, fallback);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.value.value(), 42);
    EXPECT_EQ(primary_calls, 1);
    EXPECT_EQ(fallback_calls, 0);
    
    // Test primary fails, fallback succeeds
    auto fail_primary = [&]() -> int {
        primary_calls++;
        throw std::runtime_error("Primary failed");
    };
    
    primary_calls = fallback_calls = 0;
    result = ErrorRecovery::WithFallback<int>(fail_primary, fallback);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.value.value(), 99);
    EXPECT_EQ(primary_calls, 1);
    EXPECT_EQ(fallback_calls, 1);
    
    // Test both fail
    auto fail_fallback = [&]() -> int {
        fallback_calls++;
        throw std::runtime_error("Fallback failed");
    };
    
    primary_calls = fallback_calls = 0;
    result = ErrorRecovery::WithFallback<int>(fail_primary, fail_fallback);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(primary_calls, 1);
    EXPECT_EQ(fallback_calls, 1);
}

TEST_F(ErrorHandlingComprehensiveTest, CircuitBreaker)
{
    ErrorRecovery::CircuitBreaker::Config config;
    config.failure_threshold = 3;
    config.timeout = std::chrono::seconds(1);
    config.success_threshold = 2;
    
    ErrorRecovery::CircuitBreaker breaker(config);
    
    // Initially closed
    EXPECT_EQ(breaker.GetState(), ErrorRecovery::CircuitBreaker::State::CLOSED);
    
    // Cause failures to open circuit
    for (int i = 0; i < 3; ++i) {
        auto result = breaker.Execute<int>([]() -> int {
            throw std::runtime_error("Failure");
        });
        EXPECT_FALSE(result.success);
    }
    
    // Should be open now
    EXPECT_EQ(breaker.GetState(), ErrorRecovery::CircuitBreaker::State::OPEN);
    
    // Calls should fail fast
    auto result = breaker.Execute<int>([]() -> int {
        return 42; // This won't be called
    });
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error_message.find("Circuit breaker is OPEN") != std::string::npos);
    
    // Wait for timeout and test half-open state
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    
    // First call should put it in half-open
    result = breaker.Execute<int>([]() -> int {
        return 42;
    });
    EXPECT_TRUE(result.success);
    EXPECT_EQ(breaker.GetState(), ErrorRecovery::CircuitBreaker::State::HALF_OPEN);
    
    // Another success should close the circuit
    result = breaker.Execute<int>([]() -> int {
        return 42;
    });
    EXPECT_TRUE(result.success);
    EXPECT_EQ(breaker.GetState(), ErrorRecovery::CircuitBreaker::State::CLOSED);
}

TEST_F(ErrorHandlingComprehensiveTest, SafeExecution)
{
    // Test successful operation
    auto success_op = []() -> int {
        return 42;
    };
    
    auto result = ErrorRecovery::SafeExecute<int>(success_op, "test_operation");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.value.value(), 42);
    
    // Test failing operation
    auto fail_op = []() -> int {
        throw std::runtime_error("Test failure");
    };
    
    result = ErrorRecovery::SafeExecute<int>(fail_op, "failing_operation");
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.error_message.find("failing_operation failed") != std::string::npos);
    
    // Test NeoException
    auto neo_fail_op = []() -> int {
        throw NeoException(NeoException::ErrorCode::INVALID_ARGUMENT, "Neo test failure");
    };
    
    result = ErrorRecovery::SafeExecute<int>(neo_fail_op, "neo_failing_operation");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code, NeoException::ErrorCode::INVALID_ARGUMENT);
}

TEST_F(ErrorHandlingComprehensiveTest, ExceptionCategorization)
{
    // Test retriable exceptions
    NetworkException net_ex("Connection failed");
    EXPECT_TRUE(ErrorRecovery::IsRetriableException(net_ex));
    EXPECT_TRUE(ErrorRecovery::IsTransientException(net_ex));
    EXPECT_FALSE(ErrorRecovery::IsFatalException(net_ex));
    
    // Test non-retriable exceptions
    SerializationException ser_ex("Invalid format");
    EXPECT_FALSE(ErrorRecovery::IsRetriableException(ser_ex));
    EXPECT_FALSE(ErrorRecovery::IsTransientException(ser_ex));
    EXPECT_FALSE(ErrorRecovery::IsFatalException(ser_ex));
    
    // Test fatal exceptions
    StorageException storage_ex(NeoException::ErrorCode::STORAGE_CORRUPTION, "Corrupted data");
    EXPECT_FALSE(ErrorRecovery::IsRetriableException(storage_ex));
    EXPECT_FALSE(ErrorRecovery::IsTransientException(storage_ex));
    EXPECT_TRUE(ErrorRecovery::IsFatalException(storage_ex));
}

TEST_F(ErrorHandlingComprehensiveTest, StandardRetryConfigs)
{
    // Test network retry config
    auto network_config = ErrorRecovery::NetworkRetryConfig();
    EXPECT_EQ(network_config.max_attempts, 5);
    EXPECT_TRUE(network_config.exponential_backoff);
    EXPECT_GT(network_config.base_delay.count(), 0);
    
    // Test database retry config
    auto db_config = ErrorRecovery::DatabaseRetryConfig();
    EXPECT_EQ(db_config.max_attempts, 3);
    EXPECT_TRUE(db_config.exponential_backoff);
    
    // Test file operation retry config
    auto file_config = ErrorRecovery::FileOperationRetryConfig();
    EXPECT_EQ(file_config.max_attempts, 3);
    EXPECT_LT(file_config.base_delay, network_config.base_delay);
    
    // Test cryptography retry config
    auto crypto_config = ErrorRecovery::CryptographyRetryConfig();
    EXPECT_EQ(crypto_config.max_attempts, 2);
    EXPECT_FALSE(crypto_config.exponential_backoff);
}

// Integration tests
TEST_F(ErrorHandlingComprehensiveTest, IntegratedErrorHandling)
{
    // Test validation + exception throwing
    try {
        VALIDATE_RANGE(150, 0, 100);
        FAIL() << "Should have thrown exception";
    } catch (const NeoException& e) {
        EXPECT_EQ(e.GetErrorCode(), NeoException::ErrorCode::OUT_OF_RANGE);
    }
    
    // Test validation + retry
    int attempt = 0;
    auto flaky_validation_op = [&]() -> int {
        attempt++;
        if (attempt < 3) {
            VALIDATE_RANGE(150, 0, 100); // Will throw
        }
        return 42;
    };
    
    attempt = 0;
    auto result = SAFE_EXECUTE(flaky_validation_op);
    
    // Should fail because validation error is not retriable
    EXPECT_FALSE(result.success);
    EXPECT_EQ(attempt, 1);
}

// Performance tests
TEST_F(ErrorHandlingComprehensiveTest, PerformanceTest)
{
    const int iterations = 1000;
    
    // Test validation performance
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto result = Validator::ValidateRange(i, 0, iterations);
        EXPECT_TRUE(result.is_valid);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should be fast (less than 1ms per validation on average)
    EXPECT_LT(duration.count() / iterations, 1000);
    
    // Test safe execution performance
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto result = ErrorRecovery::SafeExecute<int>([]() { return 42; });
        EXPECT_TRUE(result.success);
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should be reasonably fast
    EXPECT_LT(duration.count() / iterations, 10000); // Less than 10ms per operation
}