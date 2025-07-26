// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/ledger/test_trimmedblock.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_LEDGER_TEST_TRIMMEDBLOCK_CPP_H
#define TESTS_UNIT_LEDGER_TEST_TRIMMEDBLOCK_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/ledger/trimmed_block.h>

namespace neo {
namespace test {

class TrimmedBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for TrimmedBlock testing
        block_hash = io::UInt256::Parse("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
        previous_hash = io::UInt256::Parse("abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890");
        merkle_root = io::UInt256::Parse("fedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321");
        
        // Create test transactions
        tx_hashes.push_back(io::UInt256::Parse("1111111111111111111111111111111111111111111111111111111111111111"));
        tx_hashes.push_back(io::UInt256::Parse("2222222222222222222222222222222222222222222222222222222222222222"));
        tx_hashes.push_back(io::UInt256::Parse("3333333333333333333333333333333333333333333333333333333333333333"));
        
        // Create trimmed block
        trimmed_block = std::make_shared<ledger::TrimmedBlock>(
            1,                    // version
            previous_hash,        // previous hash
            merkle_root,          // merkle root
            1640995200,           // timestamp (2022-01-01)
            12345,                // nonce
            100,                  // index
            0,                    // primary index
            next_consensus,       // next consensus
            tx_hashes             // transaction hashes
        );
        
        empty_trimmed_block = std::make_shared<ledger::TrimmedBlock>();
    }

    void TearDown() override {
        // Clean up test fixtures
        trimmed_block.reset();
        empty_trimmed_block.reset();
        tx_hashes.clear();
    }

    // Helper methods and test data for TrimmedBlock testing
    io::UInt256 block_hash;
    io::UInt256 previous_hash;
    io::UInt256 merkle_root;
    io::UInt160 next_consensus;
    std::vector<io::UInt256> tx_hashes;
    std::shared_ptr<ledger::TrimmedBlock> trimmed_block;
    std::shared_ptr<ledger::TrimmedBlock> empty_trimmed_block;
};

// TrimmedBlock test methods converted from C# UT_TrimmedBlock.cs functionality

TEST_F(TrimmedBlockTest, ConstructorWithFullData) {
    EXPECT_EQ(trimmed_block->GetVersion(), 1);
    EXPECT_EQ(trimmed_block->GetPreviousHash(), previous_hash);
    EXPECT_EQ(trimmed_block->GetMerkleRoot(), merkle_root);
    EXPECT_EQ(trimmed_block->GetTimestamp(), 1640995200);
    EXPECT_EQ(trimmed_block->GetNonce(), 12345);
    EXPECT_EQ(trimmed_block->GetIndex(), 100);
    EXPECT_EQ(trimmed_block->GetPrimaryIndex(), 0);
    EXPECT_EQ(trimmed_block->GetNextConsensus(), next_consensus);
}

TEST_F(TrimmedBlockTest, DefaultConstructor) {
    EXPECT_EQ(empty_trimmed_block->GetVersion(), 0);
    EXPECT_EQ(empty_trimmed_block->GetIndex(), 0);
    EXPECT_EQ(empty_trimmed_block->GetTransactionCount(), 0);
}

TEST_F(TrimmedBlockTest, GetTransactionHashes) {
    auto hashes = trimmed_block->GetTransactionHashes();
    EXPECT_EQ(hashes.size(), tx_hashes.size());
    
    for (size_t i = 0; i < tx_hashes.size(); ++i) {
        EXPECT_EQ(hashes[i], tx_hashes[i]);
    }
}

TEST_F(TrimmedBlockTest, GetTransactionCount) {
    EXPECT_EQ(trimmed_block->GetTransactionCount(), 3);
    EXPECT_EQ(empty_trimmed_block->GetTransactionCount(), 0);
}

TEST_F(TrimmedBlockTest, ContainsTransaction) {
    EXPECT_TRUE(trimmed_block->ContainsTransaction(tx_hashes[0]));
    EXPECT_TRUE(trimmed_block->ContainsTransaction(tx_hashes[1]));
    EXPECT_TRUE(trimmed_block->ContainsTransaction(tx_hashes[2]));
    
    io::UInt256 non_existent = io::UInt256::Parse("9999999999999999999999999999999999999999999999999999999999999999");
    EXPECT_FALSE(trimmed_block->ContainsTransaction(non_existent));
}

TEST_F(TrimmedBlockTest, GetHash) {
    auto hash = trimmed_block->GetHash();
    EXPECT_NE(hash, io::UInt256()); // Should not be empty
    
    // Hash should be deterministic
    auto hash2 = trimmed_block->GetHash();
    EXPECT_EQ(hash, hash2);
}

TEST_F(TrimmedBlockTest, GetSize) {
    size_t size = trimmed_block->GetSize();
    EXPECT_GT(size, 0);
    
    // Should include header size plus transaction hash list
    size_t expected_min_size = 80 + (tx_hashes.size() * 32); // Header + hashes
    EXPECT_GE(size, expected_min_size);
}

TEST_F(TrimmedBlockTest, Serialization) {
    // Test serialization
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);
    trimmed_block->Serialize(writer);
    
    // Test deserialization
    stream.seekg(0);
    io::BinaryReader reader(stream);
    auto deserialized_block = ledger::TrimmedBlock::Deserialize(reader);
    
    EXPECT_EQ(deserialized_block.GetVersion(), trimmed_block->GetVersion());
    EXPECT_EQ(deserialized_block.GetPreviousHash(), trimmed_block->GetPreviousHash());
    EXPECT_EQ(deserialized_block.GetMerkleRoot(), trimmed_block->GetMerkleRoot());
    EXPECT_EQ(deserialized_block.GetIndex(), trimmed_block->GetIndex());
    EXPECT_EQ(deserialized_block.GetTransactionCount(), trimmed_block->GetTransactionCount());
}

TEST_F(TrimmedBlockTest, ToJson) {
    auto json_obj = trimmed_block->ToJson();
    EXPECT_NE(json_obj, nullptr);
    
    // Should contain key fields
    auto hash_json = json_obj->Get("hash");
    auto index_json = json_obj->Get("index");
    auto tx_json = json_obj->Get("tx");
    
    EXPECT_NE(hash_json, nullptr);
    EXPECT_NE(index_json, nullptr);
    EXPECT_NE(tx_json, nullptr);
}

TEST_F(TrimmedBlockTest, FromJson) {
    auto json_obj = trimmed_block->ToJson();
    auto block_from_json = ledger::TrimmedBlock::FromJson(json_obj);
    
    EXPECT_EQ(block_from_json.GetVersion(), trimmed_block->GetVersion());
    EXPECT_EQ(block_from_json.GetPreviousHash(), trimmed_block->GetPreviousHash());
    EXPECT_EQ(block_from_json.GetIndex(), trimmed_block->GetIndex());
    EXPECT_EQ(block_from_json.GetTransactionCount(), trimmed_block->GetTransactionCount());
}

TEST_F(TrimmedBlockTest, Clone) {
    auto cloned_block = trimmed_block->Clone();
    
    EXPECT_NE(cloned_block.get(), trimmed_block.get()); // Different objects
    EXPECT_EQ(cloned_block->GetHash(), trimmed_block->GetHash());
    EXPECT_EQ(cloned_block->GetIndex(), trimmed_block->GetIndex());
    EXPECT_EQ(cloned_block->GetTransactionCount(), trimmed_block->GetTransactionCount());
}

TEST_F(TrimmedBlockTest, EqualityComparison) {
    auto same_block = std::make_shared<ledger::TrimmedBlock>(
        1, previous_hash, merkle_root, 1640995200, 12345, 100, 0, next_consensus, tx_hashes
    );
    
    EXPECT_TRUE(*trimmed_block == *same_block);
    EXPECT_FALSE(*trimmed_block == *empty_trimmed_block);
}

TEST_F(TrimmedBlockTest, GetTransactionIndex) {
    int index0 = trimmed_block->GetTransactionIndex(tx_hashes[0]);
    int index1 = trimmed_block->GetTransactionIndex(tx_hashes[1]);
    int index2 = trimmed_block->GetTransactionIndex(tx_hashes[2]);
    
    EXPECT_EQ(index0, 0);
    EXPECT_EQ(index1, 1);
    EXPECT_EQ(index2, 2);
    
    io::UInt256 non_existent = io::UInt256::Parse("9999999999999999999999999999999999999999999999999999999999999999");
    int not_found = trimmed_block->GetTransactionIndex(non_existent);
    EXPECT_EQ(not_found, -1);
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_LEDGER_TEST_TRIMMEDBLOCK_CPP_H
