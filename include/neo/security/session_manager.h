#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <random>
#include <memory>
#include <algorithm>
#include <vector>

namespace neo {
namespace security {

/**
 * @brief Session information
 */
struct Session {
    std::string token;
    std::string user_id;
    std::string ip_address;
    std::chrono::steady_clock::time_point created_at;
    std::chrono::steady_clock::time_point last_accessed_at;
    std::unordered_map<std::string, std::string> data;
    bool is_active;
    
    // Legacy compatibility
    std::string id;
    std::string userId;
    std::string ipAddress;
    std::chrono::steady_clock::time_point createdAt;
    std::chrono::steady_clock::time_point lastAccessedAt;
    bool isValid;
    
    Session() : is_active(false), isValid(false) {}
    
    Session(const std::string& sessionToken, const std::string& user, const std::string& ip)
        : token(sessionToken), user_id(user), ip_address(ip), is_active(true),
          id(sessionToken), userId(user), ipAddress(ip), isValid(true) {
        auto now = std::chrono::steady_clock::now();
        created_at = now;
        last_accessed_at = now;
        createdAt = now;
        lastAccessedAt = now;
    }
};

/**
 * @brief Session manager for handling user sessions
 */
class SessionManager {
public:
    /**
     * @brief Configuration for session manager
     */
    struct Config {
        std::chrono::minutes session_timeout = std::chrono::minutes(30);
        size_t max_sessions_per_user = 5;
        bool enable_ip_binding = true;
        bool enable_secure_cookies = true;
    };
    
    /**
     * @brief Constructor with configuration
     * @param config Session manager configuration
     */
    explicit SessionManager(const Config& config = Config())
        : config_(config), sessionTimeout_(std::chrono::duration_cast<std::chrono::seconds>(config.session_timeout).count()) {
        // Initialize random generator for session IDs
        std::random_device rd;
        rng_ = std::mt19937(rd());
    }
    
    /**
     * @brief Constructor
     * @param sessionTimeout Session timeout in seconds (default 30 minutes)
     */
    SessionManager(size_t sessionTimeout)
        : sessionTimeout_(sessionTimeout) {
        config_.session_timeout = std::chrono::seconds(sessionTimeout);
        // Initialize random generator for session IDs
        std::random_device rd;
        rng_ = std::mt19937(rd());
    }
    
    /**
     * @brief Create a new session
     * @param user_id User identifier
     * @param ip_address IP address of the client
     * @return Session object
     */
    Session CreateSession(const std::string& user_id, const std::string& ip_address) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check max sessions per user
        auto& userSessionList = userSessions_[user_id];
        if (userSessionList.size() >= config_.max_sessions_per_user) {
            // Remove oldest session
            if (!userSessionList.empty()) {
                InvalidateSession(userSessionList.front());
                userSessionList.erase(userSessionList.begin());
            }
        }
        
        std::string sessionToken = GenerateSessionId();
        Session session(sessionToken, user_id, ip_address);
        sessions_[sessionToken] = std::make_shared<Session>(session);
        userSessionList.push_back(sessionToken);
        
        return session;
    }
    
    
    /**
     * @brief Validate a session with IP binding check
     * @param token Session token
     * @param ip_address IP address to validate against
     * @return true if valid, false otherwise
     */
    bool ValidateSession(const std::string& token, const std::string& ip_address) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = sessions_.find(token);
        if (it == sessions_.end()) {
            return false;
        }
        
        auto session = it->second;
        
        // Check if session has expired
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - session->last_accessed_at);
        
        if (elapsed > config_.session_timeout) {
            // Session expired
            InvalidateSession(token);
            return false;
        }
        
        // Check IP binding if enabled
        if (config_.enable_ip_binding && session->ip_address != ip_address) {
            return false;  // Potential hijack attempt
        }
        
        // Update last accessed time
        session->last_accessed_at = now;
        session->lastAccessedAt = now;
        return session->is_active;
    }
    
    /**
     * @brief Get session by ID
     * @param sessionId Session identifier
     * @return Session pointer or nullptr if not found/expired
     */
    std::shared_ptr<Session> GetSession(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = sessions_.find(sessionId);
        if (it == sessions_.end()) {
            return nullptr;
        }
        
        auto session = it->second;
        
        // Check if session has expired
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - session->lastAccessedAt).count();
        
        if (elapsed > sessionTimeout_) {
            // Session expired
            InvalidateSession(sessionId);
            return nullptr;
        }
        
        // Update last accessed time
        session->lastAccessedAt = now;
        session->last_accessed_at = now;
        return session;
    }
    
    /**
     * @brief Validate a session
     * @param sessionId Session identifier
     * @return true if valid, false otherwise
     */
    bool ValidateSession(const std::string& sessionId) {
        auto session = GetSession(sessionId);
        return session != nullptr && session->isValid;
    }
    
    /**
     * @brief Invalidate a session
     * @param sessionId Session identifier
     */
    void InvalidateSession(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = sessions_.find(sessionId);
        if (it != sessions_.end()) {
            auto session = it->second;
            session->isValid = false;
            session->is_active = false;
            
            // Remove from user sessions
            auto userIt = userSessions_.find(session->userId);
            if (userIt == userSessions_.end()) {
                userIt = userSessions_.find(session->user_id);
            }
            
            if (userIt != userSessions_.end()) {
                auto& userSessionList = userIt->second;
                userSessionList.erase(
                    std::remove(userSessionList.begin(), userSessionList.end(), sessionId),
                    userSessionList.end()
                );
                
                if (userSessionList.empty()) {
                    userSessions_.erase(userIt);
                }
            }
            
            sessions_.erase(it);
        }
    }
    
    /**
     * @brief Invalidate all sessions for a user
     * @param userId User identifier
     */
    void InvalidateUserSessions(const std::string& userId) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = userSessions_.find(userId);
        if (it != userSessions_.end()) {
            for (const auto& sessionId : it->second) {
                auto sessionIt = sessions_.find(sessionId);
                if (sessionIt != sessions_.end()) {
                    sessionIt->second->isValid = false;
                    sessionIt->second->is_active = false;
                    sessions_.erase(sessionIt);
                }
            }
            userSessions_.erase(it);
        }
    }
    
    /**
     * @brief Store data in session
     * @param sessionId Session identifier
     * @param key Data key
     * @param value Data value
     * @return true if successful
     */
    bool StoreSessionData(const std::string& sessionId, const std::string& key, const std::string& value) {
        auto session = GetSession(sessionId);
        if (session) {
            session->data[key] = value;
            return true;
        }
        return false;
    }
    
    /**
     * @brief Get data from session
     * @param sessionId Session identifier
     * @param key Data key
     * @return Data value or empty string if not found
     */
    std::string GetSessionData(const std::string& sessionId, const std::string& key) {
        auto session = GetSession(sessionId);
        if (session) {
            auto it = session->data.find(key);
            if (it != session->data.end()) {
                return it->second;
            }
        }
        return "";
    }
    
    /**
     * @brief Clean up expired sessions
     */
    void CleanupExpiredSessions() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        std::vector<std::string> toRemove;
        
        for (const auto& [sessionId, session] : sessions_) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - session->lastAccessedAt).count();
            
            if (elapsed > sessionTimeout_) {
                toRemove.push_back(sessionId);
            }
        }
        
        for (const auto& sessionId : toRemove) {
            InvalidateSession(sessionId);
        }
    }
    
    /**
     * @brief Get active session count
     * @return Number of active sessions
     */
    size_t GetActiveSessionCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sessions_.size();
    }
    
    /**
     * @brief Get sessions for a user
     * @param userId User identifier
     * @return Vector of session IDs
     */
    std::vector<std::string> GetUserSessions(const std::string& userId) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = userSessions_.find(userId);
        if (it != userSessions_.end()) {
            return it->second;
        }
        return {};
    }

private:
    /**
     * @brief Generate a unique session ID
     * @return Session ID string
     */
    std::string GenerateSessionId() {
        static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        
        std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);
        
        std::string sessionId;
        sessionId.reserve(32);
        
        for (int i = 0; i < 32; ++i) {
            sessionId += alphanum[dis(rng_)];
        }
        
        return sessionId;
    }

private:
    Config config_;
    size_t sessionTimeout_;  // in seconds
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_;
    std::unordered_map<std::string, std::vector<std::string>> userSessions_;
    std::mt19937 rng_;
};

} // namespace security
} // namespace neo