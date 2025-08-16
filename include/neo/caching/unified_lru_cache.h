#pragma once

/**
 * @file unified_lru_cache.h
 * @brief Unified LRU (Least Recently Used) cache implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 * 
 * This file consolidates multiple duplicate LRUCache implementations
 * into a single, thread-safe, feature-rich cache class.
 */

#include <algorithm>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace neo::caching {

/**
 * @brief Thread-safe LRU (Least Recently Used) cache
 * 
 * This class consolidates all LRUCache implementations from:
 * - neo::ledger::blockchain_cache::LRUCache
 * - neo::cache::LRUCache
 * - neo::io::LRUCache
 * - neo::io::caching::LRUCache
 * 
 * @tparam TKey Key type
 * @tparam TValue Value type
 * @tparam THash Hash function for key type (default: std::hash<TKey>)
 * @tparam TEqual Equality function for key type (default: std::equal_to<TKey>)
 */
template<typename TKey, 
         typename TValue,
         typename THash = std::hash<TKey>,
         typename TEqual = std::equal_to<TKey>>
class UnifiedLRUCache {
public:
    using key_type = TKey;
    using value_type = TValue;
    using size_type = size_t;
    using callback_type = std::function<void(const TKey&, const TValue&)>;

private:
    // Internal node structure
    struct CacheNode {
        TKey key;
        TValue value;
        typename std::list<CacheNode>::iterator position;
    };

    using NodeList = std::list<CacheNode>;
    using NodeMap = std::unordered_map<TKey, typename NodeList::iterator, THash, TEqual>;

public:
    /**
     * @brief Construct cache with maximum capacity
     * @param max_size Maximum number of entries (0 = unlimited)
     * @param thread_safe Enable thread safety (default: true)
     */
    explicit UnifiedLRUCache(size_type max_size = 1000, bool thread_safe = true)
        : max_size_(max_size), thread_safe_(thread_safe), hits_(0), misses_(0) {}

    /**
     * @brief Destructor
     */
    ~UnifiedLRUCache() {
        clear();
    }

    // Disable copy operations
    UnifiedLRUCache(const UnifiedLRUCache&) = delete;
    UnifiedLRUCache& operator=(const UnifiedLRUCache&) = delete;

    // Enable move operations
    UnifiedLRUCache(UnifiedLRUCache&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex_);
        max_size_ = other.max_size_;
        thread_safe_ = other.thread_safe_;
        list_ = std::move(other.list_);
        map_ = std::move(other.map_);
        hits_ = other.hits_;
        misses_ = other.misses_;
        eviction_callback_ = std::move(other.eviction_callback_);
    }

    UnifiedLRUCache& operator=(UnifiedLRUCache&& other) noexcept {
        if (this != &other) {
            std::lock_guard<std::mutex> lock1(mutex_);
            std::lock_guard<std::mutex> lock2(other.mutex_);
            max_size_ = other.max_size_;
            thread_safe_ = other.thread_safe_;
            list_ = std::move(other.list_);
            map_ = std::move(other.map_);
            hits_ = other.hits_;
            misses_ = other.misses_;
            eviction_callback_ = std::move(other.eviction_callback_);
        }
        return *this;
    }

    // ============= Core Operations =============

    /**
     * @brief Insert or update a key-value pair
     * @param key The key
     * @param value The value
     * @return true if inserted, false if updated
     */
    bool put(const TKey& key, const TValue& value) {
        auto lock = getLock();
        
        auto it = map_.find(key);
        if (it != map_.end()) {
            // Update existing entry and move to front
            it->second->value = value;
            moveToFront(it->second);
            return false;
        }

        // Insert new entry
        if (max_size_ > 0 && list_.size() >= max_size_) {
            evictLRU();
        }

        list_.emplace_front(CacheNode{key, value, list_.begin()});
        list_.front().position = list_.begin();
        map_[key] = list_.begin();
        return true;
    }

    /**
     * @brief Get value by key
     * @param key The key to look up
     * @return Optional containing value if found
     */
    std::optional<TValue> get(const TKey& key) {
        auto lock = getLock();
        
        auto it = map_.find(key);
        if (it == map_.end()) {
            misses_++;
            return std::nullopt;
        }

        hits_++;
        moveToFront(it->second);
        return it->second->value;
    }

    /**
     * @brief Get value by key with default
     * @param key The key to look up
     * @param default_value Value to return if key not found
     * @return The value or default_value
     */
    TValue get_or_default(const TKey& key, const TValue& default_value) {
        auto result = get(key);
        return result.has_value() ? result.value() : default_value;
    }

    /**
     * @brief Get or create value using factory function
     * @param key The key
     * @param factory Function to create value if not found
     * @return The value (either from cache or newly created)
     */
    TValue get_or_create(const TKey& key, std::function<TValue()> factory) {
        auto lock = getLock();
        
        auto it = map_.find(key);
        if (it != map_.end()) {
            hits_++;
            moveToFront(it->second);
            return it->second->value;
        }

        misses_++;
        TValue value = factory();
        
        // Insert without recursive locking
        if (max_size_ > 0 && list_.size() >= max_size_) {
            evictLRU();
        }
        
        list_.emplace_front(CacheNode{key, value, list_.begin()});
        list_.front().position = list_.begin();
        map_[key] = list_.begin();
        
        return value;
    }

    /**
     * @brief Check if key exists
     * @param key The key to check
     * @return true if key exists
     */
    bool contains(const TKey& key) const {
        auto lock = getLock();
        return map_.find(key) != map_.end();
    }

    /**
     * @brief Remove entry by key
     * @param key The key to remove
     * @return true if removed, false if not found
     */
    bool remove(const TKey& key) {
        auto lock = getLock();
        
        auto it = map_.find(key);
        if (it == map_.end()) {
            return false;
        }

        list_.erase(it->second);
        map_.erase(it);
        return true;
    }

    /**
     * @brief Clear all entries
     */
    void clear() {
        auto lock = getLock();
        
        if (eviction_callback_) {
            for (const auto& node : list_) {
                eviction_callback_(node.key, node.value);
            }
        }
        
        list_.clear();
        map_.clear();
        hits_ = 0;
        misses_ = 0;
    }

    // ============= Configuration =============

    /**
     * @brief Set maximum cache size
     * @param max_size New maximum size (0 = unlimited)
     */
    void set_max_size(size_type max_size) {
        auto lock = getLock();
        max_size_ = max_size;
        
        while (max_size_ > 0 && list_.size() > max_size_) {
            evictLRU();
        }
    }

    /**
     * @brief Set eviction callback
     * @param callback Function called when entry is evicted
     */
    void set_eviction_callback(callback_type callback) {
        auto lock = getLock();
        eviction_callback_ = callback;
    }

    // ============= Statistics =============

    /**
     * @brief Get current size
     */
    size_type size() const {
        auto lock = getLock();
        return list_.size();
    }

    /**
     * @brief Check if cache is empty
     */
    bool empty() const {
        auto lock = getLock();
        return list_.empty();
    }

    /**
     * @brief Get maximum size
     */
    size_type max_size() const {
        return max_size_;
    }

    /**
     * @brief Get cache hit count
     */
    uint64_t hits() const {
        auto lock = getLock();
        return hits_;
    }

    /**
     * @brief Get cache miss count
     */
    uint64_t misses() const {
        auto lock = getLock();
        return misses_;
    }

    /**
     * @brief Get hit rate (0.0 to 1.0)
     */
    double hit_rate() const {
        auto lock = getLock();
        uint64_t total = hits_ + misses_;
        return total > 0 ? static_cast<double>(hits_) / total : 0.0;
    }

    /**
     * @brief Reset statistics
     */
    void reset_stats() {
        auto lock = getLock();
        hits_ = 0;
        misses_ = 0;
    }

    // ============= Iteration =============

    /**
     * @brief Apply function to all entries (oldest to newest)
     */
    void for_each(std::function<void(const TKey&, const TValue&)> func) const {
        auto lock = getLock();
        for (auto it = list_.rbegin(); it != list_.rend(); ++it) {
            func(it->key, it->value);
        }
    }

    /**
     * @brief Get all keys (oldest to newest)
     */
    std::vector<TKey> keys() const {
        auto lock = getLock();
        std::vector<TKey> result;
        result.reserve(list_.size());
        for (auto it = list_.rbegin(); it != list_.rend(); ++it) {
            result.push_back(it->key);
        }
        return result;
    }

private:
    // Move node to front of list (most recently used)
    void moveToFront(typename NodeList::iterator node) {
        if (node == list_.begin()) {
            return;  // Already at front
        }
        list_.splice(list_.begin(), list_, node);
    }

    // Evict least recently used entry
    void evictLRU() {
        if (list_.empty()) {
            return;
        }

        const auto& back = list_.back();
        if (eviction_callback_) {
            eviction_callback_(back.key, back.value);
        }

        map_.erase(back.key);
        list_.pop_back();
    }

    // Get lock (only if thread-safe mode is enabled)
    std::unique_lock<std::mutex> getLock() const {
        if (thread_safe_) {
            return std::unique_lock<std::mutex>(mutex_);
        }
        return std::unique_lock<std::mutex>();
    }

private:
    size_type max_size_;
    bool thread_safe_;
    NodeList list_;  // Front = most recently used, Back = least recently used
    NodeMap map_;
    mutable std::mutex mutex_;
    uint64_t hits_;
    uint64_t misses_;
    callback_type eviction_callback_;
};

// ============= Compatibility Aliases =============

// Default LRU cache types
template<typename K, typename V>
using LRUCache = UnifiedLRUCache<K, V>;

// Fixed-size cache
template<typename K, typename V, size_t MaxSize>
class FixedLRUCache : public UnifiedLRUCache<K, V> {
public:
    FixedLRUCache() : UnifiedLRUCache<K, V>(MaxSize) {}
};

} // namespace neo::caching

// Compatibility namespaces
namespace neo::cache {
    template<typename K, typename V>
    using LRUCache = neo::caching::UnifiedLRUCache<K, V>;
}

namespace neo::io {
    template<typename K, typename V>
    using LRUCache = neo::caching::UnifiedLRUCache<K, V>;
}

namespace neo::io::caching {
    template<typename K, typename V>
    using LRUCache = neo::caching::UnifiedLRUCache<K, V>;
}

namespace neo::ledger {
    template<typename K, typename V>
    using LRUCache = neo::caching::UnifiedLRUCache<K, V>;
}