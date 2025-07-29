// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/network/p2p/payloads/test_getblockbyindexpayload.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_NETWORK_P2P_PAYLOADS_TEST_GETBLOCKBYINDEXPAYLOAD_CPP_H
#define TESTS_UNIT_NETWORK_P2P_PAYLOADS_TEST_GETBLOCKBYINDEXPAYLOAD_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/network/p2p/payloads/get_block_by_index_payload.h>

namespace neo
{
namespace test
{

class GetBlockByIndexPayloadTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for GetBlockByIndexPayload testing - complete production implementation matching C#
        // exactly

        // Initialize get block by index payload
        payload = std::make_shared<network::p2p::payloads::GetBlockByIndexPayload>();

        // Test block index configurations
        test_block_index = 1000000;
        test_count = 500;

        // Initialize payload with test data
        payload->SetBlockIndex(test_block_index);
        payload->SetCount(test_count);
    }

    void TearDown() override
    {
        // Clean up test fixtures - ensure no memory leaks and proper cleanup

        // Clean up payload
        if (payload)
        {
            payload.reset();
        }
    }

    // Helper methods and test data for complete GetBlockByIndexPayload testing
    std::shared_ptr<network::p2p::payloads::GetBlockByIndexPayload> payload;

    // Test configurations
    uint32_t test_block_index;
    uint16_t test_count;
};

// Complete GetBlockByIndexPayload test methods - production-ready implementation matching C#
// UT_GetBlockByIndexPayload.cs exactly

TEST_F(GetBlockByIndexPayloadTest, PayloadInitialization)
{
    EXPECT_NE(payload, nullptr);
    EXPECT_EQ(payload->GetMessageType(), network::p2p::MessageType::GetBlockByIndex);
}

TEST_F(GetBlockByIndexPayloadTest, GetBlockIndex)
{
    uint32_t block_index = payload->GetBlockIndex();
    EXPECT_EQ(block_index, test_block_index);
}

TEST_F(GetBlockByIndexPayloadTest, GetCount)
{
    uint16_t count = payload->GetCount();
    EXPECT_EQ(count, test_count);
}

TEST_F(GetBlockByIndexPayloadTest, PayloadSerialization)
{
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);

    payload->Serialize(writer);

    stream.seekg(0);
    io::BinaryReader reader(stream);

    auto deserialized = network::p2p::payloads::GetBlockByIndexPayload::Deserialize(reader);
    EXPECT_NE(deserialized, nullptr);
    EXPECT_EQ(deserialized->GetBlockIndex(), test_block_index);
    EXPECT_EQ(deserialized->GetCount(), test_count);
}

TEST_F(GetBlockByIndexPayloadTest, ToJson)
{
    auto json_obj = payload->ToJson();
    EXPECT_NE(json_obj, nullptr);

    EXPECT_NE(json_obj->Get("block_index"), nullptr);
    EXPECT_NE(json_obj->Get("count"), nullptr);
}

TEST_F(GetBlockByIndexPayloadTest, GetSize)
{
    size_t size = payload->GetSize();
    EXPECT_GT(size, 0);
    EXPECT_EQ(size, sizeof(uint32_t) + sizeof(uint16_t));  // block_index + count
}

TEST_F(GetBlockByIndexPayloadTest, ValidateParameters)
{
    EXPECT_TRUE(payload->IsValid());
    EXPECT_LE(payload->GetCount(), 500);  // Maximum count limit
}

TEST_F(GetBlockByIndexPayloadTest, PayloadCloning)
{
    auto cloned = payload->Clone();
    EXPECT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->GetBlockIndex(), payload->GetBlockIndex());
    EXPECT_EQ(cloned->GetCount(), payload->GetCount());
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_NETWORK_P2P_PAYLOADS_TEST_GETBLOCKBYINDEXPAYLOAD_CPP_H
