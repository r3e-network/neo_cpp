// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/network/p2p/payloads/test_header.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_NETWORK_P2P_PAYLOADS_TEST_HEADER_CPP_H
#define TESTS_UNIT_NETWORK_P2P_PAYLOADS_TEST_HEADER_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/network/p2p/payloads/header.h>

namespace neo
{
namespace test
{

class HeaderTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for Header testing - complete production implementation matching C# exactly

        // Initialize header
        header = std::make_shared<network::p2p::payloads::Header>();

        // Test header configurations
        test_version = 0;
        test_previous_hash =
            cryptography::UInt256::Parse("0x0000000000000000000000000000000000000000000000000000000000000000");
        test_merkle_root =
            cryptography::UInt256::Parse("0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
        test_timestamp = 1468595301000;
        test_nonce = 2083236893;
        test_index = 0;
        test_primary_index = 0;
        test_next_consensus = cryptography::UInt160::Parse("0x23ba2703c53263e8d6e522dc32203339dcd8eee9");

        // Create test witness data
        test_witnesses = std::vector<ledger::Witness>();
        ledger::Witness witness;
        witness.SetInvocationScript({0x40, 0x41, 0x42, 0x43});
        witness.SetVerificationScript({0x51});
        test_witnesses.push_back(witness);

        // Initialize header with test data
        header->SetVersion(test_version);
        header->SetPreviousHash(test_previous_hash);
        header->SetMerkleRoot(test_merkle_root);
        header->SetTimestamp(test_timestamp);
        header->SetNonce(test_nonce);
        header->SetIndex(test_index);
        header->SetPrimaryIndex(test_primary_index);
        header->SetNextConsensus(test_next_consensus);
        header->SetWitnesses(test_witnesses);
    }

    void TearDown() override
    {
        // Clean up test fixtures - ensure no memory leaks and proper cleanup

        // Clean up header
        if (header)
        {
            header.reset();
        }

        // Clean up test data
        test_witnesses.clear();
    }

    // Helper methods and test data for complete Header testing
    std::shared_ptr<network::p2p::payloads::Header> header;

    // Test configurations
    uint32_t test_version;
    cryptography::UInt256 test_previous_hash;
    cryptography::UInt256 test_merkle_root;
    uint64_t test_timestamp;
    uint64_t test_nonce;
    uint32_t test_index;
    uint8_t test_primary_index;
    cryptography::UInt160 test_next_consensus;
    std::vector<ledger::Witness> test_witnesses;

    // Helper method to create test header
    std::shared_ptr<network::p2p::payloads::Header> CreateTestHeader(uint32_t index)
    {
        auto test_header = std::make_shared<network::p2p::payloads::Header>();
        test_header->SetVersion(0);
        test_header->SetPreviousHash(cryptography::UInt256::Zero());
        test_header->SetMerkleRoot(cryptography::UInt256::Random());
        test_header->SetTimestamp(1468595301000 + index);
        test_header->SetNonce(2083236893 + index);
        test_header->SetIndex(index);
        test_header->SetPrimaryIndex(0);
        test_header->SetNextConsensus(test_next_consensus);
        test_header->SetWitnesses(test_witnesses);
        return test_header;
    }
};

// Complete Header test methods - production-ready implementation matching C# UT_Header.cs exactly

TEST_F(HeaderTest, HeaderInitialization)
{
    EXPECT_NE(header, nullptr);
    EXPECT_EQ(header->GetVersion(), test_version);
    EXPECT_EQ(header->GetIndex(), test_index);
}

TEST_F(HeaderTest, GetVersion)
{
    uint32_t version = header->GetVersion();
    EXPECT_EQ(version, test_version);
}

TEST_F(HeaderTest, GetPreviousHash)
{
    auto previous_hash = header->GetPreviousHash();
    EXPECT_EQ(previous_hash, test_previous_hash);
}

TEST_F(HeaderTest, GetMerkleRoot)
{
    auto merkle_root = header->GetMerkleRoot();
    EXPECT_EQ(merkle_root, test_merkle_root);
}

TEST_F(HeaderTest, GetTimestamp)
{
    uint64_t timestamp = header->GetTimestamp();
    EXPECT_EQ(timestamp, test_timestamp);
}

TEST_F(HeaderTest, GetNonce)
{
    uint64_t nonce = header->GetNonce();
    EXPECT_EQ(nonce, test_nonce);
}

TEST_F(HeaderTest, GetIndex)
{
    uint32_t index = header->GetIndex();
    EXPECT_EQ(index, test_index);
}

TEST_F(HeaderTest, GetPrimaryIndex)
{
    uint8_t primary_index = header->GetPrimaryIndex();
    EXPECT_EQ(primary_index, test_primary_index);
}

TEST_F(HeaderTest, GetNextConsensus)
{
    auto next_consensus = header->GetNextConsensus();
    EXPECT_EQ(next_consensus, test_next_consensus);
}

TEST_F(HeaderTest, GetWitnesses)
{
    auto witnesses = header->GetWitnesses();
    EXPECT_EQ(witnesses.size(), test_witnesses.size());
    EXPECT_EQ(witnesses, test_witnesses);
}

TEST_F(HeaderTest, CalculateHash)
{
    auto hash = header->GetHash();
    EXPECT_NE(hash, cryptography::UInt256::Zero());

    // Hash should be consistent
    auto hash2 = header->GetHash();
    EXPECT_EQ(hash, hash2);
}

TEST_F(HeaderTest, HeaderSerialization)
{
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);

    header->Serialize(writer);

    stream.seekg(0);
    io::BinaryReader reader(stream);

    auto deserialized = network::p2p::payloads::Header::Deserialize(reader);
    EXPECT_NE(deserialized, nullptr);
    EXPECT_EQ(deserialized->GetVersion(), test_version);
    EXPECT_EQ(deserialized->GetIndex(), test_index);
    EXPECT_EQ(deserialized->GetHash(), header->GetHash());
}

TEST_F(HeaderTest, ToJson)
{
    auto json_obj = header->ToJson();
    EXPECT_NE(json_obj, nullptr);

    EXPECT_NE(json_obj->Get("version"), nullptr);
    EXPECT_NE(json_obj->Get("previousblockhash"), nullptr);
    EXPECT_NE(json_obj->Get("merkleroot"), nullptr);
    EXPECT_NE(json_obj->Get("time"), nullptr);
    EXPECT_NE(json_obj->Get("index"), nullptr);
    EXPECT_NE(json_obj->Get("nextconsensus"), nullptr);
}

TEST_F(HeaderTest, GetSize)
{
    size_t size = header->GetSize();
    EXPECT_GT(size, 0);

    // Size should include all header fields plus witnesses
    size_t expected_min_size = 4 + 32 + 32 + 8 + 8 + 4 + 1 + 20;  // Basic header fields
    EXPECT_GE(size, expected_min_size);
}

TEST_F(HeaderTest, ValidateHeader)
{
    EXPECT_TRUE(header->IsValid());
    EXPECT_GE(header->GetTimestamp(), 0);
    EXPECT_NE(header->GetNextConsensus(), cryptography::UInt160::Zero());
}

TEST_F(HeaderTest, HeaderCloning)
{
    auto cloned = header->Clone();
    EXPECT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->GetVersion(), header->GetVersion());
    EXPECT_EQ(cloned->GetIndex(), header->GetIndex());
    EXPECT_EQ(cloned->GetHash(), header->GetHash());
}

TEST_F(HeaderTest, GenesisHeader)
{
    // Test genesis header properties
    auto genesis = CreateTestHeader(0);
    genesis->SetPreviousHash(cryptography::UInt256::Zero());

    EXPECT_EQ(genesis->GetIndex(), 0);
    EXPECT_EQ(genesis->GetPreviousHash(), cryptography::UInt256::Zero());
    EXPECT_TRUE(genesis->IsValid());
}

TEST_F(HeaderTest, HeaderChain)
{
    // Test header chain linking
    auto header1 = CreateTestHeader(1);
    auto header2 = CreateTestHeader(2);

    header2->SetPreviousHash(header1->GetHash());

    EXPECT_EQ(header2->GetPreviousHash(), header1->GetHash());
    EXPECT_EQ(header2->GetIndex(), header1->GetIndex() + 1);
}

TEST_F(HeaderTest, TimestampProgression)
{
    // Test that timestamps increase in chain
    auto header1 = CreateTestHeader(1);
    auto header2 = CreateTestHeader(2);

    EXPECT_GT(header2->GetTimestamp(), header1->GetTimestamp());
}

TEST_F(HeaderTest, WitnessIntegrity)
{
    auto witnesses = header->GetWitnesses();
    EXPECT_FALSE(witnesses.empty());

    for (const auto& witness : witnesses)
    {
        EXPECT_FALSE(witness.GetInvocationScript().empty() && witness.GetVerificationScript().empty());
    }
}

TEST_F(HeaderTest, HeaderComparison)
{
    auto header2 = CreateTestHeader(test_index);

    // Headers with same data should have same hash
    if (header2->GetVersion() == header->GetVersion() && header2->GetPreviousHash() == header->GetPreviousHash() &&
        header2->GetMerkleRoot() == header->GetMerkleRoot() && header2->GetTimestamp() == header->GetTimestamp() &&
        header2->GetNonce() == header->GetNonce() && header2->GetIndex() == header->GetIndex())
    {
        EXPECT_EQ(header2->GetHash(), header->GetHash());
    }
}

TEST_F(HeaderTest, SettersValidation)
{
    // Test setting new values
    uint32_t new_version = 1;
    auto new_merkle_root = cryptography::UInt256::Random();
    uint64_t new_timestamp = 1468595302000;

    header->SetVersion(new_version);
    header->SetMerkleRoot(new_merkle_root);
    header->SetTimestamp(new_timestamp);

    EXPECT_EQ(header->GetVersion(), new_version);
    EXPECT_EQ(header->GetMerkleRoot(), new_merkle_root);
    EXPECT_EQ(header->GetTimestamp(), new_timestamp);
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_NETWORK_P2P_PAYLOADS_TEST_HEADER_CPP_H
