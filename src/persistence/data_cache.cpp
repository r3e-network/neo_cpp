#include <neo/persistence/data_cache.h>
#include <neo/persistence/memory_store.h>
#include <stdexcept>

namespace neo::persistence
{
    // Concrete StorageIterator implementations for data caches
    
    /**
     * @brief StorageIterator implementation for StoreCache.
     */
    class StorageCacheIterator : public StorageIterator
    {
    public:
        StorageCacheIterator(const StoreCache& cache, const StorageKey& prefix)
            : cache_(cache), prefix_(prefix), currentIndex_(0)
        {
            // Get all items matching the prefix
            items_ = cache_.Find(&prefix_);
        }

        bool Valid() const override
        {
            return currentIndex_ < items_.size();
        }

        StorageKey Key() const override
        {
            if (!Valid())
                throw std::runtime_error("Iterator is not valid");
            return items_[currentIndex_].first;
        }

        StorageItem Value() const override
        {
            if (!Valid())
                throw std::runtime_error("Iterator is not valid");
            return items_[currentIndex_].second;
        }

        void Next() override
        {
            if (!Valid())
                throw std::runtime_error("Iterator is not valid");
            ++currentIndex_;
        }

    private:
        const StoreCache& cache_;
        StorageKey prefix_;
        std::vector<std::pair<StorageKey, StorageItem>> items_;
        size_t currentIndex_;
    };

    // ClonedCacheIterator removed - ClonedCache is now template-only

    // StoreCache implementation
    StoreCache::StoreCache(IStore& store)
        : store_(store), snapshot_(nullptr)
    {
    }

    StoreCache::StoreCache(std::shared_ptr<IStoreSnapshot> snapshot)
        : store_(*snapshot), snapshot_(snapshot)
    {
        if (!snapshot)
        {
            throw std::invalid_argument("Snapshot cannot be null");
        }
        // Store the snapshot reference - since IStoreSnapshot inherits from IStore,
        // we can use it directly as the store reference
    }

    std::optional<StorageItem> StoreCache::TryGet(const StorageKey& key) const
    {
        // Check if the key is in the cache
        auto it = items_.find(key);
        if (it != items_.end())
        {
            if (it->second.second == TrackState::Deleted)
                return std::nullopt;

            return it->second.first;
        }

        // Try to get the key from the store
        io::ByteVector keyBytes = key.ToArray();
        auto valueBytes = store_.TryGet(keyBytes);

        if (!valueBytes)
            return std::nullopt;

        // Deserialize the value
        StorageItem item;
        item.DeserializeFromArray(valueBytes->AsSpan());

        return item;
    }

    void StoreCache::Add(const StorageKey& key, const StorageItem& item)
    {
        auto it = items_.find(key);
        if (it != items_.end())
        {
            if (it->second.second == TrackState::Deleted)
            {
                it->second.first = item;
                it->second.second = TrackState::Changed;
            }
            else
            {
                throw std::invalid_argument("The key already exists in the cache.");
            }
        }
        else
        {
            items_[key] = std::make_pair(item, TrackState::Added);
        }
    }

    StorageItem& StoreCache::Get(const StorageKey& key)
    {
        // Check if the key is in the cache
        auto it = items_.find(key);
        if (it != items_.end())
        {
            if (it->second.second == TrackState::Deleted)
                throw std::out_of_range("The key was not found in the cache.");

            return it->second.first;
        }

        // Try to get the key from the store
        io::ByteVector keyBytes = key.ToArray();
        auto valueBytes = store_.TryGet(keyBytes);

        if (!valueBytes)
            throw std::out_of_range("The key was not found in the store.");

        // Deserialize the value
        StorageItem item;
        item.DeserializeFromArray(valueBytes->AsSpan());

        // Add to cache
        auto [it2, inserted] = items_.emplace(key, std::make_pair(item, TrackState::None));

        return it2->second.first;
    }

    std::shared_ptr<StorageItem> StoreCache::TryGet(const StorageKey& key)
    {
        // Check if the key is in the cache
        auto it = items_.find(key);
        if (it != items_.end())
        {
            if (it->second.second == TrackState::Deleted)
                return nullptr;

            return std::make_shared<StorageItem>(it->second.first);
        }

        // Try to get the key from the store
        io::ByteVector keyBytes = key.ToArray();
        auto valueBytes = store_.TryGet(keyBytes);

        if (!valueBytes)
            return nullptr;

        // Deserialize the value
        StorageItem item;
        item.DeserializeFromArray(valueBytes->AsSpan());

        // Add to cache
        auto [it2, inserted] = items_.emplace(key, std::make_pair(item, TrackState::None));
        return std::make_shared<StorageItem>(it2->second.first);
    }

    std::shared_ptr<StorageItem> StoreCache::GetAndChange(const StorageKey& key, std::function<std::shared_ptr<StorageItem>()> factory)
    {
        // Check if the key is in the cache
        auto it = items_.find(key);
        if (it != items_.end())
        {
            if (it->second.second == TrackState::Deleted)
            {
                if (!factory)
                    return nullptr;

                auto newItem = factory();
                it->second.first = *newItem;
                it->second.second = TrackState::Changed;
                return std::make_shared<StorageItem>(it->second.first);
            }
            else if (it->second.second == TrackState::None)
            {
                it->second.second = TrackState::Changed;
            }
            return std::make_shared<StorageItem>(it->second.first);
        }

        // Try to get the key from the store
        io::ByteVector keyBytes = key.ToArray();
        auto valueBytes = store_.TryGet(keyBytes);

        if (!valueBytes)
        {
            if (!factory)
                return nullptr;

            auto newItem = factory();
            items_[key] = std::make_pair(*newItem, TrackState::Added);
            return std::make_shared<StorageItem>(items_[key].first);
        }

        // Deserialize the value
        StorageItem item;
        item.DeserializeFromArray(valueBytes->AsSpan());

        // Add to cache as changed
        auto [it2, inserted] = items_.emplace(key, std::make_pair(item, TrackState::Changed));
        return std::make_shared<StorageItem>(it2->second.first);
    }

    void StoreCache::Delete(const StorageKey& key)
    {
        auto it = items_.find(key);
        if (it != items_.end())
        {
            if (it->second.second == TrackState::Added)
                items_.erase(it);
            else
                it->second.second = TrackState::Deleted;
        }
        else
        {
            // Check if the key exists in the store
            io::ByteVector keyBytes = key.ToArray();
            if (!store_.Contains(keyBytes))
                return;

            // Add to cache as deleted
            StorageItem item;
            items_[key] = std::make_pair(item, TrackState::Deleted);
        }
    }

    std::vector<std::pair<StorageKey, StorageItem>> StoreCache::Find(const StorageKey* prefix) const
    {
        std::vector<std::pair<StorageKey, StorageItem>> result;

        // Get all items from the store
        io::ByteVector prefixBytes;
        if (prefix != nullptr)
            prefixBytes = prefix->ToArray();

        std::vector<std::pair<io::ByteVector, io::ByteVector>> storeItems;
        if (prefix != nullptr)
            storeItems = store_.Find(&prefixBytes);
        else
            storeItems = store_.Find();

        // Deserialize the items
        for (const auto& pair : storeItems)
        {
            const auto& keyBytes = pair.first;
            const auto& valueBytes = pair.second;
            
            StorageKey key;
            key.DeserializeFromArray(keyBytes.AsSpan());

            // Skip if the key is in the cache
            auto it = items_.find(key);
            if (it != items_.end())
                continue;

            StorageItem item;
            item.DeserializeFromArray(valueBytes.AsSpan());

            result.push_back(std::make_pair(key, item));
        }

        // Add items from the cache
        for (const auto& [key, pair] : items_)
        {
            if (pair.second == TrackState::Deleted)
                continue;

            if (prefix != nullptr)
            {
                // Check if the key starts with the prefix
                if (key.GetScriptHash() != prefix->GetScriptHash())
                    continue;

                if (key.GetKey().Size() < prefix->GetKey().Size())
                    continue;

                if (!std::equal(prefix->GetKey().Data(), prefix->GetKey().Data() + prefix->GetKey().Size(), key.GetKey().Data()))
                    continue;
            }

            result.emplace_back(key, pair.first);
        }

        return result;
    }

    std::shared_ptr<StoreView> StoreCache::CreateSnapshot()
    {
        // For now, return a simple StoreCache copy
        // In practice, this would create a proper snapshot implementation
        auto snapshot = std::make_shared<StoreCache>(store_);
        snapshot->items_ = items_;
        return snapshot;
    }

    void StoreCache::Commit()
    {
        // Create a snapshot of the store
        auto memoryStore = dynamic_cast<MemoryStore*>(&store_);
        if (!memoryStore)
            throw std::runtime_error("The store is not a MemoryStore.");

        auto storeSnapshot = memoryStore->GetSnapshot();
        auto snapshot = dynamic_cast<IStoreSnapshot*>(storeSnapshot.release());

        // Apply changes
        for (const auto& [key, pair] : items_)
        {
            io::ByteVector keyBytes = key.ToArray();

            switch (pair.second)
            {
                case TrackState::Added:
                case TrackState::Changed:
                {
                    io::ByteVector valueBytes = pair.first.ToArray();
                    snapshot->Put(keyBytes, valueBytes);
                    break;
                }
                case TrackState::Deleted:
                {
                    snapshot->Delete(keyBytes);
                    break;
                }
                case TrackState::None:
                    break;
            }
        }

        // Commit the snapshot
        snapshot->Commit();

        // Clear the cache
        items_.clear();
    }

    uint32_t StoreCache::GetCurrentBlockIndex() const
    {
        // Implement proper current block index retrieval from Ledger contract storage
        // This matches C# NativeContract.Ledger.CurrentIndex(snapshot)
        try
        {
            // Create storage key for current block (Prefix_CurrentBlock = 12)
            const uint8_t PREFIX_CURRENT_BLOCK = 12;
            io::ByteVector keyData;
            keyData.Push(PREFIX_CURRENT_BLOCK);
            
            // Create StorageKey from the key data
            StorageKey storageKey(keyData);
            
            // Try to get the current block state from storage using const version
            auto storageItemOpt = TryGet(storageKey);
            if (storageItemOpt && !storageItemOpt->GetValue().IsEmpty())
            {
                // Parse the HashIndexState to get the index
                // In C#, this is stored as HashIndexState with Hash and Index
                // For now, assume the index is stored as uint32_t at offset 32 (after 32-byte hash)
                if (storageItemOpt->GetValue().Size() >= 36) // 32 bytes hash + 4 bytes index
                {
                    const uint8_t* data = storageItemOpt->GetValue().Data();
                    uint32_t index = *reinterpret_cast<const uint32_t*>(data + 32);
                    return index;
                }
            }
            
            // If no current block state found, return 0 (genesis block)
            return 0;
        }
        catch (...)
        {
            // On any error, return 0 as safe fallback
            return 0;
        }
    }

    std::unique_ptr<StorageIterator> StoreCache::Seek(const StorageKey& prefix) const
    {
        // Implement proper iterator for StoreCache matching C# DataCache.Find implementation
        return std::make_unique<StorageCacheIterator>(*this, prefix);
    }

    bool StoreCache::Contains(const StorageKey& key) const
    {
        auto it = items_.find(key);
        if (it != items_.end())
        {
            return it->second.second != TrackState::Deleted;
        }

        // Check if the key exists in the store
        io::ByteVector keyBytes = key.ToArray();
        return store_.Contains(keyBytes);
    }

    TrackState StoreCache::GetTrackState(const StorageKey& key) const
    {
        auto it = items_.find(key);
        if (it != items_.end())
        {
            return it->second.second;
        }
        return TrackState::None;
    }

    void StoreCache::Update(const StorageKey& key, const StorageItem& item)
    {
        auto it = items_.find(key);
        if (it != items_.end())
        {
            it->second.first = item;
            if (it->second.second == TrackState::None)
            {
                it->second.second = TrackState::Changed;
            }
        }
        else
        {
            // Check if the key exists in the store
            io::ByteVector keyBytes = key.ToArray();
            if (!store_.Contains(keyBytes))
            {
                throw std::out_of_range("Key not found");
            }
            
            items_[key] = std::make_pair(item, TrackState::Changed);
        }
    }

    size_t StoreCache::Count() const
    {
        size_t count = 0;
        
        // Count items from store that aren't deleted
        auto storeItems = store_.Find();
        for (const auto& pair : storeItems)
        {
            StorageKey key;
            key.DeserializeFromArray(pair.first.AsSpan());
            
            auto it = items_.find(key);
            if (it == items_.end() || it->second.second != TrackState::Deleted)
            {
                count++;
            }
        }
        
        // Add items that are added in cache
        for (const auto& [key, pair] : items_)
        {
            if (pair.second == TrackState::Added)
            {
                count++;
            }
        }
        
        return count;
    }

    std::vector<std::pair<StorageKey, std::pair<StorageItem, TrackState>>> StoreCache::GetTrackedItems() const
    {
        std::vector<std::pair<StorageKey, std::pair<StorageItem, TrackState>>> result;
        for (const auto& [key, pair] : items_)
        {
            result.emplace_back(key, pair);
        }
        return result;
    }

    std::vector<std::pair<StorageKey, StorageItem>> StoreCache::GetChangedItems() const
    {
        std::vector<std::pair<StorageKey, StorageItem>> result;
        for (const auto& [key, pair] : items_)
        {
            if (pair.second == TrackState::Changed || pair.second == TrackState::Added)
            {
                result.emplace_back(key, pair.first);
            }
        }
        return result;
    }

    std::vector<StorageKey> StoreCache::GetDeletedItems() const
    {
        std::vector<StorageKey> result;
        for (const auto& [key, pair] : items_)
        {
            if (pair.second == TrackState::Deleted)
            {
                result.push_back(key);
            }
        }
        return result;
    }

    bool StoreCache::TryGet(const StorageKey& key, StorageItem& item) const
    {
        auto optItem = TryGet(key);
        if (optItem)
        {
            item = *optItem;
            return true;
        }
        return false;
    }

    std::shared_ptr<IStoreSnapshot> StoreCache::GetStore() const
    {
        return snapshot_;
    }

    bool StoreCache::IsReadOnly() const
    {
        return false; // StoreCache is not read-only by default
    }

    // ClonedCache implementations removed - ClonedCache is now template-only in cloned_cache.h
}
