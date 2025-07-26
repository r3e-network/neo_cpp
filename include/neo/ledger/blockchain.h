#pragma once

// Network payload includes disabled since network module is disabled
// #include <neo/network/p2p/payloads/block.h>
// #include <neo/network/p2p/payloads/header.h>
// #include <neo/network/p2p/payloads/header_cache.h>
#include <neo/ledger/block.h>
#include <neo/ledger/block_header.h>
#include <neo/ledger/transaction.h>
// #include <neo/network/p2p/payloads/extensible_payload.h> // Disabled since network module is disabled
#include <neo/persistence/data_cache.h>
#include <neo/config/protocol_settings.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/io/fixed8.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/vm_types.h>
#include <neo/ledger/verify_result.h>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <functional>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <queue>

// Forward declaration for NeoSystem
namespace neo { class NeoSystem; }

namespace neo::ledger
{
    // Forward declarations
    class MemoryPool;

    // Type aliases - using ledger types since network module is disabled
    using Header = BlockHeader;  // Use BlockHeader from ledger instead of network payload
    // Note: Block class is defined in this namespace as neo::ledger::Block


    /**
     * @brief Represents an unverified block list for a specific height.
     */
    struct UnverifiedBlocksList
    {
        std::vector<std::shared_ptr<Block>> blocks;
        std::unordered_set<std::string> nodes; // Node identifiers that sent blocks
    };

    /**
     * @brief Event data for application execution.
     */
    struct ApplicationExecuted
    {
        std::shared_ptr<Transaction> transaction;
        std::shared_ptr<smartcontract::ApplicationEngine> engine;
        smartcontract::VMState vm_state;
        uint64_t gas_consumed;
        std::string exception_message;
        std::vector<smartcontract::LogEntry> logs;
        std::vector<smartcontract::NotifyEntry> notifications;
    };

    /**
     * @brief Event data for block persistence completion.
     */
    struct PersistCompleted
    {
        std::shared_ptr<Block> block;
    };

    /**
     * @brief Data for block import operations.
     */
    struct ImportData
    {
        std::vector<std::shared_ptr<Block>> blocks;
        bool verify = true;
    };

    /**
     * @brief Enhanced Blockchain processing engine - core of the Neo node.
     * This implementation matches the C# Blockchain.cs functionality with
     * sophisticated block processing, transaction verification, and event handling.
     */
    class Blockchain
    {
    public:
        // Event callback types
        using CommittingHandler = std::function<void(std::shared_ptr<NeoSystem>, std::shared_ptr<Block>, 
                                                    std::shared_ptr<persistence::DataCache>, 
                                                    const std::vector<ApplicationExecuted>&)>;
        using CommittedHandler = std::function<void(std::shared_ptr<NeoSystem>, std::shared_ptr<Block>)>;
        using BlockPersistenceHandler = std::function<void(std::shared_ptr<Block>)>;
        using TransactionHandler = std::function<void(std::shared_ptr<Transaction>, VerifyResult)>;
        // Network module disabled - IInventory handler commented out
        // using InventoryHandler = std::function<void(std::shared_ptr<network::p2p::payloads::IInventory>, VerifyResult)>;

        /**
         * @brief Constructs the Blockchain processing engine.
         * @param system The NeoSystem that contains this blockchain.
         */
        explicit Blockchain(std::shared_ptr<neo::NeoSystem> system);

        /**
         * @brief Destructor.
         */
        ~Blockchain();

        // Disable copy and move
        Blockchain(const Blockchain&) = delete;
        Blockchain& operator=(const Blockchain&) = delete;
        Blockchain(Blockchain&&) = delete;
        Blockchain& operator=(Blockchain&&) = delete;

        /**
         * @brief Initializes the blockchain with genesis block if needed.
         */
        void Initialize();

        /**
         * @brief Starts the blockchain processing.
         */
        void Start();

        /**
         * @brief Stops the blockchain processing.
         */
        void Stop();

        /**
         * @brief Gets the current block height.
         * @return The current block height.
         */
        uint32_t GetHeight() const;

        /**
         * @brief Gets the current block index (alias for GetHeight).
         * @return The current block index.
         */
        uint32_t GetCurrentBlockIndex() const { return GetHeight(); }

        /**
         * @brief Gets the current block hash.
         * @return The current block hash.
         */
        io::UInt256 GetCurrentBlockHash() const;

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
         * @brief Gets a block hash by index.
         * @param index The index of the block.
         * @return The block hash, or UInt256::Zero() if not found.
         */
        io::UInt256 GetBlockHash(uint32_t index) const;

        /**
         * @brief Gets a block header by hash.
         * @param hash The hash of the block.
         * @return The block header, or nullptr if not found.
         */
        std::shared_ptr<Header> GetBlockHeader(const io::UInt256& hash) const;

        /**
         * @brief Gets a block header by index.
         * @param index The index of the block.
         * @return The block header, or nullptr if not found.
         */
        std::shared_ptr<Header> GetBlockHeader(uint32_t index) const;

        /**
         * @brief Gets a transaction by hash.
         * @param hash The hash of the transaction.
         * @return The transaction, or nullptr if not found.
         */
        std::shared_ptr<Transaction> GetTransaction(const io::UInt256& hash) const;

        /**
         * @brief Gets the height/index of the block containing the specified transaction.
         * @param hash The hash of the transaction.
         * @return The block height, or -1 if not found.
         */
        int32_t GetTransactionHeight(const io::UInt256& hash) const;

        /**
         * @brief Gets a contract state by script hash.
         * @param hash The script hash of the contract.
         * @return The contract state, or nullptr if not found.
         */
        std::shared_ptr<smartcontract::ContractState> GetContract(const io::UInt160& hash) const;

        /**
         * @brief Checks if a block exists in the blockchain.
         * @param hash The hash of the block.
         * @return True if the block exists, false otherwise.
         */
        bool ContainsBlock(const io::UInt256& hash) const;

        /**
         * @brief Checks if a transaction exists in the blockchain.
         * @param hash The hash of the transaction.
         * @return True if the transaction exists, false otherwise.
         */
        bool ContainsTransaction(const io::UInt256& hash) const;

        /**
         * @brief Processes a new block received from the network.
         * @param block The block to process.
         * @return The verification result.
         */
        VerifyResult OnNewBlock(std::shared_ptr<Block> block);

        /**
         * @brief Processes new headers received from the network.
         * @param headers The headers to process.
         */
        void OnNewHeaders(const std::vector<std::shared_ptr<Header>>& headers);

        /**
         * @brief Processes a new transaction received from the network.
         * @param transaction The transaction to process.
         * @return The verification result.
         */
        VerifyResult OnNewTransaction(std::shared_ptr<Transaction> transaction);

        // Network module disabled - ExtensiblePayload methods commented out
        // VerifyResult OnNewExtensiblePayload(std::shared_ptr<network::p2p::payloads::ExtensiblePayload> payload);

        /**
         * @brief Imports blocks into the blockchain.
         * @param import_data The import data containing blocks and verification flag.
         * @return True if import succeeded, false otherwise.
         */
        bool ImportBlocks(const ImportData& import_data);

        /**
         * @brief Fills the memory pool with transactions for consensus.
         * @param transactions The transactions to add to the memory pool.
         */
        void FillMemoryPool(const std::vector<std::shared_ptr<Transaction>>& transactions);

        /**
         * @brief Re-verifies inventories that may have become valid.
         * @param inventories The inventories to re-verify.
         */
        // Network module disabled - IInventory methods commented out
        // void ReverifyInventories(const std::vector<std::shared_ptr<network::p2p::payloads::IInventory>>& inventories);

        // Event registration methods
        void RegisterCommittingHandler(CommittingHandler handler);
        void RegisterCommittedHandler(CommittedHandler handler);
        void RegisterBlockPersistenceHandler(BlockPersistenceHandler handler);
        void RegisterTransactionHandler(TransactionHandler handler);
        // Network module disabled - InventoryHandler registration commented out
        // void RegisterInventoryHandler(InventoryHandler handler);

        /**
         * @brief Gets the header cache.
         * @return The header cache.
         */
        // HeaderCache disabled since network module is disabled
        // std::shared_ptr<HeaderCache> GetHeaderCache() const { return header_cache_; }

        /**
         * @brief Gets the Neo system.
         * @return The Neo system.
         */
        std::shared_ptr<NeoSystem> GetSystem() const { return system_; }

    private:
        // Core processing methods
        void ProcessBlock(std::shared_ptr<Block> block);
        void PersistBlock(std::shared_ptr<Block> block);
        bool VerifyBlock(std::shared_ptr<Block> block, std::shared_ptr<persistence::DataCache> snapshot);
        void AddUnverifiedBlockToCache(std::shared_ptr<Block> block, const std::string& node_id);
        void ProcessUnverifiedBlocks(uint32_t height);
        std::vector<ApplicationExecuted> ExecuteBlockScripts(std::shared_ptr<Block> block, 
                                                             std::shared_ptr<persistence::DataCache> snapshot);
        
        // Event firing methods
        void FireCommittingEvent(std::shared_ptr<Block> block, 
                                std::shared_ptr<persistence::DataCache> snapshot,
                                const std::vector<ApplicationExecuted>& app_executed);
        void FireCommittedEvent(std::shared_ptr<Block> block);
        void FireBlockPersistedEvent(std::shared_ptr<Block> block);
        void FireTransactionEvent(std::shared_ptr<Transaction> transaction, VerifyResult result);
        // Network module disabled - IInventory methods commented out
        // void FireInventoryEvent(std::shared_ptr<network::p2p::payloads::IInventory> inventory, VerifyResult result);

        // Utility methods
        std::unordered_set<io::UInt160> UpdateExtensibleWitnessWhiteList(std::shared_ptr<persistence::DataCache> snapshot);
        bool IsGenesisBlockInitialized() const;
        void InitializeGenesisBlock();

        // Threading and synchronization
        void ProcessingThreadFunction();
        void IdleProcessingFunction();

        // Member variables
        std::shared_ptr<NeoSystem> system_;
        // std::shared_ptr<HeaderCache> header_cache_;  // Disabled since network module is disabled
        std::shared_ptr<persistence::DataCache> data_cache_;
        
        // Block caches
        std::unordered_map<io::UInt256, std::shared_ptr<Block>> block_cache_;
        std::unordered_map<uint32_t, std::shared_ptr<UnverifiedBlocksList>> block_cache_unverified_;
        
        // Extensible payload whitelist
        std::unordered_set<io::UInt160> extensible_witness_whitelist_;
        bool extensible_whitelist_cached_;
        
        // Event handlers
        std::vector<CommittingHandler> committing_handlers_;
        std::vector<CommittedHandler> committed_handlers_;
        std::vector<BlockPersistenceHandler> block_persistence_handlers_;
        std::vector<TransactionHandler> transaction_handlers_;
        // std::vector<InventoryHandler> inventory_handlers_;  // Disabled since network module is disabled
        
        // Threading
        std::atomic<bool> running_;
        std::thread processing_thread_;
        std::mutex processing_mutex_;
        std::condition_variable processing_cv_;
        std::queue<std::function<void()>> processing_queue_;
        
        // Synchronization
        mutable std::shared_mutex blockchain_mutex_;
        mutable std::mutex event_mutex_;
        
        // Configuration
        static constexpr int MaxTxToReverifyPerIdle = 10;
        static constexpr uint32_t MaxUnverifiedBlocks = 10000;
    };

    /**
     * @brief Interface for blockchain inventory items.
     */
    class IInventory
    {
    public:
        virtual ~IInventory() = default;
        virtual io::UInt256 GetHash() const = 0;
        virtual bool Verify(std::shared_ptr<config::ProtocolSettings> settings,
                           std::shared_ptr<persistence::DataCache> snapshot) const = 0;
    };

} // namespace neo::ledger
