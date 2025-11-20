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
#include <neo/network/p2p/channels_config.h>
#include <neo/network/p2p/local_node.h>
#include <neo/node/neo_system.h>
#include <neo/protocol_settings.h>
#include <neo/rpc/rpc_server.h>
#include <neo/settings.h>

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
    std::string configPath_;
    std::string dataPath_;
    Settings settings_;
    std::shared_ptr<ProtocolSettings> protocolSettings_;
    network::p2p::ChannelsConfig networkConfig_;

    // Core blockchain components
    std::shared_ptr<ledger::Blockchain> blockchain_;
    std::shared_ptr<ledger::MemoryPool> memoryPool_;

    // System wrapper
    std::shared_ptr<NeoSystem> neoSystem_;
    std::shared_ptr<network::p2p::LocalNode> localNode_;

    // RPC and API
    std::shared_ptr<rpc::RpcServer> rpcServer_;

    // Consensus
    std::shared_ptr<consensus::ConsensusService> consensusService_;
    bool consensusAutoStart_{false};

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
     * @brief Get the current header height
     * @return Current header height
     */
    uint32_t GetHeaderHeight() const;

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

    /**
     * @brief Get the consensus service instance (may be null)
     * @return Consensus service shared pointer
     */
    std::shared_ptr<consensus::ConsensusService> GetConsensusService() const { return consensusService_; }

    /**
     * @brief Manually start consensus when auto-start is disabled.
     * @return true if consensus successfully starts.
     */
    bool StartConsensusManually();

    /**
     * @brief Restart consensus service.
     * @return true if restart succeeds.
     */
    bool RestartConsensus();

    /**
     * @brief Indicates whether consensus is configured to auto-start.
     */
    bool IsConsensusAutoStartEnabled() const { return consensusAutoStart_; }

   private:
    // Initialization methods (implemented in neo_node_initialization.cpp)
    void InitializeLogging();
    bool LoadSettings();
    bool InitializeNeoSystem();
    bool InitializeNetwork();
    bool InitializeRpcServer();
    bool InitializeConsensus();

    // Processing methods (implemented in neo_node_processing.cpp)
    void MainLoop();
    void ReportStatus();
    size_t GetMemoryUsage() const;
};
}  // namespace neo::node
