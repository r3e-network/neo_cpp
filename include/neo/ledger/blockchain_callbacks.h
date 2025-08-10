#pragma once

#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace neo::ledger
{
/**
 * @brief Manages blockchain callbacks.
 */
class BlockchainCallbacks
{
   public:
    /**
     * @brief Callback for block persistence.
     */
    using BlockPersistenceCallback = std::function<void(std::shared_ptr<Block>)>;

    /**
     * @brief Callback for transaction execution.
     */
    using TransactionExecutionCallback = std::function<void(std::shared_ptr<Transaction>)>;

    /**
     * @brief Constructs a BlockchainCallbacks.
     */
    BlockchainCallbacks();

    /**
     * @brief Registers a callback for block persistence.
     * @param callback The callback.
     * @return The callback ID.
     */
    int32_t RegisterBlockPersistenceCallback(BlockPersistenceCallback callback);

    /**
     * @brief Unregisters a callback for block persistence.
     * @param id The callback ID.
     */
    void UnregisterBlockPersistenceCallback(int32_t id);

    /**
     * @brief Registers a callback for transaction execution.
     * @param callback The callback.
     * @return The callback ID.
     */
    int32_t RegisterTransactionExecutionCallback(TransactionExecutionCallback callback);

    /**
     * @brief Unregisters a callback for transaction execution.
     * @param id The callback ID.
     */
    void UnregisterTransactionExecutionCallback(int32_t id);

    /**
     * @brief Notifies block persistence callbacks.
     * @param block The block.
     */
    void NotifyBlockPersistence(std::shared_ptr<Block> block);

    /**
     * @brief Notifies transaction execution callbacks.
     * @param transaction The transaction.
     */
    void NotifyTransactionExecution(std::shared_ptr<Transaction> transaction);

   private:
    mutable std::mutex mutex_;
    std::unordered_map<int32_t, BlockPersistenceCallback> blockPersistenceCallbacks_;
    std::unordered_map<int32_t, TransactionExecutionCallback> transactionExecutionCallbacks_;
    int32_t nextCallbackId_;
};
}  // namespace neo::ledger
