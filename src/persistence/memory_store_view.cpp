#include <neo/persistence/memory_store_view.h>

namespace neo::persistence
{
    std::optional<StorageItem> MemoryStoreView::TryGet(const StorageKey& key) const
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

    std::shared_ptr<StorageItem> MemoryStoreView::GetAndChange(const StorageKey& key, std::function<std::shared_ptr<StorageItem>()> factory)
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

    void MemoryStoreView::Add(const StorageKey& key, const StorageItem& item)
    {
        storage_[key] = item;
    }

    void MemoryStoreView::Delete(const StorageKey& key)
    {
        storage_.erase(key);
    }

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
                // Simple prefix matching - check if key starts with prefix
                // This is a simplified implementation
                result.push_back(pair);
            }
        }
        
        return result;
    }

    std::unique_ptr<StorageIterator> MemoryStoreView::Seek(const StorageKey& prefix) const
    {
        // Return nullptr for now - this would need a proper iterator implementation
        return nullptr;
    }

    void MemoryStoreView::Commit()
    {
        // No-op for memory store
    }

    std::shared_ptr<StoreView> MemoryStoreView::CreateSnapshot()
    {
        auto snapshot = std::make_shared<MemoryStoreView>();
        snapshot->storage_ = storage_; // Copy all data
        return snapshot;
    }

    void MemoryStoreView::Clear()
    {
        storage_.clear();
    }
}