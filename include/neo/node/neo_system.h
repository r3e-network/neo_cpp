/**
 * @file neo_system.h
 * @brief Main Neo system coordinator
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/network/p2p_server.h>
#include <neo/persistence/data_cache.h>
#include <neo/protocol_settings.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/native_contract.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace neo::node
{
/**
 * @brief Represents the Neo system that manages all core blockchain components.
 *
 * This class is the main entry point for the Neo blockchain system,
 * coordinating between blockchain, network, persistence, and smart contract layers.
 */
class NeoSystem
{
   public:
    /**
     * @brief Constructs a NeoSystem with the given settings.
     * @param protocolSettings The protocol settings
     * @param storageEngine The storage engine name (e.g., "LevelDB", "RocksDB")
     * @param storePath The path for blockchain data storage
     */
    NeoSystem(std::shared_ptr<ProtocolSettings> protocolSettings, const std::string& storageEngine = "LevelDB",
              const std::string& storePath = "./data");

    /**
     * @brief Destructor
     */
    ~NeoSystem();

    /**
     * @brief Starts the Neo system
     * @return true if started successfully, false otherwise
     */
    bool Start();

    /**
     * @brief Stops the Neo system
     */
    void Stop();

    /**
     * @brief Checks if the system is running
     * @return true if running, false otherwise
     */
    bool IsRunning() const;

    /**
     * @brief Gets the protocol settings
     * @return The protocol settings
     */
    std::shared_ptr<ProtocolSettings> GetProtocolSettings() const;

    /**
     * @brief Gets the blockchain instance
     * @return The blockchain
     */
    std::shared_ptr<ledger::Blockchain> GetBlockchain() const;

    /**
     * @brief Gets the memory pool instance
     * @return The memory pool
     */
    std::shared_ptr<ledger::MemoryPool> GetMemoryPool() const;

    /**
     * @brief Gets the memory pool instance (alias for GetMemoryPool).
     * @return The memory pool
     */
    std::shared_ptr<ledger::MemoryPool> GetMemPool() const { return GetMemoryPool(); }

    /**
     * @brief Gets the local P2P node instance.
     * @return The local P2P node
     */
    std::shared_ptr<network::P2PServer> GetLocalNode() const { return GetP2PServer(); }

    /**
     * @brief Gets the P2P server instance
     * @return The P2P server
     */
    std::shared_ptr<network::P2PServer> GetP2PServer() const;

    /**
     * @brief Gets the data cache instance
     * @return The data cache
     */
    std::shared_ptr<persistence::DataCache> GetDataCache() const;

    /**
     * @brief Gets a snapshot of the data cache (alias for GetDataCache).
     * @return The data cache snapshot
     */
    std::shared_ptr<persistence::DataCache> GetSnapshot() const { return GetDataCache(); }

    /**
     * @brief Creates an application engine for smart contract execution
     * @param trigger The trigger type
     * @param container The script container (usually a transaction)
     * @param persistingBlock The block being persisted
     * @param gas The gas limit
     * @return The application engine
     */
    std::unique_ptr<smartcontract::ApplicationEngine> CreateApplicationEngine(
        smartcontract::TriggerType trigger, const io::ISerializable* container,
        const ledger::Block* persistingBlock = nullptr, int64_t gas = smartcontract::ApplicationEngine::TestModeGas);

    /**
     * @brief Registers a native contract
     * @param contract The native contract to register
     */
    void RegisterNativeContract(std::shared_ptr<smartcontract::native::NativeContract> contract);

    /**
     * @brief Gets a native contract by hash
     * @param hash The contract hash
     * @return The native contract, or nullptr if not found
     */
    smartcontract::native::NativeContract* GetNativeContract(const io::UInt160& hash) const;

    /**
     * @brief Gets all registered native contracts
     * @return Vector of all native contracts
     */
    std::vector<std::shared_ptr<smartcontract::native::NativeContract>> GetNativeContracts() const;

    /**
     * @brief Gets the current block height
     * @return The current block height
     */
    uint32_t GetCurrentBlockHeight() const;

    /**
     * @brief Gets the current block index (alias for GetCurrentBlockHeight).
     * @return The current block index
     */
    uint32_t GetCurrentBlockIndex() const { return GetCurrentBlockHeight(); }

    /**
     * @brief Gets the current block hash
     * @return The current block hash
     */
    io::UInt256 GetCurrentBlockHash() const;

    /**
     * @brief Validates and relays a transaction
     * @param transaction The transaction to relay
     * @return true if transaction was accepted, false otherwise
     */
    bool RelayTransaction(std::shared_ptr<ledger::Transaction> transaction);

    /**
     * @brief Validates and relays a block
     * @param block The block to relay
     * @return true if block was accepted, false otherwise
     */
    bool RelayBlock(std::shared_ptr<ledger::Block> block);

    /**
     * @brief Registers a callback for when a new block is persisted
     * @param callback The callback function
     * @return Callback ID for later removal
     */
    int32_t RegisterBlockPersistCallback(std::function<void(std::shared_ptr<ledger::Block>)> callback);

    /**
     * @brief Unregisters a block persist callback
     * @param callbackId The callback ID to remove
     */
    void UnregisterBlockPersistCallback(int32_t callbackId);

    /**
     * @brief Gets system statistics
     * @return JSON object containing system statistics
     */
    std::string GetSystemStats() const;

   private:
    // Core components
    std::shared_ptr<ProtocolSettings> protocolSettings_;
    std::shared_ptr<persistence::DataCache> dataCache_;
    std::shared_ptr<ledger::Blockchain> blockchain_;
    std::shared_ptr<ledger::MemoryPool> memoryPool_;
    std::shared_ptr<network::P2PServer> p2pServer_;

    // Native contracts
    std::vector<std::shared_ptr<smartcontract::native::NativeContract>> nativeContracts_;
    std::unordered_map<io::UInt160, smartcontract::native::NativeContract*> nativeContractMap_;

    // System state
    bool running_;
    std::string storageEngine_;
    std::string storePath_;

    // Callbacks
    std::unordered_map<int32_t, std::function<void(std::shared_ptr<ledger::Block>)>> blockPersistCallbacks_;
    int32_t nextCallbackId_;
    mutable std::mutex callbackMutex_;

    // Initialization methods
    bool InitializeStorage();
    bool InitializeBlockchain();
    bool InitializeMemoryPool();
    bool InitializeNetworking();
    bool InitializeNativeContracts();

    // Cleanup methods
    void CleanupStorage();
    void CleanupNetworking();
    void CleanupNativeContracts();
};
}  // namespace neo::node