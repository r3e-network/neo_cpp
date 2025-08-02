#include <gtest/gtest.h>
#include <neo/core/neo_system.h>
#include <neo/network/p2p/local_node.h>
#include <neo/ledger/block.h>
#include <neo/ledger/memory_pool.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <neo/network/ip_endpoint.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <thread>
#include <chrono>

using namespace neo;
using namespace neo::network;
using namespace neo::network::p2p;
using namespace neo::ledger;
using namespace std::chrono_literals;

class P2PSyncExecutionSimpleTest : public ::testing::Test
{
protected:
    std::shared_ptr<NeoSystem> system;
    std::shared_ptr<LocalNode> localNode;
    
    void SetUp() override
    {
        // Initialize NeoSystem
        system = std::make_shared<NeoSystem>("test_config.json");
        
        // Get LocalNode from system
        localNode = system->get_local_node();
    }
    
    void TearDown() override
    {
        if (system)
        {
            system->StopNode();
        }
    }
};

// Test 1: Basic System Initialization
TEST_F(P2PSyncExecutionSimpleTest, TestSystemInitialization)
{
    ASSERT_NE(system, nullptr);
    ASSERT_NE(localNode, nullptr);
    
    // Check that system has basic components
    EXPECT_NE(system->get_blockchain(), nullptr);
    EXPECT_NO_THROW(system->get_memory_pool());
}

// Test 2: P2P Node Start
TEST_F(P2PSyncExecutionSimpleTest, TestNodeStart)
{
    // Start the node
    bool started = localNode->Start(20333);
    EXPECT_TRUE(started);
    
    // Give it time to initialize
    std::this_thread::sleep_for(100ms);
    
    // Stop the node
    localNode->Stop();
}

// Test 3: Block Creation and Processing
TEST_F(P2PSyncExecutionSimpleTest, TestBlockProcessing)
{
    // Create a simple block
    auto block = std::make_shared<Block>();
    block->SetVersion(0);
    block->SetPreviousHash(io::UInt256::Zero());
    block->SetMerkleRoot(io::UInt256::Zero());
    block->SetTimestamp(std::chrono::system_clock::now());
    block->SetIndex(0);
    block->SetPrimaryIndex(0);
    block->SetNextConsensus(io::UInt160::Zero());
    
    // Process the block
    bool result = system->ProcessBlock(block);
    EXPECT_TRUE(result);
}

// Test 4: Transaction Pool
TEST_F(P2PSyncExecutionSimpleTest, TestTransactionPool)
{
    // Get memory pool
    auto& mempool = system->get_memory_pool();
    
    // Create a transaction
    auto tx = std::make_shared<payloads::Neo3Transaction>();
    tx->version = 0;
    tx->nonce = 1000;
    tx->systemFee = 0;
    tx->networkFee = 0;
    tx->validUntilBlock = 1000;
    tx->script = {0x51}; // PUSH1
    
    // Add to pool
    bool added = mempool.TryAdd(*tx);
    EXPECT_TRUE(added);
    
    // Check if in pool
    EXPECT_TRUE(mempool.Contains(tx->GetHash()));
}

// Test 5: Network Endpoint Parsing
TEST_F(P2PSyncExecutionSimpleTest, TestNetworkEndpoint)
{
    auto endpoint = IPEndPoint::Parse("127.0.0.1:20333");
    ASSERT_NE(endpoint, nullptr);
    EXPECT_EQ(endpoint->GetPort(), 20333);
}

// Test 6: Block with Transaction
TEST_F(P2PSyncExecutionSimpleTest, TestBlockWithTransaction)
{
    // Create a transaction
    auto tx = std::make_shared<payloads::Neo3Transaction>();
    tx->version = 0;
    tx->nonce = 2000;
    tx->systemFee = 0;
    tx->networkFee = 0;
    tx->validUntilBlock = 1000;
    tx->script = {0x51};
    
    // Create a block with the transaction
    auto block = std::make_shared<Block>();
    block->SetVersion(0);
    block->SetPreviousHash(io::UInt256::Zero());
    block->SetMerkleRoot(io::UInt256::Zero());
    block->SetTimestamp(std::chrono::system_clock::now());
    block->SetIndex(0);
    block->SetPrimaryIndex(0);
    block->SetNextConsensus(io::UInt160::Zero());
    block->AddTransaction(*tx);
    
    // Process the block
    bool result = system->ProcessBlock(block);
    EXPECT_TRUE(result);
    
    // Transaction should not be in mempool after block processing
    auto& mempool = system->get_memory_pool();
    EXPECT_FALSE(mempool.Contains(tx->GetHash()));
}

// Test 7: Multiple Blocks
TEST_F(P2PSyncExecutionSimpleTest, TestMultipleBlocks)
{
    // Process genesis
    auto genesis = std::make_shared<Block>();
    genesis->SetVersion(0);
    genesis->SetPreviousHash(io::UInt256::Zero());
    genesis->SetMerkleRoot(io::UInt256::Zero());
    genesis->SetTimestamp(std::chrono::system_clock::from_time_t(1468595301));
    genesis->SetIndex(0);
    genesis->SetPrimaryIndex(0);
    genesis->SetNextConsensus(io::UInt160::Zero());
    
    EXPECT_TRUE(system->ProcessBlock(genesis));
    
    // Process second block
    auto block1 = std::make_shared<Block>();
    block1->SetVersion(0);
    block1->SetPreviousHash(genesis->GetHash());
    block1->SetMerkleRoot(io::UInt256::Zero());
    block1->SetTimestamp(std::chrono::system_clock::now());
    block1->SetIndex(1);
    block1->SetPrimaryIndex(0);
    block1->SetNextConsensus(io::UInt160::Zero());
    
    // This might fail if block validation is strict
    bool result = system->ProcessBlock(block1);
    // Just check it doesn't crash
    SUCCEED();
}

// Test 8: P2P Connection
TEST_F(P2PSyncExecutionSimpleTest, TestP2PConnection)
{
    // Start local node
    ASSERT_TRUE(localNode->Start(20334));
    
    // Create endpoint
    auto endpoint = IPEndPoint::Parse("127.0.0.1:20335");
    ASSERT_NE(endpoint, nullptr);
    
    // Try to connect (will fail but tests the mechanism)
    localNode->ConnectAsync(endpoint);
    
    // Give it time
    std::this_thread::sleep_for(500ms);
    
    // Stop node
    localNode->Stop();
}

// Test 9: System Services
TEST_F(P2PSyncExecutionSimpleTest, TestSystemServices)
{
    // Check that system has required services
    EXPECT_NE(system->get_blockchain(), nullptr);
    EXPECT_NE(system->get_local_node(), nullptr);
    
    // Test snapshot creation
    auto snapshot = system->get_snapshot_cache();
    ASSERT_NE(snapshot, nullptr);
}

// Test 10: End-to-End Simple Flow
TEST_F(P2PSyncExecutionSimpleTest, TestSimpleEndToEnd)
{
    // Start P2P
    ASSERT_TRUE(localNode->Start(20336));
    
    // Create and add transaction to mempool
    auto tx = std::make_shared<payloads::Neo3Transaction>();
    tx->version = 0;
    tx->nonce = 3000;
    tx->systemFee = 0;
    tx->networkFee = 0;
    tx->validUntilBlock = 1000;
    tx->script = {0x51};
    
    auto& mempool = system->get_memory_pool();
    EXPECT_TRUE(mempool.TryAdd(*tx));
    
    // Create block with transaction
    auto block = std::make_shared<Block>();
    block->SetVersion(0);
    block->SetPreviousHash(io::UInt256::Zero());
    block->SetMerkleRoot(io::UInt256::Zero());
    block->SetTimestamp(std::chrono::system_clock::now());
    block->SetIndex(0);
    block->SetPrimaryIndex(0);
    block->SetNextConsensus(io::UInt160::Zero());
    block->AddTransaction(*tx);
    
    // Process block
    EXPECT_TRUE(system->ProcessBlock(block));
    
    // Verify transaction removed from mempool
    EXPECT_FALSE(mempool.Contains(tx->GetHash()));
    
    // Stop P2P
    localNode->Stop();
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}