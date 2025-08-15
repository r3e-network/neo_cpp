#include <gtest/gtest.h>
#include <neo/security/session_manager.h>
#include <thread>
#include <chrono>

using namespace neo::security;
using namespace std::chrono_literals;

class SessionManagementTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.session_timeout = std::chrono::minutes(30);
        config_.max_sessions_per_user = 5;
        config_.enable_ip_binding = true;
        config_.enable_secure_cookies = true;
        
        manager_ = std::make_unique<SessionManager>(config_);
    }
    
    SessionManager::Config config_;
    std::unique_ptr<SessionManager> manager_;
};

TEST_F(SessionManagementTest, CreateSession) {
    std::string user_id = "user123";
    std::string ip_address = "192.168.1.1";
    
    auto session = manager_->CreateSession(user_id, ip_address);
    
    EXPECT_FALSE(session.token.empty());
    EXPECT_EQ(session.user_id, user_id);
    EXPECT_EQ(session.ip_address, ip_address);
    EXPECT_TRUE(session.is_active);
}

TEST_F(SessionManagementTest, ValidateValidSession) {
    std::string user_id = "user456";
    std::string ip_address = "192.168.1.2";
    
    auto session = manager_->CreateSession(user_id, ip_address);
    
    EXPECT_TRUE(manager_->ValidateSession(session.token, ip_address));
}

TEST_F(SessionManagementTest, InvalidateExpiredSession) {
    std::string user_id = "user789";
    std::string ip_address = "192.168.1.3";
    
    // Create session with very short timeout
    config_.session_timeout = std::chrono::milliseconds(100);
    manager_ = std::make_unique<SessionManager>(config_);
    
    auto session = manager_->CreateSession(user_id, ip_address);
    
    // Wait for expiration
    std::this_thread::sleep_for(200ms);
    
    EXPECT_FALSE(manager_->ValidateSession(session.token, ip_address));
}

TEST_F(SessionManagementTest, IPBindingPreventsHijacking) {
    std::string user_id = "user_secure";
    std::string original_ip = "192.168.1.10";
    std::string attacker_ip = "10.0.0.1";
    
    auto session = manager_->CreateSession(user_id, original_ip);
    
    // Original IP should work
    EXPECT_TRUE(manager_->ValidateSession(session.token, original_ip));
    
    // Different IP should fail (hijack attempt)
    EXPECT_FALSE(manager_->ValidateSession(session.token, attacker_ip));
}

TEST_F(SessionManagementTest, MaxSessionsPerUser) {
    std::string user_id = "user_limited";
    
    // Create max allowed sessions
    std::vector<Session> sessions;
    for (size_t i = 0; i < config_.max_sessions_per_user; ++i) {
        std::string ip = "192.168.1." + std::to_string(20 + i);
        sessions.push_back(manager_->CreateSession(user_id, ip));
        EXPECT_FALSE(sessions.back().token.empty());
    }
    
    // Next session should evict oldest
    std::string new_ip = "192.168.1.30";
    auto new_session = manager_->CreateSession(user_id, new_ip);
    EXPECT_FALSE(new_session.token.empty());
    
    // First session should be invalidated
    EXPECT_FALSE(manager_->ValidateSession(sessions[0].token, "192.168.1.20"));
    
    // Last session should still be valid
    EXPECT_TRUE(manager_->ValidateSession(sessions.back().token, 
                                         "192.168.1." + std::to_string(20 + sessions.size() - 1)));
}

TEST_F(SessionManagementTest, RevokeSession) {
    std::string user_id = "user_revoke";
    std::string ip_address = "192.168.1.40";
    
    auto session = manager_->CreateSession(user_id, ip_address);
    
    // Session should be valid initially
    EXPECT_TRUE(manager_->ValidateSession(session.token, ip_address));
    
    // Revoke the session
    EXPECT_TRUE(manager_->RevokeSession(session.token));
    
    // Session should no longer be valid
    EXPECT_FALSE(manager_->ValidateSession(session.token, ip_address));
}

TEST_F(SessionManagementTest, RevokeAllUserSessions) {
    std::string user_id = "user_revoke_all";
    
    // Create multiple sessions for the user
    std::vector<Session> sessions;
    for (int i = 0; i < 3; ++i) {
        std::string ip = "192.168.1." + std::to_string(50 + i);
        sessions.push_back(manager_->CreateSession(user_id, ip));
    }
    
    // Revoke all sessions for the user
    manager_->RevokeUserSessions(user_id);
    
    // All sessions should be invalid
    for (size_t i = 0; i < sessions.size(); ++i) {
        std::string ip = "192.168.1." + std::to_string(50 + i);
        EXPECT_FALSE(manager_->ValidateSession(sessions[i].token, ip));
    }
}

TEST_F(SessionManagementTest, RefreshSession) {
    std::string user_id = "user_refresh";
    std::string ip_address = "192.168.1.60";
    
    auto session = manager_->CreateSession(user_id, ip_address);
    auto original_expiry = session.expires_at;
    
    // Wait a bit
    std::this_thread::sleep_for(100ms);
    
    // Refresh the session
    EXPECT_TRUE(manager_->RefreshSession(session.token));
    
    // Get updated session info
    auto refreshed = manager_->GetSession(session.token);
    EXPECT_TRUE(refreshed.has_value());
    EXPECT_GT(refreshed->expires_at, original_expiry);
}

TEST_F(SessionManagementTest, GetUserSessions) {
    std::string user_id = "user_list";
    
    // Create multiple sessions
    for (int i = 0; i < 3; ++i) {
        std::string ip = "192.168.1." + std::to_string(70 + i);
        manager_->CreateSession(user_id, ip);
    }
    
    auto user_sessions = manager_->GetUserSessions(user_id);
    EXPECT_EQ(user_sessions.size(), 3);
    
    for (const auto& session : user_sessions) {
        EXPECT_EQ(session.user_id, user_id);
        EXPECT_TRUE(session.is_active);
    }
}

TEST_F(SessionManagementTest, CleanupExpiredSessions) {
    // Create sessions with very short timeout
    config_.session_timeout = std::chrono::milliseconds(100);
    manager_ = std::make_unique<SessionManager>(config_);
    
    // Create multiple sessions
    std::vector<Session> sessions;
    for (int i = 0; i < 5; ++i) {
        std::string user = "user_" + std::to_string(i);
        std::string ip = "192.168.1." + std::to_string(80 + i);
        sessions.push_back(manager_->CreateSession(user, ip));
    }
    
    EXPECT_EQ(manager_->GetActiveSessions(), 5);
    
    // Wait for expiration
    std::this_thread::sleep_for(200ms);
    
    // Cleanup expired sessions
    manager_->CleanupExpired();
    
    EXPECT_EQ(manager_->GetActiveSessions(), 0);
}

TEST_F(SessionManagementTest, ConcurrentSessionAccess) {
    std::string user_id = "user_concurrent";
    std::atomic<int> valid_count{0};
    std::atomic<int> invalid_count{0};
    
    // Create a session
    auto session = manager_->CreateSession(user_id, "192.168.1.100");
    
    // Launch multiple threads validating the session
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &session, &valid_count, &invalid_count]() {
            for (int j = 0; j < 100; ++j) {
                if (manager_->ValidateSession(session.token, "192.168.1.100")) {
                    valid_count++;
                } else {
                    invalid_count++;
                }
                std::this_thread::sleep_for(1ms);
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Session should remain valid throughout
    EXPECT_GT(valid_count, 0);
    EXPECT_EQ(invalid_count, 0);
}

TEST_F(SessionManagementTest, SessionMetadata) {
    std::string user_id = "user_metadata";
    std::string ip_address = "192.168.1.110";
    
    auto session = manager_->CreateSession(user_id, ip_address);
    
    // Add metadata
    manager_->SetSessionMetadata(session.token, "user_agent", "Mozilla/5.0");
    manager_->SetSessionMetadata(session.token, "device_id", "device123");
    
    // Retrieve metadata
    auto metadata = manager_->GetSessionMetadata(session.token);
    EXPECT_TRUE(metadata.has_value());
    EXPECT_EQ(metadata->at("user_agent"), "Mozilla/5.0");
    EXPECT_EQ(metadata->at("device_id"), "device123");
}

TEST_F(SessionManagementTest, SecureCookieGeneration) {
    std::string user_id = "user_cookie";
    std::string ip_address = "192.168.1.120";
    
    auto session = manager_->CreateSession(user_id, ip_address);
    
    // Generate secure cookie
    auto cookie = manager_->GenerateSecureCookie(session.token);
    
    EXPECT_FALSE(cookie.empty());
    EXPECT_NE(cookie.find("Secure"), std::string::npos);
    EXPECT_NE(cookie.find("HttpOnly"), std::string::npos);
    EXPECT_NE(cookie.find("SameSite=Strict"), std::string::npos);
}