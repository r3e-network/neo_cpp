// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/misc/test_jarray.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_MISC_TEST_JARRAY_CPP_H
#define TESTS_UNIT_MISC_TEST_JARRAY_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/json/jarray.h>

namespace neo
{
namespace test
{

class JArrayTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for JArray testing
        empty_array = std::make_shared<neo::json::JArray>();
        sample_array = std::make_shared<neo::json::JArray>();
        sample_array->Add(std::make_shared<neo::json::JString>("test"));
        sample_array->Add(std::make_shared<neo::json::JNumber>(42));
        sample_array->Add(std::make_shared<neo::json::JBoolean>(true));
    }

    void TearDown() override
    {
        // Clean up test fixtures - shared_ptr handles cleanup automatically
        empty_array.reset();
        sample_array.reset();
    }

    // Helper methods and test data for JArray testing
    std::shared_ptr<neo::json::JArray> empty_array;
    std::shared_ptr<neo::json::JArray> sample_array;
};

// JArray test methods converted from C# UT_JArray.cs functionality

TEST_F(JArrayTest, ConstructorCreatesEmptyArray)
{
    EXPECT_EQ(empty_array->Count(), 0);
    EXPECT_TRUE(empty_array->IsEmpty());
}

TEST_F(JArrayTest, AddItemIncreasesCount)
{
    EXPECT_EQ(empty_array->Count(), 0);
    empty_array->Add(std::make_shared<neo::json::JString>("test"));
    EXPECT_EQ(empty_array->Count(), 1);
    EXPECT_FALSE(empty_array->IsEmpty());
}

TEST_F(JArrayTest, AccessItemsByIndex)
{
    ASSERT_EQ(sample_array->Count(), 3);

    auto item0 = sample_array->Get(0);
    ASSERT_NE(item0, nullptr);
    EXPECT_EQ(item0->GetType(), neo::json::JTokenType::String);

    auto item1 = sample_array->Get(1);
    ASSERT_NE(item1, nullptr);
    EXPECT_EQ(item1->GetType(), neo::json::JTokenType::Number);

    auto item2 = sample_array->Get(2);
    ASSERT_NE(item2, nullptr);
    EXPECT_EQ(item2->GetType(), neo::json::JTokenType::Boolean);
}

TEST_F(JArrayTest, RemoveItemDecreasesCount)
{
    ASSERT_EQ(sample_array->Count(), 3);
    sample_array->RemoveAt(1);
    EXPECT_EQ(sample_array->Count(), 2);
}

TEST_F(JArrayTest, ClearRemovesAllItems)
{
    ASSERT_GT(sample_array->Count(), 0);
    sample_array->Clear();
    EXPECT_EQ(sample_array->Count(), 0);
    EXPECT_TRUE(sample_array->IsEmpty());
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_MISC_TEST_JARRAY_CPP_H
