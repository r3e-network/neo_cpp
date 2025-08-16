#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <deque>

namespace neo {
namespace security {

/**
 * @brief Rate limiter for API and network requests
 */
class RateLimiter {
public:
    /**
     * @brief Configuration for rate limiter
     */
    struct Config {
        size_t requests_per_minute = 60;
        size_t burst_size = 10;
        std::chrono::minutes ban_duration = std::chrono::minutes(5);
        size_t max_violations_before_ban = 5;
    };
    
    /**
     * @brief Decision enum for rate limiter responses
     */
    enum class Decision {
        ALLOW,
        RATE_LIMITED,
        BANNED
    };
    
    /**
     * @brief Constructor with configuration
     * @param config Rate limiter configuration
     */
    explicit RateLimiter(const Config& config = Config())
        : config_(config), maxRequests_(config.requests_per_minute), timeWindow_(60) {}
    
    /**
     * @brief Constructor
     * @param maxRequests Maximum requests allowed in the time window
     * @param timeWindow Time window in seconds
     */
    RateLimiter(size_t maxRequests, size_t timeWindow)
        : config_{maxRequests, 10, std::chrono::minutes(5), 5},
          maxRequests_(maxRequests), timeWindow_(timeWindow) {}
    
    /**
     * @brief Check if a request is allowed
     * @param identifier Client identifier (IP address, user ID, etc.)
     * @return Decision enum indicating if allowed, rate limited, or banned
     */
    Decision CheckRequest(const std::string& identifier) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        
        // Check if client is banned
        auto ban_it = banned_clients_.find(identifier);
        if (ban_it != banned_clients_.end()) {
            if (now < ban_it->second) {
                return Decision::BANNED;
            }
            // Ban expired, remove it
            banned_clients_.erase(ban_it);
            violations_[identifier] = 0;
        }
        
        auto& timestamps = requests_[identifier];
        
        // Remove old timestamps outside the window
        while (!timestamps.empty()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - timestamps.front()).count();
            if (elapsed >= timeWindow_) {
                timestamps.pop_front();
            } else {
                break;
            }
        }
        
        // Check if under rate limit
        if (timestamps.size() < config_.requests_per_minute) {
            timestamps.push_back(now);
            violations_[identifier] = 0;  // Reset violations on successful request
            return Decision::ALLOW;
        }
        
        // Rate limited - increment violations
        violations_[identifier]++;
        
        // Check if should ban
        if (violations_[identifier] >= config_.max_violations_before_ban) {
            banned_clients_[identifier] = now + config_.ban_duration;
            return Decision::BANNED;
        }
        
        return Decision::RATE_LIMITED;
    }
    
    /**
     * @brief Check if a request is allowed (backward compatibility)
     * @param identifier Client identifier (IP, user ID, etc.)
     * @return true if allowed, false if rate limit exceeded
     */
    bool IsAllowed(const std::string& identifier) {
        return CheckRequest(identifier) == Decision::ALLOW;
    }
    
    /**
     * @brief Reset rate limit for a specific identifier
     * @param identifier Client identifier
     */
    void Reset(const std::string& identifier) {
        std::lock_guard<std::mutex> lock(mutex_);
        requests_.erase(identifier);
        violations_.erase(identifier);
        banned_clients_.erase(identifier);
    }
    
    /**
     * @brief Clear all rate limit data
     */
    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        requests_.clear();
        violations_.clear();
        banned_clients_.clear();
    }
    
    /**
     * @brief Get remaining requests for an identifier
     * @param identifier Client identifier
     * @return Number of remaining requests
     */
    size_t GetRemainingRequests(const std::string& identifier) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        auto& timestamps = requests_[identifier];
        
        // Clean old timestamps
        while (!timestamps.empty()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - timestamps.front()).count();
            if (elapsed >= timeWindow_) {
                timestamps.pop_front();
            } else {
                break;
            }
        }
        
        return maxRequests_ - timestamps.size();
    }
    
    /**
     * @brief Get time until rate limit resets
     * @param identifier Client identifier
     * @return Seconds until oldest request expires
     */
    size_t GetResetTime(const std::string& identifier) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = requests_.find(identifier);
        if (it == requests_.end() || it->second.empty()) {
            return 0;
        }
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - it->second.front()).count();
        
        if (elapsed >= timeWindow_) {
            return 0;
        }
        
        return timeWindow_ - elapsed;
    }
    
    /**
     * @brief Set new rate limit parameters
     * @param maxRequests Maximum requests in time window
     * @param timeWindow Time window in seconds
     */
    void SetLimits(size_t maxRequests, size_t timeWindow) {
        std::lock_guard<std::mutex> lock(mutex_);
        maxRequests_ = maxRequests;
        timeWindow_ = timeWindow;
    }
    
    /**
     * @brief Get current statistics
     * @return Map of identifiers to request counts
     */
    std::unordered_map<std::string, size_t> GetStatistics() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::unordered_map<std::string, size_t> stats;
        auto now = std::chrono::steady_clock::now();
        
        for (auto& [identifier, timestamps] : requests_) {
            // Count valid timestamps
            size_t count = 0;
            for (const auto& ts : timestamps) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    now - ts).count();
                if (elapsed < timeWindow_) {
                    count++;
                }
            }
            if (count > 0) {
                stats[identifier] = count;
            }
        }
        
        return stats;
    }

private:
    Config config_;
    size_t maxRequests_;
    size_t timeWindow_;  // in seconds
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::deque<std::chrono::steady_clock::time_point>> requests_;
    std::unordered_map<std::string, size_t> violations_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> banned_clients_;
};

} // namespace security
} // namespace neo