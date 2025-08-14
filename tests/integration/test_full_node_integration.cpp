/**
 * @file test_full_node_integration.cpp
 * @brief Comprehensive integration tests for full node functionality
 */

#include <gtest/gtest.h>
#include <neo/core/neo_system.h>
#include <neo/network/p2p_server.h>
#include <neo/ledger/blockchain.h>
#include <neo/consensus/consensus_service.h>
#include <neo/rpc/rpc_server.h>
#include <neo/network/connection_pool.h>
#include <neo/ledger/blockchain_cache.h>
#include <neo/monitoring/performance_monitor.h>
#include <thread>
#include <chrono>
#include <future>

using namespace neo;
using namespace std::chrono_literals;

class FullNodeIntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<core::NeoSystem> system_;
    std::unique_ptr<network::P2PServer> p2p_server_;
    std::unique_ptr<rpc::RpcServer> rpc_server_;
    std::unique_ptr<consensus::ConsensusService> consensus_;
    std::unique_ptr<monitoring::PerformanceMonitor> monitor_;
    
    void SetUp() override {
        // Initialize monitoring
        monitor_ = std::make_unique<monitoring::PerformanceMonitor>();
        monitor_->Start();
        
        // Initialize Neo system
        core::SystemConfig config;
        config.network_id = 12345;  // Test network
        config.data_path = "/tmp/neo_test_" + std::to_string(getpid());
        system_ = std::make_unique<core::NeoSystem>(config);
        
        // Start P2P server
        network::P2PConfig p2p_config;
        p2p_config.port = 20333;
        p2p_config.max_peers = 10;
        p2p_server_ = std::make_unique<network::P2PServer>(p2p_config);
        
        // Start RPC server
        rpc::RpcConfig rpc_config;
        rpc_config.port = 20332;
        rpc_config.enable_cors = true;
        rpc_server_ = std::make_unique<rpc::RpcServer>(rpc_config);
        
        // Initialize consensus
        consensus_ = std::make_unique<consensus::ConsensusService>(*system_);
    }
    
    void TearDown() override {
        // Graceful shutdown
        if (consensus_) consensus_->Stop();
        if (rpc_server_) rpc_server_->Stop();
        if (p2p_server_) p2p_server_->Stop();
        if (system_) system_->Shutdown();
        if (monitor_) monitor_->Stop();
        
        // Cleanup test data
        std::filesystem::remove_all(system_->GetDataPath());
    }
};

/**
 * @brief Test full node initialization and shutdown
 */
TEST_F(FullNodeIntegrationTest, NodeLifecycle) {
    // Start services
    ASSERT_NO_THROW(system_->Start());
    ASSERT_NO_THROW(p2p_server_->Start());
    ASSERT_NO_THROW(rpc_server_->Start());
    
    // Verify services are running
    EXPECT_TRUE(p2p_server_->IsRunning());
    EXPECT_TRUE(rpc_server_->IsRunning());
    EXPECT_TRUE(system_->IsRunning());
    
    // Let it run briefly
    std::this_thread::sleep_for(100ms);
    
    // Verify metrics are being collected
    auto metrics = monitor_->GetMetrics();
    EXPECT_GT(metrics.uptime_seconds, 0);
    
    // Graceful shutdown
    ASSERT_NO_THROW(rpc_server_->Stop());
    ASSERT_NO_THROW(p2p_server_->Stop());
    ASSERT_NO_THROW(system_->Shutdown());
    
    // Verify services stopped
    EXPECT_FALSE(p2p_server_->IsRunning());
    EXPECT_FALSE(rpc_server_->IsRunning());
    EXPECT_FALSE(system_->IsRunning());
}

/**
 * @brief Test block synchronization between nodes
 */
TEST_F(FullNodeIntegrationTest, BlockSynchronization) {
    // Start primary node
    system_->Start();
    p2p_server_->Start();
    
    // Create secondary node
    core::SystemConfig config2;
    config2.network_id = 12345;
    config2.data_path = "/tmp/neo_test2_" + std::to_string(getpid());
    auto system2 = std::make_unique<core::NeoSystem>(config2);
    
    network::P2PConfig p2p_config2;
    p2p_config2.port = 20334;
    auto p2p_server2 = std::make_unique<network::P2PServer>(p2p_config2);
    
    system2->Start();
    p2p_server2->Start();
    
    // Connect nodes
    p2p_server2->ConnectToPeer("127.0.0.1", 20333);
    
    // Wait for connection
    std::this_thread::sleep_for(500ms);
    
    // Verify peer connection
    EXPECT_EQ(p2p_server_->GetPeerCount(), 1);
    EXPECT_EQ(p2p_server2->GetPeerCount(), 1);
    
    // Create a block on primary node
    auto block = CreateTestBlock(1);
    system_->GetBlockchain()->AddBlock(block);
    
    // Wait for synchronization
    std::this_thread::sleep_for(1s);
    
    // Verify block synced to secondary node
    auto synced_block = system2->GetBlockchain()->GetBlock(1);
    ASSERT_TRUE(synced_block != nullptr);
    EXPECT_EQ(synced_block->GetHash(), block->GetHash());
    
    // Cleanup
    p2p_server2->Stop();
    system2->Shutdown();
    std::filesystem::remove_all(config2.data_path);
}

/**
 * @brief Test transaction propagation across network
 */
TEST_F(FullNodeIntegrationTest, TransactionPropagation) {
    // Start services
    system_->Start();
    p2p_server_->Start();
    
    // Create and add transaction to mempool
    auto tx = CreateTestTransaction();
    auto mempool = system_->GetMemoryPool();
    
    ASSERT_TRUE(mempool->TryAdd(tx));
    
    // Create peer node
    auto peer_system = CreatePeerNode(20335);
    
    // Connect to main node
    peer_system.p2p->ConnectToPeer("127.0.0.1", 20333);
    
    // Wait for propagation
    std::this_thread::sleep_for(500ms);
    
    // Verify transaction in peer's mempool
    auto peer_mempool = peer_system.system->GetMemoryPool();
    EXPECT_TRUE(peer_mempool->Contains(tx.GetHash()));
    
    // Cleanup peer
    CleanupPeerNode(peer_system);
}

/**
 * @brief Test RPC endpoint functionality
 */
TEST_F(FullNodeIntegrationTest, RpcEndpoints) {
    // Start services
    system_->Start();
    rpc_server_->Start();
    
    // Test getblockcount
    auto block_count = rpc_server_->GetBlockCount();
    EXPECT_GE(block_count, 0);
    
    // Test getconnectioncount  
    p2p_server_->Start();
    auto conn_count = rpc_server_->GetConnectionCount();
    EXPECT_GE(conn_count, 0);
    
    // Test getversion
    auto version = rpc_server_->GetVersion();
    EXPECT_FALSE(version.empty());
    
    // Test metrics endpoint
    auto metrics = rpc_server_->GetMetrics();
    EXPECT_TRUE(metrics.contains("uptime"));
    EXPECT_TRUE(metrics.contains("memory"));
    EXPECT_TRUE(metrics.contains("peers"));
}

/**
 * @brief Test consensus mechanism with multiple nodes
 */
TEST_F(FullNodeIntegrationTest, ConsensusRound) {
    const int num_consensus_nodes = 4;
    std::vector<PeerNode> consensus_nodes;
    
    // Create consensus nodes
    for (int i = 0; i < num_consensus_nodes; ++i) {
        auto node = CreateConsensusNode(20340 + i, i);
        consensus_nodes.push_back(node);
    }
    
    // Connect all nodes to each other
    for (int i = 0; i < num_consensus_nodes; ++i) {
        for (int j = i + 1; j < num_consensus_nodes; ++j) {
            consensus_nodes[i].p2p->ConnectToPeer("127.0.0.1", 20340 + j);
        }
    }
    
    // Wait for network formation
    std::this_thread::sleep_for(1s);
    
    // Start consensus on all nodes
    for (auto& node : consensus_nodes) {
        node.consensus->Start();
    }
    
    // Wait for consensus round
    std::this_thread::sleep_for(5s);
    
    // Verify new block created
    uint32_t max_height = 0;
    for (const auto& node : consensus_nodes) {
        auto height = node.system->GetBlockchain()->GetHeight();
        max_height = std::max(max_height, height);
    }
    
    EXPECT_GT(max_height, 0);
    
    // Verify all nodes have same block
    auto reference_block = consensus_nodes[0].system->GetBlockchain()->GetBlock(max_height);
    for (const auto& node : consensus_nodes) {
        auto block = node.system->GetBlockchain()->GetBlock(max_height);
        ASSERT_TRUE(block != nullptr);
        EXPECT_EQ(block->GetHash(), reference_block->GetHash());
    }
    
    // Cleanup
    for (auto& node : consensus_nodes) {
        CleanupPeerNode(node);
    }
}

/**
 * @brief Test connection pool functionality
 */
TEST_F(FullNodeIntegrationTest, ConnectionPooling) {
    network::ConnectionPool::Config pool_config;
    pool_config.max_connections = 10;
    pool_config.min_connections = 2;
    
    network::ConnectionPool pool(pool_config);
    pool.Start();
    
    // Start P2P server to accept connections
    p2p_server_->Start();
    
    // Get multiple connections
    std::vector<std::shared_ptr<network::TcpConnection>> connections;
    for (int i = 0; i < 5; ++i) {
        auto conn = pool.GetConnection("127.0.0.1", 20333);
        ASSERT_TRUE(conn != nullptr);
        connections.push_back(conn);
    }
    
    // Return connections to pool
    for (auto& conn : connections) {
        pool.ReturnConnection(conn);
    }
    
    // Verify pool statistics
    auto stats = pool.GetStats();
    EXPECT_EQ(stats.idle_connections, 5);
    EXPECT_EQ(stats.active_connections, 0);
    EXPECT_GT(stats.reused_connections, 0);
    
    pool.Stop();
}

/**
 * @brief Test blockchain cache performance
 */
TEST_F(FullNodeIntegrationTest, BlockchainCaching) {
    ledger::BlockchainCache::Config cache_config;
    cache_config.block_cache_size = 100;
    cache_config.transaction_cache_size = 1000;
    
    ledger::BlockchainCache cache(cache_config);
    
    // Add blocks to cache
    for (uint32_t i = 1; i <= 100; ++i) {
        auto block = CreateTestBlock(i);
        cache.CacheBlock(block);
    }
    
    // Test cache hits
    int hits = 0;
    for (uint32_t i = 1; i <= 100; ++i) {
        auto block = cache.GetBlock(i);
        if (block) hits++;
    }
    
    EXPECT_EQ(hits, 100);
    
    // Verify cache statistics
    auto stats = cache.GetStats();
    EXPECT_GT(stats.hit_rate, 0.8);  // Expect >80% hit rate
    EXPECT_EQ(stats.block_stats.size, 100);
}

/**
 * @brief Test performance monitoring integration
 */
TEST_F(FullNodeIntegrationTest, PerformanceMonitoring) {
    // Start services with monitoring
    system_->Start();
    p2p_server_->Start();
    rpc_server_->Start();
    
    // Perform some operations
    for (int i = 0; i < 10; ++i) {
        auto tx = CreateTestTransaction();
        system_->GetMemoryPool()->TryAdd(tx);
    }
    
    // Wait for metrics collection
    std::this_thread::sleep_for(1s);
    
    // Get performance metrics
    auto metrics = monitor_->GetMetrics();
    
    // Verify metrics collected
    EXPECT_GT(metrics.total_requests, 0);
    EXPECT_GT(metrics.uptime_seconds, 0);
    EXPECT_GE(metrics.memory_usage_mb, 0);
    EXPECT_GE(metrics.cpu_usage_percent, 0);
    
    // Test alert thresholds
    monitor_->SetAlertThreshold("memory", 1000000);  // 1GB
    monitor_->SetAlertThreshold("cpu", 80);  // 80%
    
    auto alerts = monitor_->GetActiveAlerts();
    // Should have no alerts under normal conditions
    EXPECT_EQ(alerts.size(), 0);
}

/**
 * @brief Stress test with high transaction volume
 */
TEST_F(FullNodeIntegrationTest, HighVolumeStressTest) {
    // Start services
    system_->Start();
    p2p_server_->Start();
    
    const int num_transactions = 1000;
    std::vector<std::future<bool>> futures;
    
    // Submit transactions concurrently
    for (int i = 0; i < num_transactions; ++i) {
        futures.push_back(std::async(std::launch::async, [this, i]() {
            auto tx = CreateTestTransaction(i);
            return system_->GetMemoryPool()->TryAdd(tx);
        }));
    }
    
    // Wait for all submissions
    int successful = 0;
    for (auto& future : futures) {
        if (future.get()) successful++;
    }
    
    // Expect most transactions to be accepted
    EXPECT_GT(successful, num_transactions * 0.9);
    
    // Verify system stability
    EXPECT_TRUE(system_->IsRunning());
    EXPECT_TRUE(p2p_server_->IsRunning());
    
    // Check performance metrics
    auto metrics = monitor_->GetMetrics();
    std::cout << "Processed " << successful << " transactions" << std::endl;
    std::cout << "Memory usage: " << metrics.memory_usage_mb << " MB" << std::endl;
    std::cout << "CPU usage: " << metrics.cpu_usage_percent << "%" << std::endl;
}

private:
    struct PeerNode {
        std::unique_ptr<core::NeoSystem> system;
        std::unique_ptr<network::P2PServer> p2p;
        std::unique_ptr<consensus::ConsensusService> consensus;
        std::string data_path;
    };
    
    PeerNode CreatePeerNode(uint16_t port) {
        PeerNode node;
        
        core::SystemConfig config;
        config.network_id = 12345;
        config.data_path = "/tmp/neo_peer_" + std::to_string(port);
        node.data_path = config.data_path;
        node.system = std::make_unique<core::NeoSystem>(config);
        
        network::P2PConfig p2p_config;
        p2p_config.port = port;
        node.p2p = std::make_unique<network::P2PServer>(p2p_config);
        
        node.system->Start();
        node.p2p->Start();
        
        return node;
    }
    
    PeerNode CreateConsensusNode(uint16_t port, int validator_index) {
        PeerNode node = CreatePeerNode(port);
        node.consensus = std::make_unique<consensus::ConsensusService>(*node.system);
        node.consensus->SetValidatorIndex(validator_index);
        return node;
    }
    
    void CleanupPeerNode(PeerNode& node) {
        if (node.consensus) node.consensus->Stop();
        if (node.p2p) node.p2p->Stop();
        if (node.system) node.system->Shutdown();
        std::filesystem::remove_all(node.data_path);
    }
    
    std::shared_ptr<ledger::Block> CreateTestBlock(uint32_t index) {
        auto block = std::make_shared<ledger::Block>();
        block->SetIndex(index);
        block->SetTimestamp(std::chrono::system_clock::now());
        // Set other block properties...
        return block;
    }
    
    ledger::Transaction CreateTestTransaction(int nonce = 0) {
        ledger::Transaction tx;
        tx.SetNonce(nonce);
        tx.SetSystemFee(100);
        tx.SetNetworkFee(10);
        // Set other transaction properties...
        return tx;
    }
};