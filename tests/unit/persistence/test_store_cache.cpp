#include <gtest/gtest.h>
#include <neo/persistence/store_cache.h>
#include <neo/persistence/memory_store.h>

namespace neo::persistence::tests
{
    class StoreCacheTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            store = std::make_shared<MemoryStore>();
            
            // Add some initial data to store first
            StorageKey key1(1, {0x01, 0x02});
            StorageItem item1({0x11, 0x12, 0x13});
            store->Put(key1.ToArray(), item1.ToArray());
            
            StorageKey key2(2, {0x03, 0x04});
            StorageItem item2({0x21, 0x22, 0x23});
            store->Put(key2.ToArray(), item2.ToArray());
            
            // Create snapshot after adding data
            snapshot = store->GetSnapshot();
        }

        std::shared_ptr<MemoryStore> store;
        std::shared_ptr<IStoreSnapshot> snapshot;
    };

    TEST_F(StoreCacheTest, TestConstructor)
    {
        StoreCache cache(snapshot);
        
        EXPECT_EQ(snapshot.get(), cache.GetStore().get());
        EXPECT_EQ(snapshot->IsReadOnly(), cache.IsReadOnly());
    }

    TEST_F(StoreCacheTest, TestConstructorNullStore)
    {
        EXPECT_THROW(StoreCache(nullptr), std::invalid_argument);
    }

    TEST_F(StoreCacheTest, TestGetFromStore)
    {
        StoreCache cache(snapshot);
        
        StorageKey key1(1, {0x01, 0x02});
        EXPECT_TRUE(cache.Contains(key1));
        
        auto item = cache.Get(key1);
        EXPECT_EQ(std::vector<uint8_t>({0x11, 0x12, 0x13}), item.GetValue());
        
        // Track state should be None for items from store
        EXPECT_EQ(TrackState::None, cache.GetTrackState(key1));
    }

    TEST_F(StoreCacheTest, TestTryGetFromStore)
    {
        StoreCache cache(snapshot);
        
        StorageKey key1(1, {0x01, 0x02});
        StorageItem item;
        
        EXPECT_TRUE(cache.TryGet(key1, item));
        EXPECT_EQ(std::vector<uint8_t>({0x11, 0x12, 0x13}), item.GetValue());
        
        StorageKey non_existent(99, {0x99});
        EXPECT_FALSE(cache.TryGet(non_existent, item));
    }

    TEST_F(StoreCacheTest, TestAddNewItem)
    {
        StoreCache cache(snapshot);
        
        StorageKey new_key(3, {0x05, 0x06});
        StorageItem new_item({0x31, 0x32, 0x33});
        
        cache.Add(new_key, new_item);
        
        EXPECT_TRUE(cache.Contains(new_key));
        auto retrieved = cache.Get(new_key);
        EXPECT_EQ(new_item.GetValue(), retrieved.GetValue());
        
        // Track state should be Added
        EXPECT_EQ(TrackState::Added, cache.GetTrackState(new_key));
        
        // Should not be in store yet
        EXPECT_FALSE(snapshot->Contains(new_key.ToArray()));
    }

    TEST_F(StoreCacheTest, TestAddExistingKey)
    {
        StoreCache cache(snapshot);
        
        StorageKey key1(1, {0x01, 0x02});
        StorageItem item({0x99});
        
        EXPECT_THROW(cache.Add(key1, item), std::invalid_argument);
    }

    TEST_F(StoreCacheTest, TestUpdateExistingItem)
    {
        StoreCache cache(snapshot);
        
        StorageKey key1(1, {0x01, 0x02});
        StorageItem updated_item({0x99, 0x98, 0x97});
        
        cache.Update(key1, updated_item);
        
        auto retrieved = cache.Get(key1);
        EXPECT_EQ(updated_item.GetValue(), retrieved.GetValue());
        
        // Track state should be Changed
        EXPECT_EQ(TrackState::Changed, cache.GetTrackState(key1));
        
        // Store should still have original value
        auto store_value = snapshot->TryGet(key1.ToArray());
        EXPECT_TRUE(store_value.has_value());
        StorageItem store_item;
        store_item.DeserializeFromArray(store_value->AsSpan());
        EXPECT_EQ(std::vector<uint8_t>({0x11, 0x12, 0x13}), store_item.GetValue());
    }

    TEST_F(StoreCacheTest, TestUpdateNonExistentKey)
    {
        StoreCache cache(snapshot);
        
        StorageKey non_existent(99, {0x99});
        StorageItem item({0x99});
        
        EXPECT_THROW(cache.Update(non_existent, item), std::out_of_range);
    }

    TEST_F(StoreCacheTest, TestUpdateAddedItem)
    {
        StoreCache cache(snapshot);
        
        StorageKey new_key(3, {0x05, 0x06});
        StorageItem new_item({0x31, 0x32, 0x33});
        StorageItem updated_item({0x41, 0x42, 0x43});
        
        cache.Add(new_key, new_item);
        cache.Update(new_key, updated_item);
        
        auto retrieved = cache.Get(new_key);
        EXPECT_EQ(updated_item.GetValue(), retrieved.GetValue());
        
        // Track state should still be Added
        EXPECT_EQ(TrackState::Added, cache.GetTrackState(new_key));
    }

    TEST_F(StoreCacheTest, TestDeleteExistingItem)
    {
        StoreCache cache(snapshot);
        
        StorageKey key1(1, {0x01, 0x02});
        
        EXPECT_TRUE(cache.Contains(key1));
        cache.Delete(key1);
        EXPECT_FALSE(cache.Contains(key1));
        
        // Should throw when trying to get deleted item
        EXPECT_THROW(cache.Get(key1), std::out_of_range);
        
        // Track state should be Deleted
        EXPECT_EQ(TrackState::Deleted, cache.GetTrackState(key1));
        
        // Store should still have the item
        EXPECT_TRUE(snapshot->Contains(key1.ToArray()));
    }

    TEST_F(StoreCacheTest, TestDeleteAddedItem)
    {
        StoreCache cache(snapshot);
        
        StorageKey new_key(3, {0x05, 0x06});
        StorageItem new_item({0x31, 0x32, 0x33});
        
        cache.Add(new_key, new_item);
        cache.Delete(new_key);
        
        EXPECT_FALSE(cache.Contains(new_key));
        
        // Track state should be None (item was removed completely)
        EXPECT_EQ(TrackState::None, cache.GetTrackState(new_key));
    }

    TEST_F(StoreCacheTest, TestGetTrackedItems)
    {
        StoreCache cache(snapshot);
        
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
        
        auto tracked_items = cache.GetTrackedItems();
        
        EXPECT_EQ(3, tracked_items.size());
        
        // Find items by iterating through the vector
        bool found_added = false, found_changed = false, found_deleted = false;
        for (const auto& [key, item_state] : tracked_items) {
            if (key.GetId() == new_key.GetId()) {
                EXPECT_EQ(TrackState::Added, item_state.second);
                found_added = true;
            } else if (key.GetId() == key1.GetId()) {
                EXPECT_EQ(TrackState::Changed, item_state.second);
                found_changed = true;
            } else if (key.GetId() == key2.GetId()) {
                EXPECT_EQ(TrackState::Deleted, item_state.second);
                found_deleted = true;
            }
        }
        EXPECT_TRUE(found_added);
        EXPECT_TRUE(found_changed);
        EXPECT_TRUE(found_deleted);
    }

    TEST_F(StoreCacheTest, TestGetChangedItems)
    {
        StoreCache cache(snapshot);
        
        // Add new item
        StorageKey new_key(3, {0x05, 0x06});
        StorageItem new_item({0x31, 0x32, 0x33});
        cache.Add(new_key, new_item);
        
        // Update existing item
        StorageKey key1(1, {0x01, 0x02});
        StorageItem updated_item({0x99, 0x98, 0x97});
        cache.Update(key1, updated_item);
        
        // Delete existing item (should not be in changed items)
        StorageKey key2(2, {0x03, 0x04});
        cache.Delete(key2);
        
        auto changed_items = cache.GetChangedItems();
        
        EXPECT_EQ(2, changed_items.size());
        
        bool found_new = false;
        bool found_updated = false;
        
        for (const auto& [key, item] : changed_items)
        {
            if (key.GetId() == 3)
            {
                found_new = true;
                EXPECT_EQ(new_item.GetValue(), item.GetValue());
            }
            else if (key.GetId() == 1)
            {
                found_updated = true;
                EXPECT_EQ(updated_item.GetValue(), item.GetValue());
            }
        }
        
        EXPECT_TRUE(found_new);
        EXPECT_TRUE(found_updated);
    }

    TEST_F(StoreCacheTest, TestGetDeletedItems)
    {
        StoreCache cache(snapshot);
        
        // Delete existing items
        StorageKey key1(1, {0x01, 0x02});
        StorageKey key2(2, {0x03, 0x04});
        cache.Delete(key1);
        cache.Delete(key2);
        
        auto deleted_items = cache.GetDeletedItems();
        
        EXPECT_EQ(2, deleted_items.size());
        EXPECT_TRUE(std::find(deleted_items.begin(), deleted_items.end(), key1) != deleted_items.end());
        EXPECT_TRUE(std::find(deleted_items.begin(), deleted_items.end(), key2) != deleted_items.end());
    }

    TEST_F(StoreCacheTest, TestCount)
    {
        StoreCache cache(snapshot);
        
        // Initial count should match store
        EXPECT_EQ(2, cache.Count()); // We added 2 items in SetUp
        
        // Add item
        StorageKey new_key(3, {0x05, 0x06});
        StorageItem new_item({0x31, 0x32, 0x33});
        cache.Add(new_key, new_item);
        
        EXPECT_EQ(3, cache.Count());
        
        // Delete item
        StorageKey key1(1, {0x01, 0x02});
        cache.Delete(key1);
        
        EXPECT_EQ(2, cache.Count());
    }

    TEST_F(StoreCacheTest, TestFind)
    {
        StoreCache cache(snapshot);
        
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
        
        auto items = cache.Find();
        
        // Should have updated item and new item, but not deleted item
        EXPECT_EQ(2, items.size());
        
        bool found_updated = false;
        bool found_new = false;
        bool found_deleted = false;
        
        for (const auto& [key, item] : items)
        {
            if (key.GetId() == 1)
            {
                found_updated = true;
                EXPECT_EQ(updated_item.GetValue(), item.GetValue());
            }
            else if (key.GetId() == 3)
            {
                found_new = true;
                EXPECT_EQ(new_item.GetValue(), item.GetValue());
            }
            else if (key.GetId() == 2)
            {
                found_deleted = true;
            }
        }
        
        EXPECT_TRUE(found_updated);
        EXPECT_TRUE(found_new);
        EXPECT_FALSE(found_deleted);
    }

    TEST_F(StoreCacheTest, TestTrackStateEnum)
    {
        // Test TrackState enum values
        EXPECT_EQ(0, static_cast<uint8_t>(TrackState::None));
        EXPECT_EQ(1, static_cast<uint8_t>(TrackState::Added));
        EXPECT_EQ(2, static_cast<uint8_t>(TrackState::Changed));
        EXPECT_EQ(3, static_cast<uint8_t>(TrackState::Deleted));
    }

    TEST_F(StoreCacheTest, TestSeekDirectionEnum)
    {
        // Test SeekDirection enum values
        EXPECT_EQ(0, static_cast<uint8_t>(SeekDirection::Forward));
        EXPECT_EQ(1, static_cast<uint8_t>(SeekDirection::Backward));
    }
}
