/**
 * @file lru_cache.h
 * @brief Caching mechanisms
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace neo::io::caching
{
/**
 * @brief A thread-safe LRU (Least Recently Used) cache implementation.
 * @tparam TKey The type of the keys.
 * @tparam TValue The type of the values.
 * @tparam THash The hash function for the keys.
 */
template <typename TKey, typename TValue, typename THash = std::hash<TKey>>
class LRUCache
{
   public:
    /**
     * @brief Constructs an LRUCache with the specified capacity.
     * @param capacity The maximum number of items the cache can hold.
     */
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    /**
     * @brief Adds or updates an item in the cache.
     * @param key The key of the item.
     * @param value The value of the item.
     */
    void Add(const TKey& key, const TValue& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if the key already exists
        auto it = cache_.find(key);
        if (it != cache_.end())
        {
            // Move the item to the front of the list
            items_.erase(it->second.first);
            items_.push_front(std::make_pair(key, value));
            it->second.first = items_.begin();
            it->second.second = value;
        }
        else
        {
            // Check if the cache is full
            if (items_.size() >= capacity_)
            {
                // Remove the least recently used item
                auto last = items_.back();
                cache_.erase(last.first);
                items_.pop_back();
            }

            // Add the new item
            items_.push_front(std::make_pair(key, value));
            cache_[key] = std::make_pair(items_.begin(), value);
        }
    }

    /**
     * @brief Tries to get an item from the cache.
     * @param key The key of the item.
     * @param value The value of the item if found.
     * @return True if the item was found, false otherwise.
     */
    bool TryGet(const TKey& key, TValue& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if the key exists
        auto it = cache_.find(key);
        if (it == cache_.end()) return false;

        // Move the item to the front of the list
        items_.erase(it->second.first);
        items_.push_front(std::make_pair(key, it->second.second));
        it->second.first = items_.begin();

        // Return the value
        value = it->second.second;
        return true;
    }

    /**
     * @brief Gets an item from the cache.
     * @param key The key of the item.
     * @return The value of the item if found, std::nullopt otherwise.
     */
    std::optional<TValue> Get(const TKey& key)
    {
        TValue value;
        if (TryGet(key, value)) return value;
        return std::nullopt;
    }

    /**
     * @brief Removes an item from the cache.
     * @param key The key of the item.
     * @return True if the item was removed, false otherwise.
     */
    bool Remove(const TKey& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if the key exists
        auto it = cache_.find(key);
        if (it == cache_.end()) return false;

        // Remove the item
        items_.erase(it->second.first);
        cache_.erase(it);
        return true;
    }

    /**
     * @brief Clears the cache.
     */
    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        items_.clear();
        cache_.clear();
    }

    /**
     * @brief Gets the number of items in the cache.
     * @return The number of items in the cache.
     */
    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.size();
    }

    /**
     * @brief Gets the capacity of the cache.
     * @return The capacity of the cache.
     */
    size_t Capacity() const { return capacity_; }

   private:
    size_t capacity_;
    std::list<std::pair<TKey, TValue>> items_;
    std::unordered_map<TKey, std::pair<typename std::list<std::pair<TKey, TValue>>::iterator, TValue>, THash> cache_;
    mutable std::mutex mutex_;
};
}  // namespace neo::io::caching
