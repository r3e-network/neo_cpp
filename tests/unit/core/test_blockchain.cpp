// Include the class under test

// Include the class under test
#include <neo/core/blockchain.h>

// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/core/test_blockchain.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_CORE_TEST_BLOCKCHAIN_CPP_H
#define TESTS_UNIT_CORE_TEST_BLOCKCHAIN_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/core/blockchain.h>
#include <neo/core/protocol_settings.h>
#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>

namespace neo
{
namespace test
{

class BlockchainTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for Blockchain testing
        protocol_settings = std::make_shared<ProtocolSettings>();
        protocol_settings->SetNetwork(0x860833102);
        protocol_settings->SetAddressVersion(0x35);
        protocol_settings->SetInitialGasDistribution(5200000000000000);  // 52M GAS

        // Create test blockchain instance
        blockchain = core::Blockchain::Create(protocol_settings);

        // Create test block data
        genesis_hash = io::UInt256::Parse("0x0000000000000000000000000000000000000000000000000000000000000000");
        test_block_hash = io::UInt256::Parse("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
        test_tx_hash = io::UInt256::Parse("abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890");

        // Create test transaction
        test_transaction = std::make_shared<ledger::Transaction>();
        test_transaction->SetHash(test_tx_hash);
        test_transaction->SetNetworkFee(1000000);
        test_transaction->SetSystemFee(2000000);
    }

    void TearDown() override
    {
        // Clean up test fixtures
        if (blockchain)
        {
            blockchain->Stop();
            blockchain.reset();
        }
        protocol_settings.reset();
        test_transaction.reset();
    }

    // Helper methods and test data for Blockchain testing
    std::shared_ptr<ProtocolSettings> protocol_settings;
    std::shared_ptr<core::Blockchain> blockchain;
    io::UInt256 genesis_hash;
    io::UInt256 test_block_hash;
    io::UInt256 test_tx_hash;
    std::shared_ptr<ledger::Transaction> test_transaction;
};

// Blockchain test methods converted from C# UT_Blockchain.cs functionality

TEST_F(BlockchainTest, ConstructorCreatesValidBlockchain)
{
    EXPECT_NE(blockchain, nullptr);
    EXPECT_EQ(blockchain->GetSettings(), protocol_settings);
}

TEST_F(BlockchainTest, GetGenesisBlock)
{
    auto genesis_block = blockchain->GetGenesisBlock();
    EXPECT_NE(genesis_block, nullptr);
    EXPECT_EQ(genesis_block->GetIndex(), 0);
}

TEST_F(BlockchainTest, GetCurrentHeight)
{
    uint32_t height = blockchain->GetHeight();
    EXPECT_GE(height, 0);  // Should be at least genesis block
}

TEST_F(BlockchainTest, GetBestBlockHash)
{
    auto best_hash = blockchain->GetBestBlockHash();
    EXPECT_NE(best_hash, io::UInt256());  // Should not be empty
}

TEST_F(BlockchainTest, ContainsBlock)
{
    // Genesis block should always exist
    auto genesis_block = blockchain->GetGenesisBlock();
    EXPECT_TRUE(blockchain->ContainsBlock(genesis_block->GetHash()));

    // Non-existent block should not exist
    EXPECT_FALSE(blockchain->ContainsBlock(test_block_hash));
}

TEST_F(BlockchainTest, ContainsTransaction)
{
    // Test with non-existent transaction
    EXPECT_FALSE(blockchain->ContainsTransaction(test_tx_hash));
}

TEST_F(BlockchainTest, GetBlockByHash)
{
    auto genesis_block = blockchain->GetGenesisBlock();
    auto retrieved_block = blockchain->GetBlock(genesis_block->GetHash());

    EXPECT_NE(retrieved_block, nullptr);
    EXPECT_EQ(retrieved_block->GetHash(), genesis_block->GetHash());
    EXPECT_EQ(retrieved_block->GetIndex(), 0);
}

TEST_F(BlockchainTest, GetBlockByIndex)
{
    auto genesis_block = blockchain->GetBlock(0);
    EXPECT_NE(genesis_block, nullptr);
    EXPECT_EQ(genesis_block->GetIndex(), 0);
}

TEST_F(BlockchainTest, GetNonExistentBlock)
{
    auto non_existent = blockchain->GetBlock(test_block_hash);
    EXPECT_EQ(non_existent, nullptr);

    auto high_index_block = blockchain->GetBlock(999999);
    EXPECT_EQ(high_index_block, nullptr);
}

TEST_F(BlockchainTest, GetTransaction)
{
    // Test with non-existent transaction
    auto non_existent_tx = blockchain->GetTransaction(test_tx_hash);
    EXPECT_EQ(non_existent_tx, nullptr);
}

TEST_F(BlockchainTest, IsRunning)
{
    // Blockchain should be running after creation
    EXPECT_TRUE(blockchain->IsRunning());

    // Stop and check
    blockchain->Stop();
    EXPECT_FALSE(blockchain->IsRunning());
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_CORE_TEST_BLOCKCHAIN_CPP_H