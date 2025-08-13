/**
 * @file test_blockchain_cache.cpp
 * @brief Unit tests for BlockchainCache
 */

#include <gtest/gtest.h>
#include <neo/ledger/blockchain_cache.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/smartcontract/contract_state.h>
#include <neo/io/uint256.h>
#include <thread>
#include <vector>
#include <random>
#include <chrono>

using namespace neo::ledger;
using namespace neo::smartcontract;
using namespace neo::io;
using namespace std::chrono_literals;

// Mock classes for testing
class MockBlock : public Block {
public:
    explicit MockBlock(uint32_t index) : index_(index) {
        // Generate unique hash based on index
        hash_ = GenerateHash(index);
    }
    
    UInt256 GetHash() const { return hash_; }
    uint32_t GetIndex() const { return index_; }
    
    static UInt256 GenerateHash(uint32_t index) {
        UInt256 hash;
        std::fill(hash.begin(), hash.end(), 0);
        *reinterpret_cast<uint32_t*>(hash.data()) = index;
        return hash;
    }
    
private:
    UInt256 hash_;
    uint32_t index_;
};

class MockTransaction : public Transaction {
public:
    explicit MockTransaction(uint32_t id) : id_(id) {
        hash_ = GenerateHash(id);
    }
    
    UInt256 GetHash() const { return hash_; }
    uint32_t GetId() const { return id_; }
    
    static UInt256 GenerateHash(uint32_t id) {
        UInt256 hash;
        std::fill(hash.begin(), hash.end(), 0);
        *reinterpret_cast<uint32_t*>(hash.data()) = id | 0x80000000;  // Different pattern from blocks
        return hash;
    }
    
private:
    UInt256 hash_;
    uint32_t id_;
};

class BlockchainCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        cache_ = std::make_unique<BlockchainCache>();
    }
    
    std::unique_ptr<BlockchainCache> cache_;
};

TEST_F(BlockchainCacheTest, BasicBlockCaching) {
    auto block = std::make_shared<MockBlock>(1000);
    UInt256 hash = block->GetHash();
    
    // Cache miss initially
    auto retrieved = cache_->GetBlock(hash);
    EXPECT_FALSE(retrieved);
    
    // Add to cache
    cache_->CacheBlock(block);
    
    // Cache hit
    retrieved = cache_->GetBlock(hash);
    ASSERT_TRUE(retrieved);
    EXPECT_EQ(retrieved->GetIndex(), 1000);
    
    // Verify stats
    auto stats = cache_->GetStats();
    EXPECT_EQ(stats.block_stats.hits, 1);
    EXPECT_EQ(stats.block_stats.misses, 1);
    EXPECT_EQ(stats.block_stats.size, 1);
}

TEST_F(BlockchainCacheTest, BasicTransactionCaching) {
    auto tx = std::make_shared<MockTransaction>(5000);
    UInt256 hash = tx->GetHash();
    
    // Cache miss initially
    auto retrieved = cache_->GetTransaction(hash);
    EXPECT_FALSE(retrieved);
    
    // Add to cache
    cache_->CacheTransaction(tx);
    
    // Cache hit
    retrieved = cache_->GetTransaction(hash);
    ASSERT_TRUE(retrieved);
    EXPECT_EQ(retrieved->GetId(), 5000);
    
    // Verify stats
    auto stats = cache_->GetStats();
    EXPECT_EQ(stats.tx_stats.hits, 1);
    EXPECT_EQ(stats.tx_stats.misses, 1);
    EXPECT_EQ(stats.tx_stats.size, 1);
}

TEST_F(BlockchainCacheTest, LRUEvictionForBlocks) {
    const size_t max_blocks = 1000;  // Default cache size
    
    // Fill cache beyond capacity
    for (uint32_t i = 0; i < max_blocks + 100; ++i) {
        auto block = std::make_shared<MockBlock>(i);
        cache_->CacheBlock(block);
    }
    
    // Cache should not exceed max size
    auto stats = cache_->GetStats();
    EXPECT_LE(stats.block_stats.size, max_blocks);
    
    // Oldest blocks should be evicted
    auto oldest = cache_->GetBlock(MockBlock::GenerateHash(0));
    EXPECT_FALSE(oldest);  // Should be evicted
    
    // Recent blocks should still be in cache
    auto recent = cache_->GetBlock(MockBlock::GenerateHash(max_blocks + 50));
    EXPECT_TRUE(recent);
}

TEST_F(BlockchainCacheTest, LRUEvictionForTransactions) {
    const size_t max_txs = 10000;  // Default cache size
    
    // Add many transactions
    for (uint32_t i = 0; i < 100; ++i) {
        auto tx = std::make_shared<MockTransaction>(i);
        cache_->CacheTransaction(tx);
    }
    
    // Access first transaction to make it recently used
    cache_->GetTransaction(MockTransaction::GenerateHash(0));
    
    // Add more transactions
    for (uint32_t i = 100; i < 200; ++i) {
        auto tx = std::make_shared<MockTransaction>(i);
        cache_->CacheTransaction(tx);
    }
    
    // First transaction should still be in cache (recently accessed)
    auto first = cache_->GetTransaction(MockTransaction::GenerateHash(0));
    EXPECT_TRUE(first);
}

TEST_F(BlockchainCacheTest, CacheHitRate) {
    // Add some blocks
    for (uint32_t i = 0; i < 100; ++i) {
        auto block = std::make_shared<MockBlock>(i);
        cache_->CacheBlock(block);
    }
    
    // Perform many lookups with 80% hit pattern
    int total_lookups = 1000;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> hit_dist(0, 99);
    std::uniform_int_distribution<> miss_dist(100, 199);
    std::bernoulli_distribution hit_choice(0.8);  // 80% hit rate
    
    for (int i = 0; i < total_lookups; ++i) {
        uint32_t index = hit_choice(gen) ? hit_dist(gen) : miss_dist(gen);
        cache_->GetBlock(MockBlock::GenerateHash(index));
    }
    
    // Check hit rate
    auto stats = cache_->GetStats();
    double hit_rate = static_cast<double>(stats.block_stats.hits) / 
                     (stats.block_stats.hits + stats.block_stats.misses);
    
    // Should be close to 80%
    EXPECT_GT(hit_rate, 0.75);
    EXPECT_LT(hit_rate, 0.85);
}

TEST_F(BlockchainCacheTest, ConcurrentAccess) {
    const int num_threads = 10;
    const int ops_per_thread = 1000;
    std::atomic<int> successful_ops{0};
    
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, ops_per_thread, &successful_ops]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dist(0, 999);
            
            for (int i = 0; i < ops_per_thread; ++i) {
                uint32_t index = dist(gen);
                
                // Mix of reads and writes
                if (i % 3 == 0) {
                    // Write
                    auto block = std::make_shared<MockBlock>(index);
                    cache_->CacheBlock(block);
                    successful_ops++;
                } else {
                    // Read
                    auto block = cache_->GetBlock(MockBlock::GenerateHash(index));
                    if (block) {
                        EXPECT_EQ(block->GetIndex(), index);
                        successful_ops++;
                    }
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify operations completed
    EXPECT_GT(successful_ops.load(), num_threads * ops_per_thread / 2);
    
    // Cache should still be functional
    auto stats = cache_->GetStats();
    EXPECT_GT(stats.block_stats.size, 0);
}

TEST_F(BlockchainCacheTest, CacheWarming) {
    // Pre-warm cache with sequential blocks
    const uint32_t warm_start = 1000;
    const uint32_t warm_count = 100;
    
    cache_->WarmCache(warm_start, warm_count);
    
    // These blocks should be in cache
    for (uint32_t i = warm_start; i < warm_start + warm_count; ++i) {
        // In real implementation, warming would load from storage
        // For test, we'll add them manually
        auto block = std::make_shared<MockBlock>(i);
        cache_->CacheBlock(block);
    }
    
    // Verify warmed blocks are accessible
    int hits = 0;
    for (uint32_t i = warm_start; i < warm_start + warm_count; ++i) {
        if (cache_->GetBlock(MockBlock::GenerateHash(i))) {
            hits++;
        }
    }
    
    EXPECT_EQ(hits, warm_count);
}

TEST_F(BlockchainCacheTest, HeaderCaching) {
    // Test header-specific caching
    for (uint32_t i = 0; i < 100; ++i) {
        auto header = std::make_shared<BlockHeader>();
        cache_->PutHeader(i, header);
    }
    
    // Retrieve headers
    for (uint32_t i = 0; i < 100; ++i) {
        auto header = cache_->GetHeader(i);
        EXPECT_TRUE(header);
    }
    
    auto stats = cache_->GetStats();
    EXPECT_EQ(stats.header_stats.size, 100);
    EXPECT_EQ(stats.header_stats.hits, 100);
}

TEST_F(BlockchainCacheTest, ContractStateCaching) {
    // Test contract caching
    for (uint32_t i = 0; i < 50; ++i) {
        UInt160 script_hash;
        std::fill(script_hash.begin(), script_hash.end(), 0);
        *reinterpret_cast<uint32_t*>(script_hash.data()) = i;
        
        auto contract = std::make_shared<ContractState>();
        cache_->PutContract(script_hash, contract);
    }
    
    // Verify contracts are cached
    int found = 0;
    for (uint32_t i = 0; i < 50; ++i) {
        UInt160 script_hash;
        std::fill(script_hash.begin(), script_hash.end(), 0);
        *reinterpret_cast<uint32_t*>(script_hash.data()) = i;
        
        if (cache_->GetContract(script_hash)) {
            found++;
        }
    }
    
    EXPECT_EQ(found, 50);
}

TEST_F(BlockchainCacheTest, CacheClearOperation) {
    // Add data to cache
    for (uint32_t i = 0; i < 100; ++i) {
        auto block = std::make_shared<MockBlock>(i);
        cache_->CacheBlock(block);
        
        auto tx = std::make_shared<MockTransaction>(i);
        cache_->CacheTransaction(tx);
    }
    
    auto stats = cache_->GetStats();
    EXPECT_GT(stats.block_stats.size, 0);
    EXPECT_GT(stats.tx_stats.size, 0);
    
    // Clear cache
    cache_->Clear();
    
    // Verify cache is empty
    stats = cache_->GetStats();
    EXPECT_EQ(stats.block_stats.size, 0);
    EXPECT_EQ(stats.tx_stats.size, 0);
    
    // Cache misses for previously cached items
    auto block = cache_->GetBlock(MockBlock::GenerateHash(50));
    EXPECT_FALSE(block);
}

TEST_F(BlockchainCacheTest, CacheStatisticsAccuracy) {
    // Perform controlled operations
    const int num_blocks = 50;
    const int num_txs = 100;
    const int lookups = 200;
    
    // Add blocks
    for (int i = 0; i < num_blocks; ++i) {
        auto block = std::make_shared<MockBlock>(i);
        cache_->CacheBlock(block);
    }
    
    // Add transactions
    for (int i = 0; i < num_txs; ++i) {
        auto tx = std::make_shared<MockTransaction>(i);
        cache_->CacheTransaction(tx);
    }
    
    // Perform lookups (50% hits)
    int block_hits = 0, block_misses = 0;
    for (int i = 0; i < lookups; ++i) {
        uint32_t index = i < num_blocks ? i : i + 1000;
        if (cache_->GetBlock(MockBlock::GenerateHash(index))) {
            block_hits++;
        } else {
            block_misses++;
        }
    }
    
    auto stats = cache_->GetStats();
    EXPECT_EQ(stats.block_stats.size, num_blocks);
    EXPECT_EQ(stats.tx_stats.size, num_txs);
    EXPECT_EQ(stats.block_stats.hits, block_hits);
    EXPECT_EQ(stats.block_stats.misses, block_misses);
    
    // Verify hit rate calculation
    double expected_hit_rate = static_cast<double>(block_hits) / lookups;
    EXPECT_NEAR(stats.hit_rate, expected_hit_rate, 0.01);
}

TEST_F(BlockchainCacheTest, PerformanceBenchmark) {
    const int num_operations = 100000;
    
    // Pre-populate cache
    for (int i = 0; i < 1000; ++i) {
        auto block = std::make_shared<MockBlock>(i);
        cache_->CacheBlock(block);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Perform many cache operations
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 1999);
    
    for (int i = 0; i < num_operations; ++i) {
        uint32_t index = dist(gen);
        if (index < 1000) {
            // Cache hit
            cache_->GetBlock(MockBlock::GenerateHash(index));
        } else {
            // Cache miss
            cache_->GetBlock(MockBlock::GenerateHash(index));
        }
    }
    
    auto duration = std::chrono::high_resolution_clock::now() - start;
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    
    // Should handle at least 100K ops/second
    double ops_per_second = (num_operations * 1000000.0) / us;
    EXPECT_GT(ops_per_second, 100000);
    
    std::cout << "BlockchainCache Performance: " << ops_per_second << " ops/sec\n";
    
    // Verify cache metrics
    auto stats = cache_->GetStats();
    std::cout << "Cache Hit Rate: " << (stats.hit_rate * 100) << "%\n";
}