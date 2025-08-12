#include <neo/io/byte_vector.h>
#include <neo/io/uint256.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/message_command.h>
#include <neo/network/p2p/payloads/get_data_payload.h>
#include <neo/network/p2p/task_manager.h>

#include <algorithm>
#include <chrono>
#include <iostream>

namespace neo::network::p2p
{
TaskManager::TaskManager(std::shared_ptr<ledger::Blockchain> blockchain, std::shared_ptr<ledger::MemoryPool> memPool)
    : blockchain_(blockchain), memPool_(memPool), running_(false)
{
}

TaskManager::~TaskManager() { Stop(); }

void TaskManager::Start()
{
    if (running_) return;

    running_ = true;
    taskThread_ = std::thread(&TaskManager::ProcessTasks, this);
}

void TaskManager::Stop()
{
    if (!running_) return;

    running_ = false;

    {
        std::lock_guard<std::mutex> lock(taskMutex_);
        taskCondition_.notify_all();
    }

    if (taskThread_.joinable()) taskThread_.join();
}

bool TaskManager::IsRunning() const { return running_; }

bool TaskManager::AddBlockTask(const io::UInt256& hash)
{
    // TODO: Implement block task management
    // Check if the block already exists
    // if (blockchain_->ContainsBlock(hash)) return false;

    // Add the task
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        blockTasks_[hash] = std::chrono::system_clock::now();
    }

    // Notify the task thread
    {
        std::lock_guard<std::mutex> lock(taskMutex_);
        taskCondition_.notify_one();
    }

    return true;
}

bool TaskManager::AddTransactionTask(const io::UInt256& hash)
{
    // TODO: Implement transaction task management
    // Check if the transaction already exists
    // if (memPool_->Contains(hash) || blockchain_->ContainsTransaction(hash)) return false;

    // Add the task
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        transactionTasks_[hash] = std::chrono::system_clock::now();
    }

    // Notify the task thread
    {
        std::lock_guard<std::mutex> lock(taskMutex_);
        taskCondition_.notify_one();
    }

    return true;
}

std::vector<io::UInt256> TaskManager::GetBlockTasks() const
{
    std::lock_guard<std::mutex> lock(tasksMutex_);
    std::vector<io::UInt256> tasks;
    tasks.reserve(blockTasks_.size());
    for (const auto& [hash, _] : blockTasks_)
    {
        tasks.push_back(hash);
    }
    return tasks;
}

std::vector<io::UInt256> TaskManager::GetTransactionTasks() const
{
    std::lock_guard<std::mutex> lock(tasksMutex_);
    std::vector<io::UInt256> tasks;
    tasks.reserve(transactionTasks_.size());
    for (const auto& [hash, _] : transactionTasks_)
    {
        tasks.push_back(hash);
    }
    return tasks;
}

bool TaskManager::RemoveBlockTask(const io::UInt256& hash)
{
    std::lock_guard<std::mutex> lock(tasksMutex_);
    return blockTasks_.erase(hash) > 0;
}

bool TaskManager::RemoveTransactionTask(const io::UInt256& hash)
{
    std::lock_guard<std::mutex> lock(tasksMutex_);
    return transactionTasks_.erase(hash) > 0;
}

void TaskManager::CleanupExpiredTasks()
{
    std::lock_guard<std::mutex> lock(tasksMutex_);

    auto now = std::chrono::system_clock::now();
    auto timeout = std::chrono::seconds(30);

    // Clean up expired block tasks
    for (auto it = blockTasks_.begin(); it != blockTasks_.end();)
    {
        if (now - it->second > timeout)
        {
            it = blockTasks_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Clean up expired transaction tasks
    for (auto it = transactionTasks_.begin(); it != transactionTasks_.end();)
    {
        if (now - it->second > timeout)
        {
            it = transactionTasks_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void TaskManager::ProcessTasks()
{
    while (running_)
    {
        std::unique_lock<std::mutex> lock(taskMutex_);
        taskCondition_.wait_for(lock, std::chrono::seconds(1));

        if (!running_) break;

        // Process block tasks
        ProcessBlockTasks();

        // Process transaction tasks
        ProcessTransactionTasks();
    }
}

void TaskManager::ProcessBlockTasks()
{
    // TODO: Implement block task processing
    // This would typically:
    // 1. Check for expired tasks
    // 2. Send GetData messages for missing blocks
    // 3. Track pending requests
    // 4. Handle timeouts and retries

    // Call the cleanup method to remove expired tasks
    CleanupExpiredTasks();
}

void TaskManager::ProcessTransactionTasks()
{
    // TODO: Implement transaction task processing
    // This would typically:
    // 1. Check for expired tasks
    // 2. Send GetData messages for missing transactions
    // 3. Track pending requests
    // 4. Handle timeouts and retries

    // Call the cleanup method to remove expired tasks
    CleanupExpiredTasks();
}

}  // namespace neo::network::p2p