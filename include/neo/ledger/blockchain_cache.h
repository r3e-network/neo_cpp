/**
 * @file blockchain_cache.h
 * @brief High-performance caching layer for blockchain data
 */

#pragma once

#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <memory>
#include <unordered_map>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <atomic>

namespace neo {
namespace ledger {

/**
 * @brief LRU cache implementation for blockchain data
 * @tparam Key Cache key type
 * @tparam Value Cache value type
 */
template<typename Key, typename Value>
class LRUCache {
private:
    using KeyValuePair = std::pair<Key, Value>;
    using ListIterator = typename std::list<KeyValuePair>::iterator;

    size_t capacity_;
    std::list<KeyValuePair> items_;
    std::unordered_map<Key, ListIterator> index_;
    mutable std::shared_mutex mutex_;
    
    // Statistics
    mutable std::atomic<uint64_t> hits_{0};
    mutable std::atomic<uint64_t> misses_{0};

public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    /**
     * @brief Get value from cache
     * @param key Key to lookup
     * @return Value if found, nullptr otherwise
     */
    std::shared_ptr<Value> Get(const Key& key) const {
        std::shared_lock lock(mutex_);
        
        auto it = index_.find(key);
        if (it == index_.end()) {
            misses_.fetch_add(1);
            return nullptr;
        }
        
        hits_.fetch_add(1);
        
        // Move to front (most recently used)
        // Note: This requires mutable or const_cast in const method
        // For thread safety, we'll return without updating position in const Get
        return std::make_shared<Value>(it->second->second);
    }

    /**
     * @brief Put value into cache
     * @param key Key to store
     * @param value Value to store
     */
    void Put(const Key& key, const Value& value) {
        std::unique_lock lock(mutex_);
        
        auto it = index_.find(key);
        if (it != index_.end()) {
            // Update existing entry and move to front
            items_.erase(it->second);
            items_.push_front({key, value});
            index_[key] = items_.begin();
            return;
        }
        
        // Add new entry
        if (items_.size() >= capacity_) {
            // Remove least recently used
            auto& lru = items_.back();
            index_.erase(lru.first);
            items_.pop_back();
        }
        
        items_.push_front({key, value});
        index_[key] = items_.begin();
    }

    /**
     * @brief Remove entry from cache
     * @param key Key to remove
     */
    void Remove(const Key& key) {
        std::unique_lock lock(mutex_);
        
        auto it = index_.find(key);
        if (it != index_.end()) {
            items_.erase(it->second);
            index_.erase(it);
        }
    }

    /**
     * @brief Clear all entries
     */
    void Clear() {
        std::unique_lock lock(mutex_);
        items_.clear();
        index_.clear();
    }

    /**
     * @brief Get cache statistics
     */
    struct Stats {
        uint64_t hits;
        uint64_t misses;
        size_t size;
        size_t capacity;
        double hit_rate;
    };

    Stats GetStats() const {
        std::shared_lock lock(mutex_);
        Stats stats;
        stats.hits = hits_.load();
        stats.misses = misses_.load();
        stats.size = items_.size();
        stats.capacity = capacity_;
        uint64_t total = stats.hits + stats.misses;
        stats.hit_rate = total > 0 ? (double)stats.hits / total : 0.0;
        return stats;
    }
};

/**
 * @brief Comprehensive blockchain cache system
 */
class BlockchainCache {
public:
    struct Config {
        size_t block_cache_size = 1000;        // Number of blocks to cache
        size_t transaction_cache_size = 10000; // Number of transactions to cache
        size_t header_cache_size = 5000;       // Number of headers to cache
        size_t contract_cache_size = 500;      // Number of contracts to cache
        size_t state_cache_size = 10000;       // Number of state items to cache
        std::chrono::seconds ttl{3600};        // Time to live (1 hour)
        bool enable_metrics = true;            // Enable performance metrics
    };

private:
    Config config_;
    
    // Multiple cache layers
    LRUCache<io::UInt256, Block> block_cache_;
    LRUCache<io::UInt256, Transaction> transaction_cache_;
    LRUCache<io::UInt256, BlockHeader> header_cache_;
    LRUCache<uint32_t, io::UInt256> height_to_hash_cache_;
    LRUCache<io::UInt160, std::vector<uint8_t>> contract_cache_;
    
    // Cache for frequently accessed data
    mutable std::shared_mutex current_block_mutex_;
    std::shared_ptr<Block> current_block_;
    uint32_t current_height_{0};
    
    // Performance metrics
    struct Metrics {
        std::atomic<uint64_t> total_requests{0};
        std::atomic<uint64_t> cache_hits{0};
        std::atomic<uint64_t> cache_misses{0};
        std::atomic<uint64_t> cache_evictions{0};
        std::chrono::steady_clock::time_point start_time;
    };
    mutable Metrics metrics_;

public:
    explicit BlockchainCache(const Config& config = Config())
        : config_(config)
        , block_cache_(config.block_cache_size)
        , transaction_cache_(config.transaction_cache_size)
        , header_cache_(config.header_cache_size)
        , height_to_hash_cache_(config.block_cache_size)
        , contract_cache_(config.contract_cache_size) {
        metrics_.start_time = std::chrono::steady_clock::now();
    }

    /**
     * @brief Get block by hash
     * @param hash Block hash
     * @return Block if found in cache
     */
    std::shared_ptr<Block> GetBlock(const io::UInt256& hash) const {
        metrics_.total_requests.fetch_add(1);
        auto block = block_cache_.Get(hash);
        if (block) {
            metrics_.cache_hits.fetch_add(1);
        } else {
            metrics_.cache_misses.fetch_add(1);
        }
        return block;
    }

    /**
     * @brief Get block by height
     * @param height Block height
     * @return Block if found in cache
     */
    std::shared_ptr<Block> GetBlock(uint32_t height) const {
        auto hash = height_to_hash_cache_.Get(height);
        if (hash) {
            return GetBlock(*hash);
        }
        return nullptr;
    }

    /**
     * @brief Cache a block
     * @param block Block to cache
     */
    void CacheBlock(const std::shared_ptr<Block>& block) {
        if (!block) return;
        
        block_cache_.Put(block->GetHash(), *block);
        height_to_hash_cache_.Put(block->GetIndex(), block->GetHash());
        
        // Update current block if newer
        {
            std::unique_lock lock(current_block_mutex_);
            if (block->GetIndex() > current_height_) {
                current_block_ = block;
                current_height_ = block->GetIndex();
            }
        }
    }

    /**
     * @brief Get transaction by hash
     * @param hash Transaction hash
     * @return Transaction if found in cache
     */
    std::shared_ptr<Transaction> GetTransaction(const io::UInt256& hash) const {
        metrics_.total_requests.fetch_add(1);
        auto tx = transaction_cache_.Get(hash);
        if (tx) {
            metrics_.cache_hits.fetch_add(1);
        } else {
            metrics_.cache_misses.fetch_add(1);
        }
        return tx;
    }

    /**
     * @brief Cache a transaction
     * @param tx Transaction to cache
     */
    void CacheTransaction(const std::shared_ptr<Transaction>& tx) {
        if (!tx) return;
        transaction_cache_.Put(tx->GetHash(), *tx);
    }

    /**
     * @brief Get current blockchain height from cache
     * @return Current height
     */
    uint32_t GetCurrentHeight() const {
        std::shared_lock lock(current_block_mutex_);
        return current_height_;
    }

    /**
     * @brief Get current block from cache
     * @return Current block
     */
    std::shared_ptr<Block> GetCurrentBlock() const {
        std::shared_lock lock(current_block_mutex_);
        return current_block_;
    }

    /**
     * @brief Clear all caches
     */
    void Clear() {
        block_cache_.Clear();
        transaction_cache_.Clear();
        header_cache_.Clear();
        height_to_hash_cache_.Clear();
        contract_cache_.Clear();
        
        std::unique_lock lock(current_block_mutex_);
        current_block_.reset();
        current_height_ = 0;
    }

    /**
     * @brief Get cache performance statistics
     */
    struct CacheStats {
        double hit_rate;
        uint64_t total_requests;
        uint64_t cache_hits;
        uint64_t cache_misses;
        std::chrono::seconds uptime;
        
        // Individual cache stats
        LRUCache<io::UInt256, Block>::Stats block_stats;
        LRUCache<io::UInt256, Transaction>::Stats tx_stats;
    };

    CacheStats GetStats() const {
        CacheStats stats;
        stats.total_requests = metrics_.total_requests.load();
        stats.cache_hits = metrics_.cache_hits.load();
        stats.cache_misses = metrics_.cache_misses.load();
        stats.hit_rate = stats.total_requests > 0 ? 
            (double)stats.cache_hits / stats.total_requests : 0.0;
        
        auto now = std::chrono::steady_clock::now();
        stats.uptime = std::chrono::duration_cast<std::chrono::seconds>(
            now - metrics_.start_time);
        
        stats.block_stats = block_cache_.GetStats();
        stats.tx_stats = transaction_cache_.GetStats();
        
        return stats;
    }

    /**
     * @brief Perform cache warming
     * @param blocks Recent blocks to pre-cache
     */
    void WarmCache(const std::vector<std::shared_ptr<Block>>& blocks) {
        for (const auto& block : blocks) {
            CacheBlock(block);
            
            // Also cache transactions in the block
            for (const auto& tx : block->GetTransactions()) {
                CacheTransaction(tx);
            }
        }
    }
};

} // namespace ledger
} // namespace neo