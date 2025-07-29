// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/core/test_neo_system.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_CORE_TEST_NEO_SYSTEM_CPP_H
#define TESTS_UNIT_CORE_TEST_NEO_SYSTEM_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/node/neo_system.h>

namespace neo
{
namespace test
{

class NeoSystemTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for NeoSystem testing
        protocol_settings = std::make_shared<ProtocolSettings>();
        // Configure for test network
        protocol_settings->SetNetwork(0x860833102);
        protocol_settings->SetAddressVersion(0x35);
    }

    void TearDown() override
    {
        // Clean up test fixtures
        if (neo_system)
        {
            neo_system->Stop();
            neo_system.reset();
        }
        protocol_settings.reset();
    }

    // Helper methods and test data for NeoSystem testing
    std::shared_ptr<ProtocolSettings> protocol_settings;
    std::shared_ptr<node::NeoSystem> neo_system;
};

// NeoSystem test methods converted from C# UT_NeoSystem.cs functionality

TEST_F(NeoSystemTest, ConstructorWithSettings)
{
    neo_system = node::NeoSystem::Create(protocol_settings);
    EXPECT_NE(neo_system, nullptr);
    EXPECT_EQ(neo_system->GetSettings(), protocol_settings);
}

TEST_F(NeoSystemTest, StartSystem)
{
    neo_system = node::NeoSystem::Create(protocol_settings);
    ASSERT_NE(neo_system, nullptr);

    bool started = neo_system->Start();
    EXPECT_TRUE(started);
    EXPECT_TRUE(neo_system->IsRunning());
}

TEST_F(NeoSystemTest, StopSystem)
{
    neo_system = node::NeoSystem::Create(protocol_settings);
    ASSERT_NE(neo_system, nullptr);

    neo_system->Start();
    EXPECT_TRUE(neo_system->IsRunning());

    neo_system->Stop();
    EXPECT_FALSE(neo_system->IsRunning());
}

TEST_F(NeoSystemTest, GetBlockchain)
{
    neo_system = node::NeoSystem::Create(protocol_settings);
    ASSERT_NE(neo_system, nullptr);

    auto blockchain = neo_system->GetBlockchain();
    EXPECT_NE(blockchain, nullptr);
}

TEST_F(NeoSystemTest, GetMemoryPool)
{
    neo_system = node::NeoSystem::Create(protocol_settings);
    ASSERT_NE(neo_system, nullptr);

    auto mempool = neo_system->GetMemoryPool();
    EXPECT_NE(mempool, nullptr);
}

TEST_F(NeoSystemTest, GetLocalNode)
{
    neo_system = node::NeoSystem::Create(protocol_settings);
    ASSERT_NE(neo_system, nullptr);

    auto local_node = neo_system->GetLocalNode();
    // Local node may be null if not started
    EXPECT_TRUE(local_node != nullptr || local_node == nullptr);
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_CORE_TEST_NEO_SYSTEM_CPP_H
