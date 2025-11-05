#include <gtest/gtest.h>

#include <neo/core/neo_system.h>
#include <neo/core/neo_system_factory.h>
#include <neo/ledger/block.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/header.h>
#include <neo/protocol_settings.h>
#include "utils/test_helpers.h"

namespace neo::ledger::tests
{

class BlockchainCompleteTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        auto settings = std::make_unique<neo::ProtocolSettings>();
        settings->SetNetwork(0x334E454F);        // TESTNET magic
        settings->SetValidatorsCount(7);         // default N3 value
        settings->SetMillisecondsPerBlock(15000);

        neo_system_ = neo::NeoSystemFactory::Create(std::move(settings), "memory", "blockchain_complete_test");
        ASSERT_NE(neo_system_, nullptr);

        blockchain_ = neo_system_->GetBlockchain();
        ASSERT_NE(blockchain_, nullptr);
    }

    void TearDown() override
    {
        if (blockchain_ != nullptr)
        {
            blockchain_->Stop();
        }
        neo_system_.reset();
        blockchain_ = nullptr;
    }

    std::shared_ptr<neo::NeoSystem> neo_system_;
    neo::ledger::Blockchain* blockchain_{nullptr};
};

TEST_F(BlockchainCompleteTest, GenesisBlockAvailable)
{
    auto block_zero = blockchain_->GetBlock(0);
    ASSERT_NE(block_zero, nullptr);

    EXPECT_EQ(block_zero->GetIndex(), 0u);
    EXPECT_EQ(block_zero->GetPrevHash(), io::UInt256::Zero());
    EXPECT_NE(block_zero->GetHash(), io::UInt256::Zero());
}

TEST_F(BlockchainCompleteTest, CurrentBlockHashMatchesStoredGenesisHash)
{
    auto current_hash = blockchain_->GetCurrentBlockHash();
    auto stored_hash = blockchain_->GetBlockHash(0);

    EXPECT_EQ(current_hash, stored_hash);
    EXPECT_NE(current_hash, io::UInt256::Zero());
}

TEST_F(BlockchainCompleteTest, HeightStartsAtGenesis)
{
    auto height = blockchain_->GetHeight();
    EXPECT_EQ(height, 0u);
    EXPECT_EQ(height, blockchain_->GetCurrentBlockIndex());
}

TEST_F(BlockchainCompleteTest, GetBlockByHashReturnsGenesis)
{
    auto genesis_hash = blockchain_->GetBlockHash(0);
    auto block = blockchain_->GetBlock(genesis_hash);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->GetIndex(), 0u);
    EXPECT_EQ(block->GetHash(), genesis_hash);
}

TEST_F(BlockchainCompleteTest, ContainsBlockDetectsGenesis)
{
    auto genesis_hash = blockchain_->GetBlockHash(0);
    EXPECT_TRUE(blockchain_->ContainsBlock(genesis_hash));

    auto random_hash = neo::tests::TestHelpers::GenerateRandomHash();
    EXPECT_FALSE(blockchain_->ContainsBlock(random_hash));
}

TEST_F(BlockchainCompleteTest, GetBlockHeaderByIndex)
{
    auto header = blockchain_->GetBlockHeader(0);
    ASSERT_NE(header, nullptr);
    EXPECT_EQ(header->GetIndex(), 0u);
    EXPECT_EQ(header->GetPrevHash(), io::UInt256::Zero());
}

TEST_F(BlockchainCompleteTest, MissingTransactionLookupReturnsNull)
{
    auto random_hash = neo::tests::TestHelpers::GenerateRandomHash();
    auto tx = blockchain_->GetTransaction(random_hash);
    EXPECT_EQ(tx, nullptr);
    EXPECT_EQ(blockchain_->GetTransactionHeight(random_hash), -1);
}

}  // namespace neo::ledger::tests
