#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "neo/consensus/consensus_service.h"
#include "neo/ledger/blockchain.h"
#include "neo/ledger/mempool.h"
#include "neo/network/p2p_server.h"
#include "neo/node/neo_system.h"
#include "neo/persistence/leveldb_store.h"
#include "neo/persistence/memory_store.h"
#include "neo/rpc/rpc_server.h"
#include "tests/utils/test_helpers.h"

namespace neo::tests::integration
{

/**
 * @brief Configuration for integration test environment
 */
struct IntegrationTestConfig
{
    // Network configuration
    std::string network_address = "127.0.0.1";
    uint16_t base_port = 30333;
    int max_nodes = 10;
    std::chrono::seconds startup_timeout = std::chrono::seconds(30);
    std::chrono::seconds shutdown_timeout = std::chrono::seconds(10);

    // Blockchain configuration
    bool use_memory_store = true;
    std::string data_directory = "./test_data";
    uint32_t genesis_block_time = 1468595301;  // Neo genesis time

    // Consensus configuration
    std::chrono::milliseconds block_time = std::chrono::milliseconds(15000);
    int validators_count = 4;

    // RPC configuration
    bool enable_rpc = false;
    uint16_t rpc_port = 40332;
    std::string rpc_username = "test";
    std::string rpc_password = "test";

    // Test configuration
    bool enable_logging = false;
    std::string log_level = "INFO";
    bool cleanup_on_exit = true;
};

/**
 * @brief Represents a single Neo node in the test network
 */
class TestNode
{
  public:
    TestNode(int node_id, const IntegrationTestConfig& config);
    ~TestNode();

    // Node lifecycle
    bool Start();
    bool Stop();
    bool IsRunning() const
    {
        return is_running_;
    }

    // Network operations
    bool ConnectTo(const TestNode& other_node);
    bool DisconnectFrom(const TestNode& other_node);
    std::vector<std::string> GetConnectedPeers() const;

    // Blockchain operations
    std::shared_ptr<ledger::Block> CreateBlock();
    bool SubmitBlock(std::shared_ptr<ledger::Block> block);
    bool SubmitTransaction(std::shared_ptr<ledger::Transaction> transaction);
    uint32_t GetBlockHeight() const;
    std::shared_ptr<ledger::Block> GetBlock(uint32_t index) const;

    // RPC operations (if enabled)
    std::string SendRPCRequest(const std::string& method, const std::string& params = "[]");

    // Consensus operations
    bool StartConsensus();
    bool StopConsensus();
    bool IsConsensusRunning() const;

    // Getters
    int GetNodeId() const
    {
        return node_id_;
    }
    uint16_t GetPort() const
    {
        return port_;
    }
    uint16_t GetRPCPort() const
    {
        return rpc_port_;
    }
    std::shared_ptr<node::NeoSystem> GetNeoSystem() const
    {
        return neo_system_;
    }

  private:
    int node_id_;
    uint16_t port_;
    uint16_t rpc_port_;
    IntegrationTestConfig config_;
    std::atomic<bool> is_running_{false};

    // Core components
    std::shared_ptr<node::NeoSystem> neo_system_;
    std::shared_ptr<network::P2PServer> p2p_server_;
    std::shared_ptr<rpc::RpcServer> rpc_server_;
    std::shared_ptr<consensus::ConsensusService> consensus_service_;
    std::shared_ptr<ledger::Blockchain> blockchain_;
    std::shared_ptr<ledger::MemoryPool> mempool_;
    std::shared_ptr<persistence::IStore> store_;

    void InitializeComponents();
    void CleanupComponents();
};

/**
 * @brief Manages a network of test nodes for integration testing
 */
class TestNetwork
{
  public:
    explicit TestNetwork(const IntegrationTestConfig& config = IntegrationTestConfig{});
    ~TestNetwork();

    // Network management
    std::shared_ptr<TestNode> CreateNode();
    bool RemoveNode(int node_id);
    bool StartAllNodes();
    bool StopAllNodes();

    // Network topology
    bool ConnectAllNodes();                       // Full mesh
    bool ConnectNodesInChain();                   // Linear chain
    bool ConnectNodesInStar(int center_node_id);  // Star topology
    bool CreateCustomTopology(const std::vector<std::pair<int, int>>& connections);

    // Network operations
    bool WaitForNetworkSync(std::chrono::seconds timeout = std::chrono::seconds(30));
    bool WaitForBlockSync(uint32_t target_height, std::chrono::seconds timeout = std::chrono::seconds(30));
    bool BroadcastTransaction(std::shared_ptr<ledger::Transaction> transaction);

    // Network testing utilities
    bool SimulateNetworkPartition(const std::vector<int>& partition1, const std::vector<int>& partition2);
    bool HealNetworkPartition();
    bool SimulateNodeFailure(int node_id);
    bool RecoverNode(int node_id);

    // Consensus testing
    bool StartConsensusOnAllNodes();
    bool StopConsensusOnAllNodes();
    bool WaitForConsensusAgreement(std::chrono::seconds timeout = std::chrono::seconds(60));

    // Getters
    std::shared_ptr<TestNode> GetNode(int node_id) const;
    std::vector<std::shared_ptr<TestNode>> GetAllNodes() const;
    int GetNodeCount() const
    {
        return static_cast<int>(nodes_.size());
    }

    // Statistics
    struct NetworkStats
    {
        int total_nodes;
        int running_nodes;
        int connected_nodes;
        uint32_t max_block_height;
        uint32_t min_block_height;
        double average_block_height;
        int total_connections;
        std::chrono::milliseconds average_ping;
    };
    NetworkStats GetNetworkStats() const;

  private:
    IntegrationTestConfig config_;
    std::vector<std::shared_ptr<TestNode>> nodes_;
    std::atomic<int> next_node_id_{0};
    std::vector<std::pair<int, int>> current_topology_;

    void CleanupAllNodes();
};

/**
 * @brief Base class for integration tests with common utilities
 */
class IntegrationTestBase : public ::testing::Test
{
  protected:
    void SetUp() override;
    void TearDown() override;

    // Test utilities
    std::shared_ptr<TestNetwork> CreateTestNetwork(const IntegrationTestConfig& config = IntegrationTestConfig{});
    std::shared_ptr<TestNode> CreateSingleNode(const IntegrationTestConfig& config = IntegrationTestConfig{});

    // Assertion helpers
    void AssertNetworkConnectivity(std::shared_ptr<TestNetwork> network, int expected_connections);
    void AssertBlockchainSync(std::shared_ptr<TestNetwork> network, uint32_t expected_height);
    void AssertTransactionPropagation(std::shared_ptr<TestNetwork> network,
                                      std::shared_ptr<ledger::Transaction> transaction,
                                      std::chrono::seconds timeout = std::chrono::seconds(10));
    void AssertConsensusProgress(std::shared_ptr<TestNetwork> network, uint32_t expected_blocks,
                                 std::chrono::seconds timeout = std::chrono::seconds(60));

    // Scenario generators
    std::vector<std::shared_ptr<ledger::Transaction>> GenerateTestTransactions(int count);
    std::shared_ptr<ledger::Block>
    GenerateTestBlock(uint32_t index, const std::vector<std::shared_ptr<ledger::Transaction>>& transactions = {});

    // Performance measurement
    struct PerformanceMetrics
    {
        std::chrono::milliseconds transaction_propagation_time;
        std::chrono::milliseconds block_propagation_time;
        std::chrono::milliseconds consensus_time;
        double transactions_per_second;
        double blocks_per_second;
        size_t memory_usage_mb;
        double cpu_usage_percent;
    };
    PerformanceMetrics MeasurePerformance(std::shared_ptr<TestNetwork> network, std::function<void()> test_function);

    // Data validation
    bool ValidateBlockchainIntegrity(std::shared_ptr<TestNode> node);
    bool ValidateNetworkConsistency(std::shared_ptr<TestNetwork> network);
    bool ValidateMempoolSync(std::shared_ptr<TestNetwork> network);

  protected:
    IntegrationTestConfig default_config_;
    std::vector<std::shared_ptr<TestNetwork>> created_networks_;
    std::vector<std::shared_ptr<TestNode>> created_nodes_;
};

/**
 * @brief Specialized test class for consensus integration tests
 */
class ConsensusIntegrationTestBase : public IntegrationTestBase
{
  protected:
    void SetUp() override;

    // Consensus-specific utilities
    bool SimulateByzantineNode(int node_id, const std::string& behavior = "conflicting_messages");
    bool WaitForViewChange(std::shared_ptr<TestNetwork> network, uint8_t expected_view);
    bool WaitForBlockCommit(std::shared_ptr<TestNetwork> network, uint32_t block_index);

    // Byzantine behaviors
    void EnableConflictingMessages(int node_id);
    void EnableDelayedMessages(int node_id, std::chrono::milliseconds delay);
    void EnableDroppedMessages(int node_id, double drop_rate);
    void EnableIncorrectPrepareRequests(int node_id);
};

/**
 * @brief Specialized test class for network integration tests
 */
class NetworkIntegrationTestBase : public IntegrationTestBase
{
  protected:
    void SetUp() override;

    // Network-specific utilities
    bool SimulateHighLatency(int node1_id, int node2_id, std::chrono::milliseconds latency);
    bool SimulatePacketLoss(int node1_id, int node2_id, double loss_rate);
    bool SimulateBandwidthLimit(int node1_id, int node2_id, size_t bytes_per_second);

    // Network monitoring
    struct NetworkTrafficStats
    {
        size_t bytes_sent;
        size_t bytes_received;
        size_t messages_sent;
        size_t messages_received;
        double average_latency_ms;
        double packet_loss_rate;
    };
    NetworkTrafficStats GetTrafficStats(int node_id) const;
};

/**
 * @brief Specialized test class for RPC integration tests
 */
class RPCIntegrationTestBase : public IntegrationTestBase
{
  protected:
    void SetUp() override;

    // RPC-specific utilities
    std::string SendRPCRequest(std::shared_ptr<TestNode> node, const std::string& method,
                               const std::string& params = "[]");
    bool TestRPCEndpoint(std::shared_ptr<TestNode> node, const std::string& endpoint);

    // RPC test scenarios
    void TestBasicRPCMethods(std::shared_ptr<TestNode> node);
    void TestBlockchainRPCMethods(std::shared_ptr<TestNode> node);
    void TestTransactionRPCMethods(std::shared_ptr<TestNode> node);
    void TestWalletRPCMethods(std::shared_ptr<TestNode> node);
};

/**
 * @brief Test fixture for load testing
 */
class LoadTestBase : public IntegrationTestBase
{
  protected:
    void SetUp() override;

    // Load testing utilities
    struct LoadTestConfig
    {
        int concurrent_transactions = 100;
        int transactions_per_second = 50;
        std::chrono::seconds test_duration = std::chrono::seconds(60);
        int concurrent_connections = 50;
        size_t message_size = 1024;
    };

    void RunTransactionLoadTest(std::shared_ptr<TestNetwork> network, const LoadTestConfig& config);
    void RunNetworkLoadTest(std::shared_ptr<TestNetwork> network, const LoadTestConfig& config);
    void RunRPCLoadTest(std::shared_ptr<TestNode> node, const LoadTestConfig& config);

    // Resource monitoring
    struct ResourceUsage
    {
        double cpu_percent;
        size_t memory_mb;
        size_t disk_io_mb;
        size_t network_io_mb;
    };
    ResourceUsage MonitorResourceUsage(int node_id) const;
};

// Utility macros for integration tests
#define ASSERT_NETWORK_SYNC(network, timeout)                                                                          \
    ASSERT_TRUE((network)->WaitForNetworkSync(timeout)) << "Network failed to sync within timeout"

#define ASSERT_BLOCK_SYNC(network, height, timeout)                                                                    \
    ASSERT_TRUE((network)->WaitForBlockSync(height, timeout)) << "Blocks failed to sync to height " << height

#define ASSERT_CONSENSUS_AGREEMENT(network, timeout)                                                                   \
    ASSERT_TRUE((network)->WaitForConsensusAgreement(timeout)) << "Consensus failed to reach agreement"

#define EXPECT_NODE_RUNNING(node)                                                                                      \
    EXPECT_TRUE((node)->IsRunning()) << "Node " << (node)->GetNodeId() << " is not running"

#define EXPECT_BLOCKCHAIN_HEIGHT(node, expected_height)                                                                \
    EXPECT_EQ((node)->GetBlockHeight(), expected_height) << "Node " << (node)->GetNodeId() << " has unexpected height"

}  // namespace neo::tests::integration