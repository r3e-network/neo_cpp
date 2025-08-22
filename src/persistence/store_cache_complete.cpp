/**
 * @file store_cache_complete.cpp
 * @brief Caching mechanisms
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/persistence/data_cache.h>
#include <neo/persistence/memory_store.h>

namespace neo::persistence
{
// StoreCache implementation

StoreCache::StoreCache(IStore& store) : store_(store), snapshot_(nullptr)
{
    // Try to cast to MemoryStore to get snapshot
    if (auto* memStore = dynamic_cast<MemoryStore*>(&store))
    {
        snapshot_ = memStore->GetSnapshot();
    }
}

StoreCache::StoreCache(std::shared_ptr<IStoreSnapshot> snapshot) : store_(*snapshot), snapshot_(snapshot)
{
    // Ensure snapshot is valid
    if (!snapshot_)
    {
        throw std::invalid_argument("Snapshot cannot be null");
    }
}

std::optional<StorageItem> StoreCache::TryGetValue(const StorageKey& key) const
{
    // Check tracked items first
    auto it = items_.find(key);
    if (it != items_.end())
    {
        if (it->second.second == TrackState::Deleted)
        {
            return std::nullopt;
        }
        return it->second.first;
    }

    // Check snapshot
    if (snapshot_)
    {
        auto data = snapshot_->TryGet(key.ToArray());
        if (data)
        {
            StorageItem item;
            item.DeserializeFromArray(*data);
            return item;
        }
    }

    return std::nullopt;
}

std::shared_ptr<StorageItem> StoreCache::TryGet(const StorageKey& key)
{
    // Check tracked items first
    auto it = items_.find(key);
    if (it != items_.end())
    {
        if (it->second.second == TrackState::Deleted)
        {
            return nullptr;
        }
        return std::make_shared<StorageItem>(it->second.first);
    }

    // Check snapshot
    if (snapshot_)
    {
        auto data = snapshot_->TryGet(key.ToArray());
        if (data)
        {
            auto item = std::make_shared<StorageItem>();
            item->DeserializeFromArray(*data);
            return item;
        }
    }

    return nullptr;
}

void StoreCache::Add(const StorageKey& key, const StorageItem& item)
{
    auto it = items_.find(key);
    if (it != items_.end())
    {
        if (it->second.second == TrackState::Deleted)
        {
            it->second = {item, TrackState::Changed};
        }
        else
        {
            // Key already exists and is not deleted
            throw std::invalid_argument("Key already exists");
        }
    }
    else
    {
        // Check if key exists in snapshot
        if (snapshot_)
        {
            auto keyArray = key.ToArray();
            if (snapshot_->Contains(keyArray))
            {
                throw std::invalid_argument("Key already exists in store");
            }
        }
        items_[key] = {item, TrackState::Added};
    }
}

StorageItem& StoreCache::Get(const StorageKey& key)
{
    auto it = items_.find(key);
    if (it != items_.end() && it->second.second != TrackState::Deleted)
    {
        return it->second.first;
    }

    auto item = TryGet(key);
    if (!item)
    {
        throw std::out_of_range("Key not found");
    }

    // Add to tracked items
    items_[key] = {*item, TrackState::None};
    return items_[key].first;
}

std::shared_ptr<StorageItem> StoreCache::GetAndChange(const StorageKey& key,
                                                      std::function<std::shared_ptr<StorageItem>()> factory)
{
    auto it = items_.find(key);
    if (it != items_.end())
    {
        if (it->second.second == TrackState::Deleted)
        {
            if (factory)
            {
                auto newItem = factory();
                if (newItem)
                {
                    it->second = {*newItem, TrackState::Added};
                    return std::make_shared<StorageItem>(it->second.first);
                }
            }
            return nullptr;
        }

        if (it->second.second == TrackState::None)
        {
            it->second.second = TrackState::Changed;
        }
        return std::make_shared<StorageItem>(it->second.first);
    }

    // Try to get from snapshot
    auto item = TryGet(key);
    if (item)
    {
        items_[key] = {*item, TrackState::Changed};
        return std::make_shared<StorageItem>(items_[key].first);
    }

    // Use factory to create new item
    if (factory)
    {
        auto newItem = factory();
        if (newItem)
        {
            items_[key] = {*newItem, TrackState::Added};
            return std::make_shared<StorageItem>(items_[key].first);
        }
    }

    return nullptr;
}

void StoreCache::Delete(const StorageKey& key)
{
    auto it = items_.find(key);
    if (it != items_.end())
    {
        if (it->second.second == TrackState::Added)
        {
            items_.erase(it);
        }
        else
        {
            it->second.second = TrackState::Deleted;
        }
    }
    else
    {
        // Check if exists in snapshot
        if (snapshot_ && snapshot_->Contains(key.ToArray()))
        {
            items_[key] = {StorageItem(), TrackState::Deleted};
        }
    }
}

std::vector<std::pair<StorageKey, StorageItem>> StoreCache::Find(const StorageKey* prefix) const
{
    std::vector<std::pair<StorageKey, StorageItem>> results;

    // Find in tracked items
    for (const auto& [key, value] : items_)
    {
        if (value.second == TrackState::Deleted) continue;

        // Check if key matches prefix
        if (!prefix || (key.GetKey().size() >= prefix->GetKey().size() &&
                        std::equal(prefix->GetKey().begin(), prefix->GetKey().end(), key.GetKey().begin())))
        {
            results.push_back({key, value.first});
        }
    }

    // Find in snapshot
    if (snapshot_)
    {
        io::ByteVector prefixBytes = prefix ? prefix->ToArray() : io::ByteVector();
        // Use Find instead of Seek
        auto entries = snapshot_->Find(&prefixBytes, SeekDirection::Forward);

        for (const auto& entry : entries)
        {
            StorageKey key;
            key.DeserializeFromArray(entry.first);

            // Skip if already in tracked items
            if (items_.find(key) != items_.end()) continue;

            StorageItem item;
            item.DeserializeFromArray(entry.second);

            results.push_back({key, item});
        }
    }

    return results;
}

std::unique_ptr<StorageIterator> StoreCache::Seek(const StorageKey& prefix) const
{
    // Create a simple iterator implementation
    class StoreCacheIterator : public StorageIterator
    {
       private:
        std::vector<std::pair<StorageKey, StorageItem>> items_;
        size_t position_ = 0;

       public:
        StoreCacheIterator(std::vector<std::pair<StorageKey, StorageItem>> items) : items_(std::move(items)) {}

        bool Valid() const override { return position_ < items_.size(); }

        StorageKey Key() const override
        {
            if (!Valid()) throw std::out_of_range("Iterator out of range");
            return items_[position_].first;
        }

        StorageItem Value() const override
        {
            if (!Valid()) throw std::out_of_range("Iterator out of range");
            return items_[position_].second;
        }

        void Next() override
        {
            if (Valid()) position_++;
        }
    };

    auto items = Find(&prefix);
    return std::make_unique<StoreCacheIterator>(std::move(items));
}

std::shared_ptr<StoreView> StoreCache::CreateSnapshot()
{
    // Create a new StoreCache that includes our changes
    auto snapshot = std::make_shared<StoreCache>(*this);
    return snapshot;
}

void StoreCache::Commit()
{
    if (!snapshot_) return;

    // Apply all changes to the underlying store
    for (const auto& [key, value] : items_)
    {
        auto keyBytes = key.ToArray();

        switch (value.second)
        {
            case TrackState::Added:
            case TrackState::Changed:
                snapshot_->Put(keyBytes, value.first.ToArray());
                break;

            case TrackState::Deleted:
                snapshot_->Delete(keyBytes);
                break;

            case TrackState::None:
                // No changes needed
                break;
        }
    }

    // Commit the snapshot
    snapshot_->Commit();

    // Clear tracked items
    items_.clear();
}

uint32_t StoreCache::GetCurrentBlockIndex() const
{
    StorageKey key(0, io::ByteVector::Parse("00"));  // Block height key
    auto item_opt = TryGetValue(key);
    if (!item_opt.has_value()) return 0;
    auto item = std::make_shared<StorageItem>(item_opt.value());
    if (item && item->GetValue().Size() >= 4)
    {
        return *reinterpret_cast<const uint32_t*>(item->GetValue().Data());
    }
    return 0;
}

bool StoreCache::Contains(const StorageKey& key) const
{
    auto it = items_.find(key);
    if (it != items_.end())
    {
        return it->second.second != TrackState::Deleted;
    }

    if (snapshot_)
    {
        return snapshot_->Contains(key.ToArray());
    }

    return false;
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
        if (it->second.second == TrackState::Deleted)
        {
            throw std::out_of_range("Cannot update deleted key");
        }
        it->second.first = item;
        if (it->second.second == TrackState::None)
        {
            it->second.second = TrackState::Changed;
        }
    }
    else
    {
        // Check if exists in snapshot
        if (snapshot_ && snapshot_->Contains(key.ToArray()))
        {
            items_[key] = {item, TrackState::Changed};
        }
        else
        {
            // Key doesn't exist anywhere
            throw std::out_of_range("Key not found");
        }
    }
}

size_t StoreCache::Count() const
{
    size_t count = 0;

    // Count non-deleted tracked items
    for (const auto& [key, value] : items_)
    {
        if (value.second != TrackState::Deleted)
        {
            count++;
        }
    }

    // Count items in snapshot that aren't tracked
    if (snapshot_)
    {
        // Use Find to get all entries
        auto entries = snapshot_->Find(nullptr, SeekDirection::Forward);
        for (const auto& entry : entries)
        {
            StorageKey key;
            key.DeserializeFromArray(entry.first);

            if (items_.find(key) == items_.end())
            {
                count++;
            }
        }
    }

    return count;
}

std::vector<std::pair<StorageKey, std::pair<StorageItem, TrackState>>> StoreCache::GetTrackedItems() const
{
    std::vector<std::pair<StorageKey, std::pair<StorageItem, TrackState>>> result;

    for (const auto& [key, value] : items_)
    {
        result.push_back({key, value});
    }

    return result;
}

std::vector<std::pair<StorageKey, StorageItem>> StoreCache::GetChangedItems() const
{
    std::vector<std::pair<StorageKey, StorageItem>> result;

    for (const auto& [key, value] : items_)
    {
        if (value.second == TrackState::Added || value.second == TrackState::Changed)
        {
            result.push_back({key, value.first});
        }
    }

    return result;
}

std::vector<StorageKey> StoreCache::GetDeletedItems() const
{
    std::vector<StorageKey> result;

    for (const auto& [key, value] : items_)
    {
        if (value.second == TrackState::Deleted)
        {
            result.push_back(key);
        }
    }

    return result;
}

bool StoreCache::TryGet(const StorageKey& key, StorageItem& item) const
{
    auto result_opt = TryGetValue(key);
    if (!result_opt.has_value()) return false;
    item = result_opt.value();
    return true;
}

std::shared_ptr<IStoreSnapshot> StoreCache::GetStore() const { return snapshot_; }

bool StoreCache::IsReadOnly() const { return false; }
}  // namespace neo::persistence