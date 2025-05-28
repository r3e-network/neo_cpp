#include <gtest/gtest.h>
#include <neo/io/lru_cache.h>
#include <string>

namespace neo::io::tests
{
    class CachingTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // Test setup
        }
    };

    TEST_F(CachingTest, TestLRUCacheBasicOperations)
    {
        LRUCache<int, std::string> cache(3);
        
        // Test insertion
        cache.Put(1, "one");
        cache.Put(2, "two");
        cache.Put(3, "three");
        
        EXPECT_EQ(3, cache.Size());
        
        // Test retrieval
        auto value1 = cache.Get(1);
        EXPECT_TRUE(value1.has_value());
        EXPECT_EQ("one", value1.value());
        
        auto value2 = cache.Get(2);
        EXPECT_TRUE(value2.has_value());
        EXPECT_EQ("two", value2.value());
        
        auto value3 = cache.Get(3);
        EXPECT_TRUE(value3.has_value());
        EXPECT_EQ("three", value3.value());
        
        // Test non-existent key
        auto value4 = cache.Get(4);
        EXPECT_FALSE(value4.has_value());
    }

    TEST_F(CachingTest, TestLRUCacheEviction)
    {
        LRUCache<int, std::string> cache(2);
        
        // Fill cache to capacity
        cache.Put(1, "one");
        cache.Put(2, "two");
        EXPECT_EQ(2, cache.Size());
        
        // Add third item, should evict least recently used
        cache.Put(3, "three");
        EXPECT_EQ(2, cache.Size());
        
        // Key 1 should be evicted
        EXPECT_FALSE(cache.Get(1).has_value());
        EXPECT_TRUE(cache.Get(2).has_value());
        EXPECT_TRUE(cache.Get(3).has_value());
    }

    TEST_F(CachingTest, TestLRUCacheUpdateExisting)
    {
        LRUCache<int, std::string> cache(3);
        
        cache.Put(1, "one");
        cache.Put(2, "two");
        cache.Put(3, "three");
        
        // Update existing key
        cache.Put(2, "TWO");
        
        auto value = cache.Get(2);
        EXPECT_TRUE(value.has_value());
        EXPECT_EQ("TWO", value.value());
        
        EXPECT_EQ(3, cache.Size());
    }

    TEST_F(CachingTest, TestLRUCacheAccessOrder)
    {
        LRUCache<int, std::string> cache(3);
        
        cache.Put(1, "one");
        cache.Put(2, "two");
        cache.Put(3, "three");
        
        // Access key 1 to make it most recently used
        cache.Get(1);
        
        // Add new item, should evict key 2 (least recently used)
        cache.Put(4, "four");
        
        EXPECT_TRUE(cache.Get(1).has_value());  // Should still exist
        EXPECT_FALSE(cache.Get(2).has_value()); // Should be evicted
        EXPECT_TRUE(cache.Get(3).has_value());  // Should still exist
        EXPECT_TRUE(cache.Get(4).has_value());  // Should exist
    }

    TEST_F(CachingTest, TestLRUCacheContains)
    {
        LRUCache<int, std::string> cache(3);
        
        cache.Put(1, "one");
        cache.Put(2, "two");
        
        EXPECT_TRUE(cache.Contains(1));
        EXPECT_TRUE(cache.Contains(2));
        EXPECT_FALSE(cache.Contains(3));
    }

    TEST_F(CachingTest, TestLRUCacheRemove)
    {
        LRUCache<int, std::string> cache(3);
        
        cache.Put(1, "one");
        cache.Put(2, "two");
        cache.Put(3, "three");
        
        EXPECT_EQ(3, cache.Size());
        
        // Remove existing key
        bool removed = cache.Remove(2);
        EXPECT_TRUE(removed);
        EXPECT_EQ(2, cache.Size());
        EXPECT_FALSE(cache.Contains(2));
        
        // Try to remove non-existent key
        bool not_removed = cache.Remove(4);
        EXPECT_FALSE(not_removed);
        EXPECT_EQ(2, cache.Size());
    }

    TEST_F(CachingTest, TestLRUCacheClear)
    {
        LRUCache<int, std::string> cache(3);
        
        cache.Put(1, "one");
        cache.Put(2, "two");
        cache.Put(3, "three");
        
        EXPECT_EQ(3, cache.Size());
        
        cache.Clear();
        
        EXPECT_EQ(0, cache.Size());
        EXPECT_FALSE(cache.Contains(1));
        EXPECT_FALSE(cache.Contains(2));
        EXPECT_FALSE(cache.Contains(3));
    }

    TEST_F(CachingTest, TestLRUCacheZeroCapacity)
    {
        LRUCache<int, std::string> cache(0);
        
        // Should not store anything
        cache.Put(1, "one");
        EXPECT_EQ(0, cache.Size());
        EXPECT_FALSE(cache.Contains(1));
        EXPECT_FALSE(cache.Get(1).has_value());
    }

    TEST_F(CachingTest, TestLRUCacheCapacityOne)
    {
        LRUCache<int, std::string> cache(1);
        
        cache.Put(1, "one");
        EXPECT_EQ(1, cache.Size());
        EXPECT_TRUE(cache.Contains(1));
        
        // Add second item, should evict first
        cache.Put(2, "two");
        EXPECT_EQ(1, cache.Size());
        EXPECT_FALSE(cache.Contains(1));
        EXPECT_TRUE(cache.Contains(2));
    }

    TEST_F(CachingTest, TestLRUCacheStringKeys)
    {
        LRUCache<std::string, int> cache(3);
        
        cache.Put("one", 1);
        cache.Put("two", 2);
        cache.Put("three", 3);
        
        EXPECT_EQ(3, cache.Size());
        
        auto value = cache.Get("two");
        EXPECT_TRUE(value.has_value());
        EXPECT_EQ(2, value.value());
        
        // Test eviction with string keys
        cache.Put("four", 4);
        EXPECT_FALSE(cache.Contains("one")); // Should be evicted
        EXPECT_TRUE(cache.Contains("two"));
        EXPECT_TRUE(cache.Contains("three"));
        EXPECT_TRUE(cache.Contains("four"));
    }

    TEST_F(CachingTest, TestLRUCacheComplexValues)
    {
        struct ComplexValue
        {
            int id;
            std::string name;
            std::vector<int> data;
            
            bool operator==(const ComplexValue& other) const
            {
                return id == other.id && name == other.name && data == other.data;
            }
        };
        
        LRUCache<int, ComplexValue> cache(2);
        
        ComplexValue val1{1, "first", {1, 2, 3}};
        ComplexValue val2{2, "second", {4, 5, 6}};
        
        cache.Put(1, val1);
        cache.Put(2, val2);
        
        auto retrieved1 = cache.Get(1);
        EXPECT_TRUE(retrieved1.has_value());
        EXPECT_EQ(val1, retrieved1.value());
        
        auto retrieved2 = cache.Get(2);
        EXPECT_TRUE(retrieved2.has_value());
        EXPECT_EQ(val2, retrieved2.value());
    }

    TEST_F(CachingTest, TestLRUCacheThreadSafety)
    {
        // Note: This is a basic test. Full thread safety testing would require
        // more sophisticated testing with multiple threads
        LRUCache<int, std::string> cache(100);
        
        // Perform many operations to test for basic consistency
        for (int i = 0; i < 1000; ++i)
        {
            cache.Put(i, "value" + std::to_string(i));
            
            if (i > 50)
            {
                auto value = cache.Get(i - 50);
                // Value may or may not exist due to eviction, but shouldn't crash
            }
        }
        
        // Cache should not exceed capacity
        EXPECT_LE(cache.Size(), 100);
    }

    TEST_F(CachingTest, TestLRUCachePerformance)
    {
        LRUCache<int, std::string> cache(1000);
        
        // Test insertion performance
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < 10000; ++i)
        {
            cache.Put(i, "value" + std::to_string(i));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Should complete in reasonable time (less than 1 second for 10k operations)
        EXPECT_LT(duration.count(), 1000);
        
        // Test retrieval performance
        start = std::chrono::high_resolution_clock::now();
        
        for (int i = 9000; i < 10000; ++i)
        {
            auto value = cache.Get(i);
            EXPECT_TRUE(value.has_value());
        }
        
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Retrieval should be very fast
        EXPECT_LT(duration.count(), 100);
    }
}
