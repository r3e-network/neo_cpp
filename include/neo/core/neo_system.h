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

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <future>
#include <unordered_set>

// Forward declarations
namespace neo {
    class ProtocolSettings;
    class Block;
    class MemoryPool;
    class HeaderCache;
    class RelayCache;
    namespace persistence {
        class StoreCache;
        class IStore;
        class IStoreProvider;
    }
    class UInt256;
    class UInt160;
    enum class ContainsTransactionType;
    
    namespace network {
        namespace p2p {
            class LocalNode;
            class TaskManager;
            class ChannelsConfig;
        }
    }
    
    namespace ledger {
        class Blockchain;
    }
    
    namespace plugins {
        class Plugin;
    }
}

namespace neo {

/**
 * @brief Represents the basic unit that contains all the components required for running of a NEO node.
 * 
 * The NeoSystem class is the main orchestrator for a Neo node, managing all core components
 * including the blockchain, network layer, memory pool, and plugin system.
 */
class NeoSystem {
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
    
    // Threading
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> shutdown_requested_{false};

public:
    /**
     * @brief Constructs a NeoSystem with the specified settings and storage provider.
     * @param settings The protocol settings for this Neo system
     * @param storage_provider_name The name of the storage provider to use (default: "memory")
     * @param storage_path The path for persistent storage (ignored for memory provider)
     */
    explicit NeoSystem(std::unique_ptr<ProtocolSettings> settings,
                      const std::string& storage_provider_name = "memory",
                      const std::string& storage_path = "");

    /**
     * @brief Constructs a NeoSystem with the specified settings and storage provider.
     * @param settings The protocol settings for this Neo system
     * @param storage_provider The storage provider to use
     * @param storage_path The path for persistent storage
     */
    NeoSystem(std::unique_ptr<ProtocolSettings> settings,
              std::shared_ptr<persistence::IStoreProvider> storage_provider,
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
    template<typename T>
    std::shared_ptr<T> get_service(std::function<bool(const T&)> filter = nullptr) const {
        std::lock_guard<std::mutex> lock(services_mutex_);
        for (const auto& service : services_) {
            if (auto typed_service = std::dynamic_pointer_cast<T>(service)) {
                if (!filter || filter(*typed_service)) {
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

    // Transaction operations
    /**
     * @brief Determines whether the specified transaction exists in the memory pool or storage.
     * @param hash The hash of the transaction
     * @return The type indicating where the transaction exists
     */
    ContainsTransactionType contains_transaction(const UInt256& hash) const;

    /**
     * @brief Determines whether the specified transaction conflicts with some on-chain transaction.
     * @param hash The hash of the transaction
     * @param signers The list of signer accounts of the transaction
     * @return true if the transaction conflicts with on-chain transaction, false otherwise
     */
    bool contains_conflict_hash(const UInt256& hash, const std::vector<UInt160>& signers) const;

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
    static Block* create_genesis_block(const ProtocolSettings& settings);

    /**
     * @brief Initializes the global plugin system.
     */
    static void initialize_plugins();

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
};

} // namespace neo

#endif // NEO_CORE_NEO_SYSTEM_H