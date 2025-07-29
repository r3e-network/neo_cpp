// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/misc/test_bytearraycomparer.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_MISC_TEST_BYTEARRAYCOMPARER_CPP_H
#define TESTS_UNIT_MISC_TEST_BYTEARRAYCOMPARER_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/extensions/byte_array_comparer.h>

namespace neo
{
namespace test
{

class ByteArrayComparerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures with various byte array data for comparison testing
        empty_array = neo::io::ByteVector();
        single_zero = neo::io::ByteVector::Parse("00");
        single_one = neo::io::ByteVector::Parse("01");
        test_data1 = neo::io::ByteVector::Parse("01020304");
        test_data2 = neo::io::ByteVector::Parse("01020304");  // Same as test_data1
        test_data3 = neo::io::ByteVector::Parse("01020305");  // Greater than test_data1
        test_data4 = neo::io::ByteVector::Parse("01020303");  // Less than test_data1
        shorter_prefix = neo::io::ByteVector::Parse("0102");  // Prefix of test_data1
    }

    void TearDown() override
    {
        // Clean up test fixtures - ByteVector manages its own memory
    }

    // Helper methods and test data
    neo::io::ByteVector empty_array;
    neo::io::ByteVector single_zero;
    neo::io::ByteVector single_one;
    neo::io::ByteVector test_data1;
    neo::io::ByteVector test_data2;
    neo::io::ByteVector test_data3;
    neo::io::ByteVector test_data4;
    neo::io::ByteVector shorter_prefix;
};

// Complete test methods converted from C# UT_ByteArrayComparer.cs functionality

TEST_F(ByteArrayComparerTest, TestCompare_Equal)
{
    // Test that identical byte arrays compare as equal (result = 0)
    EXPECT_EQ(0, neo::extensions::ByteArrayComparer::Compare(test_data1.AsSpan(), test_data2.AsSpan()));
    EXPECT_EQ(0, neo::extensions::ByteArrayComparer::Compare(empty_array.AsSpan(), neo::io::ByteVector().AsSpan()));
    EXPECT_EQ(0, neo::extensions::ByteArrayComparer::Compare(single_zero.AsSpan(),
                                                             neo::io::ByteVector::Parse("00").AsSpan()));
}

TEST_F(ByteArrayComparerTest, TestCompare_LexicographicOrder)
{
    // Test lexicographic ordering: test_data1 < test_data3 (04 < 05)
    EXPECT_LT(neo::extensions::ByteArrayComparer::Compare(test_data1.AsSpan(), test_data3.AsSpan()), 0);

    // Test lexicographic ordering: test_data4 < test_data1 (03 < 04)
    EXPECT_LT(neo::extensions::ByteArrayComparer::Compare(test_data4.AsSpan(), test_data1.AsSpan()), 0);

    // Test reverse comparisons
    EXPECT_GT(neo::extensions::ByteArrayComparer::Compare(test_data3.AsSpan(), test_data1.AsSpan()), 0);
    EXPECT_GT(neo::extensions::ByteArrayComparer::Compare(test_data1.AsSpan(), test_data4.AsSpan()), 0);
}

TEST_F(ByteArrayComparerTest, TestCompare_DifferentLengths)
{
    // Test that shorter array is less than longer array when shorter is a prefix
    EXPECT_LT(neo::extensions::ByteArrayComparer::Compare(shorter_prefix.AsSpan(), test_data1.AsSpan()), 0);
    EXPECT_GT(neo::extensions::ByteArrayComparer::Compare(test_data1.AsSpan(), shorter_prefix.AsSpan()), 0);

    // Test empty array vs non-empty
    EXPECT_LT(neo::extensions::ByteArrayComparer::Compare(empty_array.AsSpan(), single_zero.AsSpan()), 0);
    EXPECT_GT(neo::extensions::ByteArrayComparer::Compare(single_zero.AsSpan(), empty_array.AsSpan()), 0);
}

TEST_F(ByteArrayComparerTest, TestEquals)
{
    // Test equality method
    EXPECT_TRUE(neo::extensions::ByteArrayComparer::Equals(test_data1.AsSpan(), test_data2.AsSpan()));
    EXPECT_FALSE(neo::extensions::ByteArrayComparer::Equals(test_data1.AsSpan(), test_data3.AsSpan()));
    EXPECT_TRUE(neo::extensions::ByteArrayComparer::Equals(empty_array.AsSpan(), neo::io::ByteVector().AsSpan()));
}

TEST_F(ByteArrayComparerTest, TestGetHashCode)
{
    // Test that hash codes are consistent for equal data
    auto hash1 = neo::extensions::ByteArrayComparer::GetHashCode(test_data1.AsSpan());
    auto hash2 = neo::extensions::ByteArrayComparer::GetHashCode(test_data2.AsSpan());
    EXPECT_EQ(hash1, hash2);

    // Test that different data produces different hash codes (probabilistically)
    auto hash3 = neo::extensions::ByteArrayComparer::GetHashCode(test_data3.AsSpan());
    EXPECT_NE(hash1, hash3);
}

TEST_F(ByteArrayComparerTest, TestSingleByteComparison)
{
    // Test single byte comparisons
    EXPECT_LT(neo::extensions::ByteArrayComparer::Compare(single_zero.AsSpan(), single_one.AsSpan()), 0);
    EXPECT_GT(neo::extensions::ByteArrayComparer::Compare(single_one.AsSpan(), single_zero.AsSpan()), 0);
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_MISC_TEST_BYTEARRAYCOMPARER_CPP_H
