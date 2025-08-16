/**
 * @file test_error_handling.cpp
 * @brief Unit tests for error handling framework
 */

#include <gtest/gtest.h>
#include <neo/core/error_handling.h>
#include <string>
#include <vector>

using namespace neo::error;

class ErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test Result type success case
TEST_F(ErrorHandlingTest, ResultSuccess) {
    Result<int> result = Ok(42);
    
    EXPECT_TRUE(result.IsSuccess());
    EXPECT_FALSE(result.IsError());
    EXPECT_EQ(result.Value(), 42);
    EXPECT_EQ(result.ValueOr(0), 42);
}

// Test Result type error case
TEST_F(ErrorHandlingTest, ResultError) {
    Result<int> result = Err<int>(NeoException(ErrorCode::InvalidArgument, "test error"));
    
    EXPECT_FALSE(result.IsSuccess());
    EXPECT_TRUE(result.IsError());
    EXPECT_EQ(result.Error().code(), ErrorCode::InvalidArgument);
    EXPECT_EQ(result.ValueOr(0), 0);
}

// Test Result Map function
TEST_F(ErrorHandlingTest, ResultMap) {
    Result<int> result = Ok(10);
    auto mapped = result.Map([](int x) { return x * 2; });
    
    EXPECT_TRUE(mapped.IsSuccess());
    EXPECT_EQ(mapped.Value(), 20);
}

// Test Result Map on error
TEST_F(ErrorHandlingTest, ResultMapError) {
    Result<int> result = Err<int>(NeoException(ErrorCode::InvalidArgument, "error"));
    auto mapped = result.Map([](int x) { return x * 2; });
    
    EXPECT_FALSE(mapped.IsSuccess());
    EXPECT_EQ(mapped.Error().code(), ErrorCode::InvalidArgument);
}

// Test Result AndThen (monadic bind)
TEST_F(ErrorHandlingTest, ResultAndThen) {
    Result<int> result = Ok(10);
    auto chained = result.AndThen([](int x) -> Result<int> {
        if (x > 5) {
            return Ok(x * 2);
        }
        return Err<int>(NeoException(ErrorCode::OutOfRange, "too small"));
    });
    
    EXPECT_TRUE(chained.IsSuccess());
    EXPECT_EQ(chained.Value(), 20);
}

// Test chaining with error
TEST_F(ErrorHandlingTest, ResultAndThenError) {
    Result<int> result = Ok(3);
    auto chained = result.AndThen([](int x) -> Result<int> {
        if (x > 5) {
            return Ok(x * 2);
        }
        return Err<int>(NeoException(ErrorCode::OutOfRange, "too small"));
    });
    
    EXPECT_FALSE(chained.IsSuccess());
    EXPECT_EQ(chained.Error().code(), ErrorCode::OutOfRange);
}

// Test NeoException
TEST_F(ErrorHandlingTest, NeoException) {
    NeoException ex(ErrorCode::NetworkTimeout, "connection failed");
    
    EXPECT_EQ(ex.code(), ErrorCode::NetworkTimeout);
    EXPECT_EQ(ex.message(), "connection failed");
    EXPECT_NE(std::string(ex.what()).find("NetworkTimeout"), std::string::npos);
}

// Test ErrorCodeToString
TEST_F(ErrorHandlingTest, ErrorCodeToString) {
    EXPECT_STREQ(ErrorCodeToString(ErrorCode::Success), "Success");
    EXPECT_STREQ(ErrorCodeToString(ErrorCode::InvalidArgument), "Invalid argument");
    EXPECT_STREQ(ErrorCodeToString(ErrorCode::NetworkTimeout), "Network timeout");
    EXPECT_STREQ(ErrorCodeToString(ErrorCode::InvalidTransaction), "Invalid transaction");
}

// Test nested Results
TEST_F(ErrorHandlingTest, NestedResults) {
    auto divide = [](int a, int b) -> Result<int> {
        if (b == 0) {
            return Err<int>(NeoException(ErrorCode::InvalidArgument, "division by zero"));
        }
        return Ok(a / b);
    };
    
    auto calculate = [&divide](int x) -> Result<int> {
        return divide(100, x).AndThen([&divide](int y) {
            return divide(y, 2);
        });
    };
    
    auto result1 = calculate(10);  // 100/10 = 10, 10/2 = 5
    EXPECT_TRUE(result1.IsSuccess());
    EXPECT_EQ(result1.Value(), 5);
    
    auto result2 = calculate(0);  // division by zero
    EXPECT_FALSE(result2.IsSuccess());
    EXPECT_EQ(result2.Error().code(), ErrorCode::InvalidArgument);
}

// Test MapError
TEST_F(ErrorHandlingTest, MapError) {
    Result<int, std::string> result = Result<int, std::string>(std::string("original error"));
    
    auto mapped = result.MapError([](const std::string& err) {
        return "mapped: " + err;
    });
    
    EXPECT_FALSE(mapped.IsSuccess());
    EXPECT_EQ(mapped.Error(), "mapped: original error");
}

// Test ErrorGuard
TEST_F(ErrorHandlingTest, ErrorGuard) {
    bool cleanup_called = false;
    
    {
        ErrorGuard guard([&cleanup_called]() {
            cleanup_called = true;
        });
        
        // Dismiss the guard
        guard.Dismiss();
    }
    
    EXPECT_FALSE(cleanup_called);  // Should not be called when dismissed
}

// Test ErrorGuard with exception
TEST_F(ErrorHandlingTest, ErrorGuardWithException) {
    bool cleanup_called = false;
    
    try {
        ErrorGuard guard([&cleanup_called]() {
            cleanup_called = true;
        });
        
        throw std::runtime_error("test exception");
    } catch (...) {
        // Exception caught
    }
    
    EXPECT_TRUE(cleanup_called);  // Should be called on exception
}

// Test all error codes
TEST_F(ErrorHandlingTest, AllErrorCodes) {
    std::vector<ErrorCode> codes = {
        ErrorCode::Success,
        ErrorCode::UnknownError,
        ErrorCode::InvalidArgument,
        ErrorCode::OutOfRange,
        ErrorCode::FeatureNotSupported,
        ErrorCode::NetworkTimeout,
        ErrorCode::ConnectionFailed,
        ErrorCode::PeerDisconnected,
        ErrorCode::InvalidMessage,
        ErrorCode::RateLimitExceeded,
        ErrorCode::ConsensusTimeout,
        ErrorCode::InvalidBlock,
        ErrorCode::InvalidSignature,
        ErrorCode::ForkDetected,
        ErrorCode::StorageCorrupted,
        ErrorCode::StorageUnavailable,
        ErrorCode::DiskFull,
        ErrorCode::ReadError,
        ErrorCode::WriteError,
        ErrorCode::ValidationFailed,
        ErrorCode::InvalidTransaction,
        ErrorCode::InsufficientFunds,
        ErrorCode::DuplicateTransaction,
        ErrorCode::CryptoOperationFailed,
        ErrorCode::InvalidKey,
        ErrorCode::SignatureMismatch,
        ErrorCode::HashMismatch,
        ErrorCode::ContractExecutionFailed,
        ErrorCode::ContractNotFound,
        ErrorCode::GasExhausted,
        ErrorCode::StackOverflow,
        ErrorCode::InvalidOpcode
    };
    
    for (auto code : codes) {
        const char* str = ErrorCodeToString(code);
        EXPECT_NE(str, nullptr);
        EXPECT_NE(std::strlen(str), 0);
    }
}

// Test complex error propagation
TEST_F(ErrorHandlingTest, ComplexErrorPropagation) {
    auto step1 = []() -> Result<int> {
        return Ok(10);
    };
    
    auto step2 = [](int x) -> Result<std::string> {
        if (x < 5) {
            return Err<std::string>(NeoException(ErrorCode::OutOfRange, "too small"));
        }
        return Ok(std::to_string(x));
    };
    
    auto step3 = [](const std::string& s) -> Result<double> {
        try {
            double val = std::stod(s);
            return Ok(val * 2.5);
        } catch (...) {
            return Err<double>(NeoException(ErrorCode::InvalidArgument, "parse error"));
        }
    };
    
    auto pipeline = step1()
        .AndThen(step2)
        .AndThen(step3);
    
    EXPECT_TRUE(pipeline.IsSuccess());
    EXPECT_DOUBLE_EQ(pipeline.Value(), 25.0);
}