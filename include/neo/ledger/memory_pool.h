/**
 * @file memory_pool.h
 * @brief Transaction memory pool implementation for pending transactions
 * @details Manages unconfirmed transactions waiting to be included in blocks,
 *          with prioritization based on fees and verification status.
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/uint256.h>
#include <neo/ledger/event_system.h>
#include <neo/ledger/pool_item.h>
#include <neo/ledger/transaction.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>

#include <functional>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace neo::ledger
{
/**
 * @class MemoryPool
 * @brief Transaction memory pool for pending transactions awaiting block inclusion
 * @details Implements a multi-tier pool system matching the C# Neo architecture:
 *          - Sorted pool: Verified transactions ordered by fee per byte
 *          - Unsorted pool: Verified transactions awaiting sorting
 *          - Unverified pool: Transactions pending verification
 * 
 * Transactions flow: Unverified → Unsorted → Sorted → Block
 * 
 * @thread_safety Thread-safe using reader-writer locks
 * @performance Optimized for high-throughput transaction processing
 * @security Prevents double-spending and invalid transaction inclusion
 */
class MemoryPool
{
   private:
    /// @brief Reader-writer lock for thread-safe access
    mutable std::shared_mutex mutex_;

    // Main transaction pools (matches C# Neo architecture)
    /// @brief Verified transactions not yet sorted by priority
    std::unordered_map<io::UInt256, PoolItem> unsorted_transactions_;
    
    /// @brief Verified transactions sorted by fee per byte (highest first)
    std::set<PoolItem> sorted_transactions_;
    
    /// @brief Transactions awaiting verification
    std::unordered_map<io::UInt256, PoolItem> unverified_transactions_;

    /// @brief Maximum number of verified transactions to hold
    size_t max_capacity_;
    
    /// @brief Maximum number of unverified transactions to hold
    size_t max_unverified_capacity_;

    /// @brief Transaction verification callback function
    std::function<bool(const network::p2p::payloads::Neo3Transaction&)> verifier_;

    // Note: Event handlers moved to static event system for C# compatibility
    // Events are now fired through MemoryPoolEvents static class

   public:
    /**
     * @brief Constructs a memory pool with specified capacities
     * @param max_capacity Maximum verified transactions (default: 50000)
     * @param max_unverified_capacity Maximum unverified transactions (default: 5000)
     * @details Capacities prevent memory exhaustion from transaction spam
     * @post Pool is initialized and ready to accept transactions
     */
    explicit MemoryPool(size_t max_capacity = 50000, size_t max_unverified_capacity = 5000);

    /**
     * @brief Sets the transaction verification callback
     * @param verifier Function that validates transaction correctness
     * @details Verifier should check signatures, balances, and script validity
     * @pre verifier must not be null
     * @thread_safety Thread-safe, can be called while pool is active
     */
    void SetVerifier(std::function<bool(const network::p2p::payloads::Neo3Transaction&)> verifier);

    /**
     * @brief Attempts to add a transaction to the pool
     * @param transaction The transaction to add
     * @return true if successfully added, false if rejected or duplicate
     * @details Transaction is added to unverified pool first, then verified
     * @note Emits TransactionAdded event on success
     * @complexity O(log n) for sorted pool insertion
     */
    bool TryAdd(const network::p2p::payloads::Neo3Transaction& transaction);

    /**
     * @brief Removes a transaction from the pool
     * @param hash Hash of the transaction to remove
     * @details Removes from all pools (sorted, unsorted, unverified)
     * @note Typically called when transaction is included in a block
     * @complexity O(log n) for sorted pool removal
     */
    void Remove(const io::UInt256& hash);

    /**
     * @brief Checks if a transaction exists in any pool
     * @param hash Hash of the transaction to check
     * @return true if transaction is in any pool (verified or unverified)
     * @thread_safety Thread-safe, uses shared lock
     * @complexity O(1) average case
     */
    bool Contains(const io::UInt256& hash) const;

    /**
     * @brief Retrieves a transaction by its hash
     * @param hash Hash of the transaction to retrieve
     * @return Pointer to transaction if found, nullptr otherwise
     * @warning Returned pointer is only valid while pool lock is held
     * @thread_safety Thread-safe, uses shared lock
     * @complexity O(1) average case
     */
    const network::p2p::payloads::Neo3Transaction* GetTransaction(const io::UInt256& hash) const;

    /**
     * @brief Gets a pool item by its hash
     * @param hash Hash of the transaction to retrieve
     * @return Optional PoolItem if found
     * @details Returns the complete PoolItem with transaction and metadata
     * @thread_safety Thread-safe, uses shared lock
     * @complexity O(1) average case
     */
    std::optional<PoolItem> Get(const io::UInt256& hash) const;

    /**
     * @brief Gets all verified transactions sorted by priority
     * @return Vector of transactions ordered by fee per byte (highest first)
     * @details Used by consensus nodes to select transactions for blocks
     * @thread_safety Thread-safe, uses shared lock
     * @complexity O(n) where n is number of sorted transactions
     */
    std::vector<network::p2p::payloads::Neo3Transaction> GetSortedTransactions() const;

    /**
     * @brief Gets all transactions awaiting verification
     * @return Vector of unverified transactions
     * @details Used for re-verification attempts after state changes
     * @thread_safety Thread-safe, uses shared lock
     * @complexity O(n) where n is number of unverified transactions
     */
    std::vector<network::p2p::payloads::Neo3Transaction> GetUnverifiedTransactions() const;

    /**
     * @brief Gets all verified transactions (both sorted and unsorted buckets).
     * @return Vector of verified transactions.
     */
    std::vector<network::p2p::payloads::Neo3Transaction> GetVerifiedTransactions() const;

    /**
     * @brief Gets both verified and unverified transactions in a single call.
     * @param[out] verified Vector receiving verified transactions.
     * @param[out] unverified Vector receiving unverified transactions.
     */
    void GetVerifiedAndUnverifiedTransactions(std::vector<network::p2p::payloads::Neo3Transaction>& verified,
                                              std::vector<network::p2p::payloads::Neo3Transaction>& unverified) const;

    /**
     * @brief Selects highest priority transactions for block creation
     * @param max_count Maximum transactions to include
     * @return Vector of transactions optimized for maximum fees
     * @details Selects from sorted pool to maximize block fees
     * @pre max_count should respect block size limits
     * @thread_safety Thread-safe, uses shared lock
     * @complexity O(min(n, max_count)) where n is pool size
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
