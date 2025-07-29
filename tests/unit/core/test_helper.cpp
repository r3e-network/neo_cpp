// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/core/test_helper.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_CORE_TEST_HELPER_CPP_H
#define TESTS_UNIT_CORE_TEST_HELPER_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/core/helper.h>

namespace neo
{
namespace test
{

class HelperTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for Helper testing
        test_data = io::ByteVector::Parse("0102030405060708");
        empty_data = io::ByteVector();
        large_data = io::ByteVector::Parse("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    }

    void TearDown() override
    {
        // Clean up test fixtures - ByteVector manages its own memory
    }

    // Helper methods and test data for core Helper testing
    io::ByteVector test_data;
    io::ByteVector empty_data;
    io::ByteVector large_data;
};

// Helper test methods converted from C# UT_Helper.cs functionality

TEST_F(HelperTest, ReverseHexString)
{
    std::string hex_input = "0102030405060708";
    std::string expected = "0807060504030201";
    std::string result = Helper::ReverseHex(hex_input);
    EXPECT_EQ(result, expected);
}

TEST_F(HelperTest, ByteArrayToHexString)
{
    std::string expected = "0102030405060708";
    std::string result = Helper::ToHexString(test_data);
    EXPECT_EQ(result, expected);
}

TEST_F(HelperTest, EmptyByteArrayToHexString)
{
    std::string result = Helper::ToHexString(empty_data);
    EXPECT_EQ(result, "");
}

TEST_F(HelperTest, HexStringToByteArray)
{
    std::string hex_input = "0102030405060708";
    auto result = Helper::FromHexString(hex_input);
    EXPECT_EQ(result, test_data);
}

TEST_F(HelperTest, ComputeHash160)
{
    auto hash = Helper::Hash160(test_data);
    EXPECT_EQ(hash.Size(), 20);  // Hash160 should be 20 bytes
}

TEST_F(HelperTest, ComputeHash256)
{
    auto hash = Helper::Hash256(test_data);
    EXPECT_EQ(hash.Size(), 32);  // Hash256 should be 32 bytes
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_CORE_TEST_HELPER_CPP_H
