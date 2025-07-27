#pragma once

#include <functional>
#include <mutex>
#include <neo/io/uint256.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <unordered_map>
#include <vector>

namespace neo::ledger
{
/**
 * @brief Transaction memory pool for pending transactions
 */
class MemoryPool
{
  private:
    mutable std::mutex mutex_;
    std::unordered_map<io::UInt256, network::p2p::payloads::Neo3Transaction> transactions_;
    size_t max_capacity_;

    // Transaction verification function
    std::function<bool(const network::p2p::payloads::Neo3Transaction&)> verifier_;

  public:
    /**
     * @brief Constructor
     * @param max_capacity Maximum number of transactions to hold
     */
    explicit MemoryPool(size_t max_capacity = 50000);

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
     * @brief Get all transactions sorted by fee per byte (highest first)
     * @return Vector of transactions sorted by priority
     */
    std::vector<network::p2p::payloads::Neo3Transaction> GetSortedTransactions() const;

    /**
     * @brief Get transactions for block creation
     * @param max_count Maximum number of transactions to return
     * @return Vector of highest priority transactions
     */
    std::vector<network::p2p::payloads::Neo3Transaction> GetTransactionsForBlock(size_t max_count) const;

    /**
     * @brief Get current pool size
     * @return Number of transactions in pool
     */
    size_t GetSize() const;

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
        size_t transaction_count;
        size_t total_size_bytes;
        size_t max_capacity;
        double average_fee_per_byte;
    };

    Stats GetStatistics() const;

  private:
    /**
     * @brief Remove lowest priority transaction if pool is full
     */
    void EvictLowestPriority();

    /**
     * @brief Calculate transaction priority (fee per byte)
     * @param tx Transaction to calculate priority for
     * @return Priority value (higher is better)
     */
    double CalculatePriority(const network::p2p::payloads::Neo3Transaction& tx) const;
};
}  // namespace neo::ledger