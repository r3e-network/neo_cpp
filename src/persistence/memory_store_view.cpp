/**
 * @file memory_store_view.cpp
 * @brief Memory Store View
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/persistence/memory_store_view.h>

#include <map>
#include <stdexcept>

namespace neo::persistence
{
std::optional<StorageItem> MemoryStoreView::TryGetValue(const StorageKey& key) const
{
    auto it = storage_.find(key);
    if (it != storage_.end())
    {
        return it->second;
    }
    return std::nullopt;
}

std::shared_ptr<StorageItem> MemoryStoreView::TryGet(const StorageKey& key)
{
    auto it = storage_.find(key);
    if (it != storage_.end())
    {
        return std::make_shared<StorageItem>(it->second);
    }
    return nullptr;
}

std::shared_ptr<StorageItem> MemoryStoreView::GetAndChange(const StorageKey& key,
                                                           std::function<std::shared_ptr<StorageItem>()> factory)
{
    auto it = storage_.find(key);
    if (it != storage_.end())
    {
        return std::make_shared<StorageItem>(it->second);
    }

    if (factory)
    {
        auto item = factory();
        if (item)
        {
            storage_[key] = *item;
            return item;
        }
    }

    // Create default item
    auto item = std::make_shared<StorageItem>();
    storage_[key] = *item;
    return item;
}

void MemoryStoreView::Add(const StorageKey& key, const StorageItem& item) { storage_[key] = item; }

void MemoryStoreView::Delete(const StorageKey& key) { storage_.erase(key); }

std::vector<std::pair<StorageKey, StorageItem>> MemoryStoreView::Find(const StorageKey* prefix) const
{
    std::vector<std::pair<StorageKey, StorageItem>> result;

    for (const auto& pair : storage_)
    {
        if (!prefix)
        {
            result.push_back(pair);
        }
        else
        {
            // Production-ready prefix matching implementation
            // Check if the key matches the prefix pattern exactly
            const StorageKey& key = pair.first;

            // First check if contract IDs match
            if (key.GetScriptHash() != prefix->GetScriptHash())
            {
                continue;
            }

            // Then check if the key data starts with the prefix key data
            const auto& key_data = key.GetKey();
            const auto& prefix_data = prefix->GetKey();

            if (key_data.Size() < prefix_data.Size())
            {
                continue;  // Key is shorter than prefix
            }

            // Compare prefix bytes
            bool matches = true;
            for (size_t i = 0; i < prefix_data.Size(); ++i)
            {
                if (key_data[i] != prefix_data[i])
                {
                    matches = false;
                    break;
                }
            }

            if (matches)
            {
                result.push_back(pair);
            }
        }
    }

    return result;
}

std::unique_ptr<StorageIterator> MemoryStoreView::Seek(const StorageKey& prefix) const
{
    // Complete StorageIterator implementation for proper blockchain storage iteration
    class MemoryStorageIterator : public StorageIterator
    {
       private:
        std::vector<std::pair<StorageKey, StorageItem>> items_;
        size_t current_index_;
        bool valid_;

       public:
        MemoryStorageIterator(const std::unordered_map<StorageKey, StorageItem>& storage,
                              const StorageKey& search_prefix)
            : current_index_(0), valid_(false)
        {
            // Filter items that match the prefix
            for (const auto& [key, value] : storage)
            {
                // Check if key matches prefix
                if (key.GetScriptHash() == search_prefix.GetScriptHash())
                {
                    const auto& key_data = key.GetKey();
                    const auto& prefix_data = search_prefix.GetKey();

                    // Check if key data starts with prefix data
                    if (key_data.Size() >= prefix_data.Size())
                    {
                        bool matches = true;
                        for (size_t i = 0; i < prefix_data.Size(); ++i)
                        {
                            if (key_data[i] != prefix_data[i])
                            {
                                matches = false;
                                break;
                            }
                        }

                        if (matches)
                        {
                            items_.emplace_back(key, value);
                        }
                    }
                }
            }

            // Sort items by key for consistent iteration order
            std::sort(items_.begin(), items_.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

            // Set initial state
            valid_ = !items_.empty();
        }

        bool Valid() const override { return valid_ && current_index_ < items_.size(); }

        StorageKey Key() const override
        {
            if (!Valid())
            {
                throw std::runtime_error("Iterator is not valid");
            }
            return items_[current_index_].first;
        }

        StorageItem Value() const override
        {
            if (!Valid())
            {
                throw std::runtime_error("Iterator is not valid");
            }
            return items_[current_index_].second;
        }

        void Next() override
        {
            if (!Valid())
            {
                throw std::runtime_error("Iterator is not valid");
            }

            current_index_++;

            // Update validity
            valid_ = current_index_ < items_.size();
        }
    };

    return std::make_unique<MemoryStorageIterator>(storage_, prefix);
}

void MemoryStoreView::Commit()
{
    // No-op for memory store
}

std::shared_ptr<StoreView> MemoryStoreView::CreateSnapshot()
{
    auto snapshot = std::make_shared<MemoryStoreView>();
    snapshot->storage_ = storage_;  // Copy all data
    return snapshot;
}

void MemoryStoreView::Clear() { storage_.clear(); }
}  // namespace neo::persistence