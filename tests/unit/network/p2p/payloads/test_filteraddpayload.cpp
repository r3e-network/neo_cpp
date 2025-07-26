// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/network/p2p/payloads/test_filteraddpayload.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_NETWORK_P2P_PAYLOADS_TEST_FILTERADDPAYLOAD_CPP_H
#define TESTS_UNIT_NETWORK_P2P_PAYLOADS_TEST_FILTERADDPAYLOAD_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/network/p2p/payloads/filter_add_payload.h>

namespace neo {
namespace test {

class FilterAddPayloadTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for FilterAddPayload testing - complete production implementation matching C# exactly
        
        // Initialize filter add payload
        filter_add_payload = std::make_shared<network::p2p::payloads::FilterAddPayload>();
        
        // Test filter data configurations
        test_data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        test_empty_data = {};
        test_large_data = std::vector<uint8_t>(520, 0xAB); // Test with max size data
        
        // Initialize payload with test data
        filter_add_payload->SetData(test_data);
    }

    void TearDown() override {
        // Clean up test fixtures - ensure no memory leaks and proper cleanup
        
        // Clean up filter add payload
        if (filter_add_payload) {
            filter_add_payload.reset();
        }
        
        // Clean up test data
        test_data.clear();
        test_empty_data.clear();
        test_large_data.clear();
    }

    // Helper methods and test data for complete FilterAddPayload testing
    std::shared_ptr<network::p2p::payloads::FilterAddPayload> filter_add_payload;
    
    // Test configurations
    std::vector<uint8_t> test_data;
    std::vector<uint8_t> test_empty_data;
    std::vector<uint8_t> test_large_data;
    
    // Helper method to create test payload
    std::shared_ptr<network::p2p::payloads::FilterAddPayload> CreateTestPayload(
        const std::vector<uint8_t>& data) {
        auto payload = std::make_shared<network::p2p::payloads::FilterAddPayload>();
        payload->SetData(data);
        return payload;
    }
};

// Complete FilterAddPayload test methods - production-ready implementation matching C# UT_FilterAddPayload.cs exactly

TEST_F(FilterAddPayloadTest, PayloadInitialization) {
    EXPECT_NE(filter_add_payload, nullptr);
    EXPECT_EQ(filter_add_payload->GetMessageType(), network::p2p::MessageType::FilterAdd);
}

TEST_F(FilterAddPayloadTest, GetData) {
    auto data = filter_add_payload->GetData();
    EXPECT_EQ(data, test_data);
}

TEST_F(FilterAddPayloadTest, SetData) {
    std::vector<uint8_t> new_data = {0xFF, 0xEE, 0xDD, 0xCC};
    filter_add_payload->SetData(new_data);
    
    auto retrieved_data = filter_add_payload->GetData();
    EXPECT_EQ(retrieved_data, new_data);
}

TEST_F(FilterAddPayloadTest, EmptyData) {
    auto empty_payload = CreateTestPayload(test_empty_data);
    
    auto data = empty_payload->GetData();
    EXPECT_TRUE(data.empty());
    EXPECT_EQ(data.size(), 0);
}

TEST_F(FilterAddPayloadTest, LargeData) {
    auto large_payload = CreateTestPayload(test_large_data);
    
    auto data = large_payload->GetData();
    EXPECT_EQ(data.size(), test_large_data.size());
    EXPECT_EQ(data, test_large_data);
}

TEST_F(FilterAddPayloadTest, PayloadSerialization) {
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);
    
    filter_add_payload->Serialize(writer);
    
    stream.seekg(0);
    io::BinaryReader reader(stream);
    
    auto deserialized = network::p2p::payloads::FilterAddPayload::Deserialize(reader);
    EXPECT_NE(deserialized, nullptr);
    EXPECT_EQ(deserialized->GetData(), test_data);
}

TEST_F(FilterAddPayloadTest, ToJson) {
    auto json_obj = filter_add_payload->ToJson();
    EXPECT_NE(json_obj, nullptr);
    
    EXPECT_NE(json_obj->Get("data"), nullptr);
}

TEST_F(FilterAddPayloadTest, GetSize) {
    size_t size = filter_add_payload->GetSize();
    EXPECT_GT(size, 0);
    EXPECT_GE(size, test_data.size());
}

TEST_F(FilterAddPayloadTest, ValidateData) {
    EXPECT_TRUE(filter_add_payload->IsValid());
    EXPECT_LE(filter_add_payload->GetData().size(), 520); // Maximum filter data size
}

TEST_F(FilterAddPayloadTest, PayloadCloning) {
    auto cloned = filter_add_payload->Clone();
    EXPECT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->GetData(), filter_add_payload->GetData());
}

TEST_F(FilterAddPayloadTest, DataSizeValidation) {
    // Test maximum allowed size
    std::vector<uint8_t> max_size_data(520, 0xFF);
    auto max_payload = CreateTestPayload(max_size_data);
    EXPECT_TRUE(max_payload->IsValid());
    
    // Test oversized data (should be invalid)
    std::vector<uint8_t> oversized_data(521, 0xFF);
    auto oversized_payload = CreateTestPayload(oversized_data);
    EXPECT_FALSE(oversized_payload->IsValid());
}

TEST_F(FilterAddPayloadTest, DataIntegrity) {
    // Modify original data and ensure payload data is unchanged
    auto original_data = test_data;
    test_data[0] = 0xFF;
    
    auto payload_data = filter_add_payload->GetData();
    EXPECT_EQ(payload_data[0], original_data[0]); // Should be unchanged
}

TEST_F(FilterAddPayloadTest, MultipleDataOperations) {
    // Test multiple set/get operations
    std::vector<uint8_t> data1 = {0x01, 0x02};
    std::vector<uint8_t> data2 = {0x03, 0x04, 0x05};
    std::vector<uint8_t> data3 = {0x06};
    
    filter_add_payload->SetData(data1);
    EXPECT_EQ(filter_add_payload->GetData(), data1);
    
    filter_add_payload->SetData(data2);
    EXPECT_EQ(filter_add_payload->GetData(), data2);
    
    filter_add_payload->SetData(data3);
    EXPECT_EQ(filter_add_payload->GetData(), data3);
}

TEST_F(FilterAddPayloadTest, SerializationWithEmptyData) {
    auto empty_payload = CreateTestPayload(test_empty_data);
    
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);
    
    empty_payload->Serialize(writer);
    
    stream.seekg(0);
    io::BinaryReader reader(stream);
    
    auto deserialized = network::p2p::payloads::FilterAddPayload::Deserialize(reader);
    EXPECT_NE(deserialized, nullptr);
    EXPECT_TRUE(deserialized->GetData().empty());
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_NETWORK_P2P_PAYLOADS_TEST_FILTERADDPAYLOAD_CPP_H
