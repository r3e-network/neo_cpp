#pragma once

#include <list>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <unordered_map>

namespace neo::io
{
/**
 * @brief A thread-safe Least Recently Used (LRU) cache.
 * @tparam TKey The type of the keys.
 * @tparam TValue The type of the values.
 */
template <typename TKey, typename TValue>
class LRUCache
{
   public:
    /**
     * @brief Constructs an LRUCache with the specified capacity.
     * @param capacity The maximum number of items the cache can hold.
     * @throws std::invalid_argument if capacity is zero.
     */
    explicit LRUCache(size_t capacity) : capacity_(capacity)
    {
        if (capacity == 0) throw std::invalid_argument("Capacity must be greater than zero");
    }

    /**
     * @brief Tries to get a value from the cache.
     * @param key The key to look up.
     * @return The value if found, std::nullopt otherwise.
     */
    std::optional<TValue> TryGet(const TKey& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cache_.find(key);
        if (it == cache_.end()) return std::nullopt;

        // Move the key to the front of the list
        keys_.erase(it->second.first);
        keys_.push_front(key);
        it->second.first = keys_.begin();

        return it->second.second;
    }

    /**
     * @brief Adds or updates a value in the cache.
     * @param key The key.
     * @param value The value.
     */
    void Add(const TKey& key, const TValue& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // If capacity is zero, don't store anything
        if (capacity_ == 0) return;

        auto it = cache_.find(key);
        if (it != cache_.end())
        {
            // Update existing item
            keys_.erase(it->second.first);
            keys_.push_front(key);
            it->second = std::make_pair(keys_.begin(), value);
        }
        else
        {
            // Add new item
            keys_.push_front(key);
            cache_[key] = std::make_pair(keys_.begin(), value);

            // Remove oldest item if cache is full
            if (cache_.size() > capacity_)
            {
                auto last = keys_.back();
                keys_.pop_back();
                cache_.erase(last);
            }
        }
    }

    /**
     * @brief Removes a value from the cache.
     * @param key The key.
     * @return True if the key was found and removed, false otherwise.
     */
    bool Remove(const TKey& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cache_.find(key);
        if (it == cache_.end()) return false;

        keys_.erase(it->second.first);
        cache_.erase(it);

        return true;
    }

    /**
     * @brief Clears the cache.
     */
    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        keys_.clear();
        cache_.clear();
    }

    /**
     * @brief Gets the number of items in the cache.
     * @return The number of items in the cache.
     */
    size_t Count() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }

    /**
     * @brief Gets the capacity of the cache.
     * @return The capacity of the cache.
     */
    size_t Capacity() const { return capacity_; }

    // Alias methods for compatibility
    /**
     * @brief Alias for TryGet.
     * @param key The key to look up.
     * @return The value if found, std::nullopt otherwise.
     */
    std::optional<TValue> Get(const TKey& key) { return TryGet(key); }

    /**
     * @brief Alias for Add.
     * @param key The key.
     * @param value The value.
     */
    void Put(const TKey& key, const TValue& value) { Add(key, value); }

    /**
     * @brief Alias for Count.
     * @return The number of items in the cache.
     */
    size_t Size() const { return Count(); }

    /**
     * @brief Checks if a key exists in the cache.
     * @param key The key to check.
     * @return True if the key exists, false otherwise.
     */
    bool Contains(const TKey& key) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.find(key) != cache_.end();
    }

   private:
    size_t capacity_;
    std::list<TKey> keys_;
    std::unordered_map<TKey, std::pair<typename std::list<TKey>::iterator, TValue>> cache_;
    mutable std::mutex mutex_;
};
}  // namespace neo::io
