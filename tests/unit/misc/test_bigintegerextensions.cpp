// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/misc/test_bigintegerextensions.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_MISC_TEST_BIGINTEGEREXTENSIONS_CPP_H
#define TESTS_UNIT_MISC_TEST_BIGINTEGEREXTENSIONS_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/extensions/biginteger_extensions.h>

namespace neo
{
namespace test
{

class BigIntegerExtensionsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures with various BigInteger test data
        zero = neo::extensions::BigIntegerExtensions::FromString("0");
        one = neo::extensions::BigIntegerExtensions::FromString("1");
        negative_one = neo::extensions::BigIntegerExtensions::FromString("-1");
        small_positive = neo::extensions::BigIntegerExtensions::FromString("42");
        small_negative = neo::extensions::BigIntegerExtensions::FromString("-42");
        large_number = neo::extensions::BigIntegerExtensions::FromString("123456789012345678901234567890");
        max_int64 = neo::extensions::BigIntegerExtensions::FromString("9223372036854775807");
    }

    void TearDown() override
    {
        // Clean up test fixtures - BigInteger manages its own memory
    }

    // Helper methods and test data
    neo::extensions::BigIntegerExtensions::BigInteger zero;
    neo::extensions::BigIntegerExtensions::BigInteger one;
    neo::extensions::BigIntegerExtensions::BigInteger negative_one;
    neo::extensions::BigIntegerExtensions::BigInteger small_positive;
    neo::extensions::BigIntegerExtensions::BigInteger small_negative;
    neo::extensions::BigIntegerExtensions::BigInteger large_number;
    neo::extensions::BigIntegerExtensions::BigInteger max_int64;
};

// Complete test methods converted from C# UT_BigIntegerExtensions.cs functionality

TEST_F(BigIntegerExtensionsTest, TestFromString)
{
    // Test parsing from decimal string
    auto parsed_zero = neo::extensions::BigIntegerExtensions::FromString("0");
    auto parsed_positive = neo::extensions::BigIntegerExtensions::FromString("12345");
    auto parsed_negative = neo::extensions::BigIntegerExtensions::FromString("-12345");

    // Convert back to string to verify parsing
    EXPECT_EQ("0", neo::extensions::BigIntegerExtensions::ToString(parsed_zero));
    EXPECT_EQ("12345", neo::extensions::BigIntegerExtensions::ToString(parsed_positive));
    EXPECT_EQ("-12345", neo::extensions::BigIntegerExtensions::ToString(parsed_negative));
}

TEST_F(BigIntegerExtensionsTest, TestToString)
{
    // Test conversion to decimal string
    EXPECT_EQ("0", neo::extensions::BigIntegerExtensions::ToString(zero));
    EXPECT_EQ("1", neo::extensions::BigIntegerExtensions::ToString(one));
    EXPECT_EQ("-1", neo::extensions::BigIntegerExtensions::ToString(negative_one));
    EXPECT_EQ("42", neo::extensions::BigIntegerExtensions::ToString(small_positive));
    EXPECT_EQ("-42", neo::extensions::BigIntegerExtensions::ToString(small_negative));
}

TEST_F(BigIntegerExtensionsTest, TestFromHexString)
{
    // Test parsing from hexadecimal string
    auto hex_zero = neo::extensions::BigIntegerExtensions::FromHexString("0");
    auto hex_positive = neo::extensions::BigIntegerExtensions::FromHexString("FF");
    auto hex_with_prefix = neo::extensions::BigIntegerExtensions::FromHexString("0x2A");

    EXPECT_EQ("0", neo::extensions::BigIntegerExtensions::ToString(hex_zero));
    EXPECT_EQ("255", neo::extensions::BigIntegerExtensions::ToString(hex_positive));
    EXPECT_EQ("42", neo::extensions::BigIntegerExtensions::ToString(hex_with_prefix));
}

TEST_F(BigIntegerExtensionsTest, TestToHexString)
{
    // Test conversion to hexadecimal string
    EXPECT_EQ("0", neo::extensions::BigIntegerExtensions::ToHexString(zero));
    EXPECT_EQ("1", neo::extensions::BigIntegerExtensions::ToHexString(one));
    EXPECT_EQ("2A", neo::extensions::BigIntegerExtensions::ToHexString(small_positive));
}

TEST_F(BigIntegerExtensionsTest, TestArithmetic)
{
    // Test basic arithmetic operations
    auto sum = neo::extensions::BigIntegerExtensions::Add(one, one);
    EXPECT_EQ("2", neo::extensions::BigIntegerExtensions::ToString(sum));

    auto diff = neo::extensions::BigIntegerExtensions::Subtract(small_positive, one);
    EXPECT_EQ("41", neo::extensions::BigIntegerExtensions::ToString(diff));

    auto product = neo::extensions::BigIntegerExtensions::Multiply(
        small_positive, neo::extensions::BigIntegerExtensions::FromString("2"));
    EXPECT_EQ("84", neo::extensions::BigIntegerExtensions::ToString(product));
}

TEST_F(BigIntegerExtensionsTest, TestComparison)
{
    // Test comparison operations
    EXPECT_TRUE(neo::extensions::BigIntegerExtensions::IsZero(zero));
    EXPECT_FALSE(neo::extensions::BigIntegerExtensions::IsZero(one));

    EXPECT_TRUE(neo::extensions::BigIntegerExtensions::IsPositive(one));
    EXPECT_FALSE(neo::extensions::BigIntegerExtensions::IsPositive(negative_one));

    EXPECT_TRUE(neo::extensions::BigIntegerExtensions::IsNegative(negative_one));
    EXPECT_FALSE(neo::extensions::BigIntegerExtensions::IsNegative(one));
}

TEST_F(BigIntegerExtensionsTest, TestLargeNumbers)
{
    // Test operations with very large numbers
    auto large_str = neo::extensions::BigIntegerExtensions::ToString(large_number);
    EXPECT_EQ("123456789012345678901234567890", large_str);

    // Test that we can add large numbers
    auto large_sum = neo::extensions::BigIntegerExtensions::Add(large_number, one);
    EXPECT_EQ("123456789012345678901234567891", neo::extensions::BigIntegerExtensions::ToString(large_sum));
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_MISC_TEST_BIGINTEGEREXTENSIONS_CPP_H
