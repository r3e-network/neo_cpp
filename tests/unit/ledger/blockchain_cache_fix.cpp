// Add proper cache implementation methods
#include <neo/ledger/blockchain_cache.h>

namespace neo::ledger {
    
bool BlockchainCache::Add(const Block& block) {
    if (!block.GetHash().IsValid()) return false;
    
    std::lock_guard<std::mutex> lock(mutex_);
    blocks_[block.GetHash()] = std::make_shared<Block>(block);
    
    // Update LRU
    if (blocks_.size() > max_blocks_) {
        // Simple eviction - remove oldest
        auto it = blocks_.begin();
        blocks_.erase(it);
    }
    
    return true;
}

std::shared_ptr<Block> BlockchainCache::Get(const UInt256& hash) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = blocks_.find(hash);
    if (it != blocks_.end()) {
        hits_++;
        return it->second;
    }
    misses_++;
    return nullptr;
}

double BlockchainCache::GetHitRate() const {
    uint64_t total = hits_ + misses_;
    return total > 0 ? static_cast<double>(hits_) / total : 0.0;
}

}
