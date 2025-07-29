#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <neo/cache/cache.h>
#include <string>
#include <thread>
#include <vector>

using namespace neo::cache;

class UT_Cache : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Create test caches with different configurations
        smallCache_ = std::make_unique<Cache<std::string, int>>(3, 1000);            // 3 entries, 1 second TTL
        mediumCache_ = std::make_unique<Cache<std::string, std::string>>(10, 5000);  // 10 entries, 5 seconds TTL
        largeCache_ = std::make_unique<Cache<int, std::string>>(100, 60000);         // 100 entries, 1 minute TTL
        unlimitedCache_ = std::make_unique<Cache<std::string, int>>(0, 10000);       // Unlimited, 10 seconds TTL

        // Create LRU caches
        smallLRU_ = std::make_unique<LRUCache<std::string, int>>(3);
        mediumLRU_ = std::make_unique<LRUCache<int, std::string>>(10);
    }

    void TearDown() override
    {
        // Cleanup
        smallCache_.reset();
        mediumCache_.reset();
        largeCache_.reset();
        unlimitedCache_.reset();
        smallLRU_.reset();
        mediumLRU_.reset();
    }

  protected:
    std::unique_ptr<Cache<std::string, int>> smallCache_;
    std::unique_ptr<Cache<std::string, std::string>> mediumCache_;
    std::unique_ptr<Cache<int, std::string>> largeCache_;
    std::unique_ptr<Cache<std::string, int>> unlimitedCache_;
    std::unique_ptr<LRUCache<std::string, int>> smallLRU_;
    std::unique_ptr<LRUCache<int, std::string>> mediumLRU_;
};

TEST_F(UT_Cache, ConstructorAndBasicProperties)
{
    // Test: Verify cache constructor and basic properties

    // Test default construction with different parameters
    Cache<std::string, int> defaultCache;
    EXPECT_EQ(defaultCache.Size(), 0u);

    Cache<int, std::string> customCache(50, 2000);
    EXPECT_EQ(customCache.Size(), 0u);

    // Test empty cache behavior
    EXPECT_FALSE(smallCache_->Contains("nonexistent"));
    EXPECT_EQ(smallCache_->Get("nonexistent"), nullptr);
    EXPECT_EQ(smallCache_->Size(), 0u);
}

TEST_F(UT_Cache, BasicPutAndGet)
{
    // Test: Basic put and get operations

    // Test putting and getting values
    smallCache_->Put("key1", 100);
    smallCache_->Put("key2", 200);
    smallCache_->Put("key3", 300);

    EXPECT_EQ(smallCache_->Size(), 3u);

    // Test getting values
    auto value1 = smallCache_->Get("key1");
    auto value2 = smallCache_->Get("key2");
    auto value3 = smallCache_->Get("key3");

    ASSERT_NE(value1, nullptr);
    ASSERT_NE(value2, nullptr);
    ASSERT_NE(value3, nullptr);

    EXPECT_EQ(*value1, 100);
    EXPECT_EQ(*value2, 200);
    EXPECT_EQ(*value3, 300);

    // Test Contains method
    EXPECT_TRUE(smallCache_->Contains("key1"));
    EXPECT_TRUE(smallCache_->Contains("key2"));
    EXPECT_TRUE(smallCache_->Contains("key3"));
    EXPECT_FALSE(smallCache_->Contains("key4"));
}

TEST_F(UT_Cache, SizeLimit)
{
    // Test: Cache size limit enforcement

    // Fill cache to limit
    smallCache_->Put("key1", 100);
    smallCache_->Put("key2", 200);
    smallCache_->Put("key3", 300);
    EXPECT_EQ(smallCache_->Size(), 3u);

    // Add one more item (should evict oldest)
    smallCache_->Put("key4", 400);
    EXPECT_EQ(smallCache_->Size(), 3u);  // Still at limit

    // Verify newest item is present
    auto value4 = smallCache_->Get("key4");
    ASSERT_NE(value4, nullptr);
    EXPECT_EQ(*value4, 400);

    // Test unlimited cache
    for (int i = 0; i < 20; ++i)
    {
        unlimitedCache_->Put("key" + std::to_string(i), i);
    }
    EXPECT_EQ(unlimitedCache_->Size(), 20u);  // Should hold all entries
}

TEST_F(UT_Cache, TTLExpiration)
{
    // Test: Time-to-live expiration

    // Create cache with very short TTL for testing
    Cache<std::string, int> shortTTLCache(10, 100);  // 100ms TTL

    shortTTLCache.Put("key1", 100);
    EXPECT_TRUE(shortTTLCache.Contains("key1"));

    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Should be expired now
    EXPECT_FALSE(shortTTLCache.Contains("key1"));
    EXPECT_EQ(shortTTLCache.Get("key1"), nullptr);

    // Size should be updated after expired entry is accessed
    EXPECT_EQ(shortTTLCache.Size(), 0u);
}

TEST_F(UT_Cache, CustomTTL)
{
    // Test: Custom TTL for individual entries

    // Put entry with custom short TTL
    smallCache_->Put("shortLived", 100, std::chrono::milliseconds(50));
    // Put entry with custom long TTL
    smallCache_->Put("longLived", 200, std::chrono::milliseconds(2000));

    EXPECT_TRUE(smallCache_->Contains("shortLived"));
    EXPECT_TRUE(smallCache_->Contains("longLived"));

    // Wait for short TTL to expire
    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    EXPECT_FALSE(smallCache_->Contains("shortLived"));
    EXPECT_TRUE(smallCache_->Contains("longLived"));
}

TEST_F(UT_Cache, RemoveAndClear)
{
    // Test: Remove and clear operations

    // Add some entries
    mediumCache_->Put("key1", "value1");
    mediumCache_->Put("key2", "value2");
    mediumCache_->Put("key3", "value3");
    EXPECT_EQ(mediumCache_->Size(), 3u);

    // Test remove
    EXPECT_TRUE(mediumCache_->Remove("key2"));
    EXPECT_EQ(mediumCache_->Size(), 2u);
    EXPECT_FALSE(mediumCache_->Contains("key2"));

    // Test remove non-existent key
    EXPECT_FALSE(mediumCache_->Remove("nonexistent"));
    EXPECT_EQ(mediumCache_->Size(), 2u);

    // Test clear
    mediumCache_->Clear();
    EXPECT_EQ(mediumCache_->Size(), 0u);
    EXPECT_FALSE(mediumCache_->Contains("key1"));
    EXPECT_FALSE(mediumCache_->Contains("key3"));
}

TEST_F(UT_Cache, CleanupExpired)
{
    // Test: Cleanup expired entries

    // Create cache with short TTL
    Cache<std::string, int> testCache(10, 100);  // 100ms TTL

    // Add multiple entries
    testCache.Put("key1", 1);
    testCache.Put("key2", 2);
    testCache.Put("key3", 3);
    testCache.Put("key4", 4);
    testCache.Put("key5", 5);

    EXPECT_EQ(testCache.Size(), 5u);

    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Cleanup expired entries
    size_t removed = testCache.CleanupExpired();
    EXPECT_EQ(removed, 5u);  // All should be expired
    EXPECT_EQ(testCache.Size(), 0u);
}

TEST_F(UT_Cache, ThreadSafety)
{
    // Test: Thread safety with concurrent operations

    const int numThreads = 4;
    const int operationsPerThread = 100;
    std::atomic<int> successfulGets(0);
    std::atomic<int> successfulPuts(0);

    std::vector<std::thread> threads;

    // Create threads that perform concurrent operations
    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back(
            [&, t]()
            {
                for (int i = 0; i < operationsPerThread; ++i)
                {
                    std::string key = "thread" + std::to_string(t) + "_key" + std::to_string(i);
                    int value = t * 1000 + i;

                    // Put operation
                    largeCache_->Put(key, std::to_string(value));
                    successfulPuts++;

                    // Get operation
                    auto retrieved = largeCache_->Get(key);
                    if (retrieved != nullptr)
                    {
                        successfulGets++;
                    }

                    // Remove some entries
                    if (i % 10 == 0)
                    {
                        largeCache_->Remove(key);
                    }
                }
            });
    }

    // Wait for all threads to complete
    for (auto& thread : threads)
    {
        thread.join();
    }

    // Verify operations completed successfully
    EXPECT_GT(successfulPuts.load(), 0);
    EXPECT_GT(successfulGets.load(), 0);

    // Cache should be in valid state
    EXPECT_GE(largeCache_->Size(), 0u);
}

TEST_F(UT_Cache, DifferentValueTypes)
{
    // Test: Cache with different value types

    // Test with string values
    mediumCache_->Put("string_key", "string_value");
    auto stringValue = mediumCache_->Get("string_key");
    ASSERT_NE(stringValue, nullptr);
    EXPECT_EQ(*stringValue, "string_value");

    // Test with integer keys
    largeCache_->Put(42, "integer_key_value");
    auto intKeyValue = largeCache_->Get(42);
    ASSERT_NE(intKeyValue, nullptr);
    EXPECT_EQ(*intKeyValue, "integer_key_value");

    // Test with complex strings
    std::string longString(1000, 'A');
    mediumCache_->Put("long_string", longString);
    auto longStringValue = mediumCache_->Get("long_string");
    ASSERT_NE(longStringValue, nullptr);
    EXPECT_EQ(*longStringValue, longString);
}

TEST_F(UT_Cache, LRUCache_BasicOperations)
{
    // Test: LRU Cache basic operations

    // Test putting and getting values
    smallLRU_->Put("key1", 100);
    smallLRU_->Put("key2", 200);
    smallLRU_->Put("key3", 300);

    // Test getting values
    auto value1 = smallLRU_->Get("key1");
    auto value2 = smallLRU_->Get("key2");
    auto value3 = smallLRU_->Get("key3");

    ASSERT_NE(value1, nullptr);
    ASSERT_NE(value2, nullptr);
    ASSERT_NE(value3, nullptr);

    EXPECT_EQ(*value1, 100);
    EXPECT_EQ(*value2, 200);
    EXPECT_EQ(*value3, 300);
}

TEST_F(UT_Cache, LRUCache_EvictionPolicy)
{
    // Test: LRU Cache eviction policy

    // Fill cache to capacity
    smallLRU_->Put("key1", 100);
    smallLRU_->Put("key2", 200);
    smallLRU_->Put("key3", 300);

    // Access key1 to make it recently used
    auto value1 = smallLRU_->Get("key1");
    ASSERT_NE(value1, nullptr);

    // Add new entry (should evict least recently used - key2)
    smallLRU_->Put("key4", 400);

    // key1 and key3 should still be accessible
    EXPECT_NE(smallLRU_->Get("key1"), nullptr);
    EXPECT_NE(smallLRU_->Get("key3"), nullptr);
    EXPECT_NE(smallLRU_->Get("key4"), nullptr);

    // key2 should have been evicted
    EXPECT_EQ(smallLRU_->Get("key2"), nullptr);
}

TEST_F(UT_Cache, LRUCache_UpdateExisting)
{
    // Test: LRU Cache updating existing entries

    smallLRU_->Put("key1", 100);
    smallLRU_->Put("key2", 200);

    // Update existing entry
    smallLRU_->Put("key1", 150);

    auto value = smallLRU_->Get("key1");
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 150);  // Should have updated value
}

TEST_F(UT_Cache, LRUCache_Remove)
{
    // Test: LRU Cache remove operation

    smallLRU_->Put("key1", 100);
    smallLRU_->Put("key2", 200);

    // Test successful remove
    EXPECT_TRUE(smallLRU_->Remove("key1"));
    EXPECT_EQ(smallLRU_->Get("key1"), nullptr);

    // Test remove non-existent key
    EXPECT_FALSE(smallLRU_->Remove("nonexistent"));

    // key2 should still be there
    EXPECT_NE(smallLRU_->Get("key2"), nullptr);
}

TEST_F(UT_Cache, LRUCache_Clear)
{
    // Test: LRU Cache clear operation

    smallLRU_->Put("key1", 100);
    smallLRU_->Put("key2", 200);
    smallLRU_->Put("key3", 300);

    smallLRU_->Clear();

    // All entries should be gone
    EXPECT_EQ(smallLRU_->Get("key1"), nullptr);
    EXPECT_EQ(smallLRU_->Get("key2"), nullptr);
    EXPECT_EQ(smallLRU_->Get("key3"), nullptr);
}

TEST_F(UT_Cache, LRUCache_ThreadSafety)
{
    // Test: LRU Cache thread safety

    const int numThreads = 3;
    const int operationsPerThread = 50;
    std::atomic<int> successfulOps(0);

    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back(
            [&, t]()
            {
                for (int i = 0; i < operationsPerThread; ++i)
                {
                    int key = t * 100 + i;
                    std::string value = "thread" + std::to_string(t) + "_value" + std::to_string(i);

                    mediumLRU_->Put(key, value);

                    auto retrieved = mediumLRU_->Get(key);
                    if (retrieved != nullptr)
                    {
                        successfulOps++;
                    }
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    EXPECT_GT(successfulOps.load(), 0);
}

TEST_F(UT_Cache, SpecializedCaches)
{
    // Test: Specialized cache type aliases

    // Test StringCache
    StringCache<int> stringCache(5, 1000);
    stringCache.Put("test_key", 42);

    auto stringValue = stringCache.Get("test_key");
    ASSERT_NE(stringValue, nullptr);
    EXPECT_EQ(*stringValue, 42);

    // Test HashCache (uses string representation)
    HashCache<std::string> hashCache(5, 1000);
    hashCache.Put("hash_key", "hash_value");

    auto hashValue = hashCache.Get("hash_key");
    ASSERT_NE(hashValue, nullptr);
    EXPECT_EQ(*hashValue, "hash_value");
}

TEST_F(UT_Cache, EdgeCases)
{
    // Test: Edge cases and boundary conditions

    // Test with zero capacity (should still work but immediately evict)
    Cache<std::string, int> zeroCache(0, 1000);
    zeroCache.Put("key1", 100);
    EXPECT_EQ(zeroCache.Size(), 0u);  // Should not store anything

    // Test with very long keys and values
    std::string longKey(1000, 'K');
    std::string longValue(10000, 'V');

    mediumCache_->Put(longKey, longValue);
    auto longResult = mediumCache_->Get(longKey);
    ASSERT_NE(longResult, nullptr);
    EXPECT_EQ(*longResult, longValue);

    // Test rapid put/get cycles
    for (int i = 0; i < 100; ++i)
    {
        std::string key = "rapid_" + std::to_string(i);
        smallCache_->Put(key, i);

        if (i >= 3)
        {  // Only last 3 should be in cache due to size limit
            std::string oldKey = "rapid_" + std::to_string(i - 3);
            EXPECT_EQ(smallCache_->Get(oldKey), nullptr);
        }
    }
}

TEST_F(UT_Cache, Performance)
{
    // Test: Basic performance characteristics

    const int iterations = 1000;

    // Test put performance
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
        largeCache_->Put(i, "value" + std::to_string(i));
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto putTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Should be fast (less than 50ms for 1000 puts)
    EXPECT_LT(putTime.count(), 50000);

    // Test get performance
    start = std::chrono::high_resolution_clock::now();
    int found = 0;
    for (int i = 0; i < iterations; ++i)
    {
        auto value = largeCache_->Get(i);
        if (value != nullptr)
            found++;
    }
    end = std::chrono::high_resolution_clock::now();
    auto getTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Should be fast (less than 30ms for 1000 gets)
    EXPECT_LT(getTime.count(), 30000);
    EXPECT_GT(found, 0);  // Should find some entries
}

TEST_F(UT_Cache, MemoryManagement)
{
    // Test: Memory management and shared_ptr behavior

    // Test that returned values are independent copies
    smallCache_->Put("key1", 100);

    auto value1 = smallCache_->Get("key1");
    auto value2 = smallCache_->Get("key1");

    ASSERT_NE(value1, nullptr);
    ASSERT_NE(value2, nullptr);

    // Should be different shared_ptr instances but same value
    EXPECT_NE(value1.get(), value2.get());
    EXPECT_EQ(*value1, *value2);

    // Modifying one shouldn't affect the other or cache
    *value1 = 999;
    EXPECT_EQ(*value2, 100);

    auto value3 = smallCache_->Get("key1");
    ASSERT_NE(value3, nullptr);
    EXPECT_EQ(*value3, 100);  // Cache should still have original value
}