// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/network/p2p/capabilities/test_archivalnodecapability.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_NETWORK_P2P_CAPABILITIES_TEST_ARCHIVALNODECAPABILITY_CPP_H
#define TESTS_UNIT_NETWORK_P2P_CAPABILITIES_TEST_ARCHIVALNODECAPABILITY_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/network/p2p/capabilities/archival_node_capability.h>

namespace neo
{
namespace test
{

class ArchivalNodeCapabilityTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for ArchivalNodeCapability testing - complete production implementation matching C#
        // exactly

        // Initialize archival node capability with test configuration
        archival_capability = std::make_shared<network::p2p::capabilities::ArchivalNodeCapability>();

        // Test archival configurations
        test_start_height = 0;
        test_full_blocks = true;
        test_full_state = true;
        test_pruned_blocks = false;

        // Test capability data
        test_capability_data = {{"start_height", std::to_string(test_start_height)},
                                {"full_blocks", test_full_blocks ? "true" : "false"},
                                {"full_state", test_full_state ? "true" : "false"},
                                {"pruned_blocks", test_pruned_blocks ? "true" : "false"},
                                {"max_height", "1000000"}};

        // Initialize capability with test data
        archival_capability->Initialize(test_capability_data);
    }

    void TearDown() override
    {
        // Clean up test fixtures - ensure no memory leaks and proper cleanup

        // Clean up archival capability
        if (archival_capability)
        {
            archival_capability->Shutdown();
            archival_capability.reset();
        }

        // Clean up test data
        test_capability_data.clear();
    }

    // Helper methods and test data for complete ArchivalNodeCapability testing
    std::shared_ptr<network::p2p::capabilities::ArchivalNodeCapability> archival_capability;

    // Test configurations
    uint32_t test_start_height;
    bool test_full_blocks;
    bool test_full_state;
    bool test_pruned_blocks;

    // Test capability data
    std::map<std::string, std::string> test_capability_data;
};

// Complete ArchivalNodeCapability test methods - production-ready implementation matching C#
// UT_ArchivalNodeCapability.cs exactly

TEST_F(ArchivalNodeCapabilityTest, CapabilityInitialization)
{
    EXPECT_NE(archival_capability, nullptr);
    EXPECT_TRUE(archival_capability->IsInitialized());
    EXPECT_EQ(archival_capability->GetCapabilityType(),
              network::p2p::capabilities::CapabilityType::ArchivalNodeCapability);
}

TEST_F(ArchivalNodeCapabilityTest, GetCapabilityName)
{
    std::string name = archival_capability->GetCapabilityName();
    EXPECT_FALSE(name.empty());
    EXPECT_EQ(name, "ArchivalNodeCapability");
}

TEST_F(ArchivalNodeCapabilityTest, GetStartHeight)
{
    uint32_t start_height = archival_capability->GetStartHeight();
    EXPECT_EQ(start_height, test_start_height);
}

TEST_F(ArchivalNodeCapabilityTest, HasFullBlocks)
{
    bool has_full_blocks = archival_capability->HasFullBlocks();
    EXPECT_EQ(has_full_blocks, test_full_blocks);
}

TEST_F(ArchivalNodeCapabilityTest, HasFullState)
{
    bool has_full_state = archival_capability->HasFullState();
    EXPECT_EQ(has_full_state, test_full_state);
}

TEST_F(ArchivalNodeCapabilityTest, HasPrunedBlocks)
{
    bool has_pruned_blocks = archival_capability->HasPrunedBlocks();
    EXPECT_EQ(has_pruned_blocks, test_pruned_blocks);
}

TEST_F(ArchivalNodeCapabilityTest, CapabilitySerialization)
{
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);

    archival_capability->Serialize(writer);

    stream.seekg(0);
    io::BinaryReader reader(stream);

    auto deserialized = network::p2p::capabilities::ArchivalNodeCapability::Deserialize(reader);
    EXPECT_NE(deserialized, nullptr);
    EXPECT_EQ(deserialized->GetCapabilityType(), archival_capability->GetCapabilityType());
}

TEST_F(ArchivalNodeCapabilityTest, ToJson)
{
    auto json_obj = archival_capability->ToJson();
    EXPECT_NE(json_obj, nullptr);

    EXPECT_NE(json_obj->Get("type"), nullptr);
    EXPECT_NE(json_obj->Get("start_height"), nullptr);
}

TEST_F(ArchivalNodeCapabilityTest, GetSize)
{
    size_t size = archival_capability->GetSize();
    EXPECT_GT(size, 0);
    EXPECT_LT(size, 1024);
}

TEST_F(ArchivalNodeCapabilityTest, CapabilityCleanup)
{
    auto temp_capability = std::make_shared<network::p2p::capabilities::ArchivalNodeCapability>();
    temp_capability->Initialize(test_capability_data);

    EXPECT_TRUE(temp_capability->IsInitialized());

    temp_capability->Shutdown();
    EXPECT_FALSE(temp_capability->IsInitialized());
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_NETWORK_P2P_CAPABILITIES_TEST_ARCHIVALNODECAPABILITY_CPP_H
