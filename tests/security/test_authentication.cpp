#include <gtest/gtest.h>
#include <neo/rpc/auth_middleware.h>
#include <thread>
#include <chrono>
#include <set>

using namespace std::chrono_literals;

class AuthenticationTest : public ::testing::Test {
protected:
    void SetUp() override {
        secret_key_ = "test_secret_key_minimum_32_characters_long_for_security";
        auth_ = std::make_unique<neo::rpc::AuthMiddleware>(secret_key_);
    }
    
    std::string secret_key_;
    std::unique_ptr<neo::rpc::AuthMiddleware> auth_;
};

TEST_F(AuthenticationTest, GenerateToken) {
    std::string user_id = "user123";
    auto token = auth_->GenerateToken(user_id);
    
    EXPECT_FALSE(token.empty());
    EXPECT_GT(token.length(), 50); // JWT tokens are typically long
}

TEST_F(AuthenticationTest, ValidateValidToken) {
    std::string user_id = "user456";
    auto token = auth_->GenerateToken(user_id);
    
    EXPECT_TRUE(auth_->ValidateToken(token));
}

TEST_F(AuthenticationTest, InvalidateInvalidToken) {
    EXPECT_FALSE(auth_->ValidateToken("invalid_token"));
    EXPECT_FALSE(auth_->ValidateToken(""));
}

TEST_F(AuthenticationTest, AuthenticateWithBearerHeader) {
    std::string user_id = "user_bearer";
    auto token = auth_->GenerateToken(user_id);
    
    std::string header = "Bearer " + token;
    EXPECT_TRUE(auth_->Authenticate(header));
    
    // Test various invalid headers
    EXPECT_FALSE(auth_->Authenticate("Bearer invalid_token"));
    EXPECT_FALSE(auth_->Authenticate(token)); // Missing Bearer prefix
    EXPECT_FALSE(auth_->Authenticate(""));
}

TEST_F(AuthenticationTest, ExtractUserFromToken) {
    std::string user_id = "user_extract";
    auto token = auth_->GenerateToken(user_id);
    
    auto extracted_user = auth_->ExtractUser(token);
    EXPECT_EQ(extracted_user, user_id);
    
    // Invalid token should return empty
    auto invalid_user = auth_->ExtractUser("invalid_token");
    EXPECT_TRUE(invalid_user.empty());
}

TEST_F(AuthenticationTest, TokenExpiry) {
    // This test would require mock time or configurable expiry
    // For now, just verify token format
    std::string user_id = "user_expire";
    auto token = auth_->GenerateToken(user_id);
    
    // Token should have three parts separated by dots (JWT format)
    size_t dot_count = std::count(token.begin(), token.end(), '.');
    EXPECT_EQ(dot_count, 2);
}

TEST_F(AuthenticationTest, RevokeToken) {
    std::string user_id = "user_revoke";
    auto token = auth_->GenerateToken(user_id);
    
    // Token should be valid initially
    EXPECT_TRUE(auth_->ValidateToken(token));
    
    // Revoke the token
    auth_->RevokeToken(token);
    
    // Token should no longer be valid
    EXPECT_FALSE(auth_->ValidateToken(token));
}

TEST_F(AuthenticationTest, DifferentSecretKeysFail) {
    std::string user_id = "user_different_key";
    auto token = auth_->GenerateToken(user_id);
    
    // Create new auth with different key
    std::string different_key = "different_secret_key_minimum_32_characters_long!!";
    neo::rpc::AuthMiddleware different_auth(different_key);
    
    // Token from first auth should not validate with different key
    EXPECT_FALSE(different_auth.ValidateToken(token));
}

TEST_F(AuthenticationTest, ConcurrentTokenValidation) {
    std::string user_id = "user_concurrent";
    auto token = auth_->GenerateToken(user_id);
    
    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};
    
    // Launch multiple threads validating the same token
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &token, &success_count, &failure_count]() {
            for (int j = 0; j < 100; ++j) {
                if (auth_->ValidateToken(token)) {
                    success_count++;
                } else {
                    failure_count++;
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // All validations should succeed
    EXPECT_EQ(success_count, 1000);
    EXPECT_EQ(failure_count, 0);
}

TEST_F(AuthenticationTest, UniqueTokenGeneration) {
    std::string user_id = "user_unique";
    
    // Generate multiple tokens
    std::set<std::string> tokens;
    for (int i = 0; i < 10; ++i) {
        tokens.insert(auth_->GenerateToken(user_id));
    }
    
    // All tokens should be unique
    EXPECT_EQ(tokens.size(), 10);
}

TEST_F(AuthenticationTest, EmptyUserHandling) {
    // Should handle empty user ID gracefully
    auto token = auth_->GenerateToken("");
    EXPECT_FALSE(token.empty()); // Still generates a token
    EXPECT_TRUE(auth_->ValidateToken(token));
}

TEST_F(AuthenticationTest, LargeUserIdHandling) {
    // Test with very long user ID
    std::string long_user_id(1000, 'a');
    auto token = auth_->GenerateToken(long_user_id);
    EXPECT_FALSE(token.empty());
    EXPECT_TRUE(auth_->ValidateToken(token));
}
