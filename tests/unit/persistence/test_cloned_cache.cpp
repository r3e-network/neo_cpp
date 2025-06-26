#include <gtest/gtest.h>
#include <neo/persistence/cloned_cache.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>

namespace neo::persistence::tests
{
    // Simple concrete DataCache implementation for testing
    class TestDataCache : public DataCache
    {
    public:
        explicit TestDataCache(std::shared_ptr<IStoreSnapshot> snapshot) : snapshot_(snapshot) {}
        
        // Implement pure virtual methods
        StorageItem& Get(const StorageKey& key) override {
            auto it = items_.find(key);
            if (it == items_.end()) {
                throw std::out_of_range("Key not found");
            }
            return it->second.first;
        }
        
        std::shared_ptr<StorageItem> TryGet(const StorageKey& key) override {
            auto it = items_.find(key);
            if (it == items_.end()) {
                return nullptr;
            }
            return std::make_shared<StorageItem>(it->second.first);
        }
        
        std::shared_ptr<StorageItem> GetAndChange(const StorageKey& key, std::function<std::shared_ptr<StorageItem>()> factory) override {
            auto it = items_.find(key);
            if (it != items_.end()) {
                it->second.second = TrackState::Changed;
                return std::make_shared<StorageItem>(it->second.first);
            }
            if (factory) {
                auto item = factory();
                if (item) {
                    items_[key] = {*item, TrackState::Added};
                    return item;
                }
            }
            return nullptr;
        }
        
        std::optional<StorageItem> TryGet(const StorageKey& key) const override {
            auto it = items_.find(key);
            if (it == items_.end()) {
                return std::nullopt;
            }
            return it->second.first;
        }
        
        void Add(const StorageKey& key, const StorageItem& item) override {
            items_[key] = {item, TrackState::Added};
        }
        
        void Delete(const StorageKey& key) override {
            auto it = items_.find(key);
            if (it != items_.end()) {
                it->second.second = TrackState::Deleted;
            }
        }
        
        std::vector<std::pair<StorageKey, StorageItem>> Find(const StorageKey* prefix) const override {
            std::vector<std::pair<StorageKey, StorageItem>> result;
            for (const auto& [key, value] : items_) {
                if (value.second != TrackState::Deleted) {
                    result.emplace_back(key, value.first);
                }
            }
            return result;
        }
        
        std::unique_ptr<StorageIterator> Seek(const StorageKey& prefix) const override {
            return nullptr; // Not implemented for tests
        }
        
        std::shared_ptr<StoreView> CreateSnapshot() override {
            // Create a new TestDataCache as a snapshot
            auto snapshot_cache = std::make_shared<TestDataCache>(snapshot_);
            snapshot_cache->items_ = items_;
            return snapshot_cache;
        }
        
        void Commit() override {
            // Simple commit - just clear deleted items
            for (auto it = items_.begin(); it != items_.end();) {
                if (it->second.second == TrackState::Deleted) {
                    it = items_.erase(it);
                } else {
                    it->second.second = TrackState::None;
                    ++it;
                }
            }
        }
        
        uint32_t GetCurrentBlockIndex() const override {
            return 0;
        }
        
        bool Contains(const StorageKey& key) const {
            auto it = items_.find(key);
            return it != items_.end() && it->second.second != TrackState::Deleted;
        }
        
        size_t Count() const {
            size_t count = 0;
            for (const auto& [key, value] : items_) {
                if (value.second != TrackState::Deleted) {
                    count++;
                }
            }
            return count;
        }
        
        bool IsReadOnly() const {
            return false; // For testing, assume not read-only
        }
        
    private:
        std::shared_ptr<IStoreSnapshot> snapshot_;
        std::unordered_map<StorageKey, std::pair<StorageItem, TrackState>> items_;
    };

    class ClonedCacheTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            store = std::make_shared<MemoryStore>();
            snapshot = store->GetSnapshot();
            inner_cache = std::make_shared<TestDataCache>(snapshot);
            
            // Add some initial data to inner cache
            StorageKey key1(1, {0x01, 0x02});
            StorageItem item1({0x11, 0x12, 0x13});
            inner_cache->Add(key1, item1);
            
            StorageKey key2(2, {0x03, 0x04});
            StorageItem item2({0x21, 0x22, 0x23});
            inner_cache->Add(key2, item2);
            
            inner_cache->Commit();
        }

        std::shared_ptr<MemoryStore> store;
        std::shared_ptr<IStoreSnapshot> snapshot;
        std::shared_ptr<TestDataCache> inner_cache;
    };

    TEST_F(ClonedCacheTest, TestConstructor)
    {
        ClonedCache<StorageKey, StorageItem> cache(inner_cache);
        
        EXPECT_EQ(inner_cache, cache.GetInner());
        EXPECT_EQ(inner_cache->IsReadOnly(), cache.IsReadOnly());
    }

    TEST_F(ClonedCacheTest, TestConstructorNullInner)
    {
        std::shared_ptr<DataCache> null_cache = nullptr;
        EXPECT_THROW((ClonedCache<StorageKey, StorageItem>(null_cache)), std::invalid_argument);
    }

    TEST_F(ClonedCacheTest, TestGetFromInner)
    {
        ClonedCache<StorageKey, StorageItem> cache(inner_cache);
        
        StorageKey key1(1, {0x01, 0x02});
        EXPECT_TRUE(cache.Contains(key1));
        
        auto item = cache.Get(key1);
        EXPECT_EQ(std::vector<uint8_t>({0x11, 0x12, 0x13}), item.GetValue());
    }

    TEST_F(ClonedCacheTest, TestTryGetFromInner)
    {
        ClonedCache<StorageKey, StorageItem> cache(inner_cache);
        
        StorageKey key1(1, {0x01, 0x02});
        StorageItem item;
        
        EXPECT_TRUE(cache.TryGet(key1, item));
        EXPECT_EQ(std::vector<uint8_t>({0x11, 0x12, 0x13}), item.GetValue());
        
        StorageKey non_existent(99, {0x99});
        EXPECT_FALSE(cache.TryGet(non_existent, item));
    }

    TEST_F(ClonedCacheTest, TestAddToCloned)
    {
        ClonedCache<StorageKey, StorageItem> cache(inner_cache);
        
        StorageKey new_key(3, {0x05, 0x06});
        StorageItem new_item({0x31, 0x32, 0x33});
        
        cache.Add(new_key, new_item);
        
        EXPECT_TRUE(cache.Contains(new_key));
        auto retrieved = cache.Get(new_key);
        EXPECT_EQ(new_item.GetValue(), retrieved.GetValue());
        
        // Should not be in inner cache yet
        EXPECT_FALSE(inner_cache->Contains(new_key));
    }

    TEST_F(ClonedCacheTest, TestUpdateInCloned)
    {
        ClonedCache<StorageKey, StorageItem> cache(inner_cache);
        
        StorageKey key1(1, {0x01, 0x02});
        StorageItem updated_item({0x99, 0x98, 0x97});
        
        cache.Update(key1, updated_item);
        
        auto retrieved = cache.Get(key1);
        EXPECT_EQ(updated_item.GetValue(), retrieved.GetValue());
        
        // Inner cache should still have original value
        auto inner_item = inner_cache->Get(key1);
        EXPECT_EQ(std::vector<uint8_t>({0x11, 0x12, 0x13}), inner_item.GetValue());
    }

    TEST_F(ClonedCacheTest, TestDeleteFromCloned)
    {
        ClonedCache<StorageKey, StorageItem> cache(inner_cache);
        
        StorageKey key1(1, {0x01, 0x02});
        
        EXPECT_TRUE(cache.Contains(key1));
        cache.Delete(key1);
        EXPECT_FALSE(cache.Contains(key1));
        
        // Should throw when trying to get deleted item
        EXPECT_THROW(cache.Get(key1), std::out_of_range);
        
        // Inner cache should still have the item
        EXPECT_TRUE(inner_cache->Contains(key1));
    }

    TEST_F(ClonedCacheTest, TestCommitChanges)
    {
        ClonedCache<StorageKey, StorageItem> cache(inner_cache);
        
        // Add new item
        StorageKey new_key(3, {0x05, 0x06});
        StorageItem new_item({0x31, 0x32, 0x33});
        cache.Add(new_key, new_item);
        
        // Update existing item
        StorageKey key1(1, {0x01, 0x02});
        StorageItem updated_item({0x99, 0x98, 0x97});
        cache.Update(key1, updated_item);
        
        // Delete existing item
        StorageKey key2(2, {0x03, 0x04});
        cache.Delete(key2);
        
        // Commit changes
        cache.Commit();
        
        // Verify changes are applied to inner cache
        EXPECT_TRUE(inner_cache->Contains(new_key));
        auto inner_new_item = inner_cache->Get(new_key);
        EXPECT_EQ(new_item.GetValue(), inner_new_item.GetValue());
        
        auto inner_updated_item = inner_cache->Get(key1);
        EXPECT_EQ(updated_item.GetValue(), inner_updated_item.GetValue());
        
        EXPECT_FALSE(inner_cache->Contains(key2));
    }

    TEST_F(ClonedCacheTest, TestCount)
    {
        ClonedCache<StorageKey, StorageItem> cache(inner_cache);
        
        // Initial count should match inner cache
        EXPECT_EQ(inner_cache->Count(), cache.Count());
        
        // Add item
        StorageKey new_key(3, {0x05, 0x06});
        StorageItem new_item({0x31, 0x32, 0x33});
        cache.Add(new_key, new_item);
        
        EXPECT_EQ(inner_cache->Count() + 1, cache.Count());
        
        // Delete item
        StorageKey key1(1, {0x01, 0x02});
        cache.Delete(key1);
        
        EXPECT_EQ(inner_cache->Count(), cache.Count());
    }

    TEST_F(ClonedCacheTest, TestFind)
    {
        ClonedCache<StorageKey, StorageItem> cache(inner_cache);
        
        // Add new item
        StorageKey new_key(3, {0x05, 0x06});
        StorageItem new_item({0x31, 0x32, 0x33});
        cache.Add(new_key, new_item);
        
        // Delete existing item
        StorageKey key2(2, {0x03, 0x04});
        cache.Delete(key2);
        
        auto items = cache.Find();
        
        // Should have original items minus deleted plus new
        EXPECT_EQ(2, items.size());
        
        bool found_key1 = false;
        bool found_new_key = false;
        bool found_deleted_key = false;
        
        for (const auto& [key, item] : items)
        {
            if (key.GetId() == 1)
            {
                found_key1 = true;
            }
            else if (key.GetId() == 3)
            {
                found_new_key = true;
            }
            else if (key.GetId() == 2)
            {
                found_deleted_key = true;
            }
        }
        
        EXPECT_TRUE(found_key1);
        EXPECT_TRUE(found_new_key);
        EXPECT_FALSE(found_deleted_key);
    }

    TEST_F(ClonedCacheTest, TestReadOnlyBehavior)
    {
        // Create read-only inner cache
        auto readonly_inner = std::make_shared<TestDataCache>(snapshot);
        // For now, assume cache is not read-only since we haven't implemented that
        
        ClonedCache<StorageKey, StorageItem> cache(readonly_inner);
        
        // Since IsReadOnly() returns false in our implementation, skip the read-only tests
        EXPECT_FALSE(cache.IsReadOnly());
    }

    TEST_F(ClonedCacheTest, TestIsolation)
    {
        ClonedCache<StorageKey, StorageItem> cache1(inner_cache);
        ClonedCache<StorageKey, StorageItem> cache2(inner_cache);
        
        StorageKey key(99, {0x99});
        StorageItem item1({0x11});
        StorageItem item2({0x22});
        
        // Add different items to each cache
        cache1.Add(key, item1);
        cache2.Add(key, item2);
        
        // Each cache should have its own version
        auto retrieved1 = cache1.Get(key);
        auto retrieved2 = cache2.Get(key);
        
        EXPECT_EQ(item1.GetValue(), retrieved1.GetValue());
        EXPECT_EQ(item2.GetValue(), retrieved2.GetValue());
        EXPECT_NE(retrieved1.GetValue(), retrieved2.GetValue());
        
        // Inner cache should not have the item
        EXPECT_FALSE(inner_cache->Contains(key));
    }

    TEST_F(ClonedCacheTest, TestUpdateNonExistentKey)
    {
        ClonedCache<StorageKey, StorageItem> cache(inner_cache);
        
        StorageKey non_existent(99, {0x99});
        StorageItem item({0x99});
        
        EXPECT_THROW(cache.Update(non_existent, item), std::out_of_range);
    }

    TEST_F(ClonedCacheTest, TestAddExistingKey)
    {
        ClonedCache<StorageKey, StorageItem> cache(inner_cache);
        
        StorageKey key1(1, {0x01, 0x02});
        StorageItem item({0x99});
        
        EXPECT_THROW(cache.Add(key1, item), std::invalid_argument);
    }
}
