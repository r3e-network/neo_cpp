#include <chrono>
#include <gtest/gtest.h>
#include <neo/config/configuration_manager.h>
#include <neo/consensus/dbft_consensus.h>
#include <neo/core/neo_system.h>
#include <neo/monitoring/health_checks.h>
#include <neo/monitoring/metrics.h>
#include <neo/network/p2p/connection_manager.h>
#include <neo/network/p2p/local_node.h>
#include <neo/persistence/rocksdb_store.h>
#include <neo/rpc/rpc_server.h>
#include <thread>

namespace neo::tests::integration
{
/**
 * @brief Full node integration test fixture
 */
class FullNodeIntegrationTest : public ::testing::Test
{
  protected:
    std::shared_ptr<core::NeoSystem> system_;
    std::shared_ptr<config::ConfigurationManager> config_;
    std::shared_ptr<persistence::RocksDbStore> db_;
    std::shared_ptr<network::p2p::LocalNode> node_;
    std::shared_ptr<rpc::RpcServer> rpc_server_;
    std::shared_ptr<monitoring::MetricsRegistry> metrics_;
    std::shared_ptr<monitoring::HealthCheckRegistry> health_;

    void SetUp() override
    {
        // Initialize configuration
        config_ = config::ConfigurationManager::GetInstance();
        config_->AddSource(std::make_shared<config::JsonFileConfigSource>("test_config.json"));

        // Initialize database
        persistence::RocksDbConfig db_config;
        db_config.db_path = "./test_data/rocksdb";
        db_ = std::make_shared<persistence::RocksDbStore>(db_config);
        ASSERT_TRUE(db_->Open());

        // Initialize metrics
        metrics_ = monitoring::MetricsRegistry::GetInstance();
        health_ = monitoring::HealthCheckRegistry::GetInstance();

        // Initialize system
        system_ = std::make_shared<core::NeoSystem>(db_);

        // Initialize local node
        network::p2p::LocalNodeConfig node_config;
        node_config.port = 20333;  // Test port
        node_config.max_peers = 10;
        node_ = std::make_shared<network::p2p::LocalNode>(node_config, system_);

        // Initialize RPC server
        rpc::RpcConfig rpc_config;
        rpc_config.port = 20332;  // Test port
        rpc_server_ = std::make_shared<rpc::RpcServer>(rpc_config);
        rpc_server_->SetBlockchain(system_->GetBlockchain());
        rpc_server_->SetLocalNode(node_);

        // Register health checks
        health_->Register("database", std::make_shared<monitoring::DatabaseHealthCheck>(system_->GetBlockchain()));
        health_->Register("network", std::make_shared<monitoring::NetworkHealthCheck>(node_));
    }

    void TearDown() override
    {
        if (rpc_server_)
            rpc_server_->Stop();
        if (node_)
            node_->Stop();
        if (db_)
            db_->Close();

        // Clean up test data
        std::filesystem::remove_all("./test_data");
    }
};

/**
 * @brief Test node startup and shutdown
 */
TEST_F(FullNodeIntegrationTest, StartupShutdown)
{
    // Start node
    ASSERT_NO_THROW(node_->Start());
    ASSERT_TRUE(node_->IsRunning());

    // Start RPC server
    ASSERT_NO_THROW(rpc_server_->Start());

    // Wait for initialization
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Check health
    auto health_status = health_->GetOverallStatus();
    EXPECT_NE(health_status, monitoring::HealthStatus::Unhealthy);

    // Stop services
    rpc_server_->Stop();
    node_->Stop();

    ASSERT_FALSE(node_->IsRunning());
}

/**
 * @brief Test peer connection
 */
TEST_F(FullNodeIntegrationTest, PeerConnection)
{
    // Start first node
    node_->Start();

    // Create second node
    network::p2p::LocalNodeConfig node2_config;
    node2_config.port = 20334;
    auto node2 = std::make_shared<network::p2p::LocalNode>(node2_config, system_);
    node2->Start();

    // Connect nodes
    network::p2p::NetworkAddress addr;
    addr.ip = "127.0.0.1";
    addr.port = 20333;
    node2->ConnectTo(addr);

    // Wait for connection
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Check connection
    EXPECT_GT(node_->GetConnectedPeerCount(), 0);
    EXPECT_GT(node2->GetConnectedPeerCount(), 0);

    // Clean up
    node2->Stop();
}

/**
 * @brief Test RPC functionality
 */
TEST_F(FullNodeIntegrationTest, RPCFunctionality)
{
    node_->Start();
    rpc_server_->Start();

    // Wait for services to start
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Create HTTP client
    auto client = std::make_shared<HttpClient>("127.0.0.1", 20332);

    // Test getversion RPC call
    json::JObject request;
    request["jsonrpc"] = "2.0";
    request["method"] = "getversion";
    request["params"] = json::JArray();
    request["id"] = 1;

    auto response = client->Post("/", request.ToString());
    ASSERT_FALSE(response.empty());

    json::JObject result = json::JObject::Parse(response);
    EXPECT_EQ(result["jsonrpc"].AsString(), "2.0");
    EXPECT_TRUE(result.Contains("result"));
    EXPECT_EQ(result["id"].AsInteger(), 1);
}

/**
 * @brief Test transaction propagation
 */
TEST_F(FullNodeIntegrationTest, TransactionPropagation)
{
    // Start multiple nodes
    std::vector<std::shared_ptr<network::p2p::LocalNode>> nodes;

    for (int i = 0; i < 3; i++)
    {
        network::p2p::LocalNodeConfig config;
        config.port = 20335 + i;
        auto node = std::make_shared<network::p2p::LocalNode>(config, system_);
        node->Start();
        nodes.push_back(node);

        // Connect to first node
        if (i > 0)
        {
            network::p2p::NetworkAddress addr;
            addr.ip = "127.0.0.1";
            addr.port = 20335;
            node->ConnectTo(addr);
        }
    }

    // Wait for network formation
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Create and broadcast transaction from first node
    auto tx = CreateTestTransaction();
    nodes[0]->BroadcastTransaction(tx);

    // Wait for propagation
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Check all nodes received transaction
    for (const auto& node : nodes)
    {
        EXPECT_TRUE(node->GetMemoryPool()->Contains(tx.GetHash()));
    }

    // Clean up
    for (auto& node : nodes)
    {
        node->Stop();
    }
}

/**
 * @brief Test block synchronization
 */
TEST_F(FullNodeIntegrationTest, BlockSynchronization)
{
    // Create blockchain with some blocks
    for (int i = 0; i < 10; i++)
    {
        auto block = CreateTestBlock(i);
        system_->GetBlockchain()->AddBlock(block);
    }

    // Start first node
    node_->Start();

    // Create new node with empty blockchain
    auto db2 =
        std::make_shared<persistence::RocksDbStore>(persistence::RocksDbConfig{.db_path = "./test_data/rocksdb2"});
    db2->Open();

    auto system2 = std::make_shared<core::NeoSystem>(db2);

    network::p2p::LocalNodeConfig node2_config;
    node2_config.port = 20336;
    auto node2 = std::make_shared<network::p2p::LocalNode>(node2_config, system2);
    node2->Start();

    // Connect to first node
    network::p2p::NetworkAddress addr;
    addr.ip = "127.0.0.1";
    addr.port = 20333;
    node2->ConnectTo(addr);

    // Wait for synchronization
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Check synchronization
    EXPECT_EQ(system2->GetBlockchain()->GetHeight(), system_->GetBlockchain()->GetHeight());

    // Clean up
    node2->Stop();
    db2->Close();
}

/**
 * @brief Test consensus operation
 */
TEST_F(FullNodeIntegrationTest, ConsensusOperation)
{
    // Create validator nodes
    std::vector<std::shared_ptr<consensus::DbftConsensus>> consensus_nodes;
    std::vector<io::UInt160> validators;

    for (int i = 0; i < 4; i++)
    {
        auto validator_id = GenerateValidatorId(i);
        validators.push_back(validator_id);
    }

    for (int i = 0; i < 4; i++)
    {
        consensus::ConsensusConfig config;
        config.block_time = std::chrono::seconds(5);

        auto consensus = std::make_shared<consensus::DbftConsensus>(config, validators[i], validators);

        consensus_nodes.push_back(consensus);
        consensus->Start();
    }

    // Wait for consensus rounds
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // Check blocks were created
    for (const auto& consensus : consensus_nodes)
    {
        auto state = consensus->GetState();
        EXPECT_GT(state.GetBlockIndex(), 0);
    }

    // Stop consensus
    for (auto& consensus : consensus_nodes)
    {
        consensus->Stop();
    }
}

/**
 * @brief Test metrics collection
 */
TEST_F(FullNodeIntegrationTest, MetricsCollection)
{
    auto block_height = metrics_->Register<monitoring::Gauge>("neo_block_height", "Current blockchain height");
    auto peer_count = metrics_->Register<monitoring::Gauge>("neo_peer_count", "Number of connected peers");
    auto tx_count = metrics_->Register<monitoring::Counter>("neo_transactions_total", "Total transactions processed");

    // Start node
    node_->Start();

    // Simulate some activity
    block_height->Set(100);
    peer_count->Set(5);
    for (int i = 0; i < 10; i++)
    {
        tx_count->Increment();
    }

    // Export metrics
    auto prometheus_output = metrics_->ExportPrometheus();

    // Check metrics are present
    EXPECT_NE(prometheus_output.find("neo_block_height 100"), std::string::npos);
    EXPECT_NE(prometheus_output.find("neo_peer_count 5"), std::string::npos);
    EXPECT_NE(prometheus_output.find("neo_transactions_total 10"), std::string::npos);
}

/**
 * @brief Test health checks
 */
TEST_F(FullNodeIntegrationTest, HealthChecks)
{
    // Start services
    node_->Start();

    // Run health checks
    auto results = health_->RunAll();

    // Check database health
    ASSERT_TRUE(results.count("database") > 0);
    EXPECT_EQ(results["database"].status, monitoring::HealthStatus::Healthy);

    // Check network health (may be degraded due to no peers)
    ASSERT_TRUE(results.count("network") > 0);
    EXPECT_NE(results["network"].status, monitoring::HealthStatus::Unhealthy);

    // Test JSON export
    auto json_output = health_->ExportJson();
    EXPECT_FALSE(json_output.empty());
}

/**
 * @brief Stress test with multiple operations
 */
TEST_F(FullNodeIntegrationTest, StressTest)
{
    // Start node
    node_->Start();
    rpc_server_->Start();

    // Create multiple threads for different operations
    std::vector<std::thread> threads;
    std::atomic<bool> stop{false};
    std::atomic<uint64_t> operations{0};

    // Transaction creation thread
    threads.emplace_back(
        [&]()
        {
            while (!stop)
            {
                auto tx = CreateTestTransaction();
                system_->GetMemoryPool()->Add(tx);
                operations++;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });

    // RPC query thread
    threads.emplace_back(
        [&]()
        {
            auto client = std::make_shared<HttpClient>("127.0.0.1", 20332);
            while (!stop)
            {
                json::JObject request;
                request["jsonrpc"] = "2.0";
                request["method"] = "getblockcount";
                request["params"] = json::JArray();
                request["id"] = operations.load();

                client->Post("/", request.ToString());
                operations++;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });

    // Run stress test for 10 seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));
    stop = true;

    // Wait for threads
    for (auto& thread : threads)
    {
        thread.join();
    }

    // Check operations completed
    EXPECT_GT(operations.load(), 100);

    // Check system still healthy
    auto health_status = health_->GetOverallStatus();
    EXPECT_NE(health_status, monitoring::HealthStatus::Unhealthy);
}

private:
/**
 * @brief Helper to create test transaction
 */
ledger::Transaction CreateTestTransaction()
{
    // Implementation would create a valid test transaction
    return ledger::Transaction();
}

/**
 * @brief Helper to create test block
 */
ledger::Block CreateTestBlock(uint32_t index)
{
    // Implementation would create a valid test block
    return ledger::Block();
}

/**
 * @brief Helper to generate validator ID
 */
io::UInt160 GenerateValidatorId(int index)
{
    // Implementation would generate a valid validator ID
    return io::UInt160();
}
};  // namespace neo::tests::integration