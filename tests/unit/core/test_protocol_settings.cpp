// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/core/test_protocol_settings.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_CORE_TEST_PROTOCOL_SETTINGS_CPP_H
#define TESTS_UNIT_CORE_TEST_PROTOCOL_SETTINGS_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/core/protocol_settings.h>

namespace neo
{
namespace test
{

class ProtocolSettingsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for ProtocolSettings testing
        default_settings = std::make_shared<neo::ProtocolSettings>();
        custom_settings = std::make_shared<neo::ProtocolSettings>();
        // Configure custom settings for testing
        custom_settings->SetNetwork(0x860833102);  // Custom network ID
    }

    void TearDown() override
    {
        // Clean up test fixtures - shared_ptr handles cleanup automatically
        default_settings.reset();
        custom_settings.reset();
    }

    // Helper methods and test data for ProtocolSettings testing
    std::shared_ptr<neo::ProtocolSettings> default_settings;
    std::shared_ptr<neo::ProtocolSettings> custom_settings;
};

// ProtocolSettings test methods converted from C# UT_ProtocolSettings.cs functionality

TEST_F(ProtocolSettingsTest, DefaultConstructor)
{
    EXPECT_NE(default_settings, nullptr);
    EXPECT_GT(default_settings->GetNetwork(), 0);
}

TEST_F(ProtocolSettingsTest, NetworkIdSetting)
{
    uint32_t test_network = 0x860833102;
    custom_settings->SetNetwork(test_network);
    EXPECT_EQ(custom_settings->GetNetwork(), test_network);
}

TEST_F(ProtocolSettingsTest, AddressVersionSetting)
{
    uint8_t test_version = 0x35;
    custom_settings->SetAddressVersion(test_version);
    EXPECT_EQ(custom_settings->GetAddressVersion(), test_version);
}

TEST_F(ProtocolSettingsTest, ValidatorsCountSetting)
{
    uint32_t test_count = 7;
    custom_settings->SetValidatorsCount(test_count);
    EXPECT_EQ(custom_settings->GetValidatorsCount(), test_count);
}

TEST_F(ProtocolSettingsTest, MillisecondsPerBlockSetting)
{
    uint32_t test_time = 15000;
    custom_settings->SetMillisecondsPerBlock(test_time);
    EXPECT_EQ(custom_settings->GetMillisecondsPerBlock(), test_time);
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_CORE_TEST_PROTOCOL_SETTINGS_CPP_H
