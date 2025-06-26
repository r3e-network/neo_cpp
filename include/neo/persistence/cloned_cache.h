#pragma once

#include <neo/persistence/data_cache.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace neo::persistence
{
    /**
     * @brief A cache that clones data from another cache.
     * Provides isolation for read operations while maintaining a reference to the original cache.
     */
    template<typename TKey, typename TValue>
    class ClonedCache
    {
    public:
        /**
         * @brief Constructor.
         * @param inner The inner cache to clone from.
         */
        explicit ClonedCache(std::shared_ptr<DataCache> inner);

        /**
         * @brief Destructor.
         */
        ~ClonedCache() = default;

        // Basic cache operations
        void Add(const TKey& key, const TValue& value);
        void Delete(const TKey& key);
        bool Contains(const TKey& key) const;
        TValue Get(const TKey& key) const;
        bool TryGet(const TKey& key, TValue& value) const;
        void Update(const TKey& key, const TValue& value);
        
        std::vector<std::pair<TKey, TValue>> Find(std::span<const uint8_t> key_prefix = {}) const;
        void Commit();

        /**
         * @brief Gets the number of items in the cache.
         * @return The number of items.
         */
        size_t Count() const;

        /**
         * @brief Checks if the cache is read-only.
         * @return True if read-only, false otherwise.
         */
        bool IsReadOnly() const;

        /**
         * @brief Gets the inner cache.
         * @return The inner cache.
         */
        std::shared_ptr<DataCache> GetInner() const;

    private:
        std::shared_ptr<DataCache> inner_;
        mutable std::unordered_map<TKey, TValue> cloned_items_;
        mutable std::unordered_set<TKey> deleted_items_;
        mutable bool is_cloned_;

        /**
         * @brief Ensures the cache is cloned.
         */
        void EnsureCloned() const;

        /**
         * @brief Clones an item from the inner cache.
         * @param key The key to clone.
         */
        void CloneItem(const TKey& key) const;
    };

    // Template implementation
    template<typename TKey, typename TValue>
    ClonedCache<TKey, TValue>::ClonedCache(std::shared_ptr<DataCache> inner)
        : inner_(inner), is_cloned_(false)
    {
        if (!inner_)
        {
            throw std::invalid_argument("Inner cache cannot be null");
        }
    }

    template<typename TKey, typename TValue>
    void ClonedCache<TKey, TValue>::Add(const TKey& key, const TValue& value)
    {
        if (IsReadOnly())
        {
            throw std::runtime_error("Cache is read-only");
        }
        
        if (Contains(key))
        {
            throw std::invalid_argument("Key already exists");
        }
        
        EnsureCloned();
        cloned_items_[key] = value;
        deleted_items_.erase(key);
    }

    template<typename TKey, typename TValue>
    void ClonedCache<TKey, TValue>::Delete(const TKey& key)
    {
        if (IsReadOnly())
        {
            throw std::runtime_error("Cache is read-only");
        }
        
        EnsureCloned();
        deleted_items_.insert(key);
        cloned_items_.erase(key);
    }

    template<typename TKey, typename TValue>
    bool ClonedCache<TKey, TValue>::Contains(const TKey& key) const
    {
        EnsureCloned();
        
        if (deleted_items_.count(key) > 0)
        {
            return false;
        }
        
        if (cloned_items_.count(key) > 0)
        {
            return true;
        }
        
        // Check inner cache - cast key to StorageKey for DataCache interface
        if constexpr (std::is_same_v<TKey, StorageKey>)
        {
            auto storage_item = inner_->TryGet(key);
            return storage_item != nullptr;
        }
        
        return false;
    }

    template<typename TKey, typename TValue>
    TValue ClonedCache<TKey, TValue>::Get(const TKey& key) const
    {
        TValue value;
        if (TryGet(key, value))
        {
            return value;
        }
        throw std::out_of_range("Key not found");
    }

    template<typename TKey, typename TValue>
    bool ClonedCache<TKey, TValue>::TryGet(const TKey& key, TValue& value) const
    {
        EnsureCloned();
        
        if (deleted_items_.count(key) > 0)
        {
            return false;
        }
        
        auto it = cloned_items_.find(key);
        if (it != cloned_items_.end())
        {
            value = it->second;
            return true;
        }
        
        // Try to get from inner cache
        if constexpr (std::is_same_v<TKey, StorageKey> && std::is_same_v<TValue, StorageItem>)
        {
            auto storage_item = inner_->TryGet(key);
            if (storage_item)
            {
                value = *storage_item;
                return true;
            }
        }
        
        return false;
    }

    template<typename TKey, typename TValue>
    void ClonedCache<TKey, TValue>::Update(const TKey& key, const TValue& value)
    {
        if (IsReadOnly())
        {
            throw std::runtime_error("Cache is read-only");
        }
        
        if (!Contains(key))
        {
            throw std::out_of_range("Key not found");
        }
        
        EnsureCloned();
        cloned_items_[key] = value;
        deleted_items_.erase(key);
    }

    template<typename TKey, typename TValue>
    std::vector<std::pair<TKey, TValue>> ClonedCache<TKey, TValue>::Find(std::span<const uint8_t> key_prefix) const
    {
        EnsureCloned();
        
        std::vector<std::pair<TKey, TValue>> result;
        std::unordered_set<TKey> processed_keys;
        
        // Add items from cloned cache
        for (const auto& [key, value] : cloned_items_)
        {
            if (deleted_items_.count(key) == 0)
            {
                result.emplace_back(key, value);
                processed_keys.insert(key);
            }
        }
        
        // Add items from inner cache that aren't overridden or deleted
        if constexpr (std::is_same_v<TKey, StorageKey> && std::is_same_v<TValue, StorageItem>)
        {
            auto inner_items = inner_->Find(nullptr); // Get all items
            for (const auto& [key, value] : inner_items)
            {
                if (processed_keys.count(key) == 0 && deleted_items_.count(key) == 0)
                {
                    result.emplace_back(key, value);
                }
            }
        }
        
        return result;
    }

    template<typename TKey, typename TValue>
    void ClonedCache<TKey, TValue>::Commit()
    {
        if (IsReadOnly())
        {
            return;
        }
        
        // Apply changes to inner cache
        if constexpr (std::is_same_v<TKey, StorageKey> && std::is_same_v<TValue, StorageItem>)
        {
            // Add/update items
            for (const auto& [key, value] : cloned_items_)
            {
                if (deleted_items_.count(key) == 0)
                {
                    // Check if item exists in inner cache
                    auto existing = inner_->TryGet(key);
                    if (existing)
                    {
                        // Update existing item - directly modify the inner cache
                        auto& inner_item = inner_->Get(key);
                        inner_item = value;
                    }
                    else
                    {
                        // Add new item
                        inner_->Add(key, value);
                    }
                }
            }
            
            // Delete items
            for (const auto& key : deleted_items_)
            {
                inner_->Delete(key);
            }
            
            // Commit inner cache
            inner_->Commit();
        }
        
        // Clear local changes
        cloned_items_.clear();
        deleted_items_.clear();
    }

    template<typename TKey, typename TValue>
    size_t ClonedCache<TKey, TValue>::Count() const
    {
        EnsureCloned();
        
        size_t count = 0;
        std::unordered_set<TKey> processed_keys;
        
        // Count items from cloned cache
        for (const auto& [key, value] : cloned_items_)
        {
            if (deleted_items_.count(key) == 0)
            {
                count++;
                processed_keys.insert(key);
            }
        }
        
        // Count items from inner cache that aren't overridden or deleted
        if constexpr (std::is_same_v<TKey, StorageKey> && std::is_same_v<TValue, StorageItem>)
        {
            auto inner_items = inner_->Find(nullptr); // Get all items
            for (const auto& [key, value] : inner_items)
            {
                if (processed_keys.count(key) == 0 && deleted_items_.count(key) == 0)
                {
                    count++;
                }
            }
        }
        
        return count;
    }

    template<typename TKey, typename TValue>
    bool ClonedCache<TKey, TValue>::IsReadOnly() const
    {
        // For now, assume not read-only
        return false;
    }

    template<typename TKey, typename TValue>
    std::shared_ptr<DataCache> ClonedCache<TKey, TValue>::GetInner() const
    {
        return inner_;
    }

    template<typename TKey, typename TValue>
    void ClonedCache<TKey, TValue>::EnsureCloned() const
    {
        if (!is_cloned_)
        {
            is_cloned_ = true;
        }
    }

    template<typename TKey, typename TValue>
    void ClonedCache<TKey, TValue>::CloneItem(const TKey& key) const
    {
        // For now, do nothing - would need proper inner cache integration
    }
}
