/**
 * @file neo_system.h
 * @brief Main Neo system coordinator
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/common/contains_transaction_type.h>
#include <neo/protocol_settings.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/memory_pool.h>
#include <neo/persistence/data_cache.h>
#include <neo/persistence/store_cache.h>
#include <neo/network/p2p/channels_config.h>
#include <neo/network/p2p/network_synchronizer.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/smartcontract/native/native_contract.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/role_management.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace neo::ledger
{
// Forward declarations
class MemoryPool;
class Blockchain;
}  // namespace neo::ledger

namespace neo::network::p2p
{
class LocalNode;
}

namespace neo::smartcontract::native
{
class LedgerContract;
class NeoToken;
class GasToken;
class RoleManagement;
}

namespace neo::persistence
{
class IStore;
class StoreCache;
}  // namespace neo::persistence

namespace neo
{
class NeoSystem;
}

namespace neo::ledger
{
/**
 * @brief Core Neo system class that manages blockchain, mempool, and network components.
 *
 * ## Overview
 * The NeoSystem class is the central coordinator for all Neo blockchain operations.
 * It manages the blockchain state, transaction pool, network connections, and
 * provides access to native contracts and protocol settings.
 *
 * ## API Reference
 * - **Blockchain Access**: GetBlockchain(), GetStoreView()
 * - **Memory Pool**: GetMemoryPool()
 * - **Network**: GetLocalNode()
 * - **Settings**: GetSettings()
 * - **Native Contracts**: GetLedgerContract()
 *
 * ## Usage Examples
 * ```cpp
 * auto system = std::make_shared<NeoSystem>(settings);
 * auto blockchain = system->GetBlockchain();
 * auto mempool = system->GetMemoryPool();
 * auto snapshot = system->GetStoreView();
 * ```
 */
class NeoSystem : public std::enable_shared_from_this<NeoSystem>
{
   public:
    /**
     * @brief Constructs a new NeoSystem with the specified settings.
     * @param settings Protocol settings for the Neo network
     */
    explicit NeoSystem(std::shared_ptr<neo::ProtocolSettings> settings);
    NeoSystem(std::shared_ptr<neo::ProtocolSettings> settings, std::string storage_provider,
              std::string storage_path);

    /**
     * @brief Destructor
     */
    ~NeoSystem();

    /**
     * @brief Gets the blockchain instance.
     * @return Shared pointer to the blockchain
     */
    std::shared_ptr<Blockchain> GetBlockchain() const;

    /**
     * @brief Gets the memory pool instance.
     * @return Shared pointer to the memory pool
     */
    std::shared_ptr<MemoryPool> GetMemoryPool() const;

    /**
     * @brief Gets the local network node.
     * @return Shared pointer to the local node
     */
    std::shared_ptr<network::p2p::LocalNode> GetLocalNode() const;

    /**
     * @brief Gets the network synchronizer component.
     */
    std::shared_ptr<network::p2p::NetworkSynchronizer> GetNetworkSynchronizer() const { return networkSynchronizer_; }

    /**
     * @brief Gets the protocol settings.
     * @return Shared pointer to the protocol settings
     */
    std::shared_ptr<neo::ProtocolSettings> GetSettings() const;

    /**
     * @brief Gets a snapshot of the current store state.
     * @return Shared pointer to the data cache snapshot
     */
    std::shared_ptr<persistence::DataCache> GetStoreView() const;

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
     * @brief Gets the role management contract instance.
     * @return Shared pointer to the role management contract
     */
    std::shared_ptr<smartcontract::native::RoleManagement> GetRoleManagement() const;

    /**
     * @brief Starts the Neo system.
     */
    void Start();

    /**
     * @brief Stops the Neo system.
     */
    void Stop();

    /**
     * @brief Checks if the system is running.
     * @return True if running, false otherwise
     */
    bool IsRunning() const;

    /**
     * @brief Disposes of system resources.
     */
    void Dispose();

    /**
     * @brief Sets the P2P network configuration used when starting the local node.
     * @param config Channels configuration describing listening ports and limits.
     */
    void SetNetworkConfig(const network::p2p::ChannelsConfig& config);

    /**
     * @brief Gets the genesis block.
     * @return Shared pointer to the genesis block
     */
    std::shared_ptr<Block> GetGenesisBlock() const;

    /**
     * @brief Gets a native contract by script hash.
     * @param hash The script hash of the contract
     * @return Pointer to the native contract or nullptr if not found
     */
    smartcontract::native::NativeContract* GetNativeContract(const io::UInt160& hash) const;

    /**
     * @brief Gets all registered native contracts.
     * @return Vector of native contracts
     */
    std::vector<std::shared_ptr<smartcontract::native::NativeContract>> GetNativeContracts() const;

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
    std::shared_ptr<neo::ProtocolSettings> settings_;
    std::string storage_provider_;
    std::string storage_path_;
    std::shared_ptr<neo::NeoSystem> core_system_;
    std::shared_ptr<Blockchain> blockchain_;
    std::shared_ptr<MemoryPool> memory_pool_;
    std::shared_ptr<network::p2p::LocalNode> local_node_;
    std::shared_ptr<network::p2p::NetworkSynchronizer> networkSynchronizer_;
    mutable std::shared_ptr<smartcontract::native::LedgerContract> ledger_contract_;
    bool is_running_;
    bool is_disposed_;
    std::optional<network::p2p::ChannelsConfig> channels_config_;

    std::string NormalizeProviderName(const std::string& provider) const;
};

}  // namespace neo::ledger
