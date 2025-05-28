#include <neo/network/p2p/task_manager.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/io/uint256.h>
#include <neo/io/byte_vector.h>
#include <chrono>
#include <algorithm>
#include <iostream>

namespace neo::network::p2p
{
    TaskManager::TaskManager(std::shared_ptr<ledger::Blockchain> blockchain, std::shared_ptr<ledger::MemoryPool> memPool)
        : blockchain_(blockchain), memPool_(memPool), running_(false)
    {
    }

    TaskManager::~TaskManager()
    {
        Stop();
    }

    void TaskManager::Start()
    {
        if (running_)
            return;

        running_ = true;
        taskThread_ = std::thread(&TaskManager::ProcessTasks, this);
    }

    void TaskManager::Stop()
    {
        if (!running_)
            return;

        running_ = false;

        {
            std::lock_guard<std::mutex> lock(taskMutex_);
            taskCondition_.notify_all();
        }

        if (taskThread_.joinable())
            taskThread_.join();
    }

    bool TaskManager::IsRunning() const
    {
        return running_;
    }

    bool TaskManager::AddBlockTask(const io::UInt256& hash)
    {
        // Check if the block already exists
        if (blockchain_->ContainsBlock(hash))
            return false;

        // Add the task
        {
            std::lock_guard<std::mutex> lock(tasksMutex_);

            // Check if the task already exists
            if (blockTasks_.find(hash) != blockTasks_.end())
                return false;

            // Add the task
            blockTasks_[hash] = std::chrono::system_clock::now();
        }

        // Notify the task thread
        {
            std::lock_guard<std::mutex> lock(taskMutex_);
            taskCondition_.notify_all();
        }

        return true;
    }

    bool TaskManager::AddTransactionTask(const io::UInt256& hash)
    {
        // Check if the transaction already exists
        if (memPool_->ContainsTransaction(hash) || blockchain_->ContainsTransaction(hash))
            return false;

        // Add the task
        {
            std::lock_guard<std::mutex> lock(tasksMutex_);

            // Check if the task already exists
            if (transactionTasks_.find(hash) != transactionTasks_.end())
                return false;

            // Add the task
            transactionTasks_[hash] = std::chrono::system_clock::now();
        }

        // Notify the task thread
        {
            std::lock_guard<std::mutex> lock(taskMutex_);
            taskCondition_.notify_all();
        }

        return true;
    }

    std::vector<io::UInt256> TaskManager::GetBlockTasks() const
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);

        std::vector<io::UInt256> tasks;
        tasks.reserve(blockTasks_.size());

        for (const auto& pair : blockTasks_)
        {
            tasks.push_back(pair.first);
        }

        return tasks;
    }

    std::vector<io::UInt256> TaskManager::GetTransactionTasks() const
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);

        std::vector<io::UInt256> tasks;
        tasks.reserve(transactionTasks_.size());

        for (const auto& pair : transactionTasks_)
        {
            tasks.push_back(pair.first);
        }

        return tasks;
    }

    bool TaskManager::RemoveBlockTask(const io::UInt256& hash)
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);

        auto it = blockTasks_.find(hash);
        if (it == blockTasks_.end())
            return false;

        blockTasks_.erase(it);
        return true;
    }

    bool TaskManager::RemoveTransactionTask(const io::UInt256& hash)
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);

        auto it = transactionTasks_.find(hash);
        if (it == transactionTasks_.end())
            return false;

        transactionTasks_.erase(it);
        return true;
    }

    void TaskManager::ProcessTasks()
    {
        while (running_)
        {
            // Process block tasks
            ProcessBlockTasks();

            // Process transaction tasks
            ProcessTransactionTasks();

            // Clean up expired tasks
            CleanupExpiredTasks();

            // Wait for a notification or a timeout
            std::unique_lock<std::mutex> lock(taskMutex_);
            taskCondition_.wait_for(lock, std::chrono::seconds(5));
        }
    }

    void TaskManager::ProcessBlockTasks()
    {
        std::vector<io::UInt256> tasks;

        // Get the block tasks
        {
            std::lock_guard<std::mutex> lock(tasksMutex_);
            tasks.reserve(blockTasks_.size());

            for (const auto& pair : blockTasks_)
            {
                tasks.push_back(pair.first);
            }
        }

        // Process the tasks
        for (const auto& hash : tasks)
        {
            // Check if the block already exists
            if (blockchain_->ContainsBlock(hash))
            {
                RemoveBlockTask(hash);
                continue;
            }

            // TODO: Request the block from peers
        }
    }

    void TaskManager::ProcessTransactionTasks()
    {
        std::vector<io::UInt256> tasks;

        // Get the transaction tasks
        {
            std::lock_guard<std::mutex> lock(tasksMutex_);
            tasks.reserve(transactionTasks_.size());

            for (const auto& pair : transactionTasks_)
            {
                tasks.push_back(pair.first);
            }
        }

        // Process the tasks
        for (const auto& hash : tasks)
        {
            // Check if the transaction already exists
            if (memPool_->ContainsTransaction(hash) || blockchain_->ContainsTransaction(hash))
            {
                RemoveTransactionTask(hash);
                continue;
            }

            // TODO: Request the transaction from peers
        }
    }

    void TaskManager::CleanupExpiredTasks()
    {
        auto now = std::chrono::system_clock::now();
        auto expiration = std::chrono::seconds(60);

        // Clean up expired block tasks
        {
            std::lock_guard<std::mutex> lock(tasksMutex_);

            for (auto it = blockTasks_.begin(); it != blockTasks_.end();)
            {
                if (now - it->second > expiration)
                {
                    it = blockTasks_.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        // Clean up expired transaction tasks
        {
            std::lock_guard<std::mutex> lock(tasksMutex_);

            for (auto it = transactionTasks_.begin(); it != transactionTasks_.end();)
            {
                if (now - it->second > expiration)
                {
                    it = transactionTasks_.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }
}
