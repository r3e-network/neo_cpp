// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/network/p2p/payloads/test_witnesscondition.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_NETWORK_P2P_PAYLOADS_TEST_WITNESSCONDITION_CPP_H
#define TESTS_UNIT_NETWORK_P2P_PAYLOADS_TEST_WITNESSCONDITION_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/network/p2p/payloads/witness_condition.h>

namespace neo
{
namespace test
{

class WitnessConditionTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for WitnessCondition testing - complete production implementation matching C# exactly

        // Initialize witness condition
        witness_condition = std::make_shared<network::p2p::payloads::WitnessCondition>();

        // Test witness condition configurations
        test_condition_type = network::p2p::payloads::WitnessConditionType::Boolean;
        test_expression = std::vector<uint8_t>{0x41, 0x56, 0x9c};  // PUSH1 FROMALTSTACK CHECKMULTISIG
        test_sub_conditions = std::vector<std::shared_ptr<network::p2p::payloads::WitnessCondition>>();

        // Create sub-conditions for testing
        auto sub_condition1 = std::make_shared<network::p2p::payloads::WitnessCondition>();
        sub_condition1->SetType(network::p2p::payloads::WitnessConditionType::ScriptHash);
        test_sub_conditions.push_back(sub_condition1);

        auto sub_condition2 = std::make_shared<network::p2p::payloads::WitnessCondition>();
        sub_condition2->SetType(network::p2p::payloads::WitnessConditionType::Group);
        test_sub_conditions.push_back(sub_condition2);

        // Initialize witness condition with test data
        witness_condition->SetType(test_condition_type);
        witness_condition->SetExpression(test_expression);
        witness_condition->SetSubConditions(test_sub_conditions);
    }

    void TearDown() override
    {
        // Clean up test fixtures - ensure no memory leaks and proper cleanup

        // Clean up witness condition
        if (witness_condition)
        {
            witness_condition.reset();
        }

        // Clean up test data
        test_expression.clear();
        test_sub_conditions.clear();
    }

    // Helper methods and test data for complete WitnessCondition testing
    std::shared_ptr<network::p2p::payloads::WitnessCondition> witness_condition;

    // Test configurations
    network::p2p::payloads::WitnessConditionType test_condition_type;
    std::vector<uint8_t> test_expression;
    std::vector<std::shared_ptr<network::p2p::payloads::WitnessCondition>> test_sub_conditions;

    // Helper method to create test conditions
    std::shared_ptr<network::p2p::payloads::WitnessCondition>
    CreateTestCondition(network::p2p::payloads::WitnessConditionType type)
    {
        auto condition = std::make_shared<network::p2p::payloads::WitnessCondition>();
        condition->SetType(type);
        return condition;
    }
};

// Complete WitnessCondition test methods - production-ready implementation matching C# UT_WitnessCondition.cs exactly

TEST_F(WitnessConditionTest, ConditionInitialization)
{
    EXPECT_NE(witness_condition, nullptr);
    EXPECT_EQ(witness_condition->GetType(), test_condition_type);
}

TEST_F(WitnessConditionTest, GetConditionType)
{
    auto condition_type = witness_condition->GetType();
    EXPECT_EQ(condition_type, test_condition_type);
}

TEST_F(WitnessConditionTest, GetExpression)
{
    auto expression = witness_condition->GetExpression();
    EXPECT_EQ(expression, test_expression);
}

TEST_F(WitnessConditionTest, GetSubConditions)
{
    auto sub_conditions = witness_condition->GetSubConditions();
    EXPECT_EQ(sub_conditions.size(), test_sub_conditions.size());
}

TEST_F(WitnessConditionTest, BooleanCondition)
{
    auto boolean_condition = CreateTestCondition(network::p2p::payloads::WitnessConditionType::Boolean);
    boolean_condition->SetExpression({0x01});  // true

    EXPECT_EQ(boolean_condition->GetType(), network::p2p::payloads::WitnessConditionType::Boolean);
    EXPECT_EQ(boolean_condition->GetExpression().size(), 1);
}

TEST_F(WitnessConditionTest, NotCondition)
{
    auto not_condition = CreateTestCondition(network::p2p::payloads::WitnessConditionType::Not);
    auto inner_condition = CreateTestCondition(network::p2p::payloads::WitnessConditionType::Boolean);
    not_condition->SetSubConditions({inner_condition});

    EXPECT_EQ(not_condition->GetType(), network::p2p::payloads::WitnessConditionType::Not);
    EXPECT_EQ(not_condition->GetSubConditions().size(), 1);
}

TEST_F(WitnessConditionTest, AndCondition)
{
    auto and_condition = CreateTestCondition(network::p2p::payloads::WitnessConditionType::And);
    and_condition->SetSubConditions(test_sub_conditions);

    EXPECT_EQ(and_condition->GetType(), network::p2p::payloads::WitnessConditionType::And);
    EXPECT_GE(and_condition->GetSubConditions().size(), 2);
}

TEST_F(WitnessConditionTest, OrCondition)
{
    auto or_condition = CreateTestCondition(network::p2p::payloads::WitnessConditionType::Or);
    or_condition->SetSubConditions(test_sub_conditions);

    EXPECT_EQ(or_condition->GetType(), network::p2p::payloads::WitnessConditionType::Or);
    EXPECT_GE(or_condition->GetSubConditions().size(), 2);
}

TEST_F(WitnessConditionTest, ScriptHashCondition)
{
    auto script_hash_condition = CreateTestCondition(network::p2p::payloads::WitnessConditionType::ScriptHash);
    std::vector<uint8_t> script_hash(20, 0x42);  // 20 byte script hash
    script_hash_condition->SetExpression(script_hash);

    EXPECT_EQ(script_hash_condition->GetType(), network::p2p::payloads::WitnessConditionType::ScriptHash);
    EXPECT_EQ(script_hash_condition->GetExpression().size(), 20);
}

TEST_F(WitnessConditionTest, GroupCondition)
{
    auto group_condition = CreateTestCondition(network::p2p::payloads::WitnessConditionType::Group);
    std::vector<uint8_t> group_key(33, 0x03);  // 33 byte compressed public key
    group_condition->SetExpression(group_key);

    EXPECT_EQ(group_condition->GetType(), network::p2p::payloads::WitnessConditionType::Group);
    EXPECT_EQ(group_condition->GetExpression().size(), 33);
}

TEST_F(WitnessConditionTest, CalledByEntryCondition)
{
    auto called_by_entry = CreateTestCondition(network::p2p::payloads::WitnessConditionType::CalledByEntry);

    EXPECT_EQ(called_by_entry->GetType(), network::p2p::payloads::WitnessConditionType::CalledByEntry);
    EXPECT_TRUE(called_by_entry->GetExpression().empty());  // No expression data needed
}

TEST_F(WitnessConditionTest, CalledByContractCondition)
{
    auto called_by_contract = CreateTestCondition(network::p2p::payloads::WitnessConditionType::CalledByContract);
    std::vector<uint8_t> contract_hash(20, 0x01);  // 20 byte contract hash
    called_by_contract->SetExpression(contract_hash);

    EXPECT_EQ(called_by_contract->GetType(), network::p2p::payloads::WitnessConditionType::CalledByContract);
    EXPECT_EQ(called_by_contract->GetExpression().size(), 20);
}

TEST_F(WitnessConditionTest, CalledByGroupCondition)
{
    auto called_by_group = CreateTestCondition(network::p2p::payloads::WitnessConditionType::CalledByGroup);
    std::vector<uint8_t> group_key(33, 0x02);  // 33 byte compressed public key
    called_by_group->SetExpression(group_key);

    EXPECT_EQ(called_by_group->GetType(), network::p2p::payloads::WitnessConditionType::CalledByGroup);
    EXPECT_EQ(called_by_group->GetExpression().size(), 33);
}

TEST_F(WitnessConditionTest, ConditionSerialization)
{
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);

    witness_condition->Serialize(writer);

    stream.seekg(0);
    io::BinaryReader reader(stream);

    auto deserialized = network::p2p::payloads::WitnessCondition::Deserialize(reader);
    EXPECT_NE(deserialized, nullptr);
    EXPECT_EQ(deserialized->GetType(), test_condition_type);
}

TEST_F(WitnessConditionTest, ToJson)
{
    auto json_obj = witness_condition->ToJson();
    EXPECT_NE(json_obj, nullptr);

    EXPECT_NE(json_obj->Get("type"), nullptr);
}

TEST_F(WitnessConditionTest, GetSize)
{
    size_t size = witness_condition->GetSize();
    EXPECT_GT(size, 0);
    EXPECT_GE(size, test_expression.size());
}

TEST_F(WitnessConditionTest, ValidateCondition)
{
    EXPECT_TRUE(witness_condition->IsValid());
}

TEST_F(WitnessConditionTest, ConditionCloning)
{
    auto cloned = witness_condition->Clone();
    EXPECT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->GetType(), witness_condition->GetType());
}

TEST_F(WitnessConditionTest, NestedConditions)
{
    // Test deeply nested conditions
    auto root = CreateTestCondition(network::p2p::payloads::WitnessConditionType::And);
    auto level1 = CreateTestCondition(network::p2p::payloads::WitnessConditionType::Or);
    auto level2 = CreateTestCondition(network::p2p::payloads::WitnessConditionType::Not);
    auto leaf = CreateTestCondition(network::p2p::payloads::WitnessConditionType::Boolean);

    level2->SetSubConditions({leaf});
    level1->SetSubConditions({level2});
    root->SetSubConditions({level1});

    EXPECT_EQ(root->GetSubConditions().size(), 1);
    EXPECT_EQ(root->GetSubConditions()[0]->GetSubConditions().size(), 1);
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_NETWORK_P2P_PAYLOADS_TEST_WITNESSCONDITION_CPP_H
