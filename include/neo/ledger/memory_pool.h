#pragma once

#include <functional>
#include <mutex>
#include <neo/io/uint256.h>
#include <neo/ledger/pool_item.h>
#include <neo/ledger/event_system.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace neo::ledger
{
/**
 * @brief Transaction memory pool for pending transactions
 * This matches the C# Neo MemoryPool architecture with sorted/unsorted/unverified pools
 */
class MemoryPool
{
  private:
    mutable std::shared_mutex mutex_;
    
    // Main transaction pools (matches C# Neo architecture)
    std::unordered_map<io::UInt256, PoolItem> unsorted_transactions_;  // Dictionary<UInt256, PoolItem>
    std::set<PoolItem> sorted_transactions_;                           // SortedSet<PoolItem>
    std::unordered_map<io::UInt256, PoolItem> unverified_transactions_; // Dictionary<UInt256, PoolItem>
    
    size_t max_capacity_;
    size_t max_unverified_capacity_;

    // Transaction verification function
    std::function<bool(const network::p2p::payloads::Neo3Transaction&)> verifier_;
    
    // Note: Event handlers moved to static event system for C# compatibility
    // Events are now fired through MemoryPoolEvents static class

  public:
    /**
     * @brief Constructor
     * @param max_capacity Maximum number of verified transactions to hold
     * @param max_unverified_capacity Maximum number of unverified transactions to hold
     */
    explicit MemoryPool(size_t max_capacity = 50000, size_t max_unverified_capacity = 5000);

    /**
     * @brief Set transaction verifier function
     * @param verifier Function to verify transactions
     */
    void SetVerifier(std::function<bool(const network::p2p::payloads::Neo3Transaction&)> verifier);

    /**
     * @brief Try to add transaction to pool
     * @param transaction Transaction to add
     * @return true if transaction was added, false otherwise
     */
    bool TryAdd(const network::p2p::payloads::Neo3Transaction& transaction);

    /**
     * @brief Remove transaction from pool
     * @param hash Transaction hash to remove
     */
    void Remove(const io::UInt256& hash);

    /**
     * @brief Check if transaction exists in pool
     * @param hash Transaction hash to check
     * @return true if transaction exists
     */
    bool Contains(const io::UInt256& hash) const;

    /**
     * @brief Get transaction by hash
     * @param hash Transaction hash
     * @return Pointer to transaction if found, nullptr otherwise
     */
    const network::p2p::payloads::Neo3Transaction* GetTransaction(const io::UInt256& hash) const;

    /**
     * @brief Get all verified transactions sorted by fee per byte (highest first)
     * @return Vector of transactions sorted by priority
     */
    std::vector<network::p2p::payloads::Neo3Transaction> GetSortedTransactions() const;
    
    /**
     * @brief Get all unverified transactions
     * @return Vector of unverified transactions
     */
    std::vector<network::p2p::payloads::Neo3Transaction> GetUnverifiedTransactions() const;

    /**
     * @brief Get transactions for block creation
     * @param max_count Maximum number of transactions to return
     * @return Vector of highest priority transactions
     */
    std::vector<network::p2p::payloads::Neo3Transaction> GetTransactionsForBlock(size_t max_count) const;

    /**
     * @brief Get current verified pool size
     * @return Number of verified transactions in pool
     */
    size_t GetSize() const;
    
    /**
     * @brief Get current unverified pool size
     * @return Number of unverified transactions in pool
     */
    size_t GetUnverifiedSize() const;

    /**
     * @brief Check if pool is full
     * @return true if pool has reached maximum capacity
     */
    bool IsFull() const;

    /**
     * @brief Clear all transactions from pool
     */
    void Clear();

    /**
     * @brief Get memory pool statistics
     * @return Statistics object with pool information
     */
    struct Stats
    {
        size_t verified_transaction_count;
        size_t unverified_transaction_count;
        size_t total_size_bytes;
        size_t max_capacity;
        size_t max_unverified_capacity;
        double average_fee_per_byte;
    };

    Stats GetStatistics() const;
    
    /**
     * @brief Subscribe to transaction events (C# compatibility)
     * Note: Use MemoryPoolEvents::SubscribeTransactionAdded() and MemoryPoolEvents::SubscribeTransactionRemoved()
     * for static event subscription matching C# Neo's event pattern.
     */
    
    /**
     * @brief Reverify unverified transactions
     * @param max_count Maximum number of transactions to reverify
     */
    void ReverifyTransactions(size_t max_count = 10);

  private:
    /**
     * @brief Remove lowest priority transaction if pool is full
     */
    void EvictLowestPriority();
    
    /**
     * @brief Remove lowest priority unverified transaction if pool is full
     */
    void EvictLowestPriorityUnverified();

    /**
     * @brief Calculate transaction priority (fee per byte)
     * @param tx Transaction to calculate priority for
     * @return Priority value (higher is better)
     */
    double CalculatePriority(const network::p2p::payloads::Neo3Transaction& tx) const;
    
    /**
     * @brief Move transaction from unverified to verified pool
     * @param item The pool item to move
     */
    void MoveToVerified(const PoolItem& item);
    
    /**
     * @brief Fire transaction added event through static event system
     * @param transaction The added transaction
     */
    void FireTransactionAddedEvent(std::shared_ptr<network::p2p::payloads::Neo3Transaction> transaction);
    
    /**
     * @brief Fire transaction removed event through static event system
     * @param transaction The removed transaction
     * @param reason The removal reason
     */
    void FireTransactionRemovedEvent(std::shared_ptr<network::p2p::payloads::Neo3Transaction> transaction, 
                                     TransactionRemovedEventArgs::Reason reason);
};
}  // namespace neo::ledger