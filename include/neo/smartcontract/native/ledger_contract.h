#pragma once

#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/persistence/data_cache.h>
#include <neo/smartcontract/native/hash_index_state.h>
#include <neo/smartcontract/native/native_contract.h>

#include <memory>
#include <string>

namespace neo::smartcontract::native
{
/**
 * @brief Represents the ledger native contract.
 */
class LedgerContract : public NativeContract
{
    // Friend classes for testing
    friend class NativeContractTest;
    friend class LedgerContractTest;

   public:
    /**
     * @brief The contract ID.
     */
    static constexpr int32_t ID = -4;

    /**
     * @brief The contract name.
     */
    static constexpr const char* NAME = "Ledger";

    /**
     * @brief The storage prefix for block hash.
     */
    static constexpr uint8_t PREFIX_BLOCK_HASH = 9;

    /**
     * @brief The storage prefix for current block.
     */
    static constexpr uint8_t PREFIX_CURRENT_BLOCK = 12;

    /**
     * @brief The storage prefix for block.
     */
    static constexpr uint8_t PREFIX_BLOCK = 5;

    /**
     * @brief The storage prefix for transaction.
     */
    static constexpr uint8_t PREFIX_TRANSACTION = 11;

    /**
     * @brief Constructs a LedgerContract.
     */
    LedgerContract();

    /**
     * @brief Gets the instance.
     * @return The instance.
     */
    static std::shared_ptr<LedgerContract> GetInstance();

   public:
    /**
     * @brief Checks if the contract is initialized.
     * @param snapshot The snapshot.
     * @return True if the contract is initialized, false otherwise.
     */
    bool IsInitialized(std::shared_ptr<persistence::DataCache> snapshot) const;

    /**
     * @brief Checks if a block is traceable.
     * @param engine The engine.
     * @param index The block index.
     * @return True if the block is traceable, false otherwise.
     */
    bool IsTraceableBlock(ApplicationEngine& engine, uint32_t index) const;

    /**
     * @brief Checks if a block is traceable.
     * @param snapshot The snapshot.
     * @param index The block index.
     * @param maxTraceableBlocks The maximum number of traceable blocks.
     * @return True if the block is traceable, false otherwise.
     */
    bool IsTraceableBlock(std::shared_ptr<persistence::DataCache> snapshot, uint32_t index,
                          uint32_t maxTraceableBlocks) const;

    /**
     * @brief Gets the current block hash.
     * @param snapshot The snapshot.
     * @return The current block hash.
     */
    io::UInt256 GetCurrentHash(std::shared_ptr<persistence::DataCache> snapshot) const;

    /**
     * @brief Gets the current block index.
     * @param snapshot The snapshot.
     * @return The current block index.
     */
    uint32_t GetCurrentIndex(std::shared_ptr<persistence::DataCache> snapshot) const;

    /**
     * @brief Gets the block hash for the specified index.
     * @param snapshot The snapshot.
     * @param index The block index.
     * @return The block hash.
     */
    io::UInt256 GetBlockHash(std::shared_ptr<persistence::DataCache> snapshot, uint32_t index) const;

    /**
     * @brief Gets the block for the specified hash.
     * @param snapshot The snapshot.
     * @param hash The block hash.
     * @return The block.
     */
    std::shared_ptr<ledger::Block> GetBlock(std::shared_ptr<persistence::DataCache> snapshot,
                                            const io::UInt256& hash) const;

    /**
     * @brief Gets the transaction for the specified hash.
     * @param snapshot The snapshot.
     * @param hash The transaction hash.
     * @return The transaction.
     */
    std::shared_ptr<ledger::Transaction> GetTransaction(std::shared_ptr<persistence::DataCache> snapshot,
                                                        const io::UInt256& hash) const;

    /**
     * @brief Gets the transaction height for the specified hash.
     * @param snapshot The snapshot.
     * @param hash The transaction hash.
     * @return The transaction height.
     */
    int32_t GetTransactionHeight(std::shared_ptr<persistence::DataCache> snapshot, const io::UInt256& hash) const;

    /**
     * @brief Handles the OnPersist event.
     * @param engine The engine.
     * @return True if successful, false otherwise.
     */
    bool OnPersist(ApplicationEngine& engine);

    /**
     * @brief Handles the PostPersist event.
     * @param engine The engine.
     * @return True if successful, false otherwise.
     */
    bool PostPersist(ApplicationEngine& engine);

   protected:
    /**
     * @brief Initializes the contract.
     */
    void Initialize() override;

   private:
    /**
     * @brief The current block storage key.
     */
    persistence::StorageKey currentBlockKey_;

    /**
     * @brief Handles the getHash method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetHash(ApplicationEngine& engine,
                                             const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getBlock method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetBlock(ApplicationEngine& engine,
                                              const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getTransaction method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetTransaction(ApplicationEngine& engine,
                                                    const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getTransactionHeight method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetTransactionHeight(ApplicationEngine& engine,
                                                          const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getCurrentIndex method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetCurrentIndex(ApplicationEngine& engine,
                                                     const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getCurrentHash method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetCurrentHash(ApplicationEngine& engine,
                                                    const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Converts a block to a stack item.
     * @param block The block.
     * @return The stack item.
     */
    std::shared_ptr<vm::StackItem> BlockToStackItem(std::shared_ptr<ledger::Block> block) const;

    /**
     * @brief Converts a transaction to a stack item.
     * @param tx The transaction.
     * @return The stack item.
     */
    std::shared_ptr<vm::StackItem> TransactionToStackItem(std::shared_ptr<ledger::Transaction> tx) const;
};
}  // namespace neo::smartcontract::native
