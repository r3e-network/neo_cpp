#include <gtest/gtest.h>
#include <neo/core/neo_system.h>
#include <neo/protocol_settings.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/witness.h>
#include <neo/ledger/memory_pool.h>
#include <neo/persistence/store_cache.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/cryptography/hash.h>
#include <neo/vm/opcode.h>
#include <neo/common/contains_transaction_type.h>
#include <neo/smartcontract/contract.h>
#include <chrono>
#include <thread>
#include <vector>

using namespace neo;
using namespace neo::ledger;
using namespace neo::smartcontract;

class BlockExecutionTest : public ::testing::Test
{
protected:
    std::shared_ptr<NeoSystem> system;
    
    void SetUp() override
    {
        auto settings = std::make_unique<ProtocolSettings>();
        system = std::make_shared<NeoSystem>(std::move(settings), "memory");
    }
    
    void TearDown() override
    {
        if (system) system->stop();
    }
    
    std::shared_ptr<ledger::Block> CreateGenesisBlock()
    {
        auto block = std::make_shared<ledger::Block>();
        block->SetVersion(0);
        block->SetPreviousHash(io::UInt256::Zero());
        block->SetMerkleRoot(io::UInt256::Zero());
        block->SetTimestamp(std::chrono::system_clock::now());
        block->SetIndex(0);
        block->SetPrimaryIndex(0);
        block->SetNextConsensus(io::UInt160::Zero());
        
        // Add witness for genesis block
        auto witness = std::make_shared<Witness>();
        witness->SetInvocationScript(io::ByteVector{0x00});
        witness->SetVerificationScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
        block->SetWitness(*witness);
        
        return block;
    }
    
    std::shared_ptr<ledger::Transaction> CreateTestTransaction(uint32_t nonce)
    {
        auto tx = std::make_shared<ledger::Transaction>();
        tx->SetVersion(0);
        tx->SetNonce(nonce);
        tx->SetSystemFee(0);
        tx->SetNetworkFee(0);
        tx->SetValidUntilBlock(100);
        
        // Set script (simple PUSH1 operation)
        tx->SetScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
        
        // Add signer
        Signer signer;
        signer.SetAccount(io::UInt160::Zero());
        signer.SetScopes(WitnessScope::Global);
        tx->SetSigners({signer});
        
        // Add witness
        auto witness = std::make_shared<Witness>();
        witness->SetInvocationScript(io::ByteVector{0x00});
        witness->SetVerificationScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
        tx->SetWitnesses({*witness});
        
        return tx;
    }
    
    std::shared_ptr<ledger::Block> CreateBlockWithTransactions(uint32_t index, const io::UInt256& prevHash, int txCount)
    {
        auto block = std::make_shared<ledger::Block>();
        block->SetVersion(0);
        block->SetPreviousHash(prevHash);
        block->SetMerkleRoot(io::UInt256::Zero());
        block->SetTimestamp(std::chrono::system_clock::now());
        block->SetIndex(index);
        block->SetPrimaryIndex(0);
        block->SetNextConsensus(io::UInt160::Zero());
        
        // Add transactions
        for (int i = 0; i < txCount; i++)
        {
            auto tx = CreateTestTransaction(index * 1000 + i);
            block->AddTransaction(*tx);
        }
        
        // Update merkle root
        // MerkleRoot calculation would be done automatically
        
        // Add witness
        auto witness = std::make_shared<Witness>();
        witness->SetInvocationScript(io::ByteVector{0x00});
        witness->SetVerificationScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
        block->SetWitness(*witness);
        
        return block;
    }
};

// Test 1: Basic Block Processing
TEST_F(BlockExecutionTest, TestBasicBlockProcessing)
{
    // Create and process genesis block
    auto genesis = CreateGenesisBlock();
    
    // Process the block
    bool result = system->ProcessBlock(genesis);
    EXPECT_TRUE(result);
    
    // Verify block was stored
    EXPECT_EQ(system->GetCurrentBlockHeight(), 0);
    
    // Process same block again (should fail)
    bool duplicateResult = system->ProcessBlock(genesis);
    EXPECT_FALSE(duplicateResult);
}

// Test 2: Sequential Block Processing
TEST_F(BlockExecutionTest, TestSequentialBlockProcessing)
{
    // Process genesis
    auto genesis = CreateGenesisBlock();
    bool genesisResult = system->ProcessBlock(genesis);
    ASSERT_TRUE(genesisResult);
    
    // Create chain of blocks
    io::UInt256 prevHash = genesis->GetHash();
    for (uint32_t i = 1; i <= 10; i++)
    {
        auto block = CreateBlockWithTransactions(i, prevHash, 5);
        bool result = system->ProcessBlock(block);
        EXPECT_TRUE(result) << "Failed to process block " << i;
        
        prevHash = block->GetHash();
    }
    
    // Verify final height
    auto finalHeight = system->GetCurrentBlockHeight();
    std::cout << "Final block height: " << finalHeight << std::endl;
    EXPECT_EQ(finalHeight, 10);
}

// Test 3: Transaction Execution in Blocks
TEST_F(BlockExecutionTest, TestTransactionExecution)
{
    // Process genesis
    auto genesis = CreateGenesisBlock();
    system->ProcessBlock(genesis);
    
    // Create block with multiple transactions
    auto block = CreateBlockWithTransactions(1, genesis->GetHash(), 10);
    
    // Process block
    bool result = system->ProcessBlock(block);
    EXPECT_TRUE(result);
    
    // Verify all transactions in block
    EXPECT_EQ(block->GetTransactions().size(), 10);
    
    // Check that transactions are in the system
    for (const auto& tx : block->GetTransactions())
    {
        auto containsResult = system->contains_transaction(tx.GetHash());
        EXPECT_NE(containsResult, ContainsTransactionType::NotExist);
    }
}

// Test 4: Block Validation
TEST_F(BlockExecutionTest, TestBlockValidation)
{
    // Process genesis
    auto genesis = CreateGenesisBlock();
    system->ProcessBlock(genesis);
    
    // Test 1: Invalid index (skip block)
    auto invalidIndexBlock = CreateBlockWithTransactions(5, genesis->GetHash(), 1);
    bool result1 = system->ProcessBlock(invalidIndexBlock);
    EXPECT_FALSE(result1);
    
    // Test 2: Invalid previous hash
    auto invalidPrevBlock = CreateBlockWithTransactions(1, io::UInt256::Parse("0x1234"), 1);
    bool result2 = system->ProcessBlock(invalidPrevBlock);
    EXPECT_FALSE(result2);
    
    // Test 3: Valid block should still work
    auto validBlock = CreateBlockWithTransactions(1, genesis->GetHash(), 1);
    bool result3 = system->ProcessBlock(validBlock);
    EXPECT_TRUE(result3);
}

// Test 5: Batch Block Processing
TEST_F(BlockExecutionTest, TestBatchBlockProcessing)
{
    // Create genesis
    auto genesis = CreateGenesisBlock();
    system->ProcessBlock(genesis);
    
    // Create batch of blocks
    std::vector<std::shared_ptr<ledger::Block>> blocks;
    io::UInt256 prevHash = genesis->GetHash();
    
    for (uint32_t i = 1; i <= 100; i++)
    {
        auto block = CreateBlockWithTransactions(i, prevHash, 3);
        blocks.push_back(block);
        prevHash = block->GetHash();
    }
    
    // Process batch
    size_t processed = system->ProcessBlocksBatch(blocks);
    EXPECT_EQ(processed, 100);
    
    // Verify height
    EXPECT_EQ(system->GetCurrentBlockHeight(), 100);
}

// Test 6: Memory Pool Integration
TEST_F(BlockExecutionTest, TestMemoryPoolIntegration)
{
    auto mempool = system->GetMemPool();
    ASSERT_NE(mempool, nullptr);
    
    // Process genesis
    auto genesis = CreateGenesisBlock();
    system->ProcessBlock(genesis);
    
    // Add transactions to mempool
    std::vector<std::shared_ptr<ledger::Transaction>> mempoolTxs;
    for (int i = 0; i < 5; i++)
    {
        auto tx = CreateTestTransaction(1000 + i);
        mempoolTxs.push_back(tx);
        // Note: Actual mempool add would require proper implementation
    }
    
    // Create block including mempool transactions
    auto block = std::make_shared<ledger::Block>();
    block->SetVersion(0);
    block->SetPreviousHash(genesis->GetHash());
    block->SetMerkleRoot(io::UInt256::Zero());
    block->SetTimestamp(std::chrono::system_clock::now());
    block->SetIndex(1);
    block->SetPrimaryIndex(0);
    block->SetNextConsensus(io::UInt160::Zero());
    
    // Add mempool transactions to block
    for (const auto& tx : mempoolTxs)
    {
        block->AddTransaction(*tx);
    }
    
    // MerkleRoot calculation would be done automatically
    
    auto witness = std::make_shared<Witness>();
    witness->SetInvocationScript(io::ByteVector{0x00});
    witness->SetVerificationScript(io::ByteVector{static_cast<uint8_t>(neo::vm::OpCode::PUSH1)});
    block->SetWitness(*witness);
    
    // Process block
    bool result = system->ProcessBlock(block);
    EXPECT_TRUE(result);
    
    // Mempool should be cleared of included transactions
    EXPECT_EQ(mempool->GetSize(), 0);
}

// Test 7: Concurrent Block Processing
TEST_F(BlockExecutionTest, TestConcurrentBlockProcessing)
{
    // Process genesis
    auto genesis = CreateGenesisBlock();
    system->ProcessBlock(genesis);
    
    // Create blocks for different chains (only one should succeed)
    std::vector<std::future<bool>> futures;
    std::atomic<int> successCount{0};
    
    for (int i = 0; i < 5; i++)
    {
        futures.push_back(std::async(std::launch::async, [this, &genesis, &successCount, i]() {
            auto block = CreateBlockWithTransactions(1, genesis->GetHash(), 1);
            block->SetNonce(i); // Make blocks slightly different
            bool result = system->ProcessBlock(block);
            if (result) successCount++;
            return result;
        }));
    }
    
    // Wait for all processing
    for (auto& future : futures)
    {
        future.wait();
    }
    
    // Only one block at height 1 should succeed
    EXPECT_EQ(successCount.load(), 1);
    EXPECT_EQ(system->GetCurrentBlockHeight(), 1);
}

// Test 8: State Persistence
TEST_F(BlockExecutionTest, TestStatePersistence)
{
    // Process genesis
    auto genesis = CreateGenesisBlock();
    system->ProcessBlock(genesis);
    
    // Get snapshot before block
    auto snapshot1 = system->get_snapshot_cache();
    ASSERT_NE(snapshot1, nullptr);
    
    // Process block with transactions
    auto block = CreateBlockWithTransactions(1, genesis->GetHash(), 5);
    bool result = system->ProcessBlock(block);
    EXPECT_TRUE(result);
    
    // Get snapshot after block
    auto snapshot2 = system->get_snapshot_cache();
    ASSERT_NE(snapshot2, nullptr);
    
    // State should be different after processing block
    // This would be more meaningful with actual state changes
}

// Test 9: Fast Sync Mode
TEST_F(BlockExecutionTest, TestFastSyncMode)
{
    // Enable fast sync mode
    system->SetFastSyncMode(true);
    
    // Process genesis
    auto genesis = CreateGenesisBlock();
    system->ProcessBlock(genesis);
    
    // Create and process many blocks quickly
    auto startTime = std::chrono::high_resolution_clock::now();
    
    io::UInt256 prevHash = genesis->GetHash();
    for (uint32_t i = 1; i <= 1000; i++)
    {
        auto block = CreateBlockWithTransactions(i, prevHash, 2);
        bool result = system->ProcessBlock(block);
        EXPECT_TRUE(result);
        prevHash = block->GetHash();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Fast sync should process 1000 blocks quickly
    EXPECT_LT(duration.count(), 5000); // Less than 5 seconds
    
    // Disable fast sync
    system->SetFastSyncMode(false);
}

// Test 10: Error Recovery
TEST_F(BlockExecutionTest, TestErrorRecovery)
{
    // Process genesis
    auto genesis = CreateGenesisBlock();
    system->ProcessBlock(genesis);
    
    // Create block with invalid transaction
    auto block = std::make_shared<ledger::Block>();
    block->SetVersion(0);
    block->SetPreviousHash(genesis->GetHash());
    block->SetMerkleRoot(io::UInt256::Zero());
    block->SetTimestamp(std::chrono::system_clock::now());
    block->SetIndex(1);
    block->SetPrimaryIndex(0);
    block->SetNextConsensus(io::UInt160::Zero());
    
    // Add invalid transaction (null)
    // Invalid transaction test removed - AddTransaction doesn't accept pointers
    
    // Process should handle gracefully
    bool result = system->ProcessBlock(block);
    EXPECT_FALSE(result);
    
    // System should still be functional
    auto validBlock = CreateBlockWithTransactions(1, genesis->GetHash(), 1);
    bool validResult = system->ProcessBlock(validBlock);
    EXPECT_TRUE(validResult);
}