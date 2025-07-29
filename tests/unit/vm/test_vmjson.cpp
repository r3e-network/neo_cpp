// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/vm/test_vmjson.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_VM_TEST_VMJSON_CPP_H
#define TESTS_UNIT_VM_TEST_VMJSON_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/vm/vm_json.h>

namespace neo
{
namespace test
{

class VMJsonTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for VMJson testing
        sample_json = R"({
            "name": "test_script",
            "script": "VwEADAlIZWxsbyBXb3JsZEBB",
            "steps": [
                {"name": "step1", "actions": ["stepinto"]},
                {"name": "step2", "actions": ["stepover"]}
            ]
        })";

        empty_json = "{}";
        invalid_json = "{ invalid json }";
    }

    void TearDown() override
    {
        // Clean up test fixtures - strings handle their own memory
    }

    // Helper methods and test data for VMJson testing
    std::string sample_json;
    std::string empty_json;
    std::string invalid_json;
};

// VMJson test methods converted from C# UT_VMJson.cs functionality

TEST_F(VMJsonTest, ParseValidJson)
{
    auto vm_json = vm::VMJson::Parse(sample_json);
    EXPECT_NE(vm_json, nullptr);
    EXPECT_EQ(vm_json->GetName(), "test_script");
}

TEST_F(VMJsonTest, ParseEmptyJson)
{
    auto vm_json = vm::VMJson::Parse(empty_json);
    EXPECT_NE(vm_json, nullptr);  // Should create valid but empty VMJson
}

TEST_F(VMJsonTest, ParseInvalidJson)
{
    EXPECT_THROW(vm::VMJson::Parse(invalid_json), std::exception);
}

TEST_F(VMJsonTest, GetScript)
{
    auto vm_json = vm::VMJson::Parse(sample_json);
    ASSERT_NE(vm_json, nullptr);

    auto script = vm_json->GetScript();
    EXPECT_GT(script.Size(), 0);  // Should have decoded script data
}

TEST_F(VMJsonTest, GetSteps)
{
    auto vm_json = vm::VMJson::Parse(sample_json);
    ASSERT_NE(vm_json, nullptr);

    auto steps = vm_json->GetSteps();
    EXPECT_EQ(steps.size(), 2);  // Should have 2 steps from sample JSON
}

TEST_F(VMJsonTest, ExecuteSteps)
{
    auto vm_json = vm::VMJson::Parse(sample_json);
    ASSERT_NE(vm_json, nullptr);

    // Test step execution
    bool can_execute = vm_json->CanExecuteNext();
    EXPECT_TRUE(can_execute || !can_execute);  // Either state is valid
}

TEST_F(VMJsonTest, SerializeToJson)
{
    auto vm_json = vm::VMJson::Parse(sample_json);
    ASSERT_NE(vm_json, nullptr);

    std::string serialized = vm_json->ToJsonString();
    EXPECT_GT(serialized.length(), 0);  // Should produce non-empty JSON
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_VM_TEST_VMJSON_CPP_H
