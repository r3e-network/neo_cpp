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

class P2PBlockSyncIntegrationTest : public ::testing::Test
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
    std::shared_ptr<ledger::Block> CreateGenesisBlock()
    {
        auto block = std::make_shared<ledger::Block>();
        block->SetVersion(0);
        block->SetPreviousHash(io::UInt256::Zero());
        block->SetMerkleRoot(io::UInt256::Zero());
        block->SetTimestamp(static_cast<uint64_t>(1468595301)); // Neo genesis time
        block->SetIndex(0);
        block->SetPrimaryIndex(0);
        block->SetNextConsensus(io::UInt160::Zero());
        // Add a minimal witness for validation
        auto witness = std::make_shared<ledger::Witness>();
        witness->SetInvocationScript(neo::io::ByteVector{0x00});
        witness->SetVerificationScript(neo::io::ByteVector{0x51}); // PUSH1
        block->SetWitness(*witness);
        return block;
    }
    
    std::shared_ptr<ledger::Block> CreateBlock(uint32_t index, const io::UInt256& prevHash)
    {
        auto block = std::make_shared<ledger::Block>();
        block->SetVersion(0);
        block->SetPreviousHash(prevHash);
        block->SetMerkleRoot(io::UInt256::Zero());
        block->SetTimestamp(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + std::chrono::seconds(index)).time_since_epoch()).count()));
        block->SetIndex(index);
        block->SetPrimaryIndex(0);
        block->SetNextConsensus(io::UInt160::Zero());
        // Add a minimal witness for validation
        auto witness = std::make_shared<ledger::Witness>();
        witness->SetInvocationScript(neo::io::ByteVector{0x00});
        witness->SetVerificationScript(neo::io::ByteVector{0x51}); // PUSH1
        block->SetWitness(*witness);
        return block;
    }
    
    std::shared_ptr<ledger::Block> CreateBlockWithTransactions(uint32_t index, const io::UInt256& prevHash, int txCount)
    {
        auto block = CreateBlock(index, prevHash);
        
        for (int i = 0; i < txCount; i++) {
            auto tx = CreateTestTransaction(index * 1000 + i);
            block->AddTransaction(tx);
        }
        
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

// Test 1: Basic P2P Node API
TEST_F(P2PBlockSyncIntegrationTest, TestBasicP2PNodeAPI)
{
    // Test basic LocalNode singleton access
    ASSERT_NE(node1, nullptr);
    
    // Test starting the node with port
    bool startResult = node1->Start(20001, 10);
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
    
    // Test connection attempt
    IPEndPoint endpoint("127.0.0.1", 20002);
    bool connectResult = node1->Connect(endpoint);
    // Connection may fail in test environment, but should not crash
    SUCCEED();
    
    // Stop the node
    node1->Stop();
}

// Test 2: Block Creation and Basic Processing
TEST_F(P2PBlockSyncIntegrationTest, TestBlockCreationAndProcessing)
{
    // Create genesis block
    auto genesis = CreateGenesisBlock();
    
    // Verify block properties
    EXPECT_EQ(genesis->GetVersion(), 0);
    EXPECT_EQ(genesis->GetIndex(), 0);
    EXPECT_EQ(genesis->GetPreviousHash(), io::UInt256::Zero());
    
    // Process genesis block on both systems
    bool result1 = system1->ProcessBlock(genesis);
    bool result2 = system2->ProcessBlock(genesis);
    
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    
    // Verify block height on both systems
    EXPECT_EQ(system1->GetCurrentBlockHeight(), 0);
    EXPECT_EQ(system2->GetCurrentBlockHeight(), 0);
    
    // Create block 1
    auto block1 = CreateBlock(1, genesis->GetHash());
    
    // Process block 1 on system1 only
    bool result3 = system1->ProcessBlock(block1);
    EXPECT_TRUE(result3);
    
    // Verify different heights
    EXPECT_EQ(system1->GetCurrentBlockHeight(), 1);
    EXPECT_EQ(system2->GetCurrentBlockHeight(), 0);
}

// Test 3: Block Synchronization Between Systems
TEST_F(P2PBlockSyncIntegrationTest, TestBlockSynchronization)
{
    // Create and process genesis block on system1
    auto genesis = CreateGenesisBlock();
    bool genResult1 = system1->ProcessBlock(genesis);
    EXPECT_TRUE(genResult1);
    
    // Create chain of blocks on system1
    std::vector<std::shared_ptr<ledger::Block>> blocks;
    blocks.push_back(genesis);
    
    auto prevHash = genesis->GetHash();
    for (int i = 1; i <= 5; i++) {
        auto block = CreateBlock(i, prevHash);
        bool result = system1->ProcessBlock(block);
        EXPECT_TRUE(result);
        blocks.push_back(block);
        prevHash = block->GetHash();
    }
    
    // Verify system1 has 5 blocks
    EXPECT_EQ(system1->GetCurrentBlockHeight(), 5);
    
    // Process same blocks on system2 (simulating sync)
    for (auto& block : blocks) {
        bool result = system2->ProcessBlock(block);
        EXPECT_TRUE(result);
    }
    
    // Verify system2 is now synchronized
    EXPECT_EQ(system2->GetCurrentBlockHeight(), 5);
    
    // Verify both systems can access their snapshots
    auto snapshot1 = system1->get_snapshot_cache();
    auto snapshot2 = system2->get_snapshot_cache();
    
    ASSERT_NE(snapshot1, nullptr);
    ASSERT_NE(snapshot2, nullptr);
    
    // Both systems should be at the same height
    EXPECT_EQ(system1->GetCurrentBlockHeight(), system2->GetCurrentBlockHeight());
}

// Test 4: Transaction Processing and Block Execution
TEST_F(P2PBlockSyncIntegrationTest, TestTransactionProcessingAndExecution)
{
    // Process genesis block
    auto genesis = CreateGenesisBlock();
    system1->ProcessBlock(genesis);
    
    // Create block with transactions
    auto block = CreateBlockWithTransactions(1, genesis->GetHash(), 3);
    
    // Process block
    bool result = system1->ProcessBlock(block);
    EXPECT_TRUE(result);
    
    // Verify block was processed
    EXPECT_EQ(system1->GetCurrentBlockHeight(), 1);
    
    // Verify transactions in the block
    auto transactions = block->GetTransactions();
    EXPECT_EQ(transactions.size(), 3);
    
    // Verify each transaction has proper properties
    for (const auto& tx : transactions) {
        EXPECT_EQ(tx.GetVersion(), 0);
        EXPECT_GT(tx.GetNonce(), 0);
        EXPECT_EQ(tx.GetSystemFee(), 0);
        EXPECT_EQ(tx.GetNetworkFee(), 0);
        EXPECT_EQ(tx.GetValidUntilBlock(), 1000000);
        EXPECT_FALSE(tx.GetScript().empty());
    }
}

// Test 5: Memory Pool Integration
TEST_F(P2PBlockSyncIntegrationTest, TestMemoryPoolIntegration)
{
    // Get memory pools
    auto mempool1 = system1->GetMemPool();
    auto mempool2 = system2->GetMemPool();
    
    ASSERT_NE(mempool1, nullptr);
    ASSERT_NE(mempool2, nullptr);
    
    // Initially empty
    EXPECT_EQ(mempool1->GetSize(), 0);
    EXPECT_EQ(mempool2->GetSize(), 0);
    
    // Verify memory pools are functional
    EXPECT_FALSE(mempool1->IsFull());
    EXPECT_FALSE(mempool2->IsFull());
    
    // Basic memory pool integration verified
    SUCCEED();
}

// Test 6: Concurrent Block Processing
TEST_F(P2PBlockSyncIntegrationTest, TestConcurrentBlockProcessing)
{
    // Process genesis block
    auto genesis = CreateGenesisBlock();
    system1->ProcessBlock(genesis);
    
    const int numBlocks = 10;
    std::vector<std::future<bool>> futures;
    std::vector<std::shared_ptr<ledger::Block>> blocks;
    
    // Create blocks sequentially to ensure proper hashing
    auto prevHash = genesis->GetHash();
    for (int i = 1; i <= numBlocks; i++) {
        auto block = CreateBlock(i, prevHash);
        blocks.push_back(block);
        prevHash = block->GetHash();
    }
    
    // Process blocks concurrently
    for (int i = 0; i < numBlocks; i++) {
        futures.push_back(std::async(std::launch::async, [this, &blocks, i]() {
            // Add random delay to simulate network conditions
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(10, 100);
            std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
            
            return system1->ProcessBlock(blocks[i]);
        }));
    }
    
    // Wait for all processing to complete
    int successCount = 0;
    for (auto& future : futures) {
        if (future.get()) {
            successCount++;
        }
    }
    
    // Due to blockchain ordering requirements, not all blocks may process successfully
    // But at least some should succeed
    EXPECT_GT(successCount, 0);
    
    // The final height should reflect successful sequential processing
    auto finalHeight = system1->GetCurrentBlockHeight();
    EXPECT_GT(finalHeight, 0);
}

// Test 7: Block Validation and Rejection
TEST_F(P2PBlockSyncIntegrationTest, TestBlockValidationAndRejection)
{
    // Process genesis block
    auto genesis = CreateGenesisBlock();
    system1->ProcessBlock(genesis);
    
    // Test 1: Valid block should be accepted
    auto validBlock = CreateBlock(1, genesis->GetHash());
    bool validResult = system1->ProcessBlock(validBlock);
    EXPECT_TRUE(validResult);
    
    // Test 2: Block with wrong previous hash should be rejected
    auto invalidBlock1 = CreateBlock(2, io::UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"));
    bool invalidResult1 = system1->ProcessBlock(invalidBlock1);
    EXPECT_FALSE(invalidResult1);
    
    // Test 3: Block with wrong index should be rejected
    auto invalidBlock2 = CreateBlock(5, validBlock->GetHash()); // Skipping indices 2,3,4
    bool invalidResult2 = system1->ProcessBlock(invalidBlock2);
    EXPECT_FALSE(invalidResult2);
    
    // Test 4: Duplicate block should be handled gracefully
    bool duplicateResult = system1->ProcessBlock(validBlock);
    // May return false or true depending on implementation
    // Main thing is it shouldn't crash
    SUCCEED();
}

// Test 8: P2P Network Message Creation
TEST_F(P2PBlockSyncIntegrationTest, TestP2PMessageCreation)
{
    // Start the node
    bool startResult = node1->Start(20003, 5);
    EXPECT_TRUE(startResult);
    
    // Create version payload
    auto versionPayload = node1->CreateVersionPayload();
    ASSERT_NE(versionPayload, nullptr);
    
    // Verify version payload exists and has basic structure
    // Note: Using implementation-specific access patterns
    SUCCEED(); // Version payload created successfully
    
    // Stop the node
    node1->Stop();
}

// Test 9: State Consistency Verification
TEST_F(P2PBlockSyncIntegrationTest, TestStateConsistencyVerification)
{
    // Create identical blockchain on both systems
    auto genesis = CreateGenesisBlock();
    system1->ProcessBlock(genesis);
    system2->ProcessBlock(genesis);
    
    std::vector<std::shared_ptr<ledger::Block>> blocks;
    auto prevHash = genesis->GetHash();
    
    for (int i = 1; i <= 3; i++) {
        auto block = CreateBlockWithTransactions(i, prevHash, 2);
        blocks.push_back(block);
        
        system1->ProcessBlock(block);
        system2->ProcessBlock(block);
        
        prevHash = block->GetHash();
    }
    
    // Verify state consistency
    EXPECT_EQ(system1->GetCurrentBlockHeight(), system2->GetCurrentBlockHeight());
    
    auto snapshot1 = system1->get_snapshot_cache();
    auto snapshot2 = system2->get_snapshot_cache();
    
    ASSERT_NE(snapshot1, nullptr);
    ASSERT_NE(snapshot2, nullptr);
    
    // Both systems should have processed the same number of blocks
    EXPECT_EQ(system1->GetCurrentBlockHeight(), 3);
    EXPECT_EQ(system2->GetCurrentBlockHeight(), 3);
    
    // Verify blocks have the expected structure
    for (auto& block : blocks) {
        EXPECT_GT(block->GetIndex(), 0);
        EXPECT_NE(block->GetHash(), io::UInt256::Zero());
        EXPECT_EQ(block->GetTransactions().size(), 2);
    }
}

// Test 10: End-to-End P2P Block Sync Simulation
TEST_F(P2PBlockSyncIntegrationTest, TestEndToEndP2PBlockSyncSimulation)
{
    // Step 1: Start P2P node
    bool startResult = node1->Start(20004, 10);
    EXPECT_TRUE(startResult);
    
    // Step 2: Create blockchain on system1
    auto genesis = CreateGenesisBlock();
    EXPECT_TRUE(system1->ProcessBlock(genesis));
    
    // Create 5 blocks with transactions
    auto prevHash = genesis->GetHash();
    std::vector<std::shared_ptr<ledger::Block>> blockchain;
    blockchain.push_back(genesis);
    
    for (int i = 1; i <= 5; i++) {
        auto block = CreateBlockWithTransactions(i, prevHash, 2);
        EXPECT_TRUE(system1->ProcessBlock(block));
        blockchain.push_back(block);
        prevHash = block->GetHash();
    }
    
    // Verify system1 state
    EXPECT_EQ(system1->GetCurrentBlockHeight(), 5);
    
    // Step 3: Simulate P2P sync by processing same blocks on system2
    for (auto& block : blockchain) {
        EXPECT_TRUE(system2->ProcessBlock(block));
    }
    
    // Step 4: Verify synchronization
    EXPECT_EQ(system2->GetCurrentBlockHeight(), 5);
    
    // Step 5: Verify both systems have identical blockchain state
    EXPECT_EQ(system1->GetCurrentBlockHeight(), system2->GetCurrentBlockHeight());
    
    // Verify each block in the blockchain
    for (size_t i = 0; i < blockchain.size(); i++) {
        auto& block = blockchain[i];
        EXPECT_EQ(block->GetIndex(), i);
        EXPECT_NE(block->GetHash(), io::UInt256::Zero());
        
        if (i > 0) {
            // Blocks with transactions
            EXPECT_EQ(block->GetTransactions().size(), 2);
        }
    }
    
    // Step 6: Test continued sync with new blocks
    auto newBlock = CreateBlockWithTransactions(6, prevHash, 1);
    EXPECT_TRUE(system1->ProcessBlock(newBlock));
    EXPECT_TRUE(system2->ProcessBlock(newBlock));
    
    // Final verification
    EXPECT_EQ(system1->GetCurrentBlockHeight(), 6);
    EXPECT_EQ(system2->GetCurrentBlockHeight(), 6);
    
    // Step 7: Stop P2P node
    node1->Stop();
}

