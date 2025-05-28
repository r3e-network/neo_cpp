#pragma once

#include <neo/persistence/data_cache.h>
#include <memory>
#include <unordered_map>

namespace neo::persistence
{
    /**
     * @brief A cache that clones data from another cache.
     * Provides isolation for read operations while maintaining a reference to the original cache.
     */
    template<typename TKey, typename TValue>
    class ClonedCache : public DataCache<TKey, TValue>
    {
    public:
        /**
         * @brief Constructor.
         * @param inner The inner cache to clone from.
         */
        explicit ClonedCache(std::shared_ptr<DataCache<TKey, TValue>> inner);

        /**
         * @brief Destructor.
         */
        ~ClonedCache() override = default;

        // DataCache interface implementation
        void Add(const TKey& key, const TValue& value) override;
        void Delete(const TKey& key) override;
        bool Contains(const TKey& key) const override;
        TValue Get(const TKey& key) const override;
        bool TryGet(const TKey& key, TValue& value) const override;
        void Update(const TKey& key, const TValue& value) override;
        
        std::vector<std::pair<TKey, TValue>> Find(std::span<const uint8_t> key_prefix = {}) const override;
        void Commit() override;

        /**
         * @brief Gets the number of items in the cache.
         * @return The number of items.
         */
        size_t Count() const override;

        /**
         * @brief Checks if the cache is read-only.
         * @return True if read-only, false otherwise.
         */
        bool IsReadOnly() const override;

    protected:
        /**
         * @brief Gets the inner cache.
         * @return The inner cache.
         */
        std::shared_ptr<DataCache<TKey, TValue>> GetInner() const;

        /**
         * @brief Adds an item to the cache.
         * @param key The key.
         * @param value The value.
         */
        void AddInternal(const TKey& key, const TValue& value) override;

        /**
         * @brief Deletes an item from the cache.
         * @param key The key.
         */
        void DeleteInternal(const TKey& key) override;

        /**
         * @brief Gets an item from the cache.
         * @param key The key.
         * @return The value.
         */
        TValue GetInternal(const TKey& key) const override;

        /**
         * @brief Tries to get an item from the cache.
         * @param key The key.
         * @param value The value output.
         * @return True if found, false otherwise.
         */
        bool TryGetInternal(const TKey& key, TValue& value) const override;

        /**
         * @brief Updates an item in the cache.
         * @param key The key.
         * @param value The value.
         */
        void UpdateInternal(const TKey& key, const TValue& value) override;

    private:
        std::shared_ptr<DataCache<TKey, TValue>> inner_;
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
    ClonedCache<TKey, TValue>::ClonedCache(std::shared_ptr<DataCache<TKey, TValue>> inner)
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
        
        return inner_->Contains(key);
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
        
        return inner_->TryGet(key, value);
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
        
        // Add items from inner cache
        auto inner_items = inner_->Find(key_prefix);
        for (const auto& [key, value] : inner_items)
        {
            if (deleted_items_.count(key) == 0)
            {
                result.emplace_back(key, value);
            }
        }
        
        // Add items from cloned cache
        for (const auto& [key, value] : cloned_items_)
        {
            // Check if key matches prefix (simplified implementation)
            result.emplace_back(key, value);
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
        for (const auto& key : deleted_items_)
        {
            inner_->Delete(key);
        }
        
        for (const auto& [key, value] : cloned_items_)
        {
            if (inner_->Contains(key))
            {
                inner_->Update(key, value);
            }
            else
            {
                inner_->Add(key, value);
            }
        }
        
        inner_->Commit();
        
        // Clear local changes
        cloned_items_.clear();
        deleted_items_.clear();
    }

    template<typename TKey, typename TValue>
    size_t ClonedCache<TKey, TValue>::Count() const
    {
        EnsureCloned();
        return inner_->Count() + cloned_items_.size() - deleted_items_.size();
    }

    template<typename TKey, typename TValue>
    bool ClonedCache<TKey, TValue>::IsReadOnly() const
    {
        return inner_->IsReadOnly();
    }

    template<typename TKey, typename TValue>
    std::shared_ptr<DataCache<TKey, TValue>> ClonedCache<TKey, TValue>::GetInner() const
    {
        return inner_;
    }

    template<typename TKey, typename TValue>
    void ClonedCache<TKey, TValue>::AddInternal(const TKey& key, const TValue& value)
    {
        Add(key, value);
    }

    template<typename TKey, typename TValue>
    void ClonedCache<TKey, TValue>::DeleteInternal(const TKey& key)
    {
        Delete(key);
    }

    template<typename TKey, typename TValue>
    TValue ClonedCache<TKey, TValue>::GetInternal(const TKey& key) const
    {
        return Get(key);
    }

    template<typename TKey, typename TValue>
    bool ClonedCache<TKey, TValue>::TryGetInternal(const TKey& key, TValue& value) const
    {
        return TryGet(key, value);
    }

    template<typename TKey, typename TValue>
    void ClonedCache<TKey, TValue>::UpdateInternal(const TKey& key, const TValue& value)
    {
        Update(key, value);
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
        TValue value;
        if (inner_->TryGet(key, value))
        {
            cloned_items_[key] = value;
        }
    }
}
