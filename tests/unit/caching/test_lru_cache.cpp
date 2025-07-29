#include "neo/io/caching/lru_cache.h"
#include <functional>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>

using namespace neo::io::caching;

// Demo LRU Cache implementation for testing (matching C# test structure)
class DemoLRUCache
{
  public:
    explicit DemoLRUCache(size_t max_capacity) : cache_(max_capacity) {}

    void Add(const std::string& item)
    {
        int key = GetKeyForItem(item);
        cache_.Add(key, item);
    }

    bool Contains(const std::string& item)
    {
        int key = GetKeyForItem(item);
        std::string value;
        return cache_.TryGet(key, value);
    }

    bool TryGet(int key, std::string& value)
    {
        return cache_.TryGet(key, value);
    }

    void Clear()
    {
        cache_.Clear();
    }

    size_t GetCount() const
    {
        return cache_.Size();
    }

    int GetKeyForItem(const std::string& item)
    {
        // Use string hash as key (matching C# GetHashCode behavior)
        return static_cast<int>(std::hash<std::string>{}(item));
    }

  private:
    LRUCache<int, std::string> cache_;
};

class LRUCacheTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Initialize with capacity of 3 (same as C# test)
        cache_ = std::make_unique<DemoLRUCache>(3);
    }

    std::unique_ptr<DemoLRUCache> cache_;
};

TEST_F(LRUCacheTest, BasicOperations)
{
    // Test initial state
    EXPECT_EQ(cache_->GetCount(), 0);
    EXPECT_FALSE(cache_->Contains("1"));

    // Add first item
    cache_->Add("1");
    EXPECT_EQ(cache_->GetCount(), 1);
    EXPECT_TRUE(cache_->Contains("1"));

    // Add second item
    cache_->Add("2");
    EXPECT_EQ(cache_->GetCount(), 2);
    EXPECT_TRUE(cache_->Contains("1"));
    EXPECT_TRUE(cache_->Contains("2"));

    // Add third item (at capacity)
    cache_->Add("3");
    EXPECT_EQ(cache_->GetCount(), 3);
    EXPECT_TRUE(cache_->Contains("1"));
    EXPECT_TRUE(cache_->Contains("2"));
    EXPECT_TRUE(cache_->Contains("3"));
}

TEST_F(LRUCacheTest, LRUEviction)
{
    // Fill cache to capacity
    cache_->Add("1");
    cache_->Add("2");
    cache_->Add("3");
    EXPECT_EQ(cache_->GetCount(), 3);

    // Add fourth item - should evict least recently used ("1")
    cache_->Add("4");
    EXPECT_EQ(cache_->GetCount(), 3);
    EXPECT_FALSE(cache_->Contains("1"));  // Evicted
    EXPECT_TRUE(cache_->Contains("2"));
    EXPECT_TRUE(cache_->Contains("3"));
    EXPECT_TRUE(cache_->Contains("4"));
}

TEST_F(LRUCacheTest, AccessUpdatesLRUOrder)
{
    // Fill cache
    cache_->Add("1");
    cache_->Add("2");
    cache_->Add("3");

    // Access "1" to make it most recently used
    std::string value;
    EXPECT_TRUE(cache_->TryGet(cache_->GetKeyForItem("1"), value));
    EXPECT_EQ(value, "1");

    // Add new item - should evict "2" (now least recently used)
    cache_->Add("4");
    EXPECT_EQ(cache_->GetCount(), 3);
    EXPECT_TRUE(cache_->Contains("1"));   // Should still be here (recently accessed)
    EXPECT_FALSE(cache_->Contains("2"));  // Should be evicted
    EXPECT_TRUE(cache_->Contains("3"));
    EXPECT_TRUE(cache_->Contains("4"));
}

TEST_F(LRUCacheTest, MultipleAccessPattern)
{
    // Fill cache
    cache_->Add("1");
    cache_->Add("2");
    cache_->Add("3");

    // Access "2" to move it to front
    std::string value;
    EXPECT_TRUE(cache_->TryGet(cache_->GetKeyForItem("2"), value));
    EXPECT_EQ(value, "2");

    // Add "4" - should evict "1" (least recently used)
    cache_->Add("4");
    EXPECT_FALSE(cache_->Contains("1"));
    EXPECT_TRUE(cache_->Contains("2"));
    EXPECT_TRUE(cache_->Contains("3"));
    EXPECT_TRUE(cache_->Contains("4"));

    // Access "3" to move it to front
    EXPECT_TRUE(cache_->TryGet(cache_->GetKeyForItem("3"), value));
    EXPECT_EQ(value, "3");

    // Add "5" - should evict "2" (now least recently used)
    cache_->Add("5");
    EXPECT_FALSE(cache_->Contains("2"));
    EXPECT_TRUE(cache_->Contains("3"));
    EXPECT_TRUE(cache_->Contains("4"));
    EXPECT_TRUE(cache_->Contains("5"));
}

TEST_F(LRUCacheTest, DuplicateAddition)
{
    cache_->Add("1");
    cache_->Add("2");
    cache_->Add("3");

    // Add duplicate item - should update, not create new entry
    cache_->Add("1");
    EXPECT_EQ(cache_->GetCount(), 3);
    EXPECT_TRUE(cache_->Contains("1"));
    EXPECT_TRUE(cache_->Contains("2"));
    EXPECT_TRUE(cache_->Contains("3"));
}

TEST_F(LRUCacheTest, TryGetNonExistentItem)
{
    cache_->Add("1");

    std::string value;
    EXPECT_FALSE(cache_->TryGet(999, value));  // Non-existent key
    EXPECT_TRUE(value.empty());                // Value should remain unchanged
}

TEST_F(LRUCacheTest, Clear)
{
    cache_->Add("1");
    cache_->Add("2");
    cache_->Add("3");
    EXPECT_EQ(cache_->GetCount(), 3);

    cache_->Clear();
    EXPECT_EQ(cache_->GetCount(), 0);
    EXPECT_FALSE(cache_->Contains("1"));
    EXPECT_FALSE(cache_->Contains("2"));
    EXPECT_FALSE(cache_->Contains("3"));
}

TEST_F(LRUCacheTest, ZeroCapacity)
{
    auto zero_cache = std::make_unique<DemoLRUCache>(0);

    zero_cache->Add("1");
    EXPECT_EQ(zero_cache->GetCount(), 0);
    EXPECT_FALSE(zero_cache->Contains("1"));
}

TEST_F(LRUCacheTest, SingleItemCapacity)
{
    auto single_cache = std::make_unique<DemoLRUCache>(1);

    single_cache->Add("1");
    EXPECT_EQ(single_cache->GetCount(), 1);
    EXPECT_TRUE(single_cache->Contains("1"));

    single_cache->Add("2");
    EXPECT_EQ(single_cache->GetCount(), 1);
    EXPECT_FALSE(single_cache->Contains("1"));
    EXPECT_TRUE(single_cache->Contains("2"));
}

TEST_F(LRUCacheTest, LargeCapacity)
{
    auto large_cache = std::make_unique<DemoLRUCache>(1000);

    // Add many items
    for (int i = 0; i < 500; ++i)
    {
        large_cache->Add(std::to_string(i));
    }

    EXPECT_EQ(large_cache->GetCount(), 500);

    // All items should still be present
    for (int i = 0; i < 500; ++i)
    {
        EXPECT_TRUE(large_cache->Contains(std::to_string(i)));
    }
}

TEST_F(LRUCacheTest, AccessFrequencyPattern)
{
    cache_->Add("frequent");
    cache_->Add("medium");
    cache_->Add("rare");

    // Access "frequent" multiple times
    std::string value;
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_TRUE(cache_->TryGet(cache_->GetKeyForItem("frequent"), value));
    }

    // Access "medium" once
    EXPECT_TRUE(cache_->TryGet(cache_->GetKeyForItem("medium"), value));

    // Don't access "rare"

    // Add new items - "rare" should be evicted first
    cache_->Add("new1");
    EXPECT_FALSE(cache_->Contains("rare"));
    EXPECT_TRUE(cache_->Contains("frequent"));
    EXPECT_TRUE(cache_->Contains("medium"));
    EXPECT_TRUE(cache_->Contains("new1"));
}

TEST_F(LRUCacheTest, ThreadSafety)
{
    // Basic thread safety test (more comprehensive tests would use multiple threads)
    std::vector<std::thread> threads;

    for (int t = 0; t < 4; ++t)
    {
        threads.emplace_back(
            [this, t]()
            {
                for (int i = 0; i < 10; ++i)
                {
                    cache_->Add(std::to_string(t * 10 + i));

                    std::string value;
                    cache_->TryGet(cache_->GetKeyForItem(std::to_string(t * 10 + i)), value);
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // Cache should maintain consistency
    EXPECT_LE(cache_->GetCount(), 3);  // Should not exceed capacity
}

TEST_F(LRUCacheTest, MemoryManagement)
{
    // Test that items are properly destroyed when evicted
    struct CountedString
    {
        static int instance_count;
        std::string value;

        CountedString(const std::string& v) : value(v)
        {
            ++instance_count;
        }
        CountedString(const CountedString& other) : value(other.value)
        {
            ++instance_count;
        }
        ~CountedString()
        {
            --instance_count;
        }

        operator std::string() const
        {
            return value;
        }
    };

    class CountedCache : public LRUCache<int, CountedString>
    {
      public:
        explicit CountedCache(size_t capacity) : LRUCache<int, CountedString>(capacity) {}

      protected:
        int GetKeyForItem(const CountedString& item) override
        {
            return static_cast<int>(std::hash<std::string>{}(item.value));
        }
    };

    CountedString::instance_count = 0;

    {
        CountedCache counted_cache(2);
        counted_cache.Add(CountedString("1"));
        counted_cache.Add(CountedString("2"));
        EXPECT_EQ(CountedString::instance_count, 2);

        counted_cache.Add(CountedString("3"));        // Should evict "1"
        EXPECT_LE(CountedString::instance_count, 2);  // "1" should be destroyed
    }

    // All items should be destroyed when cache is destroyed
    EXPECT_EQ(CountedString::instance_count, 0);
}

// Static member definition
int LRUCacheTest::CountedString::instance_count = 0;