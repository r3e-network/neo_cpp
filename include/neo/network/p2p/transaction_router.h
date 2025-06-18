#pragma once

#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <neo/io/uint256.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

namespace neo::network::p2p
{
    // Use Neo3Transaction from payloads namespace
    using Neo3Transaction = payloads::Neo3Transaction;
    
    /**
     * @brief Routes transactions in the P2P network.
     */
    class TransactionRouter
    {
    public:
        /**
         * @brief Constructs a TransactionRouter.
         * @param blockchain The blockchain.
         * @param memPool The memory pool.
         */
        TransactionRouter(std::shared_ptr<ledger::Blockchain> blockchain, std::shared_ptr<ledger::MemoryPool> memPool);

        /**
         * @brief Destructor.
         */
        ~TransactionRouter();

        /**
         * @brief Starts the transaction router.
         */
        void Start();

        /**
         * @brief Stops the transaction router.
         */
        void Stop();

        /**
         * @brief Checks if the transaction router is running.
         * @return True if the transaction router is running, false otherwise.
         */
        bool IsRunning() const;

        /**
         * @brief Adds a transaction to the router.
         * @param transaction The transaction.
         * @return True if the transaction was added, false otherwise.
         */
        bool AddTransaction(std::shared_ptr<Neo3Transaction> transaction);

        /**
         * @brief Gets the transactions in the router.
         * @return The transactions.
         */
        std::vector<std::shared_ptr<Neo3Transaction>> GetTransactions() const;

        /**
         * @brief Removes a transaction from the router.
         * @param hash The hash of the transaction.
         * @return True if the transaction was removed, false otherwise.
         */
        bool RemoveTransaction(const io::UInt256& hash);

    private:
        std::shared_ptr<ledger::Blockchain> blockchain_;
        std::shared_ptr<ledger::MemoryPool> memPool_;
        std::unordered_map<io::UInt256, std::shared_ptr<Neo3Transaction>> transactions_;
        mutable std::mutex transactionsMutex_;
        std::atomic<bool> running_;
        std::thread routerThread_;
        std::condition_variable routerCondition_;
        std::mutex routerMutex_;

        /**
         * @brief Processes transactions.
         */
        void ProcessTransactions();

        /**
         * @brief Cleans up expired transactions.
         */
        void CleanupExpiredTransactions();
    };
}
