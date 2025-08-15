#include <gtest/gtest.h>
#include <neo/security/rate_limiter.h>
#include <thread>
#include <chrono>

using namespace neo::security;
using namespace std::chrono_literals;

class RateLimiterTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.requests_per_minute = 10; // Low for testing
        config_.burst_size = 3;
        config_.ban_duration = std::chrono::minutes(1);
        config_.max_violations_before_ban = 2;
        
        limiter_ = std::make_unique<RateLimiter>(config_);
    }
    
    RateLimiter::Config config_;
    std::unique_ptr<RateLimiter> limiter_;
};

TEST_F(RateLimiterTest, AllowsRequestsUnderLimit) {
    std::string client_ip = "192.168.1.1";
    
    // Should allow first few requests
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(limiter_->CheckRequest(client_ip), RateLimiter::Decision::ALLOW);
    }
}

TEST_F(RateLimiterTest, RateLimitsExcessiveRequests) {
    std::string client_ip = "192.168.1.2";
    
    // Fill up the allowed requests
    for (int i = 0; i < 10; ++i) {
        limiter_->CheckRequest(client_ip);
    }
    
    // Next request should be rate limited
    EXPECT_EQ(limiter_->CheckRequest(client_ip), RateLimiter::Decision::RATE_LIMITED);
}

TEST_F(RateLimiterTest, BansAfterViolations) {
    std::string client_ip = "192.168.1.3";
    
    // Fill up allowed requests
    for (int i = 0; i < 10; ++i) {
        limiter_->CheckRequest(client_ip);
    }
    
    // Trigger violations
    for (int i = 0; i < config_.max_violations_before_ban; ++i) {
        EXPECT_EQ(limiter_->CheckRequest(client_ip), RateLimiter::Decision::RATE_LIMITED);
    }
    
    // Should be banned now
    EXPECT_EQ(limiter_->CheckRequest(client_ip), RateLimiter::Decision::BANNED);
}

TEST_F(RateLimiterTest, ResetClientClearsLimits) {
    std::string client_ip = "192.168.1.4";
    
    // Use up some requests
    for (int i = 0; i < 5; ++i) {
        limiter_->CheckRequest(client_ip);
    }
    
    // Reset the client
    limiter_->ResetClient(client_ip);
    
    // Should allow requests again
    EXPECT_EQ(limiter_->CheckRequest(client_ip), RateLimiter::Decision::ALLOW);
}

TEST_F(RateLimiterTest, ManualBanWorks) {
    std::string client_ip = "192.168.1.5";
    
    // Manually ban the client
    limiter_->BanClient(client_ip);
    
    // Should be banned
    EXPECT_EQ(limiter_->CheckRequest(client_ip), RateLimiter::Decision::BANNED);
}

TEST_F(RateLimiterTest, UnbanClientWorks) {
    std::string client_ip = "192.168.1.6";
    
    // Ban then unban
    limiter_->BanClient(client_ip);
    EXPECT_EQ(limiter_->CheckRequest(client_ip), RateLimiter::Decision::BANNED);
    
    limiter_->UnbanClient(client_ip);
    EXPECT_EQ(limiter_->CheckRequest(client_ip), RateLimiter::Decision::ALLOW);
}

TEST_F(RateLimiterTest, TracksMultipleClients) {
    std::string client1 = "192.168.1.10";
    std::string client2 = "192.168.1.11";
    
    // Use up requests for client1
    for (int i = 0; i < 10; ++i) {
        limiter_->CheckRequest(client1);
    }
    
    // Client1 should be rate limited
    EXPECT_EQ(limiter_->CheckRequest(client1), RateLimiter::Decision::RATE_LIMITED);
    
    // Client2 should still be allowed
    EXPECT_EQ(limiter_->CheckRequest(client2), RateLimiter::Decision::ALLOW);
}

TEST_F(RateLimiterTest, MetricsTracking) {
    std::string client1 = "192.168.1.20";
    std::string client2 = "192.168.1.21";
    
    // Make some requests
    limiter_->CheckRequest(client1);
    limiter_->CheckRequest(client2);
    
    EXPECT_EQ(limiter_->GetActiveClients(), 2);
    
    // Ban a client
    limiter_->BanClient(client1);
    EXPECT_EQ(limiter_->GetBannedClients(), 1);
}

TEST_F(RateLimiterTest, ConcurrentAccess) {
    std::string client_ip = "192.168.1.30";
    std::atomic<int> allowed_count{0};
    std::atomic<int> limited_count{0};
    
    // Launch multiple threads making requests
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &client_ip, &allowed_count, &limited_count]() {
            for (int j = 0; j < 5; ++j) {
                auto decision = limiter_->CheckRequest(client_ip);
                if (decision == RateLimiter::Decision::ALLOW) {
                    allowed_count++;
                } else if (decision == RateLimiter::Decision::RATE_LIMITED) {
                    limited_count++;
                }
                std::this_thread::sleep_for(10ms);
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Should have handled requests safely
    EXPECT_GT(allowed_count, 0);
    EXPECT_GT(limited_count, 0);
    EXPECT_EQ(allowed_count + limited_count, 50);
}

TEST_F(RateLimiterTest, BurstHandling) {
    std::string client_ip = "192.168.1.40";
    
    // Should allow burst size immediately
    for (size_t i = 0; i < config_.burst_size; ++i) {
        EXPECT_EQ(limiter_->CheckRequest(client_ip), RateLimiter::Decision::ALLOW);
    }
    
    // Continue until rate limit
    int additional_allowed = 0;
    while (limiter_->CheckRequest(client_ip) == RateLimiter::Decision::ALLOW) {
        additional_allowed++;
        if (additional_allowed > 20) break; // Safety limit
    }
    
    // Should have allowed some additional requests
    EXPECT_GT(additional_allowed, 0);
    EXPECT_LT(additional_allowed, 20);
}