#pragma once

#include <chrono>
#include <unordered_map>
#include <deque>
#include <mutex>
#include <string>
#include <memory>

namespace neo::rpc {

/**
 * @brief Token bucket rate limiter for RPC endpoints
 */
class RateLimiter {
public:
    struct Config {
        size_t requestsPerSecond = 10;
        size_t requestsPerMinute = 300;
        size_t burstSize = 20;
        bool enabled = true;
    };
    
    explicit RateLimiter(const Config& config = Config{})
        : config_(config), tokens_(config.burstSize) {
    }
    
    /**
     * @brief Check if a request is allowed
     * @param clientId Client identifier (IP address)
     * @param method RPC method name
     * @return true if allowed, false if rate limited
     */
    bool IsAllowed(const std::string& clientId, const std::string& method = "") {
        if (!config_.enabled) return true;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        
        // Get or create client state
        auto& client = clients_[clientId];
        
        // Clean old requests
        CleanOldRequests(client, now);
        
        // Check per-second limit
        auto oneSecondAgo = now - std::chrono::seconds(1);
        size_t recentRequests = 0;
        for (const auto& timestamp : client.requests) {
            if (timestamp > oneSecondAgo) {
                recentRequests++;
            }
        }
        
        if (recentRequests >= config_.requestsPerSecond) {
            return false;
        }
        
        // Check per-minute limit
        if (client.requests.size() >= config_.requestsPerMinute) {
            return false;
        }
        
        // Token bucket check
        RefillTokens(now);
        if (tokens_ > 0) {
            tokens_--;
            client.requests.push_back(now);
            return true;
        }
        
        return false;
    }
    
    /**
     * @brief Get current rate limit status for a client
     * @param clientId Client identifier
     * @return Remaining requests in current window
     */
    size_t GetRemainingRequests(const std::string& clientId) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = clients_.find(clientId);
        if (it == clients_.end()) {
            return config_.requestsPerMinute;
        }
        
        auto now = std::chrono::steady_clock::now();
        CleanOldRequests(it->second, now);
        
        return config_.requestsPerMinute - it->second.requests.size();
    }
    
    /**
     * @brief Reset rate limit for a specific client
     * @param clientId Client identifier
     */
    void ResetClient(const std::string& clientId) {
        std::lock_guard<std::mutex> lock(mutex_);
        clients_.erase(clientId);
    }
    
    /**
     * @brief Reset all rate limits
     */
    void ResetAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        clients_.clear();
        tokens_ = config_.burstSize;
    }
    
    /**
     * @brief Update rate limiter configuration
     * @param config New configuration
     */
    void UpdateConfig(const Config& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
        tokens_ = std::min(tokens_, config.burstSize);
    }
    
private:
    struct ClientState {
        std::deque<std::chrono::steady_clock::time_point> requests;
    };
    
    void CleanOldRequests(ClientState& client, 
                         const std::chrono::steady_clock::time_point& now) {
        auto oneMinuteAgo = now - std::chrono::minutes(1);
        
        while (!client.requests.empty() && client.requests.front() < oneMinuteAgo) {
            client.requests.pop_front();
        }
    }
    
    void RefillTokens(const std::chrono::steady_clock::time_point& now) {
        if (lastRefill_ == std::chrono::steady_clock::time_point{}) {
            lastRefill_ = now;
            return;
        }
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRefill_);
        auto tokensToAdd = (elapsed.count() * config_.requestsPerSecond) / 1000;
        
        if (tokensToAdd > 0) {
            tokens_ = std::min(tokens_ + tokensToAdd, config_.burstSize);
            lastRefill_ = now;
        }
    }
    
    Config config_;
    std::unordered_map<std::string, ClientState> clients_;
    size_t tokens_;
    std::chrono::steady_clock::time_point lastRefill_;
    mutable std::mutex mutex_;
};

/**
 * @brief Method-specific rate limiter with different limits per method
 */
class MethodRateLimiter {
public:
    struct MethodConfig {
        size_t requestsPerSecond = 10;
        size_t requestsPerMinute = 300;
        bool enabled = true;
    };
    
    MethodRateLimiter() {
        // Set default limits for different method categories
        SetMethodLimit("sendrawtransaction", {1, 10, true});      // Very restrictive
        SetMethodLimit("invokefunction", {5, 100, true});         // Moderate
        SetMethodLimit("invokescript", {5, 100, true});           // Moderate
        SetMethodLimit("getblock", {10, 300, true});              // Standard
        SetMethodLimit("getblockcount", {30, 1000, true});        // Lenient
        SetMethodLimit("getconnectioncount", {30, 1000, true});   // Lenient
    }
    
    /**
     * @brief Check if a request is allowed
     * @param clientId Client identifier
     * @param method RPC method name
     * @return true if allowed, false if rate limited
     */
    bool IsAllowed(const std::string& clientId, const std::string& method) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Get method-specific limiter or use default
        auto it = methodLimiters_.find(method);
        if (it != methodLimiters_.end()) {
            return it->second->IsAllowed(clientId, method);
        }
        
        return defaultLimiter_->IsAllowed(clientId, method);
    }
    
    /**
     * @brief Set rate limit for a specific method
     * @param method Method name
     * @param config Rate limit configuration
     */
    void SetMethodLimit(const std::string& method, const MethodConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        RateLimiter::Config limiterConfig;
        limiterConfig.requestsPerSecond = config.requestsPerSecond;
        limiterConfig.requestsPerMinute = config.requestsPerMinute;
        limiterConfig.burstSize = config.requestsPerSecond * 2;
        limiterConfig.enabled = config.enabled;
        
        methodLimiters_[method] = std::make_unique<RateLimiter>(limiterConfig);
    }
    
    /**
     * @brief Set default rate limit for methods without specific limits
     * @param config Rate limit configuration
     */
    void SetDefaultLimit(const MethodConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        RateLimiter::Config limiterConfig;
        limiterConfig.requestsPerSecond = config.requestsPerSecond;
        limiterConfig.requestsPerMinute = config.requestsPerMinute;
        limiterConfig.burstSize = config.requestsPerSecond * 2;
        limiterConfig.enabled = config.enabled;
        
        defaultLimiter_ = std::make_unique<RateLimiter>(limiterConfig);
    }
    
    /**
     * @brief Reset rate limits for a specific client
     * @param clientId Client identifier
     */
    void ResetClient(const std::string& clientId) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto& [method, limiter] : methodLimiters_) {
            limiter->ResetClient(clientId);
        }
        defaultLimiter_->ResetClient(clientId);
    }
    
private:
    std::unordered_map<std::string, std::unique_ptr<RateLimiter>> methodLimiters_;
    std::unique_ptr<RateLimiter> defaultLimiter_ = 
        std::make_unique<RateLimiter>(RateLimiter::Config{10, 300, 20, true});
    mutable std::mutex mutex_;
};

} // namespace neo::rpc