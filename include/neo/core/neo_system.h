// Copyright (C) 2015-2025 The Neo Project.
//
// neo_system.h file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef NEO_CORE_NEO_SYSTEM_H
#define NEO_CORE_NEO_SYSTEM_H

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

// Include necessary headers for complete types
#include <neo/ledger/header_cache.h>
#include <neo/ledger/memory_pool.h>

// Forward declarations
namespace neo
{
class ProtocolSettings;
class Block;
class RelayCache;
namespace persistence
{
class StoreCache;
class IStore;
class IStoreProvider;
}  // namespace persistence
class UInt256;
class UInt160;
enum class ContainsTransactionType;

namespace network
{
namespace p2p
{
class LocalNode;
class TaskManager;
class ChannelsConfig;
}  // namespace p2p
}  // namespace network

namespace cryptography
{
namespace ecc
{
class ECPoint;
}
}  // namespace cryptography

namespace ledger
{
class Blockchain;
class Block;
}  // namespace ledger

namespace plugins
{
class Plugin;
}

namespace smartcontract::native
{
class LedgerContract;
class NeoToken;
class GasToken;
class RoleManagement;
class NativeContract;
}  // namespace smartcontract::native

namespace io
{
class UInt160;
class UInt256;
}  // namespace io
}  // namespace neo

namespace neo
{

/**
 * @brief Represents the basic unit that contains all the components required for running of a NEO node.
 *
 * The NeoSystem class is the main orchestrator for a Neo node, managing all core components
 * including the blockchain, network layer, memory pool, and plugin system.
 */
class NeoSystem : public std::enable_shared_from_this<NeoSystem>
{
    // Friend class for factory initialization
    friend class NeoSystemInit;

   public:
    /**
     * @brief Event handler type for service addition events.
     */
    using ServiceAddedHandler = std::function<void(const std::shared_ptr<void>&)>;

   private:
    std::unique_ptr<ProtocolSettings> settings_;
    std::unique_ptr<persistence::IStore> store_;
    std::shared_ptr<persistence::IStoreProvider> storage_provider_;
    std::unique_ptr<RelayCache> relay_cache_;

    // Service management
    std::vector<std::shared_ptr<void>> services_;
    mutable std::mutex services_mutex_;
    std::vector<ServiceAddedHandler> service_added_handlers_;

    // Node startup control
    std::atomic<int> suspend_count_{0};
    std::mutex start_message_mutex_;
    std::unique_ptr<network::p2p::ChannelsConfig> start_message_;

    // Threading
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> shutdown_requested_{false};

    // Performance optimization flags
    std::atomic<bool> fastSyncMode_{false};  // Disabled by default; tests enable explicitly when needed
    // Ephemeral height used only in fast sync mode to avoid store I/O
    std::atomic<uint32_t> fastSyncEphemeralHeight_{0};

   public:
    /**
     * @brief Constructs a NeoSystem with the specified settings and storage provider.
     * @param settings The protocol settings for this Neo system
     * @param storage_provider_name The name of the storage provider to use (default: "memory")
     * @param storage_path The path for persistent storage (ignored for memory provider)
     */
    explicit NeoSystem(std::unique_ptr<ProtocolSettings> settings, const std::string& storage_provider_name = "memory",
                       const std::string& storage_path = "");

    /**
     * @brief Constructs a NeoSystem with the specified settings and storage provider.
     * @param settings The protocol settings for this Neo system
     * @param storage_provider The storage provider to use
     * @param storage_path The path for persistent storage
     */
    NeoSystem(std::unique_ptr<ProtocolSettings> settings, std::shared_ptr<persistence::IStoreProvider> storage_provider,
              const std::string& storage_path = "");

    /**
     * @brief Destructor - ensures proper cleanup of all components.
     */
    ~NeoSystem();

    // Non-copyable and non-movable
    NeoSystem(const NeoSystem&) = delete;
    NeoSystem& operator=(const NeoSystem&) = delete;
    NeoSystem(NeoSystem&&) = delete;
    NeoSystem& operator=(NeoSystem&&) = delete;

    // Accessors
    /**
     * @brief Gets the protocol settings of this Neo system.
     * @return Reference to the protocol settings
     */
    const ProtocolSettings& settings() const { return *settings_; }

    /**
     * @brief Gets a readonly view of the store.
     * @return A store cache for read-only operations
     */
    std::unique_ptr<persistence::StoreCache> store_view() const;

    /**
     * @brief Initializes the plugin system after construction is complete.
     * This must be called after the NeoSystem is fully constructed and shared_ptr is available.
     */
    void load_plugins();

    // Service management
    /**
     * @brief Adds a service to the Neo system.
     * @param service The service object to add
     */
    void add_service(std::shared_ptr<void> service);

    /**
     * @brief Gets a service of the specified type.
     * @tparam T The type of service to retrieve
     * @param filter Optional filter function to select specific service
     * @return Shared pointer to the service, or nullptr if not found
     */
    template <typename T>
    std::shared_ptr<T> get_service(std::function<bool(const T&)> filter = nullptr) const
    {
        std::lock_guard<std::mutex> lock(services_mutex_);
        for (const auto& service : services_)
        {
            if (auto typed_service = std::dynamic_pointer_cast<T>(service))
            {
                if (!filter || filter(*typed_service))
                {
                    return typed_service;
                }
            }
        }
        return nullptr;
    }

    /**
     * @brief Registers a handler for service addition events.
     * @param handler The handler function to register
     */
    void on_service_added(ServiceAddedHandler handler);

    // Node lifecycle management
    /**
     * @brief Starts the local node with the specified configuration.
     * @param config The channels configuration for the node
     */
    void start_node(std::unique_ptr<network::p2p::ChannelsConfig> config);

    /**
     * @brief Suspends the startup process of the local node.
     */
    void suspend_node_startup();

    /**
     * @brief Resumes the startup process of the local node.
     * @return true if the startup process was resumed, false otherwise
     */
    bool resume_node_startup();

    /**
     * @brief Stops the Neo system and all its components.
     */
    void stop();

    // Storage operations
    /**
     * @brief Loads a store at the specified path.
     * @param path The path of the storage
     * @return Unique pointer to the loaded store
     */
    std::unique_ptr<persistence::IStore> load_store(const std::string& path);

    /**
     * @brief Gets a snapshot of the blockchain storage with an execution cache.
     * @return A store cache for read/write operations
     */
    std::unique_ptr<persistence::StoreCache> get_snapshot_cache();

    /**
     * @brief Gets the underlying store.
     * @return Pointer to the store
     */
    persistence::IStore* GetStore() const { return store_.get(); }

    /**
     * @brief Gets the memory pool.
     * @return Pointer to the memory pool
     */
    ledger::MemoryPool* GetMemPool() const { return mem_pool_.get(); }

    /**
     * @brief Gets the current block height.
     * @return The current block height, or 0 if blockchain is not initialized
     */
    uint32_t GetCurrentBlockHeight() const;

    /**
     * @brief Processes a new block received from the network.
     * @param block The block to process
     * @return true if the block was successfully processed, false otherwise
     */
    bool ProcessBlock(const std::shared_ptr<ledger::Block>& block);

    /**
     * @brief Processes multiple blocks in a batch for high-performance synchronization
     * @param blocks The blocks to process
     * @return Number of blocks successfully processed
     */
    size_t ProcessBlocksBatch(const std::vector<std::shared_ptr<ledger::Block>>& blocks);

    /**
     * @brief Enables/disables fast sync mode (skips validation for initial sync)
     * @param enabled True to enable fast sync, false for full validation
     */
    void SetFastSyncMode(bool enabled) { fastSyncMode_ = enabled; }

    // Transaction operations
    /**
     * @brief Determines whether the specified transaction exists in the memory pool or storage.
     * @param hash The hash of the transaction
     * @return The type indicating where the transaction exists
     */
    ContainsTransactionType contains_transaction(const io::UInt256& hash) const;

    /**
     * @brief Determines whether the specified transaction conflicts with some on-chain transaction.
     * @param hash The hash of the transaction
     * @param signers The list of signer accounts of the transaction
     * @return true if the transaction conflicts with on-chain transaction, false otherwise
     */
    bool contains_conflict_hash(const io::UInt256& hash, const std::vector<io::UInt160>& signers) const;

    // Static factory methods
    /**
     * @brief Creates the genesis block for the Neo blockchain.
     * @param settings The protocol settings of the Neo system
     * @return Pointer to the genesis block with complete initialization
     *
     * The genesis block includes:
     * - Proper block header with correct hash calculations
     * - Initial native contract deployments (NEO, GAS, Policy, etc.)
     * - Committee and validator initialization
     * - Initial GAS distribution transactions
     * - Proper witness and signature setup
     */
    static ledger::Block* create_genesis_block(const ProtocolSettings& settings);

    /**
     * @brief Initializes the global plugin system.
     */
    static void initialize_plugins();

    // Native contract access methods
    /**
     * @brief Gets the ledger contract instance.
     * @return Shared pointer to the ledger contract
     */
    std::shared_ptr<smartcontract::native::LedgerContract> GetLedgerContract() const;

    /**
     * @brief Gets the NEO token contract instance.
     * @return Shared pointer to the NEO token contract
     */
    std::shared_ptr<smartcontract::native::NeoToken> GetNeoToken() const;

    /**
     * @brief Gets the GAS token contract instance.
     * @return Shared pointer to the GAS token contract
     */
    std::shared_ptr<smartcontract::native::GasToken> GetGasToken() const;

    /**
     * @brief Gets the blockchain instance.
     * @return Pointer to the blockchain
     */
    ledger::Blockchain* GetBlockchain() const;

    /**
     * @brief Gets the role management contract instance.
     * @return Shared pointer to the role management contract
     */
    std::shared_ptr<smartcontract::native::RoleManagement> GetRoleManagement() const;

    /**
     * @brief Gets the genesis block.
     * @return Shared pointer to the genesis block
     */
    std::shared_ptr<ledger::Block> GetGenesisBlock() const;

    /**
     * @brief Gets a native contract by script hash.
     * @param hash The script hash of the contract
     * @return Pointer to the native contract or nullptr if not found
     */
    smartcontract::native::NativeContract* GetNativeContract(const io::UInt160& hash) const;

    /**
     * @brief Gets the maximum number of traceable blocks.
     * @return Maximum traceable blocks
     */
    uint32_t GetMaxTraceableBlocks() const;

    /**
     * @brief Gets a snapshot of the current state.
     * @return Shared pointer to the data cache snapshot
     */
    std::shared_ptr<persistence::DataCache> GetSnapshot() const;

    /**
     * @brief Gets the memory pool.
     * @return Shared pointer to the memory pool
     */
    std::shared_ptr<ledger::MemoryPool> GetMemoryPool() const;

    /**
     * @brief Gets the protocol settings.
     * @return Shared pointer to the protocol settings
     */
    std::shared_ptr<ProtocolSettings> GetSettings() const;

    /**
     * @brief Checks if the system contains a transaction.
     * @param hash The transaction hash
     * @return The transaction containment status
     */
    ContainsTransactionType ContainsTransaction(const io::UInt256& hash) const;

    /**
     * @brief Checks if the system contains a conflict hash.
     * @param hash The transaction hash
     * @param signers The transaction signers
     * @return True if conflict exists, false otherwise
     */
    bool ContainsConflictHash(const io::UInt256& hash, const std::vector<io::UInt160>& signers) const;

   private:
    /**
     * @brief Initializes all core components.
     */
    void initialize_components();

    /**
     * @brief Starts background worker threads.
     */
    void start_worker_threads();

    /**
     * @brief Stops all background worker threads.
     */
    void stop_worker_threads();

    /**
     * @brief Ensures a component is properly stopped.
     * @param component_name Name of the component for logging
     * @param stop_function Function to call to stop the component
     */
    void ensure_stopped(const std::string& component_name, std::function<void()> stop_function);

    /**
     * @brief Handles unhandled exceptions in the system.
     * @param exception The exception that occurred
     */
    static void handle_unhandled_exception(const std::exception& exception);

    /**
     * @brief Calculates multi-signature address for validator set
     * @param validators The validator public keys
     * @return The consensus address for the validators
     */
    static UInt160 CalculateNextConsensus(const std::vector<cryptography::ecc::ECPoint>& validators);

    /**
     * @brief Calculates merkle root for transaction hashes
     * @param transactionHashes The transaction hashes
     * @return The merkle root hash
     */
    static UInt256 CalculateMerkleRoot(const std::vector<UInt256>& transactionHashes);

    // Additional member variables that were missing
    std::unique_ptr<ledger::Blockchain> blockchain_;
    std::unique_ptr<ledger::MemoryPool> mem_pool_;
    std::unique_ptr<ledger::HeaderCache> header_cache_;
    std::unique_ptr<network::p2p::LocalNode> local_node_;
    std::unique_ptr<network::p2p::TaskManager> task_manager_;
    ledger::Block* genesis_block_ = nullptr;
};

}  // namespace neo

#endif  // NEO_CORE_NEO_SYSTEM_H