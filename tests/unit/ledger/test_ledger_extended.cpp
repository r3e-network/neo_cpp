#include <gtest/gtest.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/block.h>
#include <neo/ledger/mempool.h>
#include <neo/core/types.h>
#include <neo/io/byte_vector.h>

using namespace neo::ledger;
using namespace neo::core;
using namespace neo::io;

class LedgerExtendedTest : public ::testing::Test
{
protected:
    std::unique_ptr<Blockchain> blockchain;
    
    void SetUp() override
    {
        blockchain = std::make_unique<Blockchain>();
        blockchain->Initialize();
    }
};

TEST_F(LedgerExtendedTest, TestGetBlock_Genesis)
{
    // Get genesis block
    auto genesisBlock = blockchain->GetBlock(0);
    
    EXPECT_NE(genesisBlock, nullptr);
    EXPECT_EQ(genesisBlock->Index, 0);
    EXPECT_EQ(genesisBlock->PrevHash, UInt256::Zero());
    EXPECT_GT(genesisBlock->Timestamp, 0);
    EXPECT_FALSE(genesisBlock->NextConsensus.IsZero());
}

TEST_F(LedgerExtendedTest, TestGetBlock_NoTransactions)
{
    // Create a block with no transactions
    Block emptyBlock;
    emptyBlock.Index = 1;
    emptyBlock.PrevHash = blockchain->GetBlock(0)->Hash;
    emptyBlock.Timestamp = std::time(nullptr);
    emptyBlock.NextConsensus = UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678");
    
    // Add block to blockchain
    EXPECT_TRUE(blockchain->AddBlock(emptyBlock));
    
    // Retrieve and verify
    auto retrievedBlock = blockchain->GetBlock(1);
    EXPECT_NE(retrievedBlock, nullptr);
    EXPECT_EQ(retrievedBlock->Transactions.size(), 0);
}

TEST_F(LedgerExtendedTest, TestGetBlockCount)
{
    uint32_t initialCount = blockchain->GetBlockCount();
    EXPECT_GE(initialCount, 1); // At least genesis block
    
    // Add a new block
    Block newBlock;
    newBlock.Index = initialCount;
    newBlock.PrevHash = blockchain->GetCurrentBlockHash();
    newBlock.Timestamp = std::time(nullptr);
    
    EXPECT_TRUE(blockchain->AddBlock(newBlock));
    EXPECT_EQ(blockchain->GetBlockCount(), initialCount + 1);
}

TEST_F(LedgerExtendedTest, TestGetBlockHeaderCount)
{
    uint32_t headerCount = blockchain->GetHeaderCount();
    uint32_t blockCount = blockchain->GetBlockCount();
    
    // Header count should be >= block count
    EXPECT_GE(headerCount, blockCount);
}

TEST_F(LedgerExtendedTest, TestGetBlockHeader)
{
    auto header = blockchain->GetHeader(0);
    
    EXPECT_NE(header, nullptr);
    EXPECT_EQ(header->Index, 0);
    EXPECT_EQ(header->PrevHash, UInt256::Zero());
    
    // Header should not contain transactions
    EXPECT_EQ(header->Transactions.size(), 0);
}

TEST_F(LedgerExtendedTest, TestGetContractState)
{
    // Test native contract lookup
    UInt160 neoContractHash = UInt160::Parse("0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5");
    auto contractState = blockchain->GetContractState(neoContractHash);
    
    EXPECT_NE(contractState, nullptr);
    EXPECT_EQ(contractState->Hash, neoContractHash);
    EXPECT_FALSE(contractState->Manifest.Name.empty());
    
    // Test non-existent contract
    UInt160 nonExistentHash = UInt160::Parse("0x0000000000000000000000000000000000000000");
    auto nullState = blockchain->GetContractState(nonExistentHash);
    EXPECT_EQ(nullState, nullptr);
}

TEST_F(LedgerExtendedTest, TestGetRawMemPool)
{
    // Get initial mempool state
    auto initialPool = blockchain->GetMemPool()->GetRawMemPool();
    size_t initialSize = initialPool.size();
    
    // Create a test transaction
    Transaction tx;
    tx.Version = 0;
    tx.Nonce = std::rand();
    tx.SystemFee = 1000000;
    tx.NetworkFee = 1000000;
    tx.ValidUntilBlock = blockchain->GetBlockCount() + 100;
    
    // Add to mempool
    EXPECT_TRUE(blockchain->GetMemPool()->TryAdd(tx));
    
    // Verify mempool updated
    auto updatedPool = blockchain->GetMemPool()->GetRawMemPool();
    EXPECT_EQ(updatedPool.size(), initialSize + 1);
    EXPECT_TRUE(updatedPool.find(tx.Hash()) != updatedPool.end());
}