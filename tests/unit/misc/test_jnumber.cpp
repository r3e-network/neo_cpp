// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/misc/test_jnumber.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_MISC_TEST_JNUMBER_CPP_H
#define TESTS_UNIT_MISC_TEST_JNUMBER_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/json/jnumber.h>

namespace neo {
namespace test {

class JNumberTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for JNumber testing
        zero_number = std::make_shared<neo::json::JNumber>(0);
        positive_number = std::make_shared<neo::json::JNumber>(42);
        negative_number = std::make_shared<neo::json::JNumber>(-17);
        float_number = std::make_shared<neo::json::JNumber>(3.14);
    }

    void TearDown() override {
        // Clean up test fixtures - shared_ptr handles cleanup automatically
        zero_number.reset();
        positive_number.reset();
        negative_number.reset();
        float_number.reset();
    }

    // Helper methods and test data for JNumber testing
    std::shared_ptr<neo::json::JNumber> zero_number;
    std::shared_ptr<neo::json::JNumber> positive_number;
    std::shared_ptr<neo::json::JNumber> negative_number;
    std::shared_ptr<neo::json::JNumber> float_number;
};

// JNumber test methods converted from C# UT_JNumber.cs functionality

TEST_F(JNumberTest, ConstructorWithInteger) {
    EXPECT_EQ(positive_number->AsInt(), 42);
    EXPECT_EQ(positive_number->GetType(), neo::json::JTokenType::Number);
}

TEST_F(JNumberTest, ConstructorWithNegativeInteger) {
    EXPECT_EQ(negative_number->AsInt(), -17);
    EXPECT_EQ(negative_number->GetType(), neo::json::JTokenType::Number);
}

TEST_F(JNumberTest, ConstructorWithZero) {
    EXPECT_EQ(zero_number->AsInt(), 0);
    EXPECT_EQ(zero_number->GetType(), neo::json::JTokenType::Number);
}

TEST_F(JNumberTest, ConstructorWithFloat) {
    EXPECT_NEAR(float_number->AsDouble(), 3.14, 0.001);
    EXPECT_EQ(float_number->GetType(), neo::json::JTokenType::Number);
}

TEST_F(JNumberTest, ToStringRepresentation) {
    EXPECT_EQ(positive_number->ToString(), "42");
    EXPECT_EQ(negative_number->ToString(), "-17");
    EXPECT_EQ(zero_number->ToString(), "0");
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_MISC_TEST_JNUMBER_CPP_H
