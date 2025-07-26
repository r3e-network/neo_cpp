// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/misc/test_jpath.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_MISC_TEST_JPATH_CPP_H
#define TESTS_UNIT_MISC_TEST_JPATH_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/json/jpath.h>

namespace neo {
namespace test {

class JPathTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for JPath testing
        sample_json = std::make_shared<neo::json::JObject>();
        sample_json->Set("name", std::make_shared<neo::json::JString>("Neo"));
        sample_json->Set("version", std::make_shared<neo::json::JNumber>(3.0));
        
        auto users_array = std::make_shared<neo::json::JArray>();
        auto user1 = std::make_shared<neo::json::JObject>();
        user1->Set("id", std::make_shared<neo::json::JNumber>(1));
        user1->Set("name", std::make_shared<neo::json::JString>("Alice"));
        users_array->Add(user1);
        
        auto user2 = std::make_shared<neo::json::JObject>();
        user2->Set("id", std::make_shared<neo::json::JNumber>(2));
        user2->Set("name", std::make_shared<neo::json::JString>("Bob"));
        users_array->Add(user2);
        
        sample_json->Set("users", users_array);
    }

    void TearDown() override {
        // Clean up test fixtures
        sample_json.reset();
    }

    // Helper methods and test data for JPath testing
    std::shared_ptr<neo::json::JObject> sample_json;
};

// JPath test methods converted from C# UT_JPath.cs functionality

TEST_F(JPathTest, SimplePropertyAccess) {
    auto result = json::JPath::Evaluate(sample_json, "name");
    ASSERT_NE(result, nullptr);
    auto string_result = std::dynamic_pointer_cast<json::JString>(result);
    ASSERT_NE(string_result, nullptr);
    EXPECT_EQ(string_result->GetValue(), "Neo");
}

TEST_F(JPathTest, NumberPropertyAccess) {
    auto result = json::JPath::Evaluate(sample_json, "version");
    ASSERT_NE(result, nullptr);
    auto number_result = std::dynamic_pointer_cast<json::JNumber>(result);
    ASSERT_NE(number_result, nullptr);
    EXPECT_EQ(number_result->GetValue(), 3.0);
}

TEST_F(JPathTest, ArrayAccess) {
    auto result = json::JPath::Evaluate(sample_json, "users");
    ASSERT_NE(result, nullptr);
    auto array_result = std::dynamic_pointer_cast<json::JArray>(result);
    ASSERT_NE(array_result, nullptr);
    EXPECT_EQ(array_result->Count(), 2);
}

TEST_F(JPathTest, ArrayIndexAccess) {
    auto result = json::JPath::Evaluate(sample_json, "users[0]");
    ASSERT_NE(result, nullptr);
    auto object_result = std::dynamic_pointer_cast<json::JObject>(result);
    ASSERT_NE(object_result, nullptr);
    
    auto name = object_result->Get("name");
    auto string_name = std::dynamic_pointer_cast<json::JString>(name);
    ASSERT_NE(string_name, nullptr);
    EXPECT_EQ(string_name->GetValue(), "Alice");
}

TEST_F(JPathTest, NestedPropertyAccess) {
    auto result = json::JPath::Evaluate(sample_json, "users[1].name");
    ASSERT_NE(result, nullptr);
    auto string_result = std::dynamic_pointer_cast<json::JString>(result);
    ASSERT_NE(string_result, nullptr);
    EXPECT_EQ(string_result->GetValue(), "Bob");
}

TEST_F(JPathTest, NonExistentProperty) {
    auto result = json::JPath::Evaluate(sample_json, "nonexistent");
    EXPECT_EQ(result, nullptr);
}

TEST_F(JPathTest, OutOfBoundsArrayAccess) {
    auto result = json::JPath::Evaluate(sample_json, "users[10]");
    EXPECT_EQ(result, nullptr);
}

TEST_F(JPathTest, InvalidPath) {
    auto result = json::JPath::Evaluate(sample_json, "users[invalid]");
    EXPECT_EQ(result, nullptr);
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_MISC_TEST_JPATH_CPP_H
