/**
 * @file blockchain.h
 * @brief Core blockchain implementation for Neo C++
 * @details This file contains the main Blockchain class which manages
 *          block processing, transaction verification, and state management
 *          for the Neo blockchain protocol.
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

// Network payload includes
#include <neo/ledger/block.h>
#include <neo/ledger/block_header.h>
#include <neo/ledger/event_system.h>
#include <neo/ledger/header.h>
#include <neo/ledger/transaction.h>
#include <neo/network/p2p/payloads/block.h>
#include <neo/network/p2p/payloads/header.h>
#include <neo/network/p2p/payloads/header_cache.h>
// #include <neo/network/p2p/payloads/extensible_payload.h> // Disabled since network module is disabled
#include <neo/config/protocol_settings.h>
#include <neo/io/fixed8.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/verify_result.h>
#include <neo/persistence/data_cache.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/vm_types.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Forward declaration for NeoSystem
namespace neo
{
class NeoSystem;
}

/**
 * @namespace neo::ledger
 * @brief Contains blockchain ledger management components
 * @details This namespace includes all classes and functions related to
 *          blockchain state management, block processing, transaction handling,
 *          and consensus integration.
 */
namespace neo::ledger
{
// Forward declarations
class MemoryPool;

}  // namespace neo::ledger

namespace neo::network::p2p::payloads { 
    class ExtensiblePayload;
    class HeaderCache;
}

namespace neo::ledger {

// Note: Block class is defined in this namespace as neo::ledger::Block
// Note: Header type alias is defined in header.h as alias for BlockHeader

/**
 * @struct UnverifiedBlocksList
 * @brief Represents an unverified block list for a specific height
 * @details Stores blocks that have been received but not yet verified,
 *          along with the nodes that sent them for tracking purposes.
 */
struct UnverifiedBlocksList
{
    /// @brief List of unverified blocks at this height
    std::vector<std::shared_ptr<Block>> blocks;
    
    /// @brief Set of node identifiers that sent blocks at this height
    std::unordered_set<std::string> nodes;
};

/**
 * @struct ApplicationExecuted
 * @brief Event data for smart contract application execution
 * @details Contains all information about a smart contract execution,
 *          including the transaction, VM state, gas consumption, and any
 *          logs or notifications generated during execution.
 */
struct ApplicationExecuted
{
    /// @brief The transaction that triggered the execution
    std::shared_ptr<Transaction> transaction;
    
    /// @brief The application engine used for execution
    std::shared_ptr<smartcontract::ApplicationEngine> engine;
    
    /// @brief Final state of the virtual machine after execution
    smartcontract::VMState vm_state;
    
    /// @brief Total gas consumed during execution
    uint64_t gas_consumed;
    
    /// @brief Exception message if execution failed
    std::string exception_message;
    
    /// @brief Log entries generated during execution
    std::vector<smartcontract::LogEntry> logs;
    
    /// @brief Notification events generated during execution
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
 * @class Blockchain
 * @brief Enhanced blockchain processing engine - core of the Neo node
 * @details This class implements the main blockchain functionality including:
 *          - Block validation and persistence
 *          - Transaction verification and mempool management
 *          - Smart contract execution
 *          - Event emission for block and transaction processing
 *          - State management and querying
 *          - Consensus integration
 * 
 * The implementation matches the C# Blockchain.cs functionality with
 * sophisticated block processing, transaction verification, and event handling.
 * 
 * @thread_safety Thread-safe for all public methods
 * @performance Block processing optimized for parallel validation
 * @security All inputs are validated, cryptographic operations use OpenSSL
 */
class Blockchain
{
   public:
    /// @brief Handler for block committing events
    /// @details Called before a block is persisted to storage
    using CommittingHandler =
        std::function<void(std::shared_ptr<NeoSystem>, std::shared_ptr<Block>, std::shared_ptr<persistence::DataCache>,
                           const std::vector<ApplicationExecuted>&)>;
    
    /// @brief Handler for block committed events
    /// @details Called after a block has been successfully persisted
    using CommittedHandler = std::function<void(std::shared_ptr<NeoSystem>, std::shared_ptr<Block>)>;
    
    /// @brief Handler for block persistence events
    using BlockPersistenceHandler = std::function<void(std::shared_ptr<Block>)>;
    
    /// @brief Handler for transaction verification events
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

    // Event registration methods (deprecated - use static event system for C# compatibility)
    // Use BlockchainEvents::SubscribeCommitting(), BlockchainEvents::SubscribeCommitted(), etc.
    void RegisterCommittingHandler(CommittingHandler handler);
    void RegisterCommittedHandler(CommittedHandler handler);
    void RegisterBlockPersistenceHandler(BlockPersistenceHandler handler);
    void RegisterTransactionHandler(TransactionHandler handler);
    // Network module disabled - InventoryHandler registration commented out
    // void RegisterInventoryHandler(InventoryHandler handler);

  private:
    void StoreBlockInCache(const std::shared_ptr<Block>& block);
    /**
     * @brief Gets the header cache.
     * @return The header cache.
     */
    std::shared_ptr<neo::network::p2p::payloads::HeaderCache> GetHeaderCache() const { return header_cache_; }

    /**
     * @brief Gets the Neo system.
     * @return The Neo system.
     */
    std::shared_ptr<NeoSystem> GetSystem() const { return system_; }

    // ========== MISSING C# METHODS - ADDING FOR EXACT COMPATIBILITY ==========
    
    /**
     * @brief Main message processing method (C# OnReceive equivalent)
     * @param message The message to process
     */
    void ProcessMessage(const void* message);
    
    /**
     * @brief Process extensible payload (C# method)
     * @param payload The extensible payload to process
     * @return Verification result
     */
    VerifyResult OnNewExtensiblePayload(const network::p2p::payloads::ExtensiblePayload& payload);
    
    /**
     * @brief Re-verify inventories (C# method)
     * @param inventories List of inventories to re-verify
     */
    void ReverifyInventories(const std::vector<void*>& inventories);
    
    /**
     * @brief Send relay result (C# method)  
     * @param inventory The inventory that was relayed
     * @param result The relay result
     */
    void SendRelayResult(const void* inventory, VerifyResult result);
    
    /**
     * @brief Handle pre-verification completion (C# method)
     * @param transaction The transaction that completed pre-verification
     * @param result The verification result
     */
    void OnPreverifyCompleted(std::shared_ptr<Transaction> transaction, VerifyResult result);
    
    // Static event system to match C# exactly
    /**
     * @brief Global committing event (C# static event equivalent)
     */
    static void InvokeCommitting(std::shared_ptr<Block> block, 
                                std::shared_ptr<persistence::DataCache> snapshot,
                                const std::vector<ApplicationExecuted>& app_executed);
    
    /**
     * @brief Global committed event (C# static event equivalent)  
     */
    static void InvokeCommitted(std::shared_ptr<Block> block);
    
    // Actor pattern compatibility methods
    /**
     * @brief Create Props for actor system (C# method)
     * @param system The Neo system
     * @return Actor props for blockchain
     */
    static void* CreateProps(std::shared_ptr<NeoSystem> system);
    
    /**
     * @brief Get best block hash (C# property equivalent)
     * @return The best block hash
     */
    io::UInt256 GetBestBlockHash() const { return GetCurrentBlockHash(); }

   private:
    /**
     * @brief Processes a block through the validation and persistence pipeline
     * @param block The block to process
     * @thread_safety Protected by blockchain_mutex_
     * @performance Optimized for parallel transaction validation
     */
    void ProcessBlock(std::shared_ptr<Block> block);
    /**
     * @brief Persists a validated block to storage
     * @param block The block to persist
     * @pre Block must be validated
     * @post Block is persisted and events are emitted
     */
    void PersistBlock(std::shared_ptr<Block> block);
    /**
     * @brief Verifies a block against consensus rules
     * @param block The block to verify
     * @param snapshot The data cache snapshot for validation
     * @return true if block is valid, false otherwise
     * @complexity O(n) where n is number of transactions
     */
    bool VerifyBlock(std::shared_ptr<Block> block, std::shared_ptr<persistence::DataCache> snapshot);
    void AddUnverifiedBlockToCache(std::shared_ptr<Block> block, const std::string& node_id);
    void ProcessUnverifiedBlocks(uint32_t height);
    std::vector<ApplicationExecuted> ExecuteBlockScripts(std::shared_ptr<Block> block,
                                                         std::shared_ptr<persistence::DataCache> snapshot);

    // Event firing methods (updated to use static event system for C# compatibility)
    void FireCommittingEvent(std::shared_ptr<Block> block, std::shared_ptr<persistence::DataCache> snapshot,
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
    /// @brief Reference to the Neo system instance
    std::shared_ptr<NeoSystem> system_;
    
    /// @brief Cache for block headers
    std::shared_ptr<neo::network::p2p::payloads::HeaderCache> header_cache_;
    mutable std::unordered_map<io::UInt256, std::shared_ptr<BlockHeader>> header_cache_by_hash_;
    mutable std::unordered_map<uint32_t, io::UInt256> header_hash_by_index_;
    
    /// @brief Main data cache for blockchain state
    std::shared_ptr<persistence::DataCache> data_cache_;

    // Block caches
    /// @brief Cache of recently accessed blocks indexed by hash
    std::unordered_map<io::UInt256, std::shared_ptr<Block>> block_cache_;
    
    /// @brief Cache of unverified blocks indexed by height
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
    /// @brief Flag indicating if blockchain processing is active
    std::atomic<bool> running_;
    
    /// @brief Main processing thread for block validation
    std::thread processing_thread_;
    
    /// @brief Mutex for processing queue synchronization
    std::mutex processing_mutex_;
    
    /// @brief Condition variable for processing thread signaling
    std::condition_variable processing_cv_;
    
    /// @brief Queue of processing tasks to execute
    std::queue<std::function<void()>> processing_queue_;

    // Synchronization
    /// @brief Main mutex for blockchain state protection (reader-writer lock)
    mutable std::shared_mutex blockchain_mutex_;
    
    /// @brief Mutex for event handler list protection
    mutable std::mutex event_mutex_;

    // Configuration
    /// @brief Maximum transactions to re-verify during idle processing
    static constexpr int MaxTxToReverifyPerIdle = 10;
    
    /// @brief Maximum number of unverified blocks to cache
    static constexpr uint32_t MaxUnverifiedBlocks = 10000;
};

}  // namespace neo::ledger
