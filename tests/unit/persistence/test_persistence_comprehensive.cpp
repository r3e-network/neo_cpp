/**
 * @file test_persistence_comprehensive.cpp
 * @brief Comprehensive unit tests for persistence module to increase coverage
 */

#include <gtest/gtest.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/data_cache.h>
#include <neo/persistence/cloned_cache.h>
#include <neo/persistence/store_cache.h>
#include <neo/io/byte_vector.h>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

using namespace neo::persistence;
using namespace neo::io;

class PersistenceComprehensiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        store_ = std::make_shared<MemoryStore>();
    }
    
    void TearDown() override {
        store_.reset();
    }
    
    std::shared_ptr<MemoryStore> store_;
};

// ============================================================================
// MemoryStore Tests
// ============================================================================

TEST_F(PersistenceComprehensiveTest, MemoryStore_PutAndGet) {
    StorageKey key(1, ByteVector{0x01, 0x02, 0x03});
    StorageItem item(ByteVector{0xAA, 0xBB, 0xCC});
    
    store_->Put(key, item);
    
    auto retrieved = store_->Get(key);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->GetValue(), item.GetValue());
}

TEST_F(PersistenceComprehensiveTest, MemoryStore_Delete) {
    StorageKey key(2, ByteVector{0x04, 0x05});
    StorageItem item(ByteVector{0xDD, 0xEE});
    
    store_->Put(key, item);
    EXPECT_TRUE(store_->Contains(key));
    
    store_->Delete(key);
    EXPECT_FALSE(store_->Contains(key));
}

TEST_F(PersistenceComprehensiveTest, MemoryStore_Seek) {
    // Add multiple items
    for (int i = 0; i < 5; ++i) {
        StorageKey key(1, ByteVector{static_cast<uint8_t>(i)});
        StorageItem item(ByteVector{static_cast<uint8_t>(i * 2)});
        store_->Put(key, item);
    }
    
    // Seek with prefix
    StorageKey prefix(1, ByteVector{});
    auto results = store_->Seek(prefix, 10);
    EXPECT_EQ(results.size(), 5u);
}

TEST_F(PersistenceComprehensiveTest, MemoryStore_Clear) {
    // Add items
    StorageKey key1(1, ByteVector{0x01});
    StorageKey key2(2, ByteVector{0x02});
    store_->Put(key1, StorageItem(ByteVector{0xFF}));
    store_->Put(key2, StorageItem(ByteVector{0xEE}));
    
    store_->Clear();
    
    EXPECT_FALSE(store_->Contains(key1));
    EXPECT_FALSE(store_->Contains(key2));
}

// ============================================================================
// StorageKey Tests
// ============================================================================

TEST_F(PersistenceComprehensiveTest, StorageKey_Construction) {
    StorageKey key1(1, ByteVector{0x01, 0x02});
    EXPECT_EQ(key1.Id, 1);
    EXPECT_EQ(key1.Key.Size(), 2u);
    
    StorageKey key2(255, ByteVector{});
    EXPECT_EQ(key2.Id, 255);
    EXPECT_EQ(key2.Key.Size(), 0u);
}

TEST_F(PersistenceComprehensiveTest, StorageKey_Comparison) {
    StorageKey key1(1, ByteVector{0x01});
    StorageKey key2(1, ByteVector{0x01});
    StorageKey key3(1, ByteVector{0x02});
    StorageKey key4(2, ByteVector{0x01});
    
    EXPECT_EQ(key1, key2);
    EXPECT_NE(key1, key3);
    EXPECT_NE(key1, key4);
    EXPECT_LT(key1, key3);
    EXPECT_LT(key1, key4);
}

TEST_F(PersistenceComprehensiveTest, StorageKey_Serialization) {
    StorageKey key(42, ByteVector{0xAA, 0xBB, 0xCC});
    
    auto serialized = key.ToArray();
    EXPECT_GE(serialized.Size(), 4u); // At least Id + key data
    
    // First byte should be the Id
    EXPECT_EQ(serialized[0], 42);
}

// ============================================================================
// StorageItem Tests
// ============================================================================

TEST_F(PersistenceComprehensiveTest, StorageItem_Construction) {
    ByteVector data{0x11, 0x22, 0x33};
    StorageItem item(data);
    
    EXPECT_EQ(item.GetValue(), data);
    EXPECT_FALSE(item.IsConstant());
}

TEST_F(PersistenceComprehensiveTest, StorageItem_ConstantFlag) {
    StorageItem item(ByteVector{0xFF});
    EXPECT_FALSE(item.IsConstant());
    
    // Note: IsConstant appears to be read-only in the current API
    // We can only check its value, not set it
}

TEST_F(PersistenceComprehensiveTest, StorageItem_Clone) {
    StorageItem original(ByteVector{0xAA, 0xBB});
    
    StorageItem copy = original;
    EXPECT_EQ(copy.GetValue(), original.GetValue());
    EXPECT_EQ(copy.IsConstant(), original.IsConstant());
}

// ============================================================================
// DataCache Tests
// ============================================================================

TEST_F(PersistenceComprehensiveTest, DataCache_AddAndFind) {
    DataCache cache(store_);
    
    StorageKey key(1, ByteVector{0x01});
    StorageItem item(ByteVector{0xFF});
    
    cache.Add(key, item);
    
    auto found = cache.Find(key);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetValue(), item.GetValue());
}

TEST_F(PersistenceComprehensiveTest, DataCache_TryGet) {
    DataCache cache(store_);
    
    StorageKey key(2, ByteVector{0x02});
    StorageItem item(ByteVector{0xAA});
    
    cache.Add(key, item);
    
    StorageItem retrieved;
    bool success = cache.TryGet(key, retrieved);
    EXPECT_TRUE(success);
    EXPECT_EQ(retrieved.GetValue(), item.GetValue());
}

TEST_F(PersistenceComprehensiveTest, DataCache_Delete) {
    DataCache cache(store_);
    
    StorageKey key(3, ByteVector{0x03});
    StorageItem item(ByteVector{0xBB});
    
    cache.Add(key, item);
    EXPECT_TRUE(cache.Contains(key));
    
    cache.Delete(key);
    EXPECT_FALSE(cache.Contains(key));
}

TEST_F(PersistenceComprehensiveTest, DataCache_Commit) {
    DataCache cache(store_);
    
    StorageKey key(4, ByteVector{0x04});
    StorageItem item(ByteVector{0xCC});
    
    cache.Add(key, item);
    cache.Commit();
    
    // After commit, item should be in the underlying store
    auto retrieved = store_->Get(key);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->GetValue(), item.GetValue());
}

// ============================================================================
// ClonedCache Tests
// ============================================================================

TEST_F(PersistenceComprehensiveTest, ClonedCache_IndependentChanges) {
    DataCache parentCache(store_);
    
    StorageKey key1(1, ByteVector{0x01});
    StorageKey key2(2, ByteVector{0x02});
    StorageItem item1(ByteVector{0xAA});
    StorageItem item2(ByteVector{0xBB});
    
    parentCache.Add(key1, item1);
    
    // Create cloned cache
    ClonedCache clonedCache(&parentCache);
    
    // Add to cloned cache
    clonedCache.Add(key2, item2);
    
    // Parent shouldn't have key2
    EXPECT_FALSE(parentCache.Contains(key2));
    
    // Cloned should have both
    EXPECT_TRUE(clonedCache.Contains(key1));
    EXPECT_TRUE(clonedCache.Contains(key2));
}

TEST_F(PersistenceComprehensiveTest, ClonedCache_CommitToParent) {
    DataCache parentCache(store_);
    ClonedCache clonedCache(&parentCache);
    
    StorageKey key(5, ByteVector{0x05});
    StorageItem item(ByteVector{0xDD});
    
    clonedCache.Add(key, item);
    clonedCache.Commit();
    
    // After commit, parent should have the item
    EXPECT_TRUE(parentCache.Contains(key));
}

// ============================================================================
// StoreCache Tests
// ============================================================================

TEST_F(PersistenceComprehensiveTest, StoreCache_CachedAccess) {
    StoreCache cache(store_);
    
    StorageKey key(6, ByteVector{0x06});
    StorageItem item(ByteVector{0xEE});
    
    // First access - not cached
    EXPECT_FALSE(cache.Contains(key));
    
    // Add to cache
    cache.Add(key, item);
    
    // Now it should be cached
    EXPECT_TRUE(cache.Contains(key));
    
    auto found = cache.Find(key);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetValue(), item.GetValue());
}

TEST_F(PersistenceComprehensiveTest, StoreCache_GetOrAdd) {
    StoreCache cache(store_);
    
    StorageKey key(7, ByteVector{0x07});
    StorageItem defaultItem(ByteVector{0xFF});
    
    // GetOrAdd should create if not exists
    auto& item = cache.GetOrAdd(key, [&defaultItem]() { return defaultItem; });
    EXPECT_EQ(item.GetValue(), defaultItem.GetValue());
    
    // Second call should return existing
    StorageItem otherItem(ByteVector{0x00});
    auto& item2 = cache.GetOrAdd(key, [&otherItem]() { return otherItem; });
    EXPECT_EQ(item2.GetValue(), defaultItem.GetValue()); // Should still be the first one
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(PersistenceComprehensiveTest, EdgeCase_EmptyKey) {
    StorageKey emptyKey(0, ByteVector{});
    StorageItem item(ByteVector{0x01});
    
    store_->Put(emptyKey, item);
    
    auto retrieved = store_->Get(emptyKey);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->GetValue(), item.GetValue());
}

TEST_F(PersistenceComprehensiveTest, EdgeCase_LargeData) {
    // Test with large data
    ByteVector largeData(1024, 0xAB);
    StorageKey key(8, ByteVector{0x08});
    StorageItem item(largeData);
    
    store_->Put(key, item);
    
    auto retrieved = store_->Get(key);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->GetValue().Size(), 1024u);
}

TEST_F(PersistenceComprehensiveTest, EdgeCase_NonExistentKey) {
    StorageKey key(99, ByteVector{0x99});
    
    auto retrieved = store_->Get(key);
    EXPECT_FALSE(retrieved.has_value());
    
    EXPECT_FALSE(store_->Contains(key));
}

// ============================================================================
// Concurrency Tests (Basic)
// ============================================================================

TEST_F(PersistenceComprehensiveTest, Concurrency_MultipleReaders) {
    StorageKey key(10, ByteVector{0x10});
    StorageItem item(ByteVector{0xAA, 0xBB, 0xCC});
    
    store_->Put(key, item);
    
    // Simulate multiple readers
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &key, &successCount]() {
            auto retrieved = store_->Get(key);
            if (retrieved.has_value()) {
                successCount++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(successCount, 10);
}