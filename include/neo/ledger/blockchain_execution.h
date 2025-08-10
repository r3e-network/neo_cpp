#pragma once

#include <neo/ledger/block.h>
#include <neo/ledger/blockchain_callbacks.h>
#include <neo/ledger/transaction.h>
#include <neo/persistence/data_cache.h>

#include <memory>

namespace neo::ledger
{
/**
 * @brief Handles blockchain execution.
 */
class BlockchainExecution
{
   public:
    /**
     * @brief Constructs a BlockchainExecution.
     * @param callbacks The blockchain callbacks.
     */
    explicit BlockchainExecution(std::shared_ptr<BlockchainCallbacks> callbacks);

    /**
     * @brief Executes a block.
     * @param block The block.
     * @param snapshot The snapshot.
     * @return True if the block was executed successfully, false otherwise.
     */
    bool ExecuteBlock(const Block& block, std::shared_ptr<persistence::DataCache> snapshot);

    /**
     * @brief Initializes the blockchain.
     * @param snapshot The snapshot.
     */
    void Initialize(std::shared_ptr<persistence::DataCache> snapshot);

   private:
    std::shared_ptr<BlockchainCallbacks> callbacks_;
};
}  // namespace neo::ledger
