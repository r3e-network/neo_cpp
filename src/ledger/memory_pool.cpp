#include <neo/ledger/memory_pool.h>
#include <algorithm>
#include <numeric>

namespace neo::ledger
{
    MemoryPool::MemoryPool(size_t max_capacity)
        : max_capacity_(max_capacity)
    {
    }

    void MemoryPool::SetVerifier(std::function<bool(const Transaction&)> verifier)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        verifier_ = std::move(verifier);
    }

    bool MemoryPool::TryAdd(const Transaction& transaction)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto hash = transaction.GetHash();
        
        // Check if transaction already exists
        if (transactions_.find(hash) != transactions_.end())
        {
            return false;
        }
        
        // Verify transaction if verifier is set
        if (verifier_ && !verifier_(transaction))
        {
            return false;
        }
        
        // If pool is full, try to evict lowest priority transaction
        if (transactions_.size() >= max_capacity_)
        {
            EvictLowestPriority();
            
            // If still full after eviction, reject new transaction if it's lower priority
            if (transactions_.size() >= max_capacity_)
            {
                double new_priority = CalculatePriority(transaction);
                
                // Find lowest priority transaction in pool
                auto min_it = std::min_element(transactions_.begin(), transactions_.end(),
                    [this](const auto& a, const auto& b) {
                        return CalculatePriority(a.second) < CalculatePriority(b.second);
                    });
                
                if (min_it != transactions_.end() && 
                    new_priority <= CalculatePriority(min_it->second))
                {
                    return false; // New transaction has lower priority
                }
                
                // Remove the lowest priority transaction
                transactions_.erase(min_it);
            }
        }
        
        // Add the new transaction
        transactions_.emplace(hash, transaction);
        return true;
    }

    void MemoryPool::Remove(const io::UInt256& hash)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        transactions_.erase(hash);
    }

    bool MemoryPool::Contains(const io::UInt256& hash) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return transactions_.find(hash) != transactions_.end();
    }

    const Transaction* MemoryPool::GetTransaction(const io::UInt256& hash) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = transactions_.find(hash);
        return (it != transactions_.end()) ? &it->second : nullptr;
    }

    std::vector<Transaction> MemoryPool::GetSortedTransactions() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<Transaction> result;
        result.reserve(transactions_.size());
        
        for (const auto& pair : transactions_)
        {
            result.push_back(pair.second);
        }
        
        // Sort by priority (fee per byte, highest first)
        std::sort(result.begin(), result.end(),
            [this](const Transaction& a, const Transaction& b) {
                return CalculatePriority(a) > CalculatePriority(b);
            });
        
        return result;
    }

    std::vector<Transaction> MemoryPool::GetTransactionsForBlock(size_t max_count) const
    {
        auto sorted = GetSortedTransactions();
        
        if (sorted.size() <= max_count)
        {
            return sorted;
        }
        
        sorted.resize(max_count);
        return sorted;
    }

    size_t MemoryPool::GetSize() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return transactions_.size();
    }

    bool MemoryPool::IsFull() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return transactions_.size() >= max_capacity_;
    }

    void MemoryPool::Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        transactions_.clear();
    }

    MemoryPool::Stats MemoryPool::GetStatistics() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        Stats stats;
        stats.transaction_count = transactions_.size();
        stats.max_capacity = max_capacity_;
        stats.total_size_bytes = 0;
        stats.average_fee_per_byte = 0.0;
        
        if (transactions_.empty())
        {
            return stats;
        }
        
        double total_priority = 0.0;
        for (const auto& pair : transactions_)
        {
            const auto& tx = pair.second;
            stats.total_size_bytes += static_cast<size_t>(tx.GetSize());
            total_priority += CalculatePriority(tx);
        }
        
        stats.average_fee_per_byte = total_priority / static_cast<double>(transactions_.size());
        
        return stats;
    }

    void MemoryPool::EvictLowestPriority()
    {
        if (transactions_.empty())
        {
            return;
        }
        
        // Find transaction with lowest priority
        auto min_it = std::min_element(transactions_.begin(), transactions_.end(),
            [this](const auto& a, const auto& b) {
                return CalculatePriority(a.second) < CalculatePriority(b.second);
            });
        
        if (min_it != transactions_.end())
        {
            transactions_.erase(min_it);
        }
    }

    double MemoryPool::CalculatePriority(const Transaction& tx) const
    {
        // Priority is network fee per byte
        // Higher values indicate higher priority
        auto size = tx.GetSize();
        if (size == 0)
        {
            return 0.0;
        }
        
        // Use network fee if available, otherwise use system fee
        auto fee = tx.GetNetworkFee();
        if (fee == 0)
        {
            fee = tx.GetSystemFee();
        }
        
        return static_cast<double>(fee) / static_cast<double>(size);
    }
}