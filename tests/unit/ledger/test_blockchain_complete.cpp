/**
 * @file test_blockchain_complete.cpp
 * @brief Complete blockchain test suite
 */

#include <gtest/gtest.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/io/uint256.h>

namespace neo::ledger::tests {

class BlockchainCompleteTest : public ::testing::Test {
protected:
    std::unique_ptr<Blockchain> blockchain;
    
    void SetUp() override {
        blockchain = std::make_unique<Blockchain>();
    }
    
    void TearDown() override {
        blockchain.reset();
    }
};

TEST_F(BlockchainCompleteTest, GenesisBlock) {
    auto genesis = blockchain->GetGenesisBlock();
    EXPECT_NE(genesis, nullptr);
    EXPECT_EQ(genesis->Index, 0);
    EXPECT_EQ(genesis->PrevHash, io::UInt256::Zero());
}

TEST_F(BlockchainCompleteTest, GetHeight) {
    auto height = blockchain->GetHeight();
    EXPECT_GE(height, 0);
}

TEST_F(BlockchainCompleteTest, GetCurrentBlockHash) {
    auto hash = blockchain->GetCurrentBlockHash();
    EXPECT_NE(hash, io::UInt256::Zero());
}

TEST_F(BlockchainCompleteTest, GetBlock_ByIndex) {
    auto block = blockchain->GetBlock(0);
    EXPECT_NE(block, nullptr);
    EXPECT_EQ(block->Index, 0);
}

TEST_F(BlockchainCompleteTest, GetBlock_ByHash) {
    auto genesis = blockchain->GetGenesisBlock();
    auto hash = genesis->GetHash();
    auto block = blockchain->GetBlock(hash);
    EXPECT_NE(block, nullptr);
    EXPECT_EQ(block->GetHash(), hash);
}

TEST_F(BlockchainCompleteTest, ContainsBlock) {
    auto genesis = blockchain->GetGenesisBlock();
    auto hash = genesis->GetHash();
    EXPECT_TRUE(blockchain->ContainsBlock(hash));
    
    io::UInt256 randomHash;
    randomHash.Fill(0xFF);
    EXPECT_FALSE(blockchain->ContainsBlock(randomHash));
}

TEST_F(BlockchainCompleteTest, GetTransaction) {
    // Create a test transaction
    Transaction tx;
    tx.Version = 0;
    tx.Nonce = 12345;
    tx.SystemFee = 0;
    tx.NetworkFee = 0;
    tx.ValidUntilBlock = 100;
    
    auto hash = tx.GetHash();
    
    // Transaction shouldn't exist yet
    auto retrieved = blockchain->GetTransaction(hash);
    EXPECT_EQ(retrieved, nullptr);
}

TEST_F(BlockchainCompleteTest, GetBlockHeader) {
    auto header = blockchain->GetHeader(0);
    EXPECT_NE(header, nullptr);
    EXPECT_EQ(header->Index, 0);
}

TEST_F(BlockchainCompleteTest, GetNextBlockValidators) {
    auto validators = blockchain->GetNextBlockValidators();
    EXPECT_FALSE(validators.empty());
}

TEST_F(BlockchainCompleteTest, VerifyWitness) {
    io::UInt160 scriptHash;
    scriptHash.Fill(0x01);
    
    Witness witness;
    witness.InvocationScript = {0x00};
    witness.VerificationScript = {0x51}; // OP_PUSH1
    
    auto result = blockchain->VerifyWitness(scriptHash, witness, 0, 1000000);
    EXPECT_FALSE(result); // Should fail with test data
}

TEST_F(BlockchainCompleteTest, CalculateNetworkFee) {
    Transaction tx;
    tx.Version = 0;
    tx.SystemFee = 0;
    tx.NetworkFee = 0;
    
    auto fee = blockchain->CalculateNetworkFee(tx);
    EXPECT_GE(fee, 0);
}

TEST_F(BlockchainCompleteTest, GetMemoryPool) {
    auto mempool = blockchain->GetMemoryPool();
    EXPECT_NE(mempool, nullptr);
}

TEST_F(BlockchainCompleteTest, GetUnclaimedGas) {
    io::UInt160 account;
    account.Fill(0x01);
    
    auto gas = blockchain->GetUnclaimedGas(account, 100);
    EXPECT_GE(gas, 0);
}

TEST_F(BlockchainCompleteTest, BlockchainState) {
    EXPECT_FALSE(blockchain->IsStopped());
    
    blockchain->Stop();
    EXPECT_TRUE(blockchain->IsStopped());
}

} // namespace neo::ledger::tests