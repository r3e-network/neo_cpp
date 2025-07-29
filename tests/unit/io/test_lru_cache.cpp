#include <chrono>
#include <gtest/gtest.h>
#include <neo/io/lru_cache.h>
#include <string>
#include <thread>
#include <vector>

using namespace neo::io;

TEST(LRUCacheTest, Constructor)
{
    // Default constructor with capacity
    LRUCache<int, std::string> cache(5);
    EXPECT_EQ(cache.Count(), 0);
    EXPECT_EQ(cache.Capacity(), 5);

    // Constructor with zero capacity should throw
    EXPECT_THROW((LRUCache<int, std::string>(0)), std::invalid_argument);
}

TEST(LRUCacheTest, AddAndGet)
{
    LRUCache<int, std::string> cache(3);

    // Add items
    cache.Add(1, "One");
    cache.Add(2, "Two");
    cache.Add(3, "Three");

    // Check count
    EXPECT_EQ(cache.Count(), 3);

    // Get items
    auto item1 = cache.TryGet(1);
    EXPECT_TRUE(item1.has_value());
    EXPECT_EQ(*item1, "One");

    auto item2 = cache.TryGet(2);
    EXPECT_TRUE(item2.has_value());
    EXPECT_EQ(*item2, "Two");

    auto item3 = cache.TryGet(3);
    EXPECT_TRUE(item3.has_value());
    EXPECT_EQ(*item3, "Three");

    // Get non-existent item
    auto item4 = cache.TryGet(4);
    EXPECT_FALSE(item4.has_value());
}

TEST(LRUCacheTest, LRUEviction)
{
    LRUCache<int, std::string> cache(3);

    // Add items
    cache.Add(1, "One");
    cache.Add(2, "Two");
    cache.Add(3, "Three");

    // Add a new item, which should evict the oldest item (1)
    cache.Add(4, "Four");

    // Check count
    EXPECT_EQ(cache.Count(), 3);

    // Check items
    EXPECT_FALSE(cache.TryGet(1).has_value());
    EXPECT_TRUE(cache.TryGet(2).has_value());
    EXPECT_TRUE(cache.TryGet(3).has_value());
    EXPECT_TRUE(cache.TryGet(4).has_value());

    // Access item 2, which should move it to the front
    EXPECT_EQ(*cache.TryGet(2), "Two");

    // Add a new item, which should evict the oldest item (3)
    cache.Add(5, "Five");

    // Check items
    EXPECT_FALSE(cache.TryGet(1).has_value());
    EXPECT_TRUE(cache.TryGet(2).has_value());
    EXPECT_FALSE(cache.TryGet(3).has_value());
    EXPECT_TRUE(cache.TryGet(4).has_value());
    EXPECT_TRUE(cache.TryGet(5).has_value());
}

TEST(LRUCacheTest, UpdateExistingItem)
{
    LRUCache<int, std::string> cache(3);

    // Add items
    cache.Add(1, "One");
    cache.Add(2, "Two");
    cache.Add(3, "Three");

    // Update an existing item
    cache.Add(2, "Two Updated");

    // Check count
    EXPECT_EQ(cache.Count(), 3);

    // Check items
    EXPECT_TRUE(cache.TryGet(1).has_value());
    EXPECT_TRUE(cache.TryGet(2).has_value());
    EXPECT_TRUE(cache.TryGet(3).has_value());

    // Check updated value
    EXPECT_EQ(*cache.TryGet(2), "Two Updated");

    // Add a new item, which should evict the oldest item (1)
    cache.Add(4, "Four");

    // Check items
    EXPECT_FALSE(cache.TryGet(1).has_value());
    EXPECT_TRUE(cache.TryGet(2).has_value());
    EXPECT_TRUE(cache.TryGet(3).has_value());
    EXPECT_TRUE(cache.TryGet(4).has_value());
}

TEST(LRUCacheTest, Remove)
{
    LRUCache<int, std::string> cache(3);

    // Add items
    cache.Add(1, "One");
    cache.Add(2, "Two");
    cache.Add(3, "Three");

    // Remove an item
    bool removed = cache.Remove(2);
    EXPECT_TRUE(removed);

    // Check count
    EXPECT_EQ(cache.Count(), 2);

    // Check items
    EXPECT_TRUE(cache.TryGet(1).has_value());
    EXPECT_FALSE(cache.TryGet(2).has_value());
    EXPECT_TRUE(cache.TryGet(3).has_value());

    // Remove a non-existent item
    removed = cache.Remove(4);
    EXPECT_FALSE(removed);

    // Check count
    EXPECT_EQ(cache.Count(), 2);
}

TEST(LRUCacheTest, Clear)
{
    LRUCache<int, std::string> cache(3);

    // Add items
    cache.Add(1, "One");
    cache.Add(2, "Two");
    cache.Add(3, "Three");

    // Clear the cache
    cache.Clear();

    // Check count
    EXPECT_EQ(cache.Count(), 0);

    // Check items
    EXPECT_FALSE(cache.TryGet(1).has_value());
    EXPECT_FALSE(cache.TryGet(2).has_value());
    EXPECT_FALSE(cache.TryGet(3).has_value());
}

TEST(LRUCacheTest, ThreadSafety)
{
    LRUCache<int, int> cache(100);

    // Add items from multiple threads
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++)
    {
        threads.emplace_back(
            [&cache, i]()
            {
                for (int j = 0; j < 10; j++)
                {
                    int key = i * 10 + j;
                    cache.Add(key, key);

                    // Try to get an item
                    auto value = cache.TryGet(key);
                    if (value.has_value())
                    {
                        EXPECT_EQ(value.value(), key);
                    }

                    // Sleep a bit to increase the chance of race conditions
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            });
    }

    // Wait for all threads to finish
    for (auto& thread : threads)
    {
        thread.join();
    }

    // Check count
    EXPECT_LE(cache.Count(), 100);

    // Check some items
    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            int key = i * 10 + j;
            auto value = cache.TryGet(key);
            if (value.has_value())
            {
                EXPECT_EQ(value.value(), key);
            }
        }
    }
}
