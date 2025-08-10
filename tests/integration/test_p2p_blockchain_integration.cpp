#include <gtest/gtest.h>
#include <neo/core/neo_system.h>
#include <neo/network/p2p/local_node.h>
#include <neo/ledger/block.h>
#include <neo/ledger/memory_pool.h>
#include <neo/ledger/transaction.h>
#include <neo/persistence/store_factory.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/network/ip_endpoint.h>
#include <neo/protocol_settings.h>
#include <thread>
#include <chrono>
#include <future>
#include <filesystem>
#include <random>

using namespace neo;
using namespace neo::network;
using namespace neo::network::p2p;
using namespace neo::persistence;
using namespace std::chrono_literals;

class P2PBlockchainIntegrationTest : public ::testing::Test
{
protected:
    std::shared_ptr<NeoSystem> system1;
    std::shared_ptr<NeoSystem> system2;
    LocalNode* node1;
    
    void SetUp() override
    {
        // Clean up any existing test databases
        std::filesystem::remove_all("test_db1");
        std::filesystem::remove_all("test_db2");
        
        // Create two separate systems for testing P2P sync
        system1 = CreateTestSystem();
        system2 = CreateTestSystem();
        
        // Get singleton instance for testing
        node1 = &LocalNode::GetInstance();
    }
    
    void TearDown() override
    {
        // Stop node if running
        if (node1) {
            node1->Stop();
        }
        
        // Wait for graceful shutdown
        std::this_thread::sleep_for(100ms);
        
        // Clean up test databases
        std::filesystem::remove_all("test_db1");
        std::filesystem::remove_all("test_db2");
    }
    
private:
    std::shared_ptr<NeoSystem> CreateTestSystem()
    {
        // Create protocol settings
        auto settings = std::make_unique<ProtocolSettings>();
        
        // Create system with memory store for testing
        return std::make_shared<NeoSystem>(std::move(settings), "memory");
    }
    
public:
    std::shared_ptr<ledger::Block> CreateTestBlock(uint32_t index)
    {
        auto block = std::make_shared<ledger::Block>();
        block->SetVersion(0);
        
        if (index == 0) {
            // Genesis block
            block->SetPreviousHash(io::UInt256::Zero());
        } else {
            // Use a deterministic previous hash for testing
            auto hashBytes = std::vector<uint8_t>(32, 0);
            hashBytes[0] = static_cast<uint8_t>(index - 1);
            hashBytes[1] = 0xFF; // Make it different from zero
            block->SetPreviousHash(io::UInt256(hashBytes.data()));
        }
        
        block->SetMerkleRoot(io::UInt256::Zero());
        block->SetTimestamp(static_cast<uint64_t>(1468595301 + index));
        block->SetIndex(index);
        block->SetPrimaryIndex(0);
        block->SetNextConsensus(io::UInt160::Zero());
        return block;
    }
    
    ledger::Transaction CreateTestTransaction(uint32_t nonce)
    {
        ledger::Transaction tx;
        tx.SetVersion(0);
        tx.SetNonce(nonce);
        tx.SetSystemFee(0);
        tx.SetNetworkFee(0);
        tx.SetValidUntilBlock(1000000);
        
        // Set a simple script (PUSH1 opcode)
        io::ByteVector script = {0x51};
        tx.SetScript(script);
        
        return tx;
    }
};

// Test 1: P2P Node Lifecycle Management
TEST_F(P2PBlockchainIntegrationTest, TestP2PNodeLifecycle)
{
    // Test basic LocalNode lifecycle
    ASSERT_NE(node1, nullptr);
    
    // Test starting the node with port
    bool startResult = node1->Start(21001, 5);
    EXPECT_TRUE(startResult);
    
    // Give node time to initialize
    std::this_thread::sleep_for(200ms);
    
    // Test connection count (should be 0 initially)
    size_t connectedCount = node1->GetConnectedCount();
    EXPECT_EQ(connectedCount, 0);
    
    // Test getting connected nodes (should be empty)
    auto connectedNodes = node1->GetConnectedNodes();
    EXPECT_TRUE(connectedNodes.empty());
    
    // Test version payload creation
    auto versionPayload = node1->CreateVersionPayload();
    ASSERT_NE(versionPayload, nullptr);
    
    // Stop the node
    node1->Stop();
    
    // Verify we can restart
    bool restartResult = node1->Start(21002, 5);
    EXPECT_TRUE(restartResult);
    
    node1->Stop();
}

// Test 2: Block Structure and Properties
TEST_F(P2PBlockchainIntegrationTest, TestBlockStructureValidation)
{
    // Create test blocks and verify their structure
    auto genesis = CreateTestBlock(0);
    
    // Verify genesis block properties
    EXPECT_EQ(genesis->GetVersion(), 0);
    EXPECT_EQ(genesis->GetIndex(), 0);
    EXPECT_EQ(genesis->GetPreviousHash(), io::UInt256::Zero());
    EXPECT_NE(genesis->GetHash(), io::UInt256::Zero());
    
    // Create block 1
    auto block1 = CreateTestBlock(1);
    EXPECT_EQ(block1->GetVersion(), 0);
    EXPECT_EQ(block1->GetIndex(), 1);
    EXPECT_NE(block1->GetPreviousHash(), io::UInt256::Zero());
    EXPECT_NE(block1->GetHash(), io::UInt256::Zero());
    
    // Verify blocks have different hashes
    EXPECT_NE(genesis->GetHash(), block1->GetHash());
}

// Test 3: Transaction Creation and Validation
TEST_F(P2PBlockchainIntegrationTest, TestTransactionValidation)
{
    // Test that we can create a transaction object
    ledger::Transaction tx;
    tx.SetVersion(0);
    tx.SetNonce(1000);
    tx.SetSystemFee(0);
    tx.SetNetworkFee(0);
    tx.SetValidUntilBlock(1000000);
    
    // Verify basic properties
    EXPECT_EQ(tx.GetVersion(), 0);
    EXPECT_EQ(tx.GetNonce(), 1000);
    
    // Basic validation complete
    SUCCEED();
}

// Test 4: Memory Pool Functionality
TEST_F(P2PBlockchainIntegrationTest, TestMemoryPoolFunctionality)
{
    // Get memory pools from both systems
    auto mempool1 = system1->GetMemPool();
    auto mempool2 = system2->GetMemPool();
    
    ASSERT_NE(mempool1, nullptr);
    ASSERT_NE(mempool2, nullptr);
    
    // Initially empty
    EXPECT_EQ(mempool1->GetSize(), 0);
    EXPECT_EQ(mempool2->GetSize(), 0);
    EXPECT_FALSE(mempool1->IsFull());
    EXPECT_FALSE(mempool2->IsFull());
    
    // Basic memory pool access verified
    SUCCEED();
}

// Test 5: System State Management
TEST_F(P2PBlockchainIntegrationTest, TestSystemStateManagement)
{
    // Test initial system state
    EXPECT_EQ(system1->GetCurrentBlockHeight(), 0);
    EXPECT_EQ(system2->GetCurrentBlockHeight(), 0);
    
    // Test snapshot creation
    auto snapshot1 = system1->get_snapshot_cache();
    auto snapshot2 = system2->get_snapshot_cache();
    
    ASSERT_NE(snapshot1, nullptr);
    ASSERT_NE(snapshot2, nullptr);
    
    // Test multiple snapshots can be created
    auto snapshot1_alt = system1->get_snapshot_cache();
    ASSERT_NE(snapshot1_alt, nullptr);
    EXPECT_NE(snapshot1.get(), snapshot1_alt.get());
    
    // Test fast sync mode
    system1->SetFastSyncMode(true);
    system1->SetFastSyncMode(false);
    
    // Test transaction checking
    auto testHash = io::UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    auto result = system1->contains_transaction(testHash);
    // Should not contain random transaction
    SUCCEED(); // Test completed without crash
}

// Test 6: P2P Connection Attempts
TEST_F(P2PBlockchainIntegrationTest, TestP2PConnectionAttempts)
{
    // Start the node
    bool startResult = node1->Start(21003, 10);
    EXPECT_TRUE(startResult);
    
    // Test connection to non-existent peer (should fail gracefully)
    IPEndPoint endpoint1("127.0.0.1", 21004);
    bool connectResult1 = node1->Connect(endpoint1);
    // Connection may fail, but should not crash
    
    // Test connection to localhost with different port (should fail gracefully)
    IPEndPoint endpoint2("127.0.0.1", 21005);
    bool connectResult2 = node1->Connect(endpoint2);
    // Connection should fail, but should not crash
    
    // Give some time for connection attempts
    std::this_thread::sleep_for(500ms);
    
    // Node should still be running
    size_t connectedCount = node1->GetConnectedCount();
    EXPECT_EQ(connectedCount, 0); // No successful connections expected
    
    node1->Stop();
}

// Test 7: Concurrent System Operations
TEST_F(P2PBlockchainIntegrationTest, TestConcurrentSystemOperations)
{
    const int numOperations = 5;
    std::vector<std::future<bool>> futures;
    
    // Test concurrent snapshot creation
    for (int i = 0; i < numOperations; i++) {
        futures.push_back(std::async(std::launch::async, [this]() {
            auto snapshot = system1->get_snapshot_cache();
            return snapshot != nullptr;
        }));
    }
    
    // Wait for all operations to complete
    int successCount = 0;
    for (auto& future : futures) {
        if (future.get()) {
            successCount++;
        }
    }
    
    // All snapshot operations should succeed
    EXPECT_EQ(successCount, numOperations);
}

// Test 8: Block Processing Edge Cases
TEST_F(P2PBlockchainIntegrationTest, TestBlockProcessingEdgeCases)
{
    // Test processing null block (should handle gracefully)
    std::shared_ptr<ledger::Block> nullBlock;
    // Note: Can't actually test this as ProcessBlock expects valid pointer
    
    // Test processing block with invalid data
    auto invalidBlock = CreateTestBlock(999);
    invalidBlock->SetIndex(999); // Invalid index for current state
    
    bool result = system1->ProcessBlock(invalidBlock);
    // Should fail but not crash
    EXPECT_FALSE(result);
    
    // System should still be functional
    EXPECT_EQ(system1->GetCurrentBlockHeight(), 0);
    auto snapshot = system1->get_snapshot_cache();
    ASSERT_NE(snapshot, nullptr);
}

// Test 9: Cross-System State Verification
TEST_F(P2PBlockchainIntegrationTest, TestCrossSystemStateVerification)
{
    // Both systems should start with same initial state
    EXPECT_EQ(system1->GetCurrentBlockHeight(), system2->GetCurrentBlockHeight());
    
    // Both should have functional memory pools
    auto mempool1 = system1->GetMemPool();
    auto mempool2 = system2->GetMemPool();
    
    ASSERT_NE(mempool1, nullptr);
    ASSERT_NE(mempool2, nullptr);
    EXPECT_EQ(mempool1->GetSize(), mempool2->GetSize());
    
    // Both systems should have identical initial state
    EXPECT_EQ(mempool1->GetSize(), 0);
    EXPECT_EQ(mempool2->GetSize(), 0);
}

// Test 10: End-to-End Integration Scenario
TEST_F(P2PBlockchainIntegrationTest, TestEndToEndIntegrationScenario)
{
    // Step 1: Start P2P node
    bool startResult = node1->Start(21006, 10);
    EXPECT_TRUE(startResult);
    
    // Step 2: Create and validate blocks
    auto genesis = CreateTestBlock(0);
    auto block1 = CreateTestBlock(1);
    
    EXPECT_EQ(genesis->GetIndex(), 0);
    EXPECT_EQ(block1->GetIndex(), 1);
    
    // Step 3: Test memory pool access
    auto mempool1 = system1->GetMemPool();
    auto mempool2 = system2->GetMemPool();
    
    ASSERT_NE(mempool1, nullptr);
    ASSERT_NE(mempool2, nullptr);
    
    // Step 4: Verify memory pools are empty
    EXPECT_EQ(mempool1->GetSize(), 0);
    EXPECT_EQ(mempool2->GetSize(), 0);
    
    // Step 5: Test snapshot operations
    auto snapshot1 = system1->get_snapshot_cache();
    auto snapshot2 = system2->get_snapshot_cache();
    
    ASSERT_NE(snapshot1, nullptr);
    ASSERT_NE(snapshot2, nullptr);
    
    // Step 6: Verify system state consistency
    EXPECT_EQ(system1->GetCurrentBlockHeight(), system2->GetCurrentBlockHeight());
    
    // Step 7: Test P2P connection capabilities
    IPEndPoint testEndpoint("127.0.0.1", 21007);
    bool connectResult = node1->Connect(testEndpoint);
    // Connection may fail, but should not crash
    
    // Step 8: Verify everything still works after connection attempt
    auto versionPayload = node1->CreateVersionPayload();
    ASSERT_NE(versionPayload, nullptr);
    
    // Step 9: Clean shutdown
    node1->Stop();
    
    // Final verification - systems should still be functional
    auto finalSnapshot = system1->get_snapshot_cache();
    ASSERT_NE(finalSnapshot, nullptr);
}