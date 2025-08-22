#include <gtest/gtest.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/data_cache.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <memory>
#include <string>

using namespace neo::persistence;

/**
 * @brief Test fixture for CloneCache functionality
 * Converted from C# UT_CloneCache.cs
 */
class CloneCacheTest : public testing::Test
{
protected:
    std::unique_ptr<MemoryStore> store_;
    
    // Test keys and values (converted from C#)
    static inline StorageKey key1_{0, io::ByteVector{'k', 'e', 'y', '1'}};
    static inline StorageKey key2_{0, io::ByteVector{'k', 'e', 'y', '2'}};
    static inline StorageKey key3_{0, io::ByteVector{'k', 'e', 'y', '3'}};
    static inline StorageKey key4_{0, io::ByteVector{'k', 'e', 'y', '4'}};
    
    static inline StorageItem value1_{io::ByteVector{'v', 'a', 'l', 'u', 'e', '1'}};
    static inline StorageItem value2_{io::ByteVector{'v', 'a', 'l', 'u', 'e', '2'}};
    static inline StorageItem value3_{io::ByteVector{'v', 'a', 'l', 'u', 'e', '3'}};

    void SetUp() override
    {
        store_ = std::make_unique<MemoryStore>();
    }

    void TearDown() override
    {
        store_.reset();
    }
};

// Converted from C# TestCloneCache method
TEST_F(CloneCacheTest, TestCloneCache)
{
    // Create a data cache with the store
    auto store_view = store_->GetSnapshot();
    DataCache data_cache(std::move(store_view));
    
    // Clone the cache
    auto cloned_cache = data_cache.CreateSnapshot();
    
    ASSERT_NE(cloned_cache, nullptr);
    // Verify that the cloned cache is a separate instance
    EXPECT_NE(&data_cache, cloned_cache.get());
}

// Converted from C# TestAddInternal method
TEST_F(CloneCacheTest, TestAddInternal)
{
    auto store_view = store_->GetSnapshot();
    DataCache data_cache(std::move(store_view));
    auto cloned_cache = data_cache.CreateSnapshot();
    
    // Add an item to the cloned cache
    cloned_cache->Add(key1_, value1_);
    
    // Verify the item was added
    EXPECT_TRUE(cloned_cache->Contains(key1_));
    
    auto retrieved = cloned_cache->TryGet(key1_);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->GetValue(), value1_.GetValue());
}

// Test cache isolation (original cache should not see changes in clone)
TEST_F(CloneCacheTest, TestCacheIsolation)
{
    auto store_view = store_->GetSnapshot();
    DataCache data_cache(std::move(store_view));
    
    // Add item to original cache
    data_cache.Add(key1_, value1_);
    
    // Create clone after adding item
    auto cloned_cache = data_cache.CreateSnapshot();
    
    // Clone should see the original item
    EXPECT_TRUE(cloned_cache->Contains(key1_));
    
    // Add different item to clone
    cloned_cache->Add(key2_, value2_);
    
    // Original cache should not see the new item (until commit)
    EXPECT_FALSE(data_cache.Contains(key2_));
    
    // Clone should see both items
    EXPECT_TRUE(cloned_cache->Contains(key1_));
    EXPECT_TRUE(cloned_cache->Contains(key2_));
}

// Test cache updates
TEST_F(CloneCacheTest, TestCacheUpdates)
{
    auto store_view = store_->GetSnapshot();
    DataCache data_cache(std::move(store_view));
    
    // Add initial item
    data_cache.Add(key1_, value1_);
    
    auto cloned_cache = data_cache.CreateSnapshot();
    
    // Update item in clone
    cloned_cache->Update(key1_, value2_);
    
    // Verify update
    auto retrieved = cloned_cache->TryGet(key1_);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->GetValue(), value2_.GetValue());
}

// Test cache deletion
TEST_F(CloneCacheTest, TestCacheDeletion)
{
    auto store_view = store_->GetSnapshot();
    DataCache data_cache(std::move(store_view));
    
    // Add item
    data_cache.Add(key1_, value1_);
    
    auto cloned_cache = data_cache.CreateSnapshot();
    
    // Verify item exists
    EXPECT_TRUE(cloned_cache->Contains(key1_));
    
    // Delete item from clone
    cloned_cache->Delete(key1_);
    
    // Verify deletion
    EXPECT_FALSE(cloned_cache->Contains(key1_));
}
