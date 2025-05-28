#pragma once

#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/block_storage.h>
#include <neo/ledger/transaction_storage.h>
#include <neo/ledger/blockchain_callbacks.h>
#include <neo/ledger/blockchain_execution.h>
#include <neo/persistence/data_cache.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/io/fixed8.h>
#include <memory>
#include <mutex>
#include <optional>

namespace neo::ledger
{
    /**
     * @brief The result of verifying a block or transaction.
     */
    enum class VerifyResult
    {
        Succeed,
        AlreadyExists,
        AlreadyInPool,
        Invalid,
        HasConflicts,
        UnableToVerify
    };

    /**
     * @brief Represents a blockchain.
     */
    class Blockchain
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
         * @brief Constructs a Blockchain.
         * @param dataCache The data cache.
         */
        explicit Blockchain(std::shared_ptr<persistence::DataCache> dataCache);

        /**
         * @brief Gets the current block height.
         * @return The current block height.
         */
        uint32_t GetHeight() const;

        /**
         * @brief Gets the current block hash.
         * @return The current block hash.
         */
        io::UInt256 GetCurrentBlockHash() const;

        /**
         * @brief Gets the current block header.
         * @return The current block header.
         */
        std::shared_ptr<BlockHeader> GetCurrentBlockHeader() const;

        /**
         * @brief Gets the current block.
         * @return The current block.
         */
        std::shared_ptr<Block> GetCurrentBlock() const;

        /**
         * @brief Gets a block by hash.
         * @param hash The hash of the block.
         * @return The block, or nullptr if not found.
         */
        std::shared_ptr<Block> GetBlock(const io::UInt256& hash) const;

        /**
         * @brief Gets a block by index.
         * @param index The index of the block.
         * @return The block, or nullptr if not found.
         */
        std::shared_ptr<Block> GetBlock(uint32_t index) const;

        /**
         * @brief Gets a block header by hash.
         * @param hash The hash of the block.
         * @return The block header, or nullptr if not found.
         */
        std::shared_ptr<BlockHeader> GetBlockHeader(const io::UInt256& hash) const;

        /**
         * @brief Gets a block header by index.
         * @param index The index of the block.
         * @return The block header, or nullptr if not found.
         */
        std::shared_ptr<BlockHeader> GetBlockHeader(uint32_t index) const;

        /**
         * @brief Gets a transaction by hash.
         * @param hash The hash of the transaction.
         * @return The transaction, or nullptr if not found.
         */
        std::shared_ptr<Transaction> GetTransaction(const io::UInt256& hash) const;

        /**
         * @brief Adds a block to the blockchain.
         * @param block The block to add.
         * @return True if the block was added, false otherwise.
         */
        bool AddBlock(const Block& block);

        /**
         * @brief Adds a block header to the blockchain.
         * @param header The block header to add.
         * @return True if the block header was added, false otherwise.
         */
        bool AddBlockHeader(const BlockHeader& header);

        /**
         * @brief Adds a transaction to the blockchain.
         * @param transaction The transaction to add.
         * @return True if the transaction was added, false otherwise.
         */
        bool AddTransaction(const Transaction& transaction);

        /**
         * @brief Checks if a block exists.
         * @param hash The hash of the block.
         * @return True if the block exists, false otherwise.
         */
        bool ContainsBlock(const io::UInt256& hash) const;

        /**
         * @brief Checks if a transaction exists.
         * @param hash The hash of the transaction.
         * @return True if the transaction exists, false otherwise.
         */
        bool ContainsTransaction(const io::UInt256& hash) const;

        /**
         * @brief Gets the hash of a block by index.
         * @param index The index of the block.
         * @return The hash of the block, or std::nullopt if not found.
         */
        std::optional<io::UInt256> GetBlockHash(uint32_t index) const;

        /**
         * @brief Gets the next block hash.
         * @param hash The hash of the current block.
         * @return The hash of the next block, or std::nullopt if not found.
         */
        std::optional<io::UInt256> GetNextBlockHash(const io::UInt256& hash) const;

        /**
         * @brief Gets the unspent transaction outputs for a transaction.
         * @param hash The hash of the transaction.
         * @return The unspent transaction outputs.
         */
        std::vector<TransactionOutput> GetUnspentOutputs(const io::UInt256& hash) const;

        /**
         * @brief Gets the unspent transaction outputs for an address.
         * @param scriptHash The script hash of the address.
         * @return The unspent transaction outputs.
         */
        std::vector<TransactionOutput> GetUnspentOutputs(const io::UInt160& scriptHash) const;

        /**
         * @brief Gets the balance of an address.
         * @param scriptHash The script hash of the address.
         * @param assetId The asset ID.
         * @return The balance.
         */
        io::Fixed8 GetBalance(const io::UInt160& scriptHash, const io::UInt256& assetId) const;

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
         * @brief Processes a new block.
         * @param block The block.
         * @return The verification result.
         */
        VerifyResult OnNewBlock(const Block& block);

        /**
         * @brief Processes a new transaction.
         * @param transaction The transaction.
         * @return The verification result.
         */
        VerifyResult OnNewTransaction(const Transaction& transaction);

        /**
         * @brief Initializes the blockchain.
         * @param genesisBlock The genesis block.
         */
        void Initialize(const Block& genesisBlock);

    private:
        /**
         * @brief Executes a block.
         * @param block The block to execute.
         * @param snapshot The data cache snapshot.
         * @return True if execution succeeded, false otherwise.
         */
        bool ExecuteBlock(const Block& block, std::shared_ptr<persistence::DataCache> snapshot);
        std::shared_ptr<persistence::DataCache> dataCache_;
        mutable std::mutex mutex_;
        std::shared_ptr<BlockStorage> blockStorage_;
        std::shared_ptr<TransactionStorage> transactionStorage_;
        std::shared_ptr<BlockchainCallbacks> callbacks_;
        std::shared_ptr<BlockchainExecution> execution_;
        uint32_t height_;
        io::UInt256 currentBlockHash_;
    };
}
