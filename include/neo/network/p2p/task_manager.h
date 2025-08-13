/**
 * @file task_manager.h
 * @brief Management components
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_vector.h>
#include <neo/io/uint256.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace neo::network::p2p
{
/**
 * @brief Manages tasks for the P2P network.
 */
class TaskManager
{
   public:
    /**
     * @brief Constructs a TaskManager.
     * @param blockchain The blockchain.
     * @param memPool The memory pool.
     */
    TaskManager(std::shared_ptr<ledger::Blockchain> blockchain, std::shared_ptr<ledger::MemoryPool> memPool);

    /**
     * @brief Destructor.
     */
    ~TaskManager();

    /**
     * @brief Starts the task manager.
     */
    void Start();

    /**
     * @brief Stops the task manager.
     */
    void Stop();

    /**
     * @brief Checks if the task manager is running.
     * @return True if the task manager is running, false otherwise.
     */
    bool IsRunning() const;

    /**
     * @brief Adds a block task.
     * @param hash The hash of the block.
     * @return True if the task was added, false otherwise.
     */
    bool AddBlockTask(const io::UInt256& hash);

    /**
     * @brief Adds a transaction task.
     * @param hash The hash of the transaction.
     * @return True if the task was added, false otherwise.
     */
    bool AddTransactionTask(const io::UInt256& hash);

    /**
     * @brief Gets the block tasks.
     * @return The block tasks.
     */
    std::vector<io::UInt256> GetBlockTasks() const;

    /**
     * @brief Gets the transaction tasks.
     * @return The transaction tasks.
     */
    std::vector<io::UInt256> GetTransactionTasks() const;

    /**
     * @brief Removes a block task.
     * @param hash The hash of the block.
     * @return True if the task was removed, false otherwise.
     */
    bool RemoveBlockTask(const io::UInt256& hash);

    /**
     * @brief Removes a transaction task.
     * @param hash The hash of the transaction.
     * @return True if the task was removed, false otherwise.
     */
    bool RemoveTransactionTask(const io::UInt256& hash);

   private:
    std::shared_ptr<ledger::Blockchain> blockchain_;
    std::shared_ptr<ledger::MemoryPool> memPool_;
    std::unordered_map<io::UInt256, std::chrono::system_clock::time_point> blockTasks_;
    std::unordered_map<io::UInt256, std::chrono::system_clock::time_point> transactionTasks_;
    mutable std::mutex tasksMutex_;
    std::atomic<bool> running_;
    std::thread taskThread_;
    std::condition_variable taskCondition_;
    std::mutex taskMutex_;

    /**
     * @brief Processes tasks.
     */
    void ProcessTasks();

    /**
     * @brief Processes block tasks.
     */
    void ProcessBlockTasks();

    /**
     * @brief Processes transaction tasks.
     */
    void ProcessTransactionTasks();

    /**
     * @brief Cleans up expired tasks.
     */
    void CleanupExpiredTasks();
};
}  // namespace neo::network::p2p
