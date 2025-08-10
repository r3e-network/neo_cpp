#include <neo/io/byte_vector.h>
#include <neo/io/uint256.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/network/message.h>
#include <neo/network/message_command.h>
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
    // Check if the block already exists
    if (blockchain_->ContainsBlock(hash)) return false;

    // Add the task
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);

        // Check if the task already exists
        if (blockTasks_.find(hash) != blockTasks_.end()) return false;

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
    if (memPool_->ContainsTransaction(hash) || blockchain_->ContainsTransaction(hash)) return false;

    // Add the task
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);

        // Check if the task already exists
        if (transactionTasks_.find(hash) != transactionTasks_.end()) return false;

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
    if (it == blockTasks_.end()) return false;

    blockTasks_.erase(it);
    return true;
}

bool TaskManager::RemoveTransactionTask(const io::UInt256& hash)
{
    std::lock_guard<std::mutex> lock(tasksMutex_);

    auto it = transactionTasks_.find(hash);
    if (it == transactionTasks_.end()) return false;

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

        // Request the block from peers matching C# TaskManager implementation
        try
        {
            // Create GetData payload for the block
            auto getDataPayload = std::make_shared<payloads::InvPayload>();
            getDataPayload->SetType(payloads::InventoryType::Block);
            getDataPayload->AddHash(hash);

            // Create GetData message
            auto message = std::make_shared<Message>();
            message->SetCommand(MessageCommand::GetData);
            message->SetPayload(getDataPayload);

            // Send to connected peers that might have the block
            auto peers = localNode_->GetConnectedPeers();
            bool requestSent = false;

            for (const auto& peer : peers)
            {
                // Check if peer's last block index is higher than requested block
                if (peer->GetLastBlockIndex() >= blockIndex)
                {
                    peer->SendMessage(message);
                    requestSent = true;

                    // Add to pending requests to track timeout
                    pendingBlockRequests_[hash] = {std::chrono::steady_clock::now(), peer->GetId()};

                    // Don't send to all peers, just a few
                    if (requestSent) break;
                }
            }

            if (!requestSent)
            {
                // No suitable peers found, try again later
                std::cerr << "No peers available for block " << blockIndex << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error requesting block " << blockIndex << ": " << e.what() << std::endl;
        }
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

        // Request the transaction from peers matching C# TaskManager implementation
        try
        {
            // Create GetData payload for the transaction
            auto getDataPayload = std::make_shared<payloads::InvPayload>();
            getDataPayload->SetType(payloads::InventoryType::TX);
            getDataPayload->AddHash(hash);

            // Create GetData message
            auto message = std::make_shared<neo::network::Message>();
            message->SetCommand(neo::network::MessageCommand::GetData);
            message->SetPayload(getDataPayload);

            // Complete implementation: Integrate with P2P server to get connected peers
            bool requestSent = false;

            try
            {
                auto peers = localNode_->GetConnectedPeers();

                // Complete implementation: Peer communication and pending request tracking
                for (const auto& peer : peers)
                {
                    if (peer && peer->IsConnected())
                    {
                        // Send GetData request to connected peer
                        peer->SendMessage(message);
                        requestSent = true;

                        // Add to pending requests to track timeout
                        pendingTxRequests_[hash] = {
                            std::chrono::steady_clock::now(), peer->GetId(),
                            3  // Max retry attempts
                        };

                        LOG_DEBUG("Sent transaction request {} to peer {}", hash.ToString(), peer->GetId().ToString());

                        // Only send to first available peer for transactions
                        break;
                    }
                }

                if (!requestSent)
                {
                    LOG_WARNING("No connected peers available to request transaction {}", hash.ToString());

                    // Mark as failed so it can be retried later
                    MarkTransactionRequestFailed(hash);
                }
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("Error requesting transaction {}: {}", hash.ToString(), e.what());
                MarkTransactionRequestFailed(hash);
            }
            //
            //     // Send to multiple peers for better chance of getting the transaction
            //     if (requestSent && pendingTxRequests_.size() >= 3)
            //         break;
            // }
            //
            // if (!requestSent)
            // {
            //     // No peers available, try again later
            //     std::cerr << "No peers available for transaction " << hash.ToString() << std::endl;
            // }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error requesting transaction " << hash.ToString() << ": " << e.what() << std::endl;
        }
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
}  // namespace neo::network::p2p
