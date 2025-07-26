// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/misc/test_jobject.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_MISC_TEST_JOBJECT_CPP_H
#define TESTS_UNIT_MISC_TEST_JOBJECT_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/json/jobject.h>

namespace neo {
namespace test {

class JObjectTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for JObject testing
        empty_object = std::make_shared<neo::json::JObject>();
        
        test_object = std::make_shared<neo::json::JObject>();
        test_object->Set("name", std::make_shared<neo::json::JString>("Neo"));
        test_object->Set("version", std::make_shared<neo::json::JNumber>(3.0));
        test_object->Set("active", std::make_shared<neo::json::JBoolean>(true));
        test_object->Set("null_value", nullptr);
        
        nested_object = std::make_shared<neo::json::JObject>();
        auto inner = std::make_shared<neo::json::JObject>();
        inner->Set("value", std::make_shared<neo::json::JNumber>(42));
        nested_object->Set("inner", inner);
    }

    void TearDown() override {
        // Clean up test fixtures
        empty_object.reset();
        test_object.reset();
        nested_object.reset();
    }

    // Helper methods and test data for JObject testing
    std::shared_ptr<neo::json::JObject> empty_object;
    std::shared_ptr<neo::json::JObject> test_object;
    std::shared_ptr<neo::json::JObject> nested_object;
};

// JObject test methods converted from C# UT_JObject.cs functionality

TEST_F(JObjectTest, ConstructorCreatesEmptyObject) {
    EXPECT_EQ(empty_object->Count(), 0);
    EXPECT_TRUE(empty_object->IsEmpty());
}

TEST_F(JObjectTest, SetAndGetProperties) {
    auto name = test_object->Get("name");
    ASSERT_NE(name, nullptr);
    auto string_name = std::dynamic_pointer_cast<json::JString>(name);
    ASSERT_NE(string_name, nullptr);
    EXPECT_EQ(string_name->GetValue(), "Neo");
    
    auto version = test_object->Get("version");
    ASSERT_NE(version, nullptr);
    auto number_version = std::dynamic_pointer_cast<json::JNumber>(version);
    ASSERT_NE(number_version, nullptr);
    EXPECT_EQ(number_version->GetValue(), 3.0);
}

TEST_F(JObjectTest, ContainsKey) {
    EXPECT_TRUE(test_object->Contains("name"));
    EXPECT_TRUE(test_object->Contains("version"));
    EXPECT_FALSE(test_object->Contains("nonexistent"));
}

TEST_F(JObjectTest, RemoveProperty) {
    EXPECT_TRUE(test_object->Contains("name"));
    bool removed = test_object->Remove("name");
    EXPECT_TRUE(removed);
    EXPECT_FALSE(test_object->Contains("name"));
    
    bool remove_nonexistent = test_object->Remove("nonexistent");
    EXPECT_FALSE(remove_nonexistent);
}

TEST_F(JObjectTest, GetPropertyKeys) {
    auto keys = test_object->GetKeys();
    EXPECT_EQ(keys.size(), 4); // name, version, active, null_value
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "name") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "version") != keys.end());
}

TEST_F(JObjectTest, ClearObject) {
    EXPECT_GT(test_object->Count(), 0);
    test_object->Clear();
    EXPECT_EQ(test_object->Count(), 0);
    EXPECT_TRUE(test_object->IsEmpty());
}

TEST_F(JObjectTest, NestedObjectAccess) {
    auto inner = nested_object->Get("inner");
    ASSERT_NE(inner, nullptr);
    auto inner_object = std::dynamic_pointer_cast<json::JObject>(inner);
    ASSERT_NE(inner_object, nullptr);
    
    auto value = inner_object->Get("value");
    ASSERT_NE(value, nullptr);
    auto number_value = std::dynamic_pointer_cast<json::JNumber>(value);
    ASSERT_NE(number_value, nullptr);
    EXPECT_EQ(number_value->GetValue(), 42);
}

TEST_F(JObjectTest, ToJsonString) {
    std::string json_str = test_object->ToString();
    EXPECT_GT(json_str.length(), 0);
    EXPECT_NE(json_str.find("name"), std::string::npos);
    EXPECT_NE(json_str.find("Neo"), std::string::npos);
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_MISC_TEST_JOBJECT_CPP_H
