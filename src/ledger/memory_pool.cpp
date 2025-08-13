/**
 * @file memory_pool.cpp
 * @brief Transaction memory pool management with three-tier system
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/ledger/memory_pool.h>
#include <neo/logging/logger.h>

#include <algorithm>
#include <numeric>
#include <optional>

namespace neo::ledger
{

MemoryPool::MemoryPool(size_t max_capacity, size_t max_unverified_capacity)
    : max_capacity_(max_capacity), max_unverified_capacity_(max_unverified_capacity)
{
    neo::logging::Logger::Instance().Info("MemoryPool", 
        "Initialized with capacity: " + std::to_string(max_capacity) + 
        ", unverified: " + std::to_string(max_unverified_capacity));
}

void MemoryPool::SetVerifier(std::function<bool(const network::p2p::payloads::Neo3Transaction&)> verifier)
{
    std::unique_lock lock(mutex_);
    verifier_ = std::move(verifier);
}

bool MemoryPool::TryAdd(const network::p2p::payloads::Neo3Transaction& transaction)
{
    std::unique_lock lock(mutex_);

    auto hash = transaction.GetHash();

    // Check if transaction already exists in any pool
    if (Contains(hash))
    {
        return false;
    }

    // Create pool item (Transaction is an alias for Neo3Transaction)
    PoolItem item(std::make_shared<Transaction>(transaction));
    
    // If verifier is set, verify the transaction
    if (verifier_)
    {
        if (verifier_(transaction))
        {
            // Transaction is verified, add to unsorted pool
            if (unsorted_transactions_.size() + sorted_transactions_.size() >= max_capacity_)
            {
                EvictLowestPriority();
                
                // If still full, reject if lower priority
                if (unsorted_transactions_.size() + sorted_transactions_.size() >= max_capacity_)
                {
                    return false;
                }
            }
            
            unsorted_transactions_[hash] = item;
            
            // Move to sorted pool
            MoveToVerified(item);
            
            // Fire event
            FireTransactionAddedEvent(item.GetTransactionPtr());
            return true;
        }
        else
        {
            // Transaction failed verification, add to unverified pool
            if (unverified_transactions_.size() >= max_unverified_capacity_)
            {
                EvictLowestPriorityUnverified();
                
                if (unverified_transactions_.size() >= max_unverified_capacity_)
                {
                    return false;
                }
            }
            
            unverified_transactions_[hash] = item;
            
            // Fire event
            FireTransactionAddedEvent(item.GetTransactionPtr());
            return true;
        }
    }
    else
    {
        // No verifier set, add directly to unverified pool
        if (unverified_transactions_.size() >= max_unverified_capacity_)
        {
            EvictLowestPriorityUnverified();
            
            if (unverified_transactions_.size() >= max_unverified_capacity_)
            {
                return false;
            }
        }
        
        unverified_transactions_[hash] = item;
        
        // Fire event
        FireTransactionAddedEvent(item.GetTransactionPtr());
        return true;
    }
}

void MemoryPool::Remove(const io::UInt256& hash)
{
    std::unique_lock lock(mutex_);
    
    // Remove from sorted pool
    auto sorted_it = std::find_if(sorted_transactions_.begin(), sorted_transactions_.end(),
                                  [&hash](const PoolItem& item) { 
                                      return item.GetTransaction().GetHash() == hash; 
                                  });
    if (sorted_it != sorted_transactions_.end())
    {
        auto tx_ptr = sorted_it->GetTransactionPtr();
        sorted_transactions_.erase(sorted_it);
        FireTransactionRemovedEvent(tx_ptr, TransactionRemovedEventArgs::Reason::Confirmed);
        return;
    }
    
    // Remove from unsorted pool
    auto unsorted_it = unsorted_transactions_.find(hash);
    if (unsorted_it != unsorted_transactions_.end())
    {
        auto tx_ptr = unsorted_it->second.GetTransactionPtr();
        unsorted_transactions_.erase(unsorted_it);
        FireTransactionRemovedEvent(tx_ptr, TransactionRemovedEventArgs::Reason::Confirmed);
        return;
    }
    
    // Remove from unverified pool
    auto unverified_it = unverified_transactions_.find(hash);
    if (unverified_it != unverified_transactions_.end())
    {
        auto tx_ptr = unverified_it->second.GetTransactionPtr();
        unverified_transactions_.erase(unverified_it);
        FireTransactionRemovedEvent(tx_ptr, TransactionRemovedEventArgs::Reason::Expired);
        return;
    }
}

bool MemoryPool::Contains(const io::UInt256& hash) const
{
    std::shared_lock lock(mutex_);
    
    // Check sorted pool
    auto sorted_it = std::find_if(sorted_transactions_.begin(), sorted_transactions_.end(),
                                  [&hash](const PoolItem& item) { 
                                      return item.GetTransaction().GetHash() == hash; 
                                  });
    if (sorted_it != sorted_transactions_.end())
    {
        return true;
    }
    
    // Check unsorted pool
    if (unsorted_transactions_.find(hash) != unsorted_transactions_.end())
    {
        return true;
    }
    
    // Check unverified pool
    if (unverified_transactions_.find(hash) != unverified_transactions_.end())
    {
        return true;
    }
    
    return false;
}

const network::p2p::payloads::Neo3Transaction* MemoryPool::GetTransaction(const io::UInt256& hash) const
{
    std::shared_lock lock(mutex_);
    
    // Check sorted pool first
    auto sorted_it = std::find_if(sorted_transactions_.begin(), sorted_transactions_.end(),
                                  [&hash](const PoolItem& item) { 
                                      return item.GetTransaction().GetHash() == hash; 
                                  });
    if (sorted_it != sorted_transactions_.end())
    {
        return &sorted_it->GetTransaction();
    }
    
    // Check unsorted pool
    auto unsorted_it = unsorted_transactions_.find(hash);
    if (unsorted_it != unsorted_transactions_.end())
    {
        return &unsorted_it->second.GetTransaction();
    }
    
    // Check unverified pool
    auto unverified_it = unverified_transactions_.find(hash);
    if (unverified_it != unverified_transactions_.end())
    {
        return &unverified_it->second.GetTransaction();
    }
    
    return nullptr;
}

std::optional<PoolItem> MemoryPool::Get(const io::UInt256& hash) const
{
    std::shared_lock lock(mutex_);
    
    // Check sorted pool first
    auto sorted_it = std::find_if(sorted_transactions_.begin(), sorted_transactions_.end(),
                                  [&hash](const PoolItem& item) { 
                                      return item.GetTransaction().GetHash() == hash; 
                                  });
    if (sorted_it != sorted_transactions_.end())
    {
        return *sorted_it;
    }
    
    // Check unsorted pool
    auto unsorted_it = unsorted_transactions_.find(hash);
    if (unsorted_it != unsorted_transactions_.end())
    {
        return unsorted_it->second;
    }
    
    // Check unverified pool
    auto unverified_it = unverified_transactions_.find(hash);
    if (unverified_it != unverified_transactions_.end())
    {
        return unverified_it->second;
    }
    
    return std::nullopt;
}

std::vector<network::p2p::payloads::Neo3Transaction> MemoryPool::GetSortedTransactions() const
{
    std::shared_lock lock(mutex_);
    
    std::vector<network::p2p::payloads::Neo3Transaction> result;
    result.reserve(sorted_transactions_.size());
    
    for (const auto& item : sorted_transactions_)
    {
        result.push_back(item.GetTransaction());
    }
    
    return result;
}

std::vector<network::p2p::payloads::Neo3Transaction> MemoryPool::GetUnverifiedTransactions() const
{
    std::shared_lock lock(mutex_);
    
    std::vector<network::p2p::payloads::Neo3Transaction> result;
    result.reserve(unverified_transactions_.size());
    
    for (const auto& [hash, item] : unverified_transactions_)
    {
        result.push_back(item.GetTransaction());
    }
    
    return result;
}

std::vector<network::p2p::payloads::Neo3Transaction> MemoryPool::GetTransactionsForBlock(size_t max_count) const
{
    std::shared_lock lock(mutex_);
    
    std::vector<network::p2p::payloads::Neo3Transaction> result;
    result.reserve(std::min(max_count, sorted_transactions_.size()));
    
    size_t count = 0;
    for (const auto& item : sorted_transactions_)
    {
        if (count >= max_count)
            break;
        
        result.push_back(item.GetTransaction());
        count++;
    }
    
    return result;
}

size_t MemoryPool::GetSize() const
{
    std::shared_lock lock(mutex_);
    return sorted_transactions_.size() + unsorted_transactions_.size();
}

size_t MemoryPool::GetUnverifiedSize() const
{
    std::shared_lock lock(mutex_);
    return unverified_transactions_.size();
}

bool MemoryPool::IsFull() const
{
    std::shared_lock lock(mutex_);
    return sorted_transactions_.size() + unsorted_transactions_.size() >= max_capacity_;
}

void MemoryPool::Clear()
{
    std::unique_lock lock(mutex_);
    sorted_transactions_.clear();
    unsorted_transactions_.clear();
    unverified_transactions_.clear();
}

MemoryPool::Stats MemoryPool::GetStatistics() const
{
    std::shared_lock lock(mutex_);
    
    Stats stats;
    stats.verified_transaction_count = sorted_transactions_.size() + unsorted_transactions_.size();
    stats.unverified_transaction_count = unverified_transactions_.size();
    stats.max_capacity = max_capacity_;
    stats.max_unverified_capacity = max_unverified_capacity_;
    stats.total_size_bytes = 0;
    stats.average_fee_per_byte = 0.0;
    
    double total_fee_per_byte = 0.0;
    size_t total_count = 0;
    
    // Calculate from sorted pool
    for (const auto& item : sorted_transactions_)
    {
        const auto& tx = item.GetTransaction();
        stats.total_size_bytes += static_cast<size_t>(tx.GetSize());
        total_fee_per_byte += CalculatePriority(tx);
        total_count++;
    }
    
    // Calculate from unsorted pool
    for (const auto& [hash, item] : unsorted_transactions_)
    {
        const auto& tx = item.GetTransaction();
        stats.total_size_bytes += static_cast<size_t>(tx.GetSize());
        total_fee_per_byte += CalculatePriority(tx);
        total_count++;
    }
    
    // Calculate from unverified pool
    for (const auto& [hash, item] : unverified_transactions_)
    {
        const auto& tx = item.GetTransaction();
        stats.total_size_bytes += static_cast<size_t>(tx.GetSize());
    }
    
    if (total_count > 0)
    {
        stats.average_fee_per_byte = total_fee_per_byte / static_cast<double>(total_count);
    }
    
    return stats;
}

void MemoryPool::ReverifyTransactions(size_t max_count)
{
    if (!verifier_)
        return;
    
    std::unique_lock lock(mutex_);
    
    std::vector<io::UInt256> to_move;
    size_t count = 0;
    
    for (const auto& [hash, item] : unverified_transactions_)
    {
        if (count >= max_count)
            break;
        
        if (verifier_(item.GetTransaction()))
        {
            to_move.push_back(hash);
        }
        count++;
    }
    
    // Move verified transactions
    for (const auto& hash : to_move)
    {
        auto it = unverified_transactions_.find(hash);
        if (it != unverified_transactions_.end())
        {
            auto item = it->second;
            unverified_transactions_.erase(it);
            
            // Check capacity
            if (unsorted_transactions_.size() + sorted_transactions_.size() < max_capacity_)
            {
                unsorted_transactions_[hash] = item;
                MoveToVerified(item);
            }
        }
    }
}

void MemoryPool::EvictLowestPriority()
{
    if (sorted_transactions_.empty() && unsorted_transactions_.empty())
        return;
    
    // Since sorted_transactions_ is ordered by priority (highest first),
    // the lowest priority is at the end
    if (!sorted_transactions_.empty())
    {
        auto it = std::prev(sorted_transactions_.end());
        auto tx_ptr = it->GetTransactionPtr();
        sorted_transactions_.erase(it);
        FireTransactionRemovedEvent(tx_ptr, TransactionRemovedEventArgs::Reason::Evicted);
    }
    else if (!unsorted_transactions_.empty())
    {
        // Find lowest priority in unsorted pool
        auto min_it = std::min_element(unsorted_transactions_.begin(), unsorted_transactions_.end(),
                                       [this](const auto& a, const auto& b) {
                                           return CalculatePriority(a.second.GetTransaction()) < 
                                                  CalculatePriority(b.second.GetTransaction());
                                       });
        
        if (min_it != unsorted_transactions_.end())
        {
            auto tx_ptr = min_it->second.GetTransactionPtr();
            unsorted_transactions_.erase(min_it);
            FireTransactionRemovedEvent(tx_ptr, TransactionRemovedEventArgs::Reason::Evicted);
        }
    }
}

void MemoryPool::EvictLowestPriorityUnverified()
{
    if (unverified_transactions_.empty())
        return;
    
    // Find oldest unverified transaction (FIFO for unverified)
    auto oldest_it = unverified_transactions_.begin();
    
    if (oldest_it != unverified_transactions_.end())
    {
        auto tx_ptr = oldest_it->second.GetTransactionPtr();
        unverified_transactions_.erase(oldest_it);
        FireTransactionRemovedEvent(tx_ptr, TransactionRemovedEventArgs::Reason::Evicted);
    }
}

double MemoryPool::CalculatePriority(const network::p2p::payloads::Neo3Transaction& tx) const
{
    // Priority is network fee per byte
    auto size = tx.GetSize();
    if (size == 0)
        return 0.0;
    
    auto fee = tx.GetNetworkFee();
    return static_cast<double>(fee) / static_cast<double>(size);
}

void MemoryPool::MoveToVerified(const PoolItem& item)
{
    // Remove from unsorted if present
    auto hash = item.GetTransaction().GetHash();
    unsorted_transactions_.erase(hash);
    
    // Add to sorted pool
    sorted_transactions_.insert(item);
}

void MemoryPool::FireTransactionAddedEvent(std::shared_ptr<Transaction> transaction)
{
    // Fire event through static event system
    TransactionAddedEventArgs args;
    args.transaction = transaction;
    MemoryPoolEvents::FireTransactionAdded(args);
}

void MemoryPool::FireTransactionRemovedEvent(std::shared_ptr<Transaction> transaction,
                                             TransactionRemovedEventArgs::Reason reason)
{
    // Fire event through static event system
    TransactionRemovedEventArgs args;
    args.transaction = transaction;
    args.reason = reason;
    MemoryPoolEvents::FireTransactionRemoved(args);
}

}  // namespace neo::ledger