#include <gtest/gtest.h>
#include <neo/persistence/cloned_cache.h>
#include <neo/persistence/data_cache.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>

using namespace neo::persistence;
using namespace neo::io;

TEST(DataCacheTest, StoreCache)
{
    // Create a store
    MemoryStore store;

    // Create a cache
    auto snapshot = store.GetSnapshot();
    StoreCache cache(std::shared_ptr<IStoreSnapshot>(snapshot.release()));

    // Create a storage key and item
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    ByteVector key = ByteVector::Parse("0102030405");
    StorageKey storageKey(scriptHash, key);

    ByteVector value = ByteVector::Parse("0607080910");
    StorageItem storageItem(value);

    // Try to get a non-existent key
    auto item = cache.TryGet(storageKey);
    EXPECT_TRUE(item == nullptr);

    // Add the item
    cache.Add(storageKey, storageItem);

    // Try to get the key
    auto item2 = cache.TryGet(storageKey);
    EXPECT_TRUE(item2 != nullptr);
    EXPECT_EQ(item2->GetValue(), value);

    // Get the key
    auto& item3 = cache.Get(storageKey);
    EXPECT_EQ(item3.GetValue(), value);

    // Modify the item
    ByteVector value2 = ByteVector::Parse("1112131415");
    item3.SetValue(value2);

    // Try to get the key again
    auto item4 = cache.TryGet(storageKey);
    EXPECT_TRUE(item4 != nullptr);
    EXPECT_EQ(item4->GetValue(), value2);

    // Delete the key
    cache.Delete(storageKey);

    // Try to get the key again
    auto item5 = cache.TryGet(storageKey);
    EXPECT_TRUE(item5 == nullptr);

    // Try to get the key from the store
    ByteVector keyBytes = storageKey.ToArray();
    auto valueBytes = store.TryGet(keyBytes);
    EXPECT_FALSE(valueBytes.has_value());

    // Add the item again
    cache.Add(storageKey, storageItem);

    // Commit the cache
    cache.Commit();

    // Try to get the key from the store
    auto valueBytes2 = store.TryGet(keyBytes);
    EXPECT_TRUE(valueBytes2.has_value());

    // Deserialize the value
    StorageItem storageItem2;
    storageItem2.DeserializeFromArray(valueBytes2->AsSpan());
    EXPECT_EQ(storageItem2.GetValue(), value);
}

TEST(DataCacheTest, StoreCacheFind)
{
    // Create a store
    MemoryStore store;

    // Add some items to the store
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");

    StorageKey storageKey1(scriptHash, ByteVector::Parse("0102030405"));
    StorageItem storageItem1(ByteVector::Parse("0607080910"));
    ByteVector keyBytes1 = storageKey1.ToArray();
    ByteVector valueBytes1 = storageItem1.ToArray();
    store.Put(keyBytes1, valueBytes1);

    StorageKey storageKey2(scriptHash, ByteVector::Parse("0102031415"));
    StorageItem storageItem2(ByteVector::Parse("1617181920"));
    ByteVector keyBytes2 = storageKey2.ToArray();
    ByteVector valueBytes2 = storageItem2.ToArray();
    store.Put(keyBytes2, valueBytes2);

    // Create a cache
    auto snapshot = store.GetSnapshot();
    StoreCache cache(std::shared_ptr<IStoreSnapshot>(snapshot.release()));

    // Add an item to the cache
    StorageKey storageKey3(scriptHash, ByteVector::Parse("0103030405"));
    StorageItem storageItem3(ByteVector::Parse("2627282930"));
    cache.Add(storageKey3, storageItem3);

    // Find all items
    auto items = cache.Find();
    EXPECT_EQ(items.size(), 3);

    // Find items with a prefix
    StorageKey prefix(scriptHash, ByteVector::Parse("0102"));
    auto items2 = cache.Find(&prefix);
    EXPECT_EQ(items2.size(), 2);

    // Delete an item
    cache.Delete(storageKey1);

    // Find all items
    auto items3 = cache.Find();
    EXPECT_EQ(items3.size(), 2);

    // Find items with a prefix
    auto items4 = cache.Find(&prefix);
    EXPECT_EQ(items4.size(), 1);

    // Commit the cache
    cache.Commit();

    // Find all items in the store
    auto storeItems = store.Find();
    EXPECT_EQ(storeItems.size(), 2);
}

TEST(DataCacheTest, ClonedCache)
{
    // Create a store
    MemoryStore store;

    // Create a cache
    auto snapshot = store.GetSnapshot();
    StoreCache storeCache(std::shared_ptr<IStoreSnapshot>(snapshot.release()));

    // Create a storage key and item
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    ByteVector key = ByteVector::Parse("0102030405");
    StorageKey storageKey(scriptHash, key);

    ByteVector value = ByteVector::Parse("0607080910");
    StorageItem storageItem(value);

    // Add the item to the store cache
    storeCache.Add(storageKey, storageItem);

    // Create a cloned cache
    auto storeCachePtr = std::make_shared<StoreCache>(std::move(storeCache));
    ClonedCache<StorageKey, StorageItem> clonedCache(storeCachePtr);

    // Try to get the existing key from inner cache through cloned cache
    StorageItem item;
    EXPECT_TRUE(clonedCache.TryGet(storageKey, item));
    EXPECT_EQ(item.GetValue(), value);

    // Add a new item to cloned cache
    StorageKey storageKey2(scriptHash, ByteVector::Parse("0102030406"));
    clonedCache.Add(storageKey2, storageItem);

    // Modify the item
    ByteVector value2 = ByteVector::Parse("1112131415");
    StorageItem updatedItem(value2);
    clonedCache.Update(storageKey, updatedItem);

    // Try to get the key again
    StorageItem item3;
    EXPECT_TRUE(clonedCache.TryGet(storageKey, item3));
    EXPECT_EQ(item3.GetValue(), value2);

    // Try to get the key from the store cache
    auto item4 = storeCachePtr->TryGet(storageKey);
    EXPECT_TRUE(item4 != nullptr);
    EXPECT_EQ(item4->GetValue(), value);

    // Commit the cloned cache
    clonedCache.Commit();

    // After commit, the cloned cache should still be able to access the inner cache
    StorageItem item5;
    EXPECT_TRUE(clonedCache.TryGet(storageKey, item5));  // Should still find item in inner cache
    EXPECT_EQ(item5.GetValue(), value2);                 // Should have updated value after commit
}

TEST(DataCacheTest, ClonedCacheFind)
{
    // Create a store
    MemoryStore store;

    // Create a cache
    auto snapshot = store.GetSnapshot();
    StoreCache storeCache(std::shared_ptr<IStoreSnapshot>(snapshot.release()));

    // Add some items to the store cache
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");

    StorageKey storageKey1(scriptHash, ByteVector::Parse("0102030405"));
    StorageItem storageItem1(ByteVector::Parse("0607080910"));
    storeCache.Add(storageKey1, storageItem1);

    StorageKey storageKey2(scriptHash, ByteVector::Parse("0102031415"));
    StorageItem storageItem2(ByteVector::Parse("1617181920"));
    storeCache.Add(storageKey2, storageItem2);

    // Create a cloned cache
    auto storeCachePtr = std::make_shared<StoreCache>(std::move(storeCache));
    ClonedCache<StorageKey, StorageItem> clonedCache(storeCachePtr);

    // Add an additional item to the cloned cache
    StorageKey storageKey3(scriptHash, ByteVector::Parse("0103030405"));
    StorageItem storageItem3(ByteVector::Parse("2627282930"));
    clonedCache.Add(storageKey3, storageItem3);

    // Find all items
    auto items = clonedCache.Find();
    EXPECT_EQ(items.size(), 3);

    // Delete an item
    clonedCache.Delete(storageKey1);

    // Find all items
    auto items3 = clonedCache.Find();
    EXPECT_EQ(items3.size(), 2);

    // Find all items in the store cache
    auto storeItems = storeCachePtr->Find();
    EXPECT_EQ(storeItems.size(), 2);

    // Commit the cloned cache
    clonedCache.Commit();

    // Find all items in the store cache (should be unchanged since commit just clears)
    auto storeItems2 = storeCachePtr->Find();
    EXPECT_EQ(storeItems2.size(), 2);
}
