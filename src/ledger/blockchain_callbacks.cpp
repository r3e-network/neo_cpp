#include <neo/ledger/blockchain_callbacks.h>
#include <iostream>

namespace neo::ledger
{
    BlockchainCallbacks::BlockchainCallbacks()
        : nextCallbackId_(0)
    {
    }

    int32_t BlockchainCallbacks::RegisterBlockPersistenceCallback(BlockPersistenceCallback callback)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        int32_t id = nextCallbackId_++;
        blockPersistenceCallbacks_[id] = std::move(callback);
        return id;
    }

    void BlockchainCallbacks::UnregisterBlockPersistenceCallback(int32_t id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        blockPersistenceCallbacks_.erase(id);
    }

    int32_t BlockchainCallbacks::RegisterTransactionExecutionCallback(TransactionExecutionCallback callback)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        int32_t id = nextCallbackId_++;
        transactionExecutionCallbacks_[id] = std::move(callback);
        return id;
    }

    void BlockchainCallbacks::UnregisterTransactionExecutionCallback(int32_t id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        transactionExecutionCallbacks_.erase(id);
    }

    void BlockchainCallbacks::NotifyBlockPersistence(std::shared_ptr<Block> block)
    {
        for (const auto& [id, callback] : blockPersistenceCallbacks_)
        {
            try
            {
                callback(block);
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Block persistence callback failed: " << ex.what() << std::endl;
            }
        }
    }

    void BlockchainCallbacks::NotifyTransactionExecution(std::shared_ptr<Transaction> transaction)
    {
        for (const auto& [id, callback] : transactionExecutionCallbacks_)
        {
            try
            {
                callback(transaction);
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Transaction execution callback failed: " << ex.what() << std::endl;
            }
        }
    }
}
