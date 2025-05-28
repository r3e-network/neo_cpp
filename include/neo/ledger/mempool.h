#pragma once

#include <neo/ledger/transaction.h>
#include <neo/ledger/blockchain.h>
#include <neo/io/uint256.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <utility>

namespace neo::ledger
{
    /**
     * @brief A hasher for transaction inputs.
     */
    struct InputHasher
    {
        /**
         * @brief Computes the hash of a transaction input.
         * @param input The input to hash.
         * @return The hash value.
         */
        size_t operator()(const std::pair<io::UInt256, uint32_t>& input) const
        {
            return std::hash<std::string>()(input.first.ToHexString() + std::to_string(input.second));
        }
    };
    /**
     * @brief Represents a transaction in the memory pool.
     */
    class PoolItem
    {
    public:
        /**
         * @brief Constructs a PoolItem.
         * @param tx The transaction.
         */
        explicit PoolItem(std::shared_ptr<Transaction> tx);

        /**
         * @brief Gets the transaction.
         * @return The transaction.
         */
        std::shared_ptr<Transaction> GetTransaction() const;

        /**
         * @brief Gets the time when the transaction was added to the pool.
         * @return The time when the transaction was added to the pool.
         */
        std::chrono::system_clock::time_point GetTime() const;

        /**
         * @brief Gets the hash of the transaction.
         * @return The hash of the transaction.
         */
        io::UInt256 GetHash() const;

    private:
        std::shared_ptr<Transaction> tx_;
        std::chrono::system_clock::time_point time_;
    };

    /**
     * @brief Represents a memory pool for transactions.
     */
    class MemoryPool
    {
    public:
        /**
         * @brief Constructs a MemoryPool.
         * @param blockchain The blockchain.
         * @param capacity The capacity of the pool.
         */
        MemoryPool(std::shared_ptr<Blockchain> blockchain, size_t capacity = 50000);

        /**
         * @brief Gets the number of transactions in the pool.
         * @return The number of transactions in the pool.
         */
        size_t Count() const;

        /**
         * @brief Gets the capacity of the pool.
         * @return The capacity of the pool.
         */
        size_t GetCapacity() const;

        /**
         * @brief Sets the capacity of the pool.
         * @param capacity The capacity of the pool.
         */
        void SetCapacity(size_t capacity);

        /**
         * @brief Gets all transactions in the pool.
         * @return All transactions in the pool.
         */
        std::vector<std::shared_ptr<Transaction>> GetTransactions() const;

        /**
         * @brief Gets a transaction from the pool.
         * @param hash The hash of the transaction.
         * @return The transaction, or nullptr if not found.
         */
        std::shared_ptr<Transaction> GetTransaction(const io::UInt256& hash) const;

        /**
         * @brief Adds a transaction to the pool.
         * @param tx The transaction to add.
         * @return True if the transaction was added, false otherwise.
         */
        bool AddTransaction(std::shared_ptr<Transaction> tx);

        /**
         * @brief Removes a transaction from the pool.
         * @param hash The hash of the transaction to remove.
         * @return True if the transaction was removed, false otherwise.
         */
        bool RemoveTransaction(const io::UInt256& hash);

        /**
         * @brief Removes transactions that are in the specified block.
         * @param block The block.
         */
        void RemoveTransactions(const Block& block);

        /**
         * @brief Removes transactions that are in the specified blocks.
         * @param blocks The blocks.
         */
        void RemoveTransactions(const std::vector<Block>& blocks);

        /**
         * @brief Removes transactions that conflict with the specified block.
         * @param block The block.
         */
        void RemoveConflicts(const Block& block);

        /**
         * @brief Removes transactions that conflict with the specified blocks.
         * @param blocks The blocks.
         */
        void RemoveConflicts(const std::vector<Block>& blocks);

        /**
         * @brief Checks if a transaction is in the pool.
         * @param hash The hash of the transaction.
         * @return True if the transaction is in the pool, false otherwise.
         */
        bool ContainsTransaction(const io::UInt256& hash) const;

        /**
         * @brief Clears the pool.
         */
        void Clear();

    private:
        std::shared_ptr<Blockchain> blockchain_;
        size_t capacity_;
        mutable std::mutex mutex_;
        std::unordered_map<io::UInt256, PoolItem> transactions_;

        void RemoveOldTransactions();
    };
}
