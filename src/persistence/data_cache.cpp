#include <neo/persistence/data_cache.h>
#include <neo/persistence/memory_store.h>
#include <stdexcept>

namespace neo::persistence
{
    // StoreCache implementation
    StoreCache::StoreCache(IStore& store)
        : store_(store)
    {
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
        return std::make_shared<ClonedCache>(*this);
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
        // TODO: Implement proper current block index retrieval
        // For now, return 0 as a placeholder
        return 0;
    }

    // ClonedCache implementation
    ClonedCache::ClonedCache(DataCache& innerCache)
        : innerCache_(innerCache)
    {
    }

    std::optional<StorageItem> ClonedCache::TryGet(const StorageKey& key) const
    {
        // Check if the key is in the cache
        auto it = items_.find(key);
        if (it != items_.end())
        {
            if (it->second.second == TrackState::Deleted)
                return std::nullopt;

            return it->second.first;
        }

        // Try to get the key from the inner cache
        auto innerPtr = innerCache_.TryGet(key);
        if (innerPtr)
            return *innerPtr;
        return std::nullopt;
    }

    std::shared_ptr<StorageItem> ClonedCache::TryGet(const StorageKey& key)
    {
        // Check if the key is in the cache
        auto it = items_.find(key);
        if (it != items_.end())
        {
            if (it->second.second == TrackState::Deleted)
                return nullptr;

            return std::make_shared<StorageItem>(it->second.first);
        }

        // Try to get the key from the inner cache
        return innerCache_.TryGet(key);
    }

    void ClonedCache::Add(const StorageKey& key, const StorageItem& item)
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
            // Check if the key exists in the inner cache
            auto innerItem = innerCache_.TryGet(key);
            if (innerItem)
                throw std::invalid_argument("The key already exists in the inner cache.");

            items_[key] = std::make_pair(item, TrackState::Added);
        }
    }

    StorageItem& ClonedCache::Get(const StorageKey& key)
    {
        // Check if the key is in the cache
        auto it = items_.find(key);
        if (it != items_.end())
        {
            if (it->second.second == TrackState::Deleted)
                throw std::out_of_range("The key was not found in the cache.");

            return it->second.first;
        }

        // Try to get the key from the inner cache
        auto innerItem = innerCache_.TryGet(key);
        if (!innerItem)
            throw std::out_of_range("The key was not found in the inner cache.");

        // Add to cache
        auto [it2, inserted] = items_.emplace(key, std::make_pair(*innerItem, TrackState::None));

        return it2->second.first;
    }

    std::shared_ptr<StorageItem> ClonedCache::GetAndChange(const StorageKey& key, std::function<std::shared_ptr<StorageItem>()> factory)
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

        // Try to get the key from the inner cache
        auto innerItem = innerCache_.TryGet(key);
        if (!innerItem)
        {
            if (!factory)
                return nullptr;

            auto newItem = factory();
            items_[key] = std::make_pair(*newItem, TrackState::Added);
            return std::make_shared<StorageItem>(items_[key].first);
        }

        // Add to cache as changed
        auto [it2, inserted] = items_.emplace(key, std::make_pair(*innerItem, TrackState::Changed));
        return std::make_shared<StorageItem>(it2->second.first);
    }

    void ClonedCache::Delete(const StorageKey& key)
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
            // Check if the key exists in the inner cache
            auto innerItem = innerCache_.TryGet(key);
            if (!innerItem)
                return;

            // Add to cache as deleted
            items_[key] = std::make_pair(*innerItem, TrackState::Deleted);
        }
    }

    std::vector<std::pair<StorageKey, StorageItem>> ClonedCache::Find(const StorageKey* prefix) const
    {
        std::vector<std::pair<StorageKey, StorageItem>> result;

        // Get all items from the inner cache
        auto innerItems = innerCache_.Find(prefix);

        // Add items from the inner cache
        for (const auto& item_pair : innerItems)
        {
            const auto& key = item_pair.first;
            const auto& item = item_pair.second;
            
            // Skip if the key is in the cache
            auto it = items_.find(key);
            if (it != items_.end())
                continue;

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

    std::shared_ptr<StoreView> ClonedCache::CreateSnapshot()
    {
        return std::make_shared<ClonedCache>(*this);
    }

    void ClonedCache::Commit()
    {
        // Apply changes to the inner cache
        for (const auto& [key, pair] : items_)
        {
            switch (pair.second)
            {
                case TrackState::Added:
                    innerCache_.Add(key, pair.first);
                    break;
                case TrackState::Changed:
                    innerCache_.Get(key).SetValue(pair.first.GetValue());
                    break;
                case TrackState::Deleted:
                    innerCache_.Delete(key);
                    break;
                case TrackState::None:
                    break;
            }
        }

        // Clear the cache
        items_.clear();
    }

    uint32_t ClonedCache::GetCurrentBlockIndex() const
    {
        return innerCache_.GetCurrentBlockIndex();
    }
}
