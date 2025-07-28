#pragma once

#include <chrono>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace neo::cache
{
/**
 * @brief Generic cache interface for Neo components.
 *
 * Provides thread-safe caching functionality with TTL support
 * for transaction verification and other blockchain operations.
 */
template <typename Key, typename Value>
class Cache
{
  public:
    using KeyType = Key;
    using ValueType = Value;
    using TimePoint = std::chrono::steady_clock::time_point;

    struct CacheEntry
    {
        Value value;
        TimePoint expiry;

        CacheEntry(const Value& v, TimePoint exp) : value(v), expiry(exp) {}
        CacheEntry(Value&& v, TimePoint exp) : value(std::move(v)), expiry(exp) {}
    };

    /**
     * @brief Constructor with maximum size and default TTL.
     * @param maxSize Maximum number of entries (0 = unlimited)
     * @param defaultTtl Default time-to-live in milliseconds
     */
    explicit Cache(size_t maxSize = 1000, uint64_t defaultTtl = 300000)  // 5 minutes default
        : maxSize_(maxSize), defaultTtl_(std::chrono::milliseconds(defaultTtl))
    {
    }

    /**
     * @brief Get value from cache.
     * @param key The key to look up
     * @return Pointer to value if found and not expired, nullptr otherwise
     */
    std::shared_ptr<Value> Get(const Key& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cache_.find(key);
        if (it == cache_.end())
        {
            return nullptr;
        }

        // Check if expired
        auto now = std::chrono::steady_clock::now();
        if (now > it->second.expiry)
        {
            cache_.erase(it);
            return nullptr;
        }

        return std::make_shared<Value>(it->second.value);
    }

    /**
     * @brief Put value in cache with default TTL.
     * @param key The key
     * @param value The value
     */
    void Put(const Key& key, const Value& value)
    {
        Put(key, value, defaultTtl_);
    }

    /**
     * @brief Put value in cache with custom TTL.
     * @param key The key
     * @param value The value
     * @param ttl Time-to-live duration
     */
    void Put(const Key& key, const Value& value, std::chrono::milliseconds ttl)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto expiry = std::chrono::steady_clock::now() + ttl;

        // If at max size, remove oldest entry
        if (maxSize_ > 0 && cache_.size() >= maxSize_)
        {
            // Simple LRU: remove first entry (not optimal but functional)
            if (!cache_.empty())
            {
                cache_.erase(cache_.begin());
            }
        }

        cache_[key] = CacheEntry(value, expiry);
    }

    /**
     * @brief Remove entry from cache.
     * @param key The key to remove
     * @return true if entry was found and removed
     */
    bool Remove(const Key& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.erase(key) > 0;
    }

    /**
     * @brief Clear all entries from cache.
     */
    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
    }

    /**
     * @brief Get current cache size.
     * @return Number of entries in cache
     */
    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }

    /**
     * @brief Check if cache contains key.
     * @param key The key to check
     * @return true if key exists and not expired
     */
    bool Contains(const Key& key)
    {
        return Get(key) != nullptr;
    }

    /**
     * @brief Clean up expired entries.
     * @return Number of entries removed
     */
    size_t CleanupExpired()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto now = std::chrono::steady_clock::now();
        size_t removed = 0;

        for (auto it = cache_.begin(); it != cache_.end();)
        {
            if (now > it->second.expiry)
            {
                it = cache_.erase(it);
                ++removed;
            }
            else
            {
                ++it;
            }
        }

        return removed;
    }

  private:
    mutable std::mutex mutex_;
    std::unordered_map<Key, CacheEntry> cache_;
    size_t maxSize_;
    std::chrono::milliseconds defaultTtl_;
};

/**
 * @brief Specialized cache for string keys.
 */
template <typename Value>
using StringCache = Cache<std::string, Value>;

/**
 * @brief Specialized cache for hash keys (UInt256).
 */
template <typename Value>
using HashCache = Cache<std::string, Value>;  // Using string representation of hash

/**
 * @brief LRU Cache implementation.
 */
template <typename Key, typename Value>
class LRUCache
{
  public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    std::shared_ptr<Value> Get(const Key& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cache_.find(key);
        if (it == cache_.end())
        {
            return nullptr;
        }

        // Move to front
        order_.splice(order_.begin(), order_, it->second.second);
        return std::make_shared<Value>(it->second.first);
    }

    void Put(const Key& key, const Value& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cache_.find(key);
        if (it != cache_.end())
        {
            // Update existing
            it->second.first = value;
            order_.splice(order_.begin(), order_, it->second.second);
            return;
        }

        // Add new entry
        if (cache_.size() >= capacity_)
        {
            // Remove least recently used
            auto last = order_.back();
            cache_.erase(last);
            order_.pop_back();
        }

        order_.push_front(key);
        cache_[key] = std::make_pair(value, order_.begin());
    }

    bool Remove(const Key& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cache_.find(key);
        if (it == cache_.end())
        {
            return false;
        }

        order_.erase(it->second.second);
        cache_.erase(it);
        return true;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        order_.clear();
    }

    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }

  private:
    mutable std::mutex mutex_;
    size_t capacity_;
    std::list<Key> order_;
    std::unordered_map<Key, std::pair<Value, typename std::list<Key>::iterator>> cache_;
};
}  // namespace neo::cache