#include <algorithm>
#include <gtest/gtest.h>
#include <neo/io/byte_vector.h>
#include <neo/persistence/memory_store.h>
#include <string>
#include <thread>

using namespace neo::persistence;
using namespace neo::io;

/**
 * @brief Test fixture for MemoryStore
 */
class MemoryStoreTest : public testing::Test
{
  protected:
    std::unique_ptr<MemoryStore> store;

    void SetUp() override
    {
        store = std::make_unique<MemoryStore>();
    }

    // Helper to create ByteVector from string
    ByteVector ToByteVector(const std::string& str)
    {
        return ByteVector(reinterpret_cast<const uint8_t*>(str.data()), str.size());
    }

    // Helper to convert ByteVector to string
    std::string ToString(const ByteVector& vec)
    {
        return std::string(reinterpret_cast<const char*>(vec.Data()), vec.Size());
    }
};

TEST_F(MemoryStoreTest, Constructor)
{
    MemoryStore store;

    // Store should be empty after construction
    EXPECT_FALSE(store.Contains(ToByteVector("key")));
}

TEST_F(MemoryStoreTest, PutAndGet)
{
    ByteVector key = ToByteVector("key1");
    ByteVector value = ToByteVector("value1");

    // Put value
    store->Put(key, value);

    // Get value
    auto result = store->TryGet(key);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(value, result.value());

    // Get with Get() method
    ByteVector getValue = store->Get(key);
    EXPECT_EQ(value, getValue);
}

TEST_F(MemoryStoreTest, TryGetNonExistent)
{
    ByteVector key = ToByteVector("nonexistent");

    auto result = store->TryGet(key);
    EXPECT_FALSE(result.has_value());
}

TEST_F(MemoryStoreTest, GetNonExistentThrows)
{
    ByteVector key = ToByteVector("nonexistent");

    EXPECT_THROW(store->Get(key), std::runtime_error);
}

TEST_F(MemoryStoreTest, Contains)
{
    ByteVector key1 = ToByteVector("key1");
    ByteVector key2 = ToByteVector("key2");
    ByteVector value = ToByteVector("value");

    EXPECT_FALSE(store->Contains(key1));
    EXPECT_FALSE(store->Contains(key2));

    store->Put(key1, value);

    EXPECT_TRUE(store->Contains(key1));
    EXPECT_FALSE(store->Contains(key2));
}

TEST_F(MemoryStoreTest, UpdateExistingKey)
{
    ByteVector key = ToByteVector("key");
    ByteVector value1 = ToByteVector("value1");
    ByteVector value2 = ToByteVector("value2");

    store->Put(key, value1);
    EXPECT_EQ(value1, store->Get(key));

    store->Put(key, value2);
    EXPECT_EQ(value2, store->Get(key));
}

TEST_F(MemoryStoreTest, Delete)
{
    ByteVector key = ToByteVector("key");
    ByteVector value = ToByteVector("value");

    store->Put(key, value);
    EXPECT_TRUE(store->Contains(key));

    store->Delete(key);
    EXPECT_FALSE(store->Contains(key));
    EXPECT_FALSE(store->TryGet(key).has_value());
}

TEST_F(MemoryStoreTest, DeleteNonExistent)
{
    ByteVector key = ToByteVector("nonexistent");

    // Delete non-existent key should not throw
    EXPECT_NO_THROW(store->Delete(key));
}

TEST_F(MemoryStoreTest, Find_AllEntries)
{
    // Add multiple entries
    store->Put(ToByteVector("key1"), ToByteVector("value1"));
    store->Put(ToByteVector("key2"), ToByteVector("value2"));
    store->Put(ToByteVector("key3"), ToByteVector("value3"));

    // Find all entries (no prefix)
    auto results = store->Find();
    EXPECT_EQ(3u, results.size());

    // Verify all entries are present
    std::vector<std::string> keys;
    for (const auto& [k, v] : results)
    {
        keys.push_back(ToString(k));
    }

    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key1") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key2") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key3") != keys.end());
}

TEST_F(MemoryStoreTest, Find_WithPrefix)
{
    // Add entries with different prefixes
    store->Put(ToByteVector("prefix1:key1"), ToByteVector("value1"));
    store->Put(ToByteVector("prefix1:key2"), ToByteVector("value2"));
    store->Put(ToByteVector("prefix2:key1"), ToByteVector("value3"));
    store->Put(ToByteVector("other:key"), ToByteVector("value4"));

    // Find with prefix
    ByteVector prefix = ToByteVector("prefix1:");
    auto results = store->Find(&prefix);

    EXPECT_EQ(2u, results.size());

    // Verify correct entries are returned
    for (const auto& [k, v] : results)
    {
        std::string keyStr = ToString(k);
        EXPECT_TRUE(keyStr.starts_with("prefix1:"));
    }
}

TEST_F(MemoryStoreTest, Seek)
{
    // Add entries
    store->Put(ToByteVector("a:1"), ToByteVector("value1"));
    store->Put(ToByteVector("a:2"), ToByteVector("value2"));
    store->Put(ToByteVector("b:1"), ToByteVector("value3"));
    store->Put(ToByteVector("b:2"), ToByteVector("value4"));

    // Seek with prefix "a:"
    ByteVector prefix = ToByteVector("a:");
    auto results = store->Seek(prefix);

    EXPECT_EQ(2u, results.size());
    for (const auto& [k, v] : results)
    {
        std::string keyStr = ToString(k);
        EXPECT_TRUE(keyStr.starts_with("a:"));
    }
}

TEST_F(MemoryStoreTest, CopyConstructor)
{
    // Add data to original store
    store->Put(ToByteVector("key1"), ToByteVector("value1"));
    store->Put(ToByteVector("key2"), ToByteVector("value2"));

    // Create copy
    MemoryStore copy(*store);

    // Verify copy has same data
    EXPECT_TRUE(copy.Contains(ToByteVector("key1")));
    EXPECT_TRUE(copy.Contains(ToByteVector("key2")));
    EXPECT_EQ(ToByteVector("value1"), copy.Get(ToByteVector("key1")));
    EXPECT_EQ(ToByteVector("value2"), copy.Get(ToByteVector("key2")));

    // Modify original
    store->Put(ToByteVector("key3"), ToByteVector("value3"));

    // Copy should not be affected
    EXPECT_FALSE(copy.Contains(ToByteVector("key3")));
}

TEST_F(MemoryStoreTest, EmptyStore)
{
    // Find on empty store
    auto results = store->Find();
    EXPECT_TRUE(results.empty());

    // Seek on empty store
    ByteVector prefix = ToByteVector("prefix");
    auto seekResults = store->Seek(prefix);
    EXPECT_TRUE(seekResults.empty());
}

TEST_F(MemoryStoreTest, LargeData)
{
    // Test with large values
    std::string largeString(1024 * 1024, 'X');  // 1MB
    ByteVector key = ToByteVector("large");
    ByteVector value = ToByteVector(largeString);

    store->Put(key, value);

    auto result = store->TryGet(key);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(value, result.value());
}

TEST_F(MemoryStoreTest, ThreadSafety)
{
    const int numThreads = 4;
    const int itemsPerThread = 100;
    std::vector<std::thread> threads;

    // Multiple threads writing
    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back(
            [this, t, itemsPerThread]()
            {
                for (int i = 0; i < itemsPerThread; ++i)
                {
                    std::string key = "thread" + std::to_string(t) + "_key" + std::to_string(i);
                    std::string value = "value" + std::to_string(t * itemsPerThread + i);
                    store->Put(ToByteVector(key), ToByteVector(value));
                }
            });
    }

    // Wait for all threads
    for (auto& thread : threads)
    {
        thread.join();
    }

    // Verify all items are present
    for (int t = 0; t < numThreads; ++t)
    {
        for (int i = 0; i < itemsPerThread; ++i)
        {
            std::string key = "thread" + std::to_string(t) + "_key" + std::to_string(i);
            EXPECT_TRUE(store->Contains(ToByteVector(key)));
        }
    }
}

TEST_F(MemoryStoreTest, Snapshot_Basic)
{
    // Add initial data
    store->Put(ToByteVector("key1"), ToByteVector("value1"));
    store->Put(ToByteVector("key2"), ToByteVector("value2"));

    // Create snapshot
    auto snapshot = store->GetSnapshot();

    // Snapshot should see initial data
    EXPECT_TRUE(snapshot->Contains(ToByteVector("key1")));
    EXPECT_TRUE(snapshot->Contains(ToByteVector("key2")));

    // Modify through snapshot
    snapshot->Put(ToByteVector("key3"), ToByteVector("value3"));
    snapshot->Delete(ToByteVector("key1"));

    // Store should not see changes yet
    EXPECT_TRUE(store->Contains(ToByteVector("key1")));
    EXPECT_FALSE(store->Contains(ToByteVector("key3")));

    // Commit snapshot
    snapshot->Commit();

    // Now store should see changes
    EXPECT_FALSE(store->Contains(ToByteVector("key1")));
    EXPECT_TRUE(store->Contains(ToByteVector("key3")));
}

TEST_F(MemoryStoreTest, MemoryStoreProvider)
{
    MemoryStoreProvider provider;

    EXPECT_EQ("Memory", provider.GetName());

    // Get store
    auto store1 = provider.GetStore("path1");
    ASSERT_NE(nullptr, store1);

    // Put data in store1
    store1->Put(ToByteVector("key"), ToByteVector("value"));

    // Get same store again - should have same data
    auto store2 = provider.GetStore("path1");
    ASSERT_NE(nullptr, store2);
    EXPECT_TRUE(store2->Contains(ToByteVector("key")));

    // Get different store - should be empty
    auto store3 = provider.GetStore("path2");
    ASSERT_NE(nullptr, store3);
    EXPECT_FALSE(store3->Contains(ToByteVector("key")));
}
