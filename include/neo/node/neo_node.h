/**
 * @file neo_node.h
 * @brief Neo Node
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>

// Neo Core Components
#include <neo/consensus/consensus_service.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/logging/logger.h>
#include <neo/network/p2p_server.h>
#include <neo/network/peer_discovery_service.h>
#include <neo/persistence/rocksdb_store.h>
#include <neo/protocol_settings.h>
#include <neo/rpc/rpc_server.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/policy_contract.h>
#include <neo/smartcontract/native/role_management.h>

namespace neo::node
{
/**
 * @brief Production-ready Neo Node Implementation
 *
 * This class encapsulates the complete Neo blockchain node functionality,
 * providing identical behavior to the C# Neo node implementation.
 */
class NeoNode
{
   private:
    // Core configuration
    std::shared_ptr<ProtocolSettings> protocolSettings_;
    std::string configPath_;
    std::string dataPath_;

    // Core blockchain components
    std::shared_ptr<persistence::LevelDBStore> store_;
    std::shared_ptr<ledger::Blockchain> blockchain_;
    std::shared_ptr<ledger::MemoryPool> memoryPool_;

    // Network layer
    std::shared_ptr<network::P2PServer> p2pServer_;
    std::shared_ptr<network::PeerDiscoveryService> peerDiscovery_;

    // Smart contract system
    std::shared_ptr<smartcontract::ApplicationEngine> applicationEngine_;

    // Native contracts
    std::shared_ptr<smartcontract::native::GasToken> gasToken_;
    std::shared_ptr<smartcontract::native::NeoToken> neoToken_;
    std::shared_ptr<smartcontract::native::PolicyContract> policyContract_;
    std::shared_ptr<smartcontract::native::RoleManagement> roleManagement_;

    // RPC and API
    std::shared_ptr<rpc::RpcServer> rpcServer_;

    // Consensus
    std::shared_ptr<consensus::ConsensusService> consensusService_;

    // Runtime state
    std::atomic<bool> running_;
    std::atomic<bool> shutdownRequested_;
    std::thread mainThread_;

    // Logging
    std::shared_ptr<logging::Logger> logger_;

   public:
    /**
     * @brief Constructor
     * @param configPath Path to configuration file
     * @param dataPath Path to blockchain data directory
     */
    NeoNode(const std::string& configPath = "config.json", const std::string& dataPath = "./data");

    /**
     * @brief Destructor - ensures clean shutdown
     */
    ~NeoNode();

    /**
     * @brief Initialize the Neo node
     * @return true if initialization successful, false otherwise
     */
    bool Initialize();

    /**
     * @brief Start the Neo node
     * @return true if started successfully, false otherwise
     */
    bool Start();

    /**
     * @brief Stop the Neo node gracefully
     */
    void Stop();

    /**
     * @brief Check if the node is running
     * @return true if running, false otherwise
     */
    bool IsRunning() const;

    /**
     * @brief Get the current blockchain height
     * @return Current block height
     */
    uint32_t GetBlockHeight() const;

    /**
     * @brief Get the number of connected peers
     * @return Number of connected peers
     */
    size_t GetConnectedPeersCount() const;

    /**
     * @brief Get memory pool transaction count
     * @return Number of transactions in memory pool
     */
    size_t GetMemoryPoolCount() const;

   private:
    // Initialization methods (implemented in neo_node_initialization.cpp)
    void InitializeLogging();
    bool LoadProtocolSettings();
    bool InitializeStorage();
    bool InitializeBlockchain();
    bool InitializeSmartContracts();
    bool InitializeNetwork();
    bool InitializeRPC();
    bool InitializeConsensus();

    // Processing methods (implemented in neo_node_processing.cpp)
    void MainLoop();
    void ProcessBlockchain();
    void ProcessMemoryPool();
    void ProcessNetwork();
    void ReportStatus();
    size_t GetMemoryUsage() const;
};
}  // namespace neo::node