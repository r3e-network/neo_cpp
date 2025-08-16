#include <gtest/gtest.h>
#include <neo/core/neo_system.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/block_sync_manager.h>
#include <neo/ledger/block.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <neo/persistence/store_factory.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/native_contract.h>
#include <neo/io/uint256.h>
#include <neo/network/p2p/payloads/block_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p/message.h>
#include <neo/ledger/memory_pool.h>
#include <neo/network/ip_endpoint.h>
#include <neo/io/memory_stream.h>
#include <thread>
#include <chrono>
#include <future>
#include <filesystem>

using namespace neo;
using namespace neo::network::p2p;
using namespace neo::ledger;
using namespace neo::persistence;
using namespace std::chrono_literals;

class P2PSyncExecutionTest : public ::testing::Test
{
protected:
    std::shared_ptr<core::NeoSystem> system;
    std::shared_ptr<LocalNode> localNode;
    std::shared_ptr<IStore> store;
    
    void SetUp() override
    {
        // Create test database
        auto factory = StoreFactory::GetInstance();
        store = factory->CreateStore("test_db", StoreOptions{
            .isReadOnly = false,
            .createIfMissing = true,
            .maxOpenFiles = 100,
            .writeBufferSize = 67108864, // 64MB
            .blockCacheSize = 1073741824 // 1GB
        });
        
        // Initialize NeoSystem
        system = std::make_shared<core::NeoSystem>(store);
        
        // Initialize LocalNode for P2P
        localNode = std::make_shared<LocalNode>(system);
    }
    
    void TearDown() override
    {
        if (localNode)
        {
            localNode->Stop();
        }
        
        // Clean up test database
        std::filesystem::remove_all("test_db");
    }
};

// Test 1: P2P Connection Establishment
TEST_F(P2PSyncExecutionTest, TestP2PConnection)
{
    // Configure test network settings
    localNode->set_listening_port(20333);
    localNode->set_nonce(12345);
    
    // Start the local node
    ASSERT_NO_THROW(localNode->Start());
    
    // Give it time to initialize
    std::this_thread::sleep_for(100ms);
    
    // Verify node is listening
    EXPECT_TRUE(localNode->is_running());
    EXPECT_EQ(localNode->get_listening_port(), 20333);
    
    // Test adding a peer
    auto endpoint = network::IPEndpoint::Parse("127.0.0.1:20334");
    ASSERT_TRUE(endpoint.has_value());
    
    // Attempt to connect to peer (this will fail in test but verifies the mechanism)
    localNode->ConnectToPeer(endpoint.value());
    
    // Give it time to attempt connection
    std::this_thread::sleep_for(500ms);
    
    // In a real test, we'd verify the connection was attempted
    // For now, just verify the node is still running
    EXPECT_TRUE(localNode->is_running());
}

// Test 2: Block Synchronization Manager
TEST_F(P2PSyncExecutionTest, TestBlockSyncManager)
{
    // Initialize block sync manager
    auto syncManager = std::make_shared<BlockSyncManager>(localNode, system);
    
    // Start sync manager
    syncManager->Start();
    
    // Create a mock block for testing
    auto block = std::make_shared<Block>();
    block->SetVersion(0);
    block->SetPreviousHash(io::UInt256::Zero());
    block->SetMerkleRoot(io::UInt256::Zero());
    block->SetTimestamp(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
    block->SetIndex(1);
    block->SetPrimaryIndex(0);
    block->SetNextConsensus(io::UInt160::Zero());
    
    // Test orphan block handling
    syncManager->OnBlockReceived(block);
    
    // Verify orphan blocks are tracked
    EXPECT_TRUE(syncManager->HasOrphanBlock(block->GetHash()));
    
    // Stop sync manager
    syncManager->Stop();
}

// Test 3: Block Processing and Execution
TEST_F(P2PSyncExecutionTest, TestBlockProcessing)
{
    // Create genesis block
    auto genesis = std::make_shared<Block>();
    genesis->SetVersion(0);
    genesis->SetPreviousHash(io::UInt256::Zero());
    genesis->SetMerkleRoot(io::UInt256::Zero());
    genesis->SetTimestamp(static_cast<uint64_t>(1468595301)); // Neo genesis time
    genesis->SetIndex(0);
    genesis->SetPrimaryIndex(0);
    genesis->SetNextConsensus(io::UInt160::Zero());
    
    // Process genesis block
    bool result = system->ProcessBlock(genesis);
    EXPECT_TRUE(result);
    
    // Verify block was stored
    auto snapshot = system->get_snapshot_cache();
    auto storedBlock = snapshot->GetBlock(genesis->GetHash());
    ASSERT_NE(storedBlock, nullptr);
    EXPECT_EQ(storedBlock->GetIndex(), 0);
}

// Test 4: Transaction Execution
TEST_F(P2PSyncExecutionTest, TestTransactionExecution)
{
    // Create a simple transfer transaction
    auto tx = std::make_shared<network::p2p::payloads::Neo3Transaction>();
    tx->version = 0;
    tx->nonce = 12345;
    tx->systemFee = 0;
    tx->networkFee = 0;
    tx->validUntilBlock = 100;
    
    // Create snapshot for execution
    auto snapshot = system->get_snapshot_cache();
    
    // Create application engine for execution
    vm::TriggerType trigger = vm::TriggerType::Application;
    auto engine = std::make_unique<vm::ApplicationEngine>(
        trigger,
        nullptr, // No container for standalone execution
        snapshot,
        tx,
        0, // No gas limit for test
        false // Not test mode
    );
    
    // Execute empty script for test
    engine->LoadScript(std::vector<uint8_t>{0x51}); // PUSH1 opcode
    auto execResult = engine->Execute();
    
    // Should execute successfully
    EXPECT_EQ(execResult, vm::VMState::HALT);
}

// Test 5: End-to-End Integration Test
TEST_F(P2PSyncExecutionTest, TestEndToEndFlow)
{
    // Step 1: Start P2P node
    localNode->set_listening_port(20335);
    localNode->Start();
    EXPECT_TRUE(localNode->is_running());
    
    // Step 2: Initialize block sync
    auto syncManager = std::make_shared<BlockSyncManager>(localNode, system);
    syncManager->Start();
    
    // Step 3: Create and process a block with transactions
    auto block = std::make_shared<Block>();
    block->SetVersion(0);
    block->SetPreviousHash(io::UInt256::Zero());
    block->SetMerkleRoot(io::UInt256::Zero());
    block->SetTimestamp(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
    block->SetIndex(0);
    block->SetPrimaryIndex(0);
    block->SetNextConsensus(io::UInt160::Zero());
    
    // Add a transaction to the block
    auto tx = std::make_shared<network::p2p::payloads::Neo3Transaction>();
    tx->version = 0;
    tx->nonce = 1;
    tx->systemFee = 0;
    tx->networkFee = 0;
    tx->validUntilBlock = 100;
    tx->script = {0x51}; // PUSH1
    
    block->AddTransaction(*tx);
    
    // Process the block
    bool processed = system->ProcessBlock(block);
    EXPECT_TRUE(processed);
    
    // Verify the block and transaction were stored
    auto snapshot = system->get_snapshot_cache();
    
    // Check block storage
    auto storedBlock = snapshot->GetBlock(block->GetHash());
    ASSERT_NE(storedBlock, nullptr);
    EXPECT_EQ(storedBlock->GetTransactions().size(), 1);
    
    // Check transaction storage
    auto storedTx = snapshot->GetTransaction(tx->GetHash());
    ASSERT_NE(storedTx, nullptr);
    EXPECT_EQ(storedTx->nonce, 1);
    
    // Check blockchain height was updated
    auto height = snapshot->GetHeight();
    EXPECT_EQ(height, 0);
    
    // Step 4: Cleanup
    syncManager->Stop();
    localNode->Stop();
}

// Test 6: P2P Message Handling
TEST_F(P2PSyncExecutionTest, TestP2PMessageHandling)
{
    localNode->set_listening_port(20336);
    localNode->Start();
    
    // Create a version message
    auto versionPayload = std::make_shared<payloads::VersionPayload>();
    versionPayload->version = 0;
    versionPayload->services = 1; // NODE_NETWORK
    versionPayload->timestamp = std::chrono::system_clock::now();
    versionPayload->port = 20336;
    versionPayload->nonce = 12345;
    versionPayload->userAgent = "NEO:3.0.0";
    versionPayload->startHeight = 0;
    versionPayload->relay = true;
    
    Message versionMsg(MessageCommand::Version, versionPayload);
    
    // Test message serialization
    std::vector<uint8_t> data;
    io::MemoryStream stream(data);
    versionMsg.Serialize(stream);
    
    EXPECT_GT(data.size(), 0);
    
    // Test message deserialization
    io::MemoryStream readStream(data);
    Message deserializedMsg;
    deserializedMsg.Deserialize(readStream);
    
    EXPECT_EQ(deserializedMsg.command, MessageCommand::Version);
    
    localNode->Stop();
}

// Test 7: Block Validation
TEST_F(P2PSyncExecutionTest, TestBlockValidation)
{
    // Create a valid block
    auto validBlock = std::make_shared<Block>();
    validBlock->SetVersion(0);
    validBlock->SetPreviousHash(io::UInt256::Zero());
    validBlock->SetMerkleRoot(io::UInt256::Zero());
    validBlock->SetTimestamp(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
    validBlock->SetIndex(0);
    validBlock->SetPrimaryIndex(0);
    validBlock->SetNextConsensus(io::UInt160::Zero());
    
    // Process should succeed
    EXPECT_TRUE(system->ProcessBlock(validBlock));
    
    // Create an invalid block (wrong previous hash for block 1)
    auto invalidBlock = std::make_shared<Block>();
    invalidBlock->SetVersion(0);
    invalidBlock->SetPreviousHash(io::UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"));
    invalidBlock->SetMerkleRoot(io::UInt256::Zero());
    invalidBlock->SetTimestamp(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
    invalidBlock->SetIndex(1);
    invalidBlock->SetPrimaryIndex(0);
    invalidBlock->SetNextConsensus(io::UInt160::Zero());
    
    // Process should fail due to wrong previous hash
    EXPECT_FALSE(system->ProcessBlock(invalidBlock));
}

// Test 8: Memory Pool Integration
TEST_F(P2PSyncExecutionTest, TestMemoryPoolIntegration)
{
    // Get the memory pool from the system
    auto& mempool = system->get_memory_pool();
    
    // Create a transaction
    auto tx = std::make_shared<network::p2p::payloads::Neo3Transaction>();
    tx->version = 0;
    tx->nonce = 1000;
    tx->systemFee = 0;
    tx->networkFee = 0;
    tx->validUntilBlock = 1000;
    tx->script = {0x51}; // PUSH1
    
    // Add to memory pool
    bool added = mempool.TryAdd(*tx);
    EXPECT_TRUE(added);
    
    // Verify transaction is in pool
    EXPECT_TRUE(mempool.Contains(tx->GetHash()));
    
    // Create a block containing this transaction
    auto block = std::make_shared<Block>();
    block->header.version = 0;
    block->header.prevHash = io::UInt256::Zero();
    block->header.merkleRoot = io::UInt256::Zero();
    block->header.timestamp = std::chrono::system_clock::now();
    block->header.index = 0;
    block->header.primaryIndex = 0;
    block->header.nextConsensus = io::UInt160::Zero();
    block->AddTransaction(*tx);
    
    // Process the block
    EXPECT_TRUE(system->ProcessBlock(block));
    
    // Transaction should be removed from memory pool
    EXPECT_FALSE(mempool.Contains(tx->GetHash()));
}

// Test 9: Concurrent Block Processing
TEST_F(P2PSyncExecutionTest, TestConcurrentBlockProcessing)
{
    const int numBlocks = 10;
    std::vector<std::future<bool>> futures;
    
    // Create genesis block first
    auto genesis = std::make_shared<Block>();
    genesis->SetVersion(0);
    genesis->SetPreviousHash(io::UInt256::Zero());
    genesis->SetMerkleRoot(io::UInt256::Zero());
            genesis->SetTimestamp(static_cast<uint64_t>(1468595301));
    genesis->SetIndex(0);
    genesis->SetPrimaryIndex(0);
    genesis->SetNextConsensus(io::UInt160::Zero());
    
    EXPECT_TRUE(system->ProcessBlock(genesis));
    
    // Try to process multiple blocks concurrently
    for (int i = 1; i <= numBlocks; i++)
    {
        futures.push_back(std::async(std::launch::async, [this, i, prevHash = genesis->GetHash()]() {
            auto block = std::make_shared<Block>();
            block->SetVersion(0);
            block->SetPreviousHash(prevHash);
            block->SetMerkleRoot(io::UInt256::Zero());
            block->SetTimestamp(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + std::chrono::seconds(i)).time_since_epoch()).count()));
            block->SetIndex(i);
            block->SetPrimaryIndex(0);
            block->SetNextConsensus(io::UInt160::Zero());
            
            // Add some delay to simulate network latency
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 100));
            
            return system->ProcessBlock(block);
        }));
    }
    
    // Wait for all blocks to be processed
    int successCount = 0;
    for (auto& future : futures)
    {
        if (future.get())
        {
            successCount++;
        }
    }
    
    // Due to ordering requirements, only some blocks may succeed
    EXPECT_GT(successCount, 0);
}

// Test 10: Network Failure Recovery
TEST_F(P2PSyncExecutionTest, TestNetworkFailureRecovery)
{
    localNode->set_listening_port(20337);
    localNode->Start();
    
    auto syncManager = std::make_shared<BlockSyncManager>(localNode, system);
    syncManager->Start();
    
    // Simulate network interruption by stopping the node
    localNode->Stop();
    
    // Wait a bit
    std::this_thread::sleep_for(500ms);
    
    // Restart the node
    localNode->Start();
    
    // Verify recovery
    EXPECT_TRUE(localNode->is_running());
    
    // Cleanup
    syncManager->Stop();
    localNode->Stop();
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}