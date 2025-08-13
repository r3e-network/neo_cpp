/**
 * @file rate_limiter.h
 * @brief RPC Rate Limiter to prevent DoS attacks
 */

#pragma once

#include <chrono>
#include <unordered_map>
#include <mutex>
#include <string>
#include <deque>

namespace neo {
namespace rpc {

/**
 * @brief Token bucket rate limiter for RPC endpoints
 */
class RateLimiter {
public:
    struct Config {
        size_t requests_per_second;      // Default: 10 requests per second
        size_t burst_size;               // Allow burst of 20 requests
        size_t requests_per_minute;     // Max 100 requests per minute
        bool enable_ip_limiting;       // Limit per IP address
        bool enable_method_limiting;   // Limit per method
        
        Config() 
            : requests_per_second(10)
            , burst_size(20)
            , requests_per_minute(100)
            , enable_ip_limiting(true)
            , enable_method_limiting(true) {}
    };

private:
    struct TokenBucket {
        size_t tokens;
        std::chrono::steady_clock::time_point last_refill;
        std::deque<std::chrono::steady_clock::time_point> minute_window;
        
        TokenBucket(size_t initial_tokens) 
            : tokens(initial_tokens)
            , last_refill(std::chrono::steady_clock::now()) {}
    };

    Config config_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, TokenBucket> ip_buckets_;
    std::unordered_map<std::string, TokenBucket> method_buckets_;
    TokenBucket global_bucket_;

public:
    explicit RateLimiter(const Config& config = Config())
        : config_(config)
        , global_bucket_(config.burst_size) {}

    /**
     * @brief Check if request is allowed
     * @param ip_address Client IP address
     * @param method RPC method name
     * @return true if request is allowed, false if rate limited
     */
    bool AllowRequest(const std::string& ip_address, const std::string& method) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        
        // Check global rate limit
        if (!CheckAndConsumeBucket(global_bucket_, now)) {
            return false;
        }
        
        // Check IP-based rate limit
        if (config_.enable_ip_limiting && !ip_address.empty()) {
            auto& bucket = GetOrCreateBucket(ip_buckets_, ip_address);
            if (!CheckAndConsumeBucket(bucket, now)) {
                return false;
            }
        }
        
        // Check method-based rate limit
        if (config_.enable_method_limiting && !method.empty()) {
            auto& bucket = GetOrCreateBucket(method_buckets_, method);
            if (!CheckAndConsumeBucket(bucket, now)) {
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Reset rate limiter for specific IP
     * @param ip_address IP address to reset
     */
    void ResetIP(const std::string& ip_address) {
        std::lock_guard<std::mutex> lock(mutex_);
        ip_buckets_.erase(ip_address);
    }

    /**
     * @brief Get current statistics
     */
    struct Stats {
        size_t total_requests = 0;
        size_t blocked_requests = 0;
        size_t unique_ips = 0;
    };

    Stats GetStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        Stats stats;
        stats.unique_ips = ip_buckets_.size();
        // Additional stats can be tracked if needed
        return stats;
    }

private:
    TokenBucket& GetOrCreateBucket(std::unordered_map<std::string, TokenBucket>& buckets, 
                                   const std::string& key) {
        auto it = buckets.find(key);
        if (it == buckets.end()) {
            it = buckets.emplace(key, TokenBucket(config_.burst_size)).first;
        }
        return it->second;
    }

    bool CheckAndConsumeBucket(TokenBucket& bucket, 
                               const std::chrono::steady_clock::time_point& now) {
        // Refill tokens based on time elapsed
        RefillTokens(bucket, now);
        
        // Check minute window
        CleanMinuteWindow(bucket, now);
        if (bucket.minute_window.size() >= config_.requests_per_minute) {
            return false; // Exceeded minute limit
        }
        
        // Check if we have tokens
        if (bucket.tokens > 0) {
            bucket.tokens--;
            bucket.minute_window.push_back(now);
            return true;
        }
        
        return false;
    }

    void RefillTokens(TokenBucket& bucket, 
                     const std::chrono::steady_clock::time_point& now) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - bucket.last_refill).count();
        
        // Refill tokens based on elapsed time
        size_t tokens_to_add = (elapsed * config_.requests_per_second) / 1000;
        if (tokens_to_add > 0) {
            bucket.tokens = std::min(bucket.tokens + tokens_to_add, config_.burst_size);
            bucket.last_refill = now;
        }
    }

    void CleanMinuteWindow(TokenBucket& bucket, 
                           const std::chrono::steady_clock::time_point& now) {
        auto minute_ago = now - std::chrono::minutes(1);
        while (!bucket.minute_window.empty() && bucket.minute_window.front() < minute_ago) {
            bucket.minute_window.pop_front();
        }
    }
};

} // namespace rpc
} // namespace neo