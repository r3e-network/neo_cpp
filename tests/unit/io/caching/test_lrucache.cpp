#include <atomic>
#include <gtest/gtest.h>
#include <neo/io/caching/lru_cache.h>
#include <string>
#include <thread>
#include <vector>

using namespace neo::io::caching;

/**
 * @brief Test fixture for LRUCache
 */
class LRUCacheTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        // No specific setup needed
    }
};

TEST_F(LRUCacheTest, Constructor)
{
    LRUCache<int, std::string> cache(5);
    EXPECT_EQ(5u, cache.Capacity());
    EXPECT_EQ(0u, cache.Size());
}

TEST_F(LRUCacheTest, AddAndGet)
{
    LRUCache<int, std::string> cache(3);

    // Add items
    cache.Add(1, "one");
    cache.Add(2, "two");
    cache.Add(3, "three");

    EXPECT_EQ(3u, cache.Size());

    // Get items
    auto value1 = cache.Get(1);
    ASSERT_TRUE(value1.has_value());
    EXPECT_EQ("one", value1.value());

    auto value2 = cache.Get(2);
    ASSERT_TRUE(value2.has_value());
    EXPECT_EQ("two", value2.value());

    auto value3 = cache.Get(3);
    ASSERT_TRUE(value3.has_value());
    EXPECT_EQ("three", value3.value());

    // Get non-existent item
    auto value4 = cache.Get(4);
    EXPECT_FALSE(value4.has_value());
}

TEST_F(LRUCacheTest, TryGet)
{
    LRUCache<int, std::string> cache(3);

    cache.Add(1, "one");
    cache.Add(2, "two");

    std::string value;

    // Try get existing item
    EXPECT_TRUE(cache.TryGet(1, value));
    EXPECT_EQ("one", value);

    EXPECT_TRUE(cache.TryGet(2, value));
    EXPECT_EQ("two", value);

    // Try get non-existent item
    EXPECT_FALSE(cache.TryGet(3, value));
}

TEST_F(LRUCacheTest, UpdateExistingItem)
{
    LRUCache<int, std::string> cache(3);

    cache.Add(1, "one");
    cache.Add(2, "two");

    // Update existing item
    cache.Add(1, "ONE");

    auto value = cache.Get(1);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ("ONE", value.value());

    EXPECT_EQ(2u, cache.Size());
}

TEST_F(LRUCacheTest, LRUEviction)
{
    LRUCache<int, std::string> cache(3);

    // Fill cache to capacity
    cache.Add(1, "one");
    cache.Add(2, "two");
    cache.Add(3, "three");

    // Add one more item - should evict least recently used (1)
    cache.Add(4, "four");

    EXPECT_EQ(3u, cache.Size());
    EXPECT_FALSE(cache.Get(1).has_value());  // 1 should be evicted
    EXPECT_TRUE(cache.Get(2).has_value());
    EXPECT_TRUE(cache.Get(3).has_value());
    EXPECT_TRUE(cache.Get(4).has_value());
}

TEST_F(LRUCacheTest, LRUEvictionWithAccess)
{
    LRUCache<int, std::string> cache(3);

    // Fill cache
    cache.Add(1, "one");
    cache.Add(2, "two");
    cache.Add(3, "three");

    // Access item 1 to make it recently used
    auto value = cache.Get(1);
    EXPECT_TRUE(value.has_value());

    // Add new item - should evict 2 (now least recently used)
    cache.Add(4, "four");

    EXPECT_EQ(3u, cache.Size());
    EXPECT_TRUE(cache.Get(1).has_value());   // 1 was accessed, so not evicted
    EXPECT_FALSE(cache.Get(2).has_value());  // 2 should be evicted
    EXPECT_TRUE(cache.Get(3).has_value());
    EXPECT_TRUE(cache.Get(4).has_value());
}

TEST_F(LRUCacheTest, Remove)
{
    LRUCache<int, std::string> cache(3);

    cache.Add(1, "one");
    cache.Add(2, "two");
    cache.Add(3, "three");

    // Remove existing item
    EXPECT_TRUE(cache.Remove(2));
    EXPECT_EQ(2u, cache.Size());
    EXPECT_FALSE(cache.Get(2).has_value());

    // Try to remove non-existent item
    EXPECT_FALSE(cache.Remove(4));
    EXPECT_EQ(2u, cache.Size());
}

TEST_F(LRUCacheTest, Clear)
{
    LRUCache<int, std::string> cache(3);

    cache.Add(1, "one");
    cache.Add(2, "two");
    cache.Add(3, "three");

    EXPECT_EQ(3u, cache.Size());

    cache.Clear();

    EXPECT_EQ(0u, cache.Size());
    EXPECT_FALSE(cache.Get(1).has_value());
    EXPECT_FALSE(cache.Get(2).has_value());
    EXPECT_FALSE(cache.Get(3).has_value());
}

TEST_F(LRUCacheTest, ComplexKeyType)
{
    // Test with string keys
    LRUCache<std::string, int> cache(2);

    cache.Add("first", 1);
    cache.Add("second", 2);

    auto value = cache.Get("first");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(1, value.value());

    cache.Add("third", 3);
    EXPECT_FALSE(cache.Get("second").has_value());  // Should be evicted
}

TEST_F(LRUCacheTest, ThreadSafety)
{
    LRUCache<int, int> cache(100);
    const int numThreads = 4;
    const int itemsPerThread = 25;

    std::vector<std::thread> threads;

    // Multiple threads adding items
    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back(
            [&cache, t, itemsPerThread]()
            {
                for (int i = 0; i < itemsPerThread; ++i)
                {
                    int key = t * itemsPerThread + i;
                    cache.Add(key, key * 10);
                }
            });
    }

    // Wait for all threads
    for (auto& thread : threads)
    {
        thread.join();
    }

    // Verify all items are present
    EXPECT_EQ(100u, cache.Size());

    threads.clear();

    // Multiple threads reading items
    std::atomic<int> successfulReads(0);
    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back(
            [&cache, &successfulReads, t, itemsPerThread]()
            {
                for (int i = 0; i < itemsPerThread; ++i)
                {
                    int key = t * itemsPerThread + i;
                    auto value = cache.Get(key);
                    if (value.has_value() && value.value() == key * 10)
                    {
                        successfulReads++;
                    }
                }
            });
    }

    // Wait for all threads
    for (auto& thread : threads)
    {
        thread.join();
    }

    EXPECT_EQ(100, successfulReads.load());
}

TEST_F(LRUCacheTest, CapacityOne)
{
    LRUCache<int, std::string> cache(1);

    cache.Add(1, "one");
    EXPECT_TRUE(cache.Get(1).has_value());

    cache.Add(2, "two");
    EXPECT_FALSE(cache.Get(1).has_value());
    EXPECT_TRUE(cache.Get(2).has_value());

    cache.Add(3, "three");
    EXPECT_FALSE(cache.Get(2).has_value());
    EXPECT_TRUE(cache.Get(3).has_value());
}

TEST_F(LRUCacheTest, RepeatedUpdates)
{
    LRUCache<int, std::string> cache(2);

    // Repeatedly update the same key
    for (int i = 0; i < 10; ++i)
    {
        cache.Add(1, "value" + std::to_string(i));
    }

    EXPECT_EQ(1u, cache.Size());
    auto value = cache.Get(1);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ("value9", value.value());
}

TEST_F(LRUCacheTest, AccessPatternTest)
{
    LRUCache<int, int> cache(3);

    // Add items in order
    cache.Add(1, 100);
    cache.Add(2, 200);
    cache.Add(3, 300);

    // Access in different order
    cache.Get(2);  // LRU order: 1, 3, 2
    cache.Get(1);  // LRU order: 3, 2, 1
    cache.Get(2);  // LRU order: 3, 1, 2

    // Add new item - should evict 3
    cache.Add(4, 400);

    EXPECT_FALSE(cache.Get(3).has_value());
    EXPECT_TRUE(cache.Get(1).has_value());
    EXPECT_TRUE(cache.Get(2).has_value());
    EXPECT_TRUE(cache.Get(4).has_value());
}
