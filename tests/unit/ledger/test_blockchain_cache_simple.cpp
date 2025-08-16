#include <gtest/gtest.h>
#include <neo/ledger/blockchain_cache.h>
#include <neo/io/uint256.h>
#include <memory>
#include <thread>
#include <chrono>

using namespace neo::ledger;
using namespace neo::io;

// Simple test structures that match the template types
struct TestBlock {
    UInt256 hash;
    uint32_t index;
    
    TestBlock() = default;
    TestBlock(uint32_t idx) : index(idx) {
        // Generate unique hash based on index
        std::memset(hash.Data(), 0, 32);
        *reinterpret_cast<uint32_t*>(hash.Data()) = index;
    }
    
    UInt256 GetHash() const { return hash; }
    uint32_t GetIndex() const { return index; }
};

struct TestTransaction {
    UInt256 hash;
    
    TestTransaction() = default;
    TestTransaction(uint32_t id) {
        // Generate unique hash based on id
        std::memset(hash.Data(), 0, 32);
        *reinterpret_cast<uint32_t*>(hash.Data()) = id | 0x80000000;
    }
    
    UInt256 GetHash() const { return hash; }
};

class SimpleCacheTest : public ::testing::Test {
protected:
    std::unique_ptr<LRUCache<UInt256, TestBlock>> block_cache;
    std::unique_ptr<LRUCache<UInt256, TestTransaction>> tx_cache;
    
    void SetUp() override {
        block_cache = std::make_unique<LRUCache<UInt256, TestBlock>>(100);
        tx_cache = std::make_unique<LRUCache<UInt256, TestTransaction>>(1000);
    }
};

TEST_F(SimpleCacheTest, BasicBlockCaching) {
    TestBlock block(1000);
    auto block_ptr = std::make_shared<TestBlock>(block);
    UInt256 hash = block.GetHash();
    
    // Cache miss initially
    auto retrieved = block_cache->Get(hash);
    EXPECT_FALSE(retrieved);
    
    // Add to cache
    block_cache->Put(hash, block_ptr);
    
    // Cache hit
    retrieved = block_cache->Get(hash);
    ASSERT_TRUE(retrieved);
    EXPECT_EQ(retrieved->GetIndex(), 1000);
    
    // Verify stats
    auto stats = block_cache->GetStats();
    EXPECT_EQ(stats.hits, 1);
    EXPECT_EQ(stats.misses, 1);
    EXPECT_EQ(stats.size, 1);
}

TEST_F(SimpleCacheTest, BasicTransactionCaching) {
    TestTransaction tx(5000);
    auto tx_ptr = std::make_shared<TestTransaction>(tx);
    UInt256 hash = tx.GetHash();
    
    // Cache miss initially
    auto retrieved = tx_cache->Get(hash);
    EXPECT_FALSE(retrieved);
    
    // Add to cache
    tx_cache->Put(hash, tx_ptr);
    
    // Cache hit
    retrieved = tx_cache->Get(hash);
    ASSERT_TRUE(retrieved);
    EXPECT_EQ(retrieved->GetHash(), hash);
    
    // Verify stats
    auto stats = tx_cache->GetStats();
    EXPECT_EQ(stats.hits, 1);
    EXPECT_EQ(stats.misses, 1);
    EXPECT_EQ(stats.size, 1);
}

TEST_F(SimpleCacheTest, LRUEviction) {
    const size_t capacity = 100;
    
    // Fill cache beyond capacity
    for (uint32_t i = 0; i < capacity + 10; ++i) {
        TestBlock block(i);
        auto block_ptr = std::make_shared<TestBlock>(block);
        block_cache->Put(block.GetHash(), block_ptr);
    }
    
    // Cache should not exceed capacity
    auto stats = block_cache->GetStats();
    EXPECT_LE(stats.size, capacity);
    
    // Oldest blocks should be evicted
    TestBlock oldest(0);
    auto retrieved = block_cache->Get(oldest.GetHash());
    EXPECT_FALSE(retrieved);  // Should be evicted
    
    // Recent blocks should still be in cache
    TestBlock recent(capacity + 5);
    retrieved = block_cache->Get(recent.GetHash());
    EXPECT_TRUE(retrieved);
}

TEST_F(SimpleCacheTest, CacheHitRate) {
    // Add some blocks
    for (uint32_t i = 0; i < 10; ++i) {
        TestBlock block(i);
        auto block_ptr = std::make_shared<TestBlock>(block);
        block_cache->Put(block.GetHash(), block_ptr);
    }
    
    // Perform lookups with 80% hit pattern
    int hits = 0;
    int misses = 0;
    
    for (int i = 0; i < 100; ++i) {
        if (i % 5 < 4) {  // 80% of the time
            // Look for existing block
            TestBlock block(i % 10);
            if (block_cache->Get(block.GetHash())) {
                hits++;
            }
        } else {
            // Look for non-existent block
            TestBlock block(100 + i);
            if (!block_cache->Get(block.GetHash())) {
                misses++;
            }
        }
    }
    
    auto stats = block_cache->GetStats();
    double hit_rate = (double)stats.hits / (stats.hits + stats.misses);
    EXPECT_GT(hit_rate, 0.7);  // Should have at least 70% hit rate
}

TEST_F(SimpleCacheTest, ConcurrentAccess) {
    const int num_threads = 10;
    const int ops_per_thread = 100;
    std::atomic<int> successful_ops(0);
    
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, &successful_ops]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                uint32_t index = (t * ops_per_thread + i) % 50;
                TestBlock block(index);
                auto block_ptr = std::make_shared<TestBlock>(block);
                
                // Alternate between put and get
                if (i % 2 == 0) {
                    block_cache->Put(block.GetHash(), block_ptr);
                    successful_ops++;
                } else {
                    if (block_cache->Get(block.GetHash())) {
                        successful_ops++;
                    }
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Should have completed many operations successfully (at least 40%)
    EXPECT_GE(successful_ops.load(), num_threads * ops_per_thread * 4 / 10);
}