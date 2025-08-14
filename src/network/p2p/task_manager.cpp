/**
 * @file task_manager.cpp
 * @brief Management components
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

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
    // Check if the block already exists in blockchain
    if (blockchain_ && blockchain_->ContainsBlock(hash)) {
        return false;
    }

    // Check if task already exists
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        if (blockTasks_.find(hash) != blockTasks_.end()) {
            return false;
        }
        
        // Add the task with current timestamp
        blockTasks_[hash] = std::chrono::system_clock::now();
    }

    // Notify the task thread to process new task
    {
        std::lock_guard<std::mutex> lock(taskMutex_);
        taskCondition_.notify_one();
    }

    return true;
}

bool TaskManager::AddTransactionTask(const io::UInt256& hash)
{
    // Check if the transaction already exists in mempool or blockchain
    if (memPool_ && memPool_->Contains(hash)) {
        return false;
    }
    
    if (blockchain_ && blockchain_->ContainsTransaction(hash)) {
        return false;
    }

    // Check if task already exists
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        if (transactionTasks_.find(hash) != transactionTasks_.end()) {
            return false;
        }
        
        // Add the task with current timestamp
        transactionTasks_[hash] = std::chrono::system_clock::now();
    }

    // Notify the task thread to process new task
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
    // Get current block tasks
    std::vector<io::UInt256> tasksToProcess;
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        for (const auto& [hash, timestamp] : blockTasks_) {
            // Check if block still doesn't exist
            if (!blockchain_ || !blockchain_->ContainsBlock(hash)) {
                tasksToProcess.push_back(hash);
            }
        }
    }
    
    // Process each block task
    for (const auto& hash : tasksToProcess) {
        // Create GetData request for the missing block
        payloads::GetDataPayload payload;
        payload.AddBlockHash(hash);
        
        // Send request to connected peers (would be done via LocalNode)
        // For now, just mark as processed
        
        // Track the request with timestamp for retry logic
        pendingBlockRequests_[hash] = std::chrono::system_clock::now();
    }
    
    // Handle timeouts and retries for pending requests
    auto now = std::chrono::system_clock::now();
    auto retryTimeout = std::chrono::seconds(10);
    
    for (auto it = pendingBlockRequests_.begin(); it != pendingBlockRequests_.end();) {
        if (now - it->second > retryTimeout) {
            // Retry the request
            AddBlockTask(it->first);
            it = pendingBlockRequests_.erase(it);
        } else {
            ++it;
        }
    }

    // Clean up expired tasks
    CleanupExpiredTasks();
}

void TaskManager::ProcessTransactionTasks()
{
    // Get current transaction tasks
    std::vector<io::UInt256> tasksToProcess;
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        for (const auto& [hash, timestamp] : transactionTasks_) {
            // Check if transaction still doesn't exist
            if ((!memPool_ || !memPool_->Contains(hash)) &&
                (!blockchain_ || !blockchain_->ContainsTransaction(hash))) {
                tasksToProcess.push_back(hash);
            }
        }
    }
    
    // Process each transaction task
    for (const auto& hash : tasksToProcess) {
        // Create GetData request for the missing transaction
        payloads::GetDataPayload payload;
        payload.AddTransactionHash(hash);
        
        // Send request to connected peers (would be done via LocalNode)
        // For now, just mark as processed
        
        // Track the request with timestamp for retry logic
        pendingTransactionRequests_[hash] = std::chrono::system_clock::now();
    }
    
    // Handle timeouts and retries for pending requests
    auto now = std::chrono::system_clock::now();
    auto retryTimeout = std::chrono::seconds(10);
    
    for (auto it = pendingTransactionRequests_.begin(); it != pendingTransactionRequests_.end();) {
        if (now - it->second > retryTimeout) {
            // Retry the request
            AddTransactionTask(it->first);
            it = pendingTransactionRequests_.erase(it);
        } else {
            ++it;
        }
    }

    // Clean up expired tasks
    CleanupExpiredTasks();
}

}  // namespace neo::network::p2p