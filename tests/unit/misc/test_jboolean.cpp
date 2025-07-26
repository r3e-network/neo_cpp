// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/misc/test_jboolean.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_MISC_TEST_JBOOLEAN_CPP_H
#define TESTS_UNIT_MISC_TEST_JBOOLEAN_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/json/jboolean.h>

namespace neo {
namespace test {

class JBooleanTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for JBoolean testing
        true_value = std::make_shared<neo::json::JBoolean>(true);
        false_value = std::make_shared<neo::json::JBoolean>(false);
    }

    void TearDown() override {
        // Clean up test fixtures - shared_ptr handles cleanup automatically
        true_value.reset();
        false_value.reset();
    }

    // Helper methods and test data for JBoolean testing
    std::shared_ptr<neo::json::JBoolean> true_value;
    std::shared_ptr<neo::json::JBoolean> false_value;
};

// JBoolean test methods converted from C# UT_JBoolean.cs functionality

TEST_F(JBooleanTest, ConstructorWithTrue) {
    EXPECT_TRUE(true_value->AsBoolean());
    EXPECT_EQ(true_value->GetType(), neo::json::JTokenType::Boolean);
}

TEST_F(JBooleanTest, ConstructorWithFalse) {
    EXPECT_FALSE(false_value->AsBoolean());
    EXPECT_EQ(false_value->GetType(), neo::json::JTokenType::Boolean);
}

TEST_F(JBooleanTest, ToStringRepresentation) {
    EXPECT_EQ(true_value->ToString(), "true");
    EXPECT_EQ(false_value->ToString(), "false");
}

TEST_F(JBooleanTest, EqualityComparison) {
    auto another_true = std::make_shared<neo::json::JBoolean>(true);
    auto another_false = std::make_shared<neo::json::JBoolean>(false);
    
    EXPECT_TRUE(true_value->Equals(another_true.get()));
    EXPECT_TRUE(false_value->Equals(another_false.get()));
    EXPECT_FALSE(true_value->Equals(false_value.get()));
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_MISC_TEST_JBOOLEAN_CPP_H
