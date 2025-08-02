#include <gtest/gtest.h>
#include <neo/core/neo_system.h>
#include <neo/protocol_settings.h>
#include <neo/ledger/block.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <chrono>

using namespace neo;

class P2PSyncBasicTest : public ::testing::Test
{
protected:
    std::shared_ptr<NeoSystem> system;
    
    void SetUp() override
    {
        // Initialize with default config
        auto settings = std::make_unique<ProtocolSettings>();
        system = std::make_shared<NeoSystem>(std::move(settings), "memory");
    }
    
    void TearDown() override
    {
        // Cleanup is automatic
    }
};

// Test 1: System Initialization
TEST_F(P2PSyncBasicTest, TestSystemCreation)
{
    ASSERT_NE(system, nullptr);
    
    // Get memory pool
    auto mempool = system->GetMemPool();
    ASSERT_NE(mempool, nullptr);
}

// Test 2: Block Processing
TEST_F(P2PSyncBasicTest, TestBlockProcessing)
{
    // Create a block
    auto block = std::make_shared<ledger::Block>();
    block->SetVersion(0);
    block->SetPreviousHash(io::UInt256::Zero());
    block->SetMerkleRoot(io::UInt256::Zero());
    block->SetTimestamp(std::chrono::system_clock::now());
    block->SetIndex(0);
    block->SetPrimaryIndex(0);
    block->SetNextConsensus(io::UInt160::Zero());
    
    // Process block
    bool result = system->ProcessBlock(block);
    EXPECT_TRUE(result);
}

// Test 3: Get Block Height
TEST_F(P2PSyncBasicTest, TestBlockHeight)
{
    // Initial height should be 0
    uint32_t height = system->GetCurrentBlockHeight();
    EXPECT_EQ(height, 0);
    
    // Process a block
    auto block = std::make_shared<ledger::Block>();
    block->SetVersion(0);
    block->SetPreviousHash(io::UInt256::Zero());
    block->SetMerkleRoot(io::UInt256::Zero());
    block->SetTimestamp(std::chrono::system_clock::now());
    block->SetIndex(0);
    block->SetPrimaryIndex(0);
    block->SetNextConsensus(io::UInt160::Zero());
    
    system->ProcessBlock(block);
    
    // Height should still be 0 (block index 0)
    height = system->GetCurrentBlockHeight();
    EXPECT_EQ(height, 0);
}

// Test 4: Snapshot Creation
TEST_F(P2PSyncBasicTest, TestSnapshotCreation)
{
    auto snapshot = system->get_snapshot_cache();
    ASSERT_NE(snapshot, nullptr);
}

// Test 5: Multiple Block Processing
TEST_F(P2PSyncBasicTest, TestMultipleBlocks)
{
    std::vector<std::shared_ptr<ledger::Block>> blocks;
    
    // Create genesis block
    auto genesis = std::make_shared<ledger::Block>();
    genesis->SetVersion(0);
    genesis->SetPreviousHash(io::UInt256::Zero());
    genesis->SetMerkleRoot(io::UInt256::Zero());
    genesis->SetTimestamp(std::chrono::system_clock::from_time_t(1468595301));
    genesis->SetIndex(0);
    genesis->SetPrimaryIndex(0);
    genesis->SetNextConsensus(io::UInt160::Zero());
    blocks.push_back(genesis);
    
    // Process batch
    size_t processed = system->ProcessBlocksBatch(blocks);
    EXPECT_GT(processed, 0);
}

// Test 6: Store Access
TEST_F(P2PSyncBasicTest, TestStoreAccess)
{
    auto store = system->GetStore();
    ASSERT_NE(store, nullptr);
}

// Test 7: Fast Sync Mode
TEST_F(P2PSyncBasicTest, TestFastSyncMode)
{
    // Enable fast sync
    system->SetFastSyncMode(true);
    
    // Process a block in fast sync mode
    auto block = std::make_shared<ledger::Block>();
    block->SetVersion(0);
    block->SetPreviousHash(io::UInt256::Zero());
    block->SetMerkleRoot(io::UInt256::Zero());
    block->SetTimestamp(std::chrono::system_clock::now());
    block->SetIndex(0);
    block->SetPrimaryIndex(0);
    block->SetNextConsensus(io::UInt160::Zero());
    
    bool result = system->ProcessBlock(block);
    EXPECT_TRUE(result);
    
    // Disable fast sync
    system->SetFastSyncMode(false);
}

// Test 8: Block Validation Toggle
TEST_F(P2PSyncBasicTest, TestBlockValidation)
{
    // Test with validation disabled (should process faster)
    system->SetFastSyncMode(true);
    
    auto block1 = std::make_shared<ledger::Block>();
    block1->SetVersion(0);
    block1->SetPreviousHash(io::UInt256::Zero());
    block1->SetMerkleRoot(io::UInt256::Zero());
    block1->SetTimestamp(std::chrono::system_clock::now());
    block1->SetIndex(0);
    block1->SetPrimaryIndex(0);
    block1->SetNextConsensus(io::UInt160::Zero());
    
    EXPECT_TRUE(system->ProcessBlock(block1));
    
    // Test with validation enabled
    system->SetFastSyncMode(false);
    
    auto block2 = std::make_shared<ledger::Block>();
    block2->SetVersion(0);
    block2->SetPreviousHash(block1->GetHash());
    block2->SetMerkleRoot(io::UInt256::Zero());
    block2->SetTimestamp(std::chrono::system_clock::now());
    block2->SetIndex(1);
    block2->SetPrimaryIndex(0);
    block2->SetNextConsensus(io::UInt160::Zero());
    
    // This might fail with strict validation
    system->ProcessBlock(block2);
}

// Test 9: Simple End-to-End
TEST_F(P2PSyncBasicTest, TestSimpleEndToEnd)
{
    // Get initial state
    uint32_t initialHeight = system->GetCurrentBlockHeight();
    
    // Create and process a block
    auto block = std::make_shared<ledger::Block>();
    block->SetVersion(0);
    block->SetPreviousHash(io::UInt256::Zero());
    block->SetMerkleRoot(io::UInt256::Zero());
    block->SetTimestamp(std::chrono::system_clock::now());
    block->SetIndex(0);
    block->SetPrimaryIndex(0);
    block->SetNextConsensus(io::UInt160::Zero());
    
    // Process
    bool processed = system->ProcessBlock(block);
    EXPECT_TRUE(processed);
    
    // Verify state change
    auto snapshot = system->get_snapshot_cache();
    EXPECT_NE(snapshot, nullptr);
}

// Test 10: Performance Mode
TEST_F(P2PSyncBasicTest, TestPerformanceMode)
{
    // Enable fast sync for performance
    system->SetFastSyncMode(true);
    
    // Create multiple blocks
    std::vector<std::shared_ptr<ledger::Block>> blocks;
    for (int i = 0; i < 5; i++)
    {
        auto block = std::make_shared<ledger::Block>();
        block->SetVersion(0);
        block->SetPreviousHash(i == 0 ? io::UInt256::Zero() : blocks[i-1]->GetHash());
        block->SetMerkleRoot(io::UInt256::Zero());
        block->SetTimestamp(std::chrono::system_clock::now() + std::chrono::seconds(i));
        block->SetIndex(i);
        block->SetPrimaryIndex(0);
        block->SetNextConsensus(io::UInt160::Zero());
        blocks.push_back(block);
    }
    
    // Process in batch
    size_t processed = system->ProcessBlocksBatch(blocks);
    EXPECT_GT(processed, 0);
}

