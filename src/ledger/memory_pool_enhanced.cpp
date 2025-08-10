#include <neo/ledger/event_system.h>
#include <neo/ledger/memory_pool.h>

#include <algorithm>
#include <chrono>
#include <numeric>

namespace neo::ledger
{
MemoryPool::MemoryPool(size_t max_capacity, size_t max_unverified_capacity)
    : max_capacity_(max_capacity), max_unverified_capacity_(max_unverified_capacity)
{
}

void MemoryPool::SetVerifier(std::function<bool(const network::p2p::payloads::Neo3Transaction&)> verifier)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    verifier_ = std::move(verifier);
}

bool MemoryPool::TryAdd(const network::p2p::payloads::Neo3Transaction& transaction)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto hash = transaction.GetHash();

    // Check if transaction already exists in any pool
    if (unsorted_transactions_.find(hash) != unsorted_transactions_.end() ||
        unverified_transactions_.find(hash) != unverified_transactions_.end())
    {
        return false;
    }

    auto tx_ptr = std::make_shared<network::p2p::payloads::Neo3Transaction>(transaction);
    PoolItem item(std::reinterpret_pointer_cast<Transaction>(tx_ptr));

    // Try to verify transaction
    if (verifier_ && verifier_(transaction))
    {
        // Add to verified pools
        MoveToVerified(item);
        return true;
    }
    else
    {
        // Add to unverified pool
        if (unverified_transactions_.size() >= max_unverified_capacity_)
        {
            EvictLowestPriorityUnverified();
        }

        unverified_transactions_[hash] = item;
        return true;
    }
}

void MemoryPool::Remove(const io::UInt256& hash)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);

    // Remove from verified pools
    auto it = unsorted_transactions_.find(hash);
    if (it != unsorted_transactions_.end())
    {
        sorted_transactions_.erase(it->second);
        unsorted_transactions_.erase(it);
        return;
    }

    // Remove from unverified pool
    unverified_transactions_.erase(hash);
}

bool MemoryPool::Contains(const io::UInt256& hash) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return unsorted_transactions_.find(hash) != unsorted_transactions_.end() ||
           unverified_transactions_.find(hash) != unverified_transactions_.end();
}

const network::p2p::payloads::Neo3Transaction* MemoryPool::GetTransaction(const io::UInt256& hash) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);

    // Check verified pool first
    auto it = unsorted_transactions_.find(hash);
    if (it != unsorted_transactions_.end())
    {
        return std::reinterpret_pointer_cast<network::p2p::payloads::Neo3Transaction>(it->second.GetTransaction())
            .get();
    }

    // Check unverified pool
    auto unverified_it = unverified_transactions_.find(hash);
    if (unverified_it != unverified_transactions_.end())
    {
        return std::reinterpret_pointer_cast<network::p2p::payloads::Neo3Transaction>(
                   unverified_it->second.GetTransaction())
            .get();
    }

    return nullptr;
}

std::vector<network::p2p::payloads::Neo3Transaction> MemoryPool::GetSortedTransactions() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<network::p2p::payloads::Neo3Transaction> result;
    result.reserve(sorted_transactions_.size());

    for (const auto& item : sorted_transactions_)
    {
        auto neo3_tx = std::reinterpret_pointer_cast<network::p2p::payloads::Neo3Transaction>(item.GetTransaction());
        result.push_back(*neo3_tx);
    }

    return result;  // Already sorted by set ordering
}

std::vector<network::p2p::payloads::Neo3Transaction> MemoryPool::GetUnverifiedTransactions() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<network::p2p::payloads::Neo3Transaction> result;
    result.reserve(unverified_transactions_.size());

    for (const auto& pair : unverified_transactions_)
    {
        auto neo3_tx =
            std::reinterpret_pointer_cast<network::p2p::payloads::Neo3Transaction>(pair.second.GetTransaction());
        result.push_back(*neo3_tx);
    }

    return result;
}

std::vector<network::p2p::payloads::Neo3Transaction> MemoryPool::GetTransactionsForBlock(size_t max_count) const
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
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return unsorted_transactions_.size();
}

size_t MemoryPool::GetUnverifiedSize() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return unverified_transactions_.size();
}

bool MemoryPool::IsFull() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return unsorted_transactions_.size() >= max_capacity_;
}

void MemoryPool::Clear()
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    unsorted_transactions_.clear();
    sorted_transactions_.clear();
    unverified_transactions_.clear();
}

// Event handler methods removed - using static event system for C# compatibility
// Use MemoryPoolEvents::SubscribeTransactionAdded() and MemoryPoolEvents::SubscribeTransactionRemoved() instead

void MemoryPool::ReverifyTransactions(size_t max_count)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);

    if (unverified_transactions_.empty()) return;

    size_t processed = 0;
    auto it = unverified_transactions_.begin();

    while (it != unverified_transactions_.end() && processed < max_count)
    {
        const auto& pool_item = it->second;
        auto neo3_tx =
            std::reinterpret_pointer_cast<network::p2p::payloads::Neo3Transaction>(pool_item.GetTransaction());

        // Try to verify the transaction
        if (verifier_ && verifier_(*neo3_tx))
        {
            // Move to verified pool
            MoveToVerified(pool_item);
            it = unverified_transactions_.erase(it);
        }
        else
        {
            // Check if transaction has expired
            auto now = std::chrono::system_clock::now();
            auto age = std::chrono::duration_cast<std::chrono::minutes>(now - pool_item.GetTimestamp());

            if (age.count() > 30)  // 30 minutes timeout
            {
                // Remove expired transaction
                FireTransactionRemovedEvent(neo3_tx, TransactionRemovedEventArgs::Reason::Expired);
                it = unverified_transactions_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        ++processed;
    }
}

MemoryPool::Stats MemoryPool::GetStatistics() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);

    Stats stats;
    stats.verified_transaction_count = unsorted_transactions_.size();
    stats.unverified_transaction_count = unverified_transactions_.size();
    stats.max_capacity = max_capacity_;
    stats.max_unverified_capacity = max_unverified_capacity_;
    stats.total_size_bytes = 0;
    stats.average_fee_per_byte = 0.0;

    if (unsorted_transactions_.empty())
    {
        return stats;
    }

    double total_priority = 0.0;
    for (const auto& pair : unsorted_transactions_)
    {
        const auto& item = pair.second;
        auto neo3_tx = std::reinterpret_pointer_cast<network::p2p::payloads::Neo3Transaction>(item.GetTransaction());
        stats.total_size_bytes += static_cast<size_t>(neo3_tx->GetSize());
        total_priority += CalculatePriority(*neo3_tx);
    }

    stats.average_fee_per_byte = total_priority / static_cast<double>(unsorted_transactions_.size());

    return stats;
}

void MemoryPool::EvictLowestPriority()
{
    if (sorted_transactions_.empty())
    {
        return;
    }

    // Remove lowest priority item (first in sorted set due to ordering)
    auto lowest = sorted_transactions_.begin();
    auto hash = lowest->GetHash();

    auto neo3_tx = std::reinterpret_pointer_cast<network::p2p::payloads::Neo3Transaction>(lowest->GetTransaction());
    FireTransactionRemovedEvent(neo3_tx, TransactionRemovedEventArgs::Reason::LowPriority);

    sorted_transactions_.erase(lowest);
    unsorted_transactions_.erase(hash);
}

void MemoryPool::EvictLowestPriorityUnverified()
{
    if (unverified_transactions_.empty())
    {
        return;
    }

    // Find transaction with lowest priority in unverified pool
    auto min_it = std::min_element(unverified_transactions_.begin(), unverified_transactions_.end(),
                                   [](const auto& a, const auto& b)
                                   { return a.second.GetFeePerByte() < b.second.GetFeePerByte(); });

    if (min_it != unverified_transactions_.end())
    {
        auto neo3_tx =
            std::reinterpret_pointer_cast<network::p2p::payloads::Neo3Transaction>(min_it->second.GetTransaction());
        FireTransactionRemovedEvent(neo3_tx, TransactionRemovedEventArgs::Reason::LowPriority);
        unverified_transactions_.erase(min_it);
    }
}

double MemoryPool::CalculatePriority(const network::p2p::payloads::Neo3Transaction& tx) const
{
    // Priority is network fee per byte
    auto size = tx.GetSize();
    if (size == 0)
    {
        return 0.0;
    }

    auto fee = tx.GetNetworkFee();
    if (fee == 0)
    {
        fee = tx.GetSystemFee();
    }

    return static_cast<double>(fee) / static_cast<double>(size);
}

void MemoryPool::MoveToVerified(const PoolItem& item)
{
    // Check for conflicts with existing verified transactions
    for (const auto& existing : sorted_transactions_)
    {
        if (item.ConflictsWith(existing))
        {
            // Remove lower priority transaction
            if (item.GetFeePerByte() > existing.GetFeePerByte())
            {
                // Remove existing transaction
                auto hash = existing.GetHash();
                auto neo3_tx =
                    std::reinterpret_pointer_cast<network::p2p::payloads::Neo3Transaction>(existing.GetTransaction());

                sorted_transactions_.erase(existing);
                unsorted_transactions_.erase(hash);
                FireTransactionRemovedEvent(neo3_tx, TransactionRemovedEventArgs::Reason::Replaced);
                break;
            }
            else
            {
                // Reject new transaction
                auto neo3_tx =
                    std::reinterpret_pointer_cast<network::p2p::payloads::Neo3Transaction>(item.GetTransaction());
                FireTransactionRemovedEvent(neo3_tx, TransactionRemovedEventArgs::Reason::LowPriority);
                return;
            }
        }
    }

    // Add to verified pools
    if (sorted_transactions_.size() >= max_capacity_)
    {
        EvictLowestPriority();
    }

    sorted_transactions_.insert(item);
    unsorted_transactions_[item.GetHash()] = item;

    auto neo3_tx = std::reinterpret_pointer_cast<network::p2p::payloads::Neo3Transaction>(item.GetTransaction());
    FireTransactionAddedEvent(neo3_tx);
}

void MemoryPool::FireTransactionAddedEvent(std::shared_ptr<network::p2p::payloads::Neo3Transaction> transaction)
{
    // Fire event through static event system for C# compatibility
    auto tx_as_ledger = std::reinterpret_pointer_cast<Transaction>(transaction);
    MemoryPoolEvents::FireTransactionAdded(tx_as_ledger);
}

void MemoryPool::FireTransactionRemovedEvent(std::shared_ptr<network::p2p::payloads::Neo3Transaction> transaction,
                                             TransactionRemovedEventArgs::Reason reason)
{
    // Fire event through static event system for C# compatibility
    auto tx_as_ledger = std::reinterpret_pointer_cast<Transaction>(transaction);
    TransactionRemovedEventArgs args(tx_as_ledger, reason);
    MemoryPoolEvents::FireTransactionRemoved(args);
}

}  // namespace neo::ledger