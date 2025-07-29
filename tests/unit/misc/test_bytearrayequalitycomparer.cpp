// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/misc/test_bytearrayequalitycomparer.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_MISC_TEST_BYTEARRAYEQUALITYCOMPARER_CPP_H
#define TESTS_UNIT_MISC_TEST_BYTEARRAYEQUALITYCOMPARER_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/extensions/byte_array_equality_comparer.h>

namespace neo
{
namespace test
{

class ByteArrayEqualityComparerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures with various byte array data
        empty_array = neo::io::ByteVector();
        single_byte = neo::io::ByteVector::Parse("42");
        test_data1 = neo::io::ByteVector::Parse("01020304");
        test_data2 = neo::io::ByteVector::Parse("01020304");  // Same as test_data1
        test_data3 = neo::io::ByteVector::Parse("01020305");  // Different from test_data1
        long_data = neo::io::ByteVector::Parse("0123456789abcdef0123456789abcdef");
    }

    void TearDown() override
    {
        // Clean up test fixtures - ByteVector manages its own memory
    }

    // Helper methods and test data
    neo::io::ByteVector empty_array;
    neo::io::ByteVector single_byte;
    neo::io::ByteVector test_data1;
    neo::io::ByteVector test_data2;
    neo::io::ByteVector test_data3;
    neo::io::ByteVector long_data;
};

// Complete test methods converted from C# UT_ByteArrayEqualityComparer.cs functionality

TEST_F(ByteArrayEqualityComparerTest, TestEquals_SameData)
{
    // Test that identical byte arrays are considered equal
    EXPECT_TRUE(neo::extensions::ByteArrayEqualityComparer::Equals(test_data1.AsSpan(), test_data2.AsSpan()));
    EXPECT_TRUE(
        neo::extensions::ByteArrayEqualityComparer::Equals(empty_array.AsSpan(), neo::io::ByteVector().AsSpan()));
    EXPECT_TRUE(neo::extensions::ByteArrayEqualityComparer::Equals(single_byte.AsSpan(),
                                                                   neo::io::ByteVector::Parse("42").AsSpan()));
}

TEST_F(ByteArrayEqualityComparerTest, TestEquals_DifferentData)
{
    // Test that different byte arrays are not considered equal
    EXPECT_FALSE(neo::extensions::ByteArrayEqualityComparer::Equals(test_data1.AsSpan(), test_data3.AsSpan()));
    EXPECT_FALSE(neo::extensions::ByteArrayEqualityComparer::Equals(empty_array.AsSpan(), single_byte.AsSpan()));
    EXPECT_FALSE(neo::extensions::ByteArrayEqualityComparer::Equals(test_data1.AsSpan(), long_data.AsSpan()));
}

TEST_F(ByteArrayEqualityComparerTest, TestEquals_DifferentLengths)
{
    // Test that arrays of different lengths are not equal
    auto short_data = neo::io::ByteVector::Parse("0102");
    auto long_data = neo::io::ByteVector::Parse("010203");
    EXPECT_FALSE(neo::extensions::ByteArrayEqualityComparer::Equals(short_data.AsSpan(), long_data.AsSpan()));
}

TEST_F(ByteArrayEqualityComparerTest, TestGetHashCode_Consistency)
{
    // Test that hash codes are consistent for the same data
    auto hash1 = neo::extensions::ByteArrayEqualityComparer::GetHashCode(test_data1.AsSpan());
    auto hash2 = neo::extensions::ByteArrayEqualityComparer::GetHashCode(test_data2.AsSpan());
    EXPECT_EQ(hash1, hash2);
}

TEST_F(ByteArrayEqualityComparerTest, TestGetHashCode_Different)
{
    // Test that different data produces different hash codes (probabilistically)
    auto hash1 = neo::extensions::ByteArrayEqualityComparer::GetHashCode(test_data1.AsSpan());
    auto hash3 = neo::extensions::ByteArrayEqualityComparer::GetHashCode(test_data3.AsSpan());
    EXPECT_NE(hash1, hash3);
}

TEST_F(ByteArrayEqualityComparerTest, TestHashFunctor)
{
    // Test that the Hash functor works correctly
    neo::extensions::ByteArrayEqualityComparer::Hash hasher;
    auto hash1 = hasher(test_data1);
    auto hash2 = hasher(test_data2);
    EXPECT_EQ(hash1, hash2);
}

TEST_F(ByteArrayEqualityComparerTest, TestEqualFunctor)
{
    // Test that the Equal functor works correctly
    neo::extensions::ByteArrayEqualityComparer::Equal comparer;
    EXPECT_TRUE(comparer(test_data1, test_data2));
    EXPECT_FALSE(comparer(test_data1, test_data3));
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_MISC_TEST_BYTEARRAYEQUALITYCOMPARER_CPP_H
