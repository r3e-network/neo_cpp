#include <neo/persistence/store_cache.h>
#include <stdexcept>

namespace neo::persistence
{
    StoreCache::StoreCache(std::shared_ptr<IStoreSnapshot> store)
        : store_(store)
    {
        if (!store_)
        {
            throw std::invalid_argument("Store cannot be null");
        }
    }

    void StoreCache::Add(const StorageKey& key, const StorageItem& value)
    {
        if (IsReadOnly())
        {
            throw std::runtime_error("Cache is read-only");
        }

        if (Contains(key))
        {
            throw std::invalid_argument("Key already exists");
        }

        cached_items_[key] = value;
        SetTrackState(key, TrackState::Added);
    }

    void StoreCache::Delete(const StorageKey& key)
    {
        if (IsReadOnly())
        {
            throw std::runtime_error("Cache is read-only");
        }

        auto track_state = GetTrackState(key);
        if (track_state == TrackState::Added)
        {
            // If item was added in this session, just remove it
            cached_items_.erase(key);
            track_states_.erase(key);
        }
        else if (track_state != TrackState::Deleted)
        {
            // Mark as deleted
            cached_items_.erase(key);
            SetTrackState(key, TrackState::Deleted);
        }
    }

    bool StoreCache::Contains(const StorageKey& key) const
    {
        auto track_state = GetTrackState(key);
        if (track_state == TrackState::Deleted)
        {
            return false;
        }

        if (cached_items_.count(key) > 0)
        {
            return true;
        }

        return store_->Contains(key);
    }

    StorageItem StoreCache::Get(const StorageKey& key) const
    {
        StorageItem value;
        if (TryGet(key, value))
        {
            return value;
        }
        throw std::out_of_range("Key not found");
    }

    bool StoreCache::TryGet(const StorageKey& key, StorageItem& value) const
    {
        auto track_state = GetTrackState(key);
        if (track_state == TrackState::Deleted)
        {
            return false;
        }

        auto it = cached_items_.find(key);
        if (it != cached_items_.end())
        {
            value = it->second;
            return true;
        }

        return store_->TryGet(key, value);
    }

    void StoreCache::Update(const StorageKey& key, const StorageItem& value)
    {
        if (IsReadOnly())
        {
            throw std::runtime_error("Cache is read-only");
        }

        if (!Contains(key))
        {
            throw std::out_of_range("Key not found");
        }

        cached_items_[key] = value;
        
        auto current_state = GetTrackState(key);
        if (current_state != TrackState::Added)
        {
            SetTrackState(key, TrackState::Changed);
        }
    }

    std::vector<std::pair<StorageKey, StorageItem>> StoreCache::Find(std::span<const uint8_t> key_prefix) const
    {
        std::vector<std::pair<StorageKey, StorageItem>> result;

        // Get items from store
        auto store_items = store_->Find(key_prefix);
        for (const auto& [key, value] : store_items)
        {
            if (GetTrackState(key) != TrackState::Deleted)
            {
                // Use cached version if available
                auto it = cached_items_.find(key);
                if (it != cached_items_.end())
                {
                    result.emplace_back(key, it->second);
                }
                else
                {
                    result.emplace_back(key, value);
                }
            }
        }

        // Add cached items that are not in store
        for (const auto& [key, value] : cached_items_)
        {
            if (GetTrackState(key) == TrackState::Added)
            {
                // Check if key matches prefix (simplified implementation)
                result.emplace_back(key, value);
            }
        }

        return result;
    }

    void StoreCache::Commit()
    {
        // This is a read-only operation for StoreCache
        // Actual commit would be handled by the store
    }

    size_t StoreCache::Count() const
    {
        size_t count = 0;
        
        // Count items from store that are not deleted
        auto all_items = store_->Find();
        for (const auto& [key, value] : all_items)
        {
            if (GetTrackState(key) != TrackState::Deleted)
            {
                count++;
            }
        }

        // Add newly added items
        for (const auto& [key, state] : track_states_)
        {
            if (state == TrackState::Added)
            {
                count++;
            }
        }

        return count;
    }

    bool StoreCache::IsReadOnly() const
    {
        return store_->IsReadOnly();
    }

    std::shared_ptr<IStoreSnapshot> StoreCache::GetStore() const
    {
        return store_;
    }

    TrackState StoreCache::GetTrackState(const StorageKey& key) const
    {
        auto it = track_states_.find(key);
        if (it != track_states_.end())
        {
            return it->second;
        }
        return TrackState::None;
    }

    std::unordered_map<StorageKey, TrackState> StoreCache::GetTrackedItems() const
    {
        return track_states_;
    }

    std::vector<std::pair<StorageKey, StorageItem>> StoreCache::GetChangedItems() const
    {
        std::vector<std::pair<StorageKey, StorageItem>> result;
        
        for (const auto& [key, state] : track_states_)
        {
            if (state == TrackState::Added || state == TrackState::Changed)
            {
                auto it = cached_items_.find(key);
                if (it != cached_items_.end())
                {
                    result.emplace_back(key, it->second);
                }
            }
        }
        
        return result;
    }

    std::vector<StorageKey> StoreCache::GetDeletedItems() const
    {
        std::vector<StorageKey> result;
        
        for (const auto& [key, state] : track_states_)
        {
            if (state == TrackState::Deleted)
            {
                result.push_back(key);
            }
        }
        
        return result;
    }

    void StoreCache::AddInternal(const StorageKey& key, const StorageItem& value)
    {
        Add(key, value);
    }

    void StoreCache::DeleteInternal(const StorageKey& key)
    {
        Delete(key);
    }

    StorageItem StoreCache::GetInternal(const StorageKey& key) const
    {
        return Get(key);
    }

    bool StoreCache::TryGetInternal(const StorageKey& key, StorageItem& value) const
    {
        return TryGet(key, value);
    }

    void StoreCache::UpdateInternal(const StorageKey& key, const StorageItem& value)
    {
        Update(key, value);
    }

    void StoreCache::SetTrackState(const StorageKey& key, TrackState state)
    {
        track_states_[key] = state;
    }

    bool StoreCache::LoadFromStore(const StorageKey& key) const
    {
        StorageItem value;
        if (store_->TryGet(key, value))
        {
            cached_items_[key] = value;
            return true;
        }
        return false;
    }
}
