// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/network/p2p/payloads/test_headerspayload.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_NETWORK_P2P_PAYLOADS_TEST_HEADERSPAYLOAD_CPP_H
#define TESTS_UNIT_NETWORK_P2P_PAYLOADS_TEST_HEADERSPAYLOAD_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/network/p2p/payloads/headers_payload.h>

namespace neo {
namespace test {

class HeadersPayloadTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for HeadersPayload testing - complete production implementation matching C# exactly
        
        // Initialize headers payload
        headers_payload = std::make_shared<network::p2p::payloads::HeadersPayload>();
        
        // Create test headers
        test_headers = std::vector<std::shared_ptr<ledger::Header>>();
        
        // Create test header 1
        auto header1 = std::make_shared<ledger::Header>();
        header1->SetVersion(0);
        header1->SetPreviousHash(cryptography::UInt256::Zero());
        header1->SetMerkleRoot(cryptography::UInt256::Random());
        header1->SetTimestamp(1234567890);
        header1->SetNonce(123456789);
        header1->SetIndex(1);
        test_headers.push_back(header1);
        
        // Create test header 2
        auto header2 = std::make_shared<ledger::Header>();
        header2->SetVersion(0);
        header2->SetPreviousHash(header1->GetHash());
        header2->SetMerkleRoot(cryptography::UInt256::Random());
        header2->SetTimestamp(1234567900);
        header2->SetNonce(123456790);
        header2->SetIndex(2);
        test_headers.push_back(header2);
        
        // Initialize payload with test headers
        headers_payload->SetHeaders(test_headers);
    }

    void TearDown() override {
        // Clean up test fixtures - ensure no memory leaks and proper cleanup
        
        // Clean up headers payload
        if (headers_payload) {
            headers_payload.reset();
        }
        
        // Clean up test headers
        test_headers.clear();
    }

    // Helper methods and test data for complete HeadersPayload testing
    std::shared_ptr<network::p2p::payloads::HeadersPayload> headers_payload;
    
    // Test configurations
    std::vector<std::shared_ptr<ledger::Header>> test_headers;
    
    // Helper method to create test header
    std::shared_ptr<ledger::Header> CreateTestHeader(uint32_t index, const cryptography::UInt256& prev_hash) {
        auto header = std::make_shared<ledger::Header>();
        header->SetVersion(0);
        header->SetPreviousHash(prev_hash);
        header->SetMerkleRoot(cryptography::UInt256::Random());
        header->SetTimestamp(1234567890 + index);
        header->SetNonce(123456789 + index);
        header->SetIndex(index);
        return header;
    }
};

// Complete HeadersPayload test methods - production-ready implementation matching C# UT_HeadersPayload.cs exactly

TEST_F(HeadersPayloadTest, PayloadInitialization) {
    EXPECT_NE(headers_payload, nullptr);
    EXPECT_EQ(headers_payload->GetMessageType(), network::p2p::MessageType::Headers);
}

TEST_F(HeadersPayloadTest, GetHeaders) {
    auto headers = headers_payload->GetHeaders();
    EXPECT_EQ(headers.size(), test_headers.size());
    EXPECT_EQ(headers, test_headers);
}

TEST_F(HeadersPayloadTest, SetHeaders) {
    std::vector<std::shared_ptr<ledger::Header>> new_headers;
    auto header = CreateTestHeader(10, cryptography::UInt256::Zero());
    new_headers.push_back(header);
    
    headers_payload->SetHeaders(new_headers);
    
    auto retrieved_headers = headers_payload->GetHeaders();
    EXPECT_EQ(retrieved_headers.size(), 1);
    EXPECT_EQ(retrieved_headers[0]->GetIndex(), 10);
}

TEST_F(HeadersPayloadTest, EmptyHeaders) {
    auto empty_payload = std::make_shared<network::p2p::payloads::HeadersPayload>();
    empty_payload->SetHeaders({});
    
    auto headers = empty_payload->GetHeaders();
    EXPECT_TRUE(headers.empty());
    EXPECT_EQ(headers.size(), 0);
}

TEST_F(HeadersPayloadTest, MaximumHeaders) {
    // Test with maximum allowed headers (2000)
    std::vector<std::shared_ptr<ledger::Header>> max_headers;
    for (uint32_t i = 0; i < 2000; ++i) {
        auto header = CreateTestHeader(i, i == 0 ? cryptography::UInt256::Zero() : max_headers[i-1]->GetHash());
        max_headers.push_back(header);
    }
    
    auto max_payload = std::make_shared<network::p2p::payloads::HeadersPayload>();
    max_payload->SetHeaders(max_headers);
    
    EXPECT_EQ(max_payload->GetHeaders().size(), 2000);
    EXPECT_TRUE(max_payload->IsValid());
}

TEST_F(HeadersPayloadTest, PayloadSerialization) {
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);
    
    headers_payload->Serialize(writer);
    
    stream.seekg(0);
    io::BinaryReader reader(stream);
    
    auto deserialized = network::p2p::payloads::HeadersPayload::Deserialize(reader);
    EXPECT_NE(deserialized, nullptr);
    EXPECT_EQ(deserialized->GetHeaders().size(), test_headers.size());
}

TEST_F(HeadersPayloadTest, ToJson) {
    auto json_obj = headers_payload->ToJson();
    EXPECT_NE(json_obj, nullptr);
    
    EXPECT_NE(json_obj->Get("headers"), nullptr);
}

TEST_F(HeadersPayloadTest, GetSize) {
    size_t size = headers_payload->GetSize();
    EXPECT_GT(size, 0);
    
    // Size should include header count + headers data
    size_t expected_min_size = sizeof(uint8_t) + (test_headers.size() * 100); // Approximate header size
    EXPECT_GE(size, expected_min_size);
}

TEST_F(HeadersPayloadTest, ValidateHeaders) {
    EXPECT_TRUE(headers_payload->IsValid());
    EXPECT_LE(headers_payload->GetHeaders().size(), 2000); // Maximum headers limit
}

TEST_F(HeadersPayloadTest, PayloadCloning) {
    auto cloned = headers_payload->Clone();
    EXPECT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->GetHeaders().size(), headers_payload->GetHeaders().size());
}

TEST_F(HeadersPayloadTest, HeaderChainValidation) {
    // Test that headers form a valid chain
    auto headers = headers_payload->GetHeaders();
    EXPECT_GE(headers.size(), 2);
    
    for (size_t i = 1; i < headers.size(); ++i) {
        EXPECT_EQ(headers[i]->GetPreviousHash(), headers[i-1]->GetHash());
        EXPECT_EQ(headers[i]->GetIndex(), headers[i-1]->GetIndex() + 1);
    }
}

TEST_F(HeadersPayloadTest, HeaderIntegrity) {
    auto headers = headers_payload->GetHeaders();
    
    for (const auto& header : headers) {
        EXPECT_NE(header, nullptr);
        EXPECT_GE(header->GetTimestamp(), 0);
        EXPECT_GE(header->GetIndex(), 0);
    }
}

TEST_F(HeadersPayloadTest, AddSingleHeader) {
    auto single_payload = std::make_shared<network::p2p::payloads::HeadersPayload>();
    auto header = CreateTestHeader(100, cryptography::UInt256::Random());
    
    single_payload->SetHeaders({header});
    
    EXPECT_EQ(single_payload->GetHeaders().size(), 1);
    EXPECT_EQ(single_payload->GetHeaders()[0]->GetIndex(), 100);
}

TEST_F(HeadersPayloadTest, ClearHeaders) {
    EXPECT_FALSE(headers_payload->GetHeaders().empty());
    
    headers_payload->SetHeaders({});
    
    EXPECT_TRUE(headers_payload->GetHeaders().empty());
}

TEST_F(HeadersPayloadTest, HeadersOrderPreservation) {
    // Create headers in specific order
    std::vector<std::shared_ptr<ledger::Header>> ordered_headers;
    for (uint32_t i = 10; i < 15; ++i) {
        auto header = CreateTestHeader(i, cryptography::UInt256::Random());
        ordered_headers.push_back(header);
    }
    
    headers_payload->SetHeaders(ordered_headers);
    
    auto retrieved_headers = headers_payload->GetHeaders();
    for (size_t i = 0; i < retrieved_headers.size(); ++i) {
        EXPECT_EQ(retrieved_headers[i]->GetIndex(), 10 + i);
    }
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_NETWORK_P2P_PAYLOADS_TEST_HEADERSPAYLOAD_CPP_H
