// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/ledger/test_hashindexstate.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_LEDGER_TEST_HASHINDEXSTATE_CPP_H
#define TESTS_UNIT_LEDGER_TEST_HASHINDEXSTATE_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/ledger/hash_index_state.h>

namespace neo {
namespace test {

class HashIndexStateTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for HashIndexState testing
        test_hash1 = io::UInt256::Parse("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
        test_hash2 = io::UInt256::Parse("abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890");
        test_hash3 = io::UInt256::Parse("fedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321");
        
        // Create test hash index states
        hash_index_state1 = std::make_shared<ledger::HashIndexState>(test_hash1, 100);
        hash_index_state2 = std::make_shared<ledger::HashIndexState>(test_hash2, 200);
        hash_index_state3 = std::make_shared<ledger::HashIndexState>(test_hash3, 300);
        
        // Create empty state for testing
        empty_hash_index_state = std::make_shared<ledger::HashIndexState>();
        
        // Create states with same hash but different indices for testing
        duplicate_hash_state = std::make_shared<ledger::HashIndexState>(test_hash1, 150);
    }

    void TearDown() override {
        // Clean up test fixtures
        hash_index_state1.reset();
        hash_index_state2.reset();
        hash_index_state3.reset();
        empty_hash_index_state.reset();
        duplicate_hash_state.reset();
    }

    // Helper methods and test data for HashIndexState testing
    io::UInt256 test_hash1;
    io::UInt256 test_hash2;
    io::UInt256 test_hash3;
    std::shared_ptr<ledger::HashIndexState> hash_index_state1;
    std::shared_ptr<ledger::HashIndexState> hash_index_state2;
    std::shared_ptr<ledger::HashIndexState> hash_index_state3;
    std::shared_ptr<ledger::HashIndexState> empty_hash_index_state;
    std::shared_ptr<ledger::HashIndexState> duplicate_hash_state;
};

// HashIndexState test methods converted from C# UT_HashIndexState.cs functionality

TEST_F(HashIndexStateTest, ConstructorWithHashAndIndex) {
    EXPECT_EQ(hash_index_state1->GetHash(), test_hash1);
    EXPECT_EQ(hash_index_state1->GetIndex(), 100);
}

TEST_F(HashIndexStateTest, DefaultConstructor) {
    EXPECT_EQ(empty_hash_index_state->GetHash(), io::UInt256());
    EXPECT_EQ(empty_hash_index_state->GetIndex(), 0);
}

TEST_F(HashIndexStateTest, SetAndGetHash) {
    empty_hash_index_state->SetHash(test_hash2);
    EXPECT_EQ(empty_hash_index_state->GetHash(), test_hash2);
}

TEST_F(HashIndexStateTest, SetAndGetIndex) {
    empty_hash_index_state->SetIndex(999);
    EXPECT_EQ(empty_hash_index_state->GetIndex(), 999);
}

TEST_F(HashIndexStateTest, EqualityComparison) {
    auto same_state = std::make_shared<ledger::HashIndexState>(test_hash1, 100);
    EXPECT_TRUE(*hash_index_state1 == *same_state);
    EXPECT_FALSE(*hash_index_state1 == *hash_index_state2);
    EXPECT_FALSE(*hash_index_state1 == *duplicate_hash_state); // Same hash, different index
}

TEST_F(HashIndexStateTest, InequalityComparison) {
    auto same_state = std::make_shared<ledger::HashIndexState>(test_hash1, 100);
    EXPECT_FALSE(*hash_index_state1 != *same_state);
    EXPECT_TRUE(*hash_index_state1 != *hash_index_state2);
    EXPECT_TRUE(*hash_index_state1 != *duplicate_hash_state);
}

TEST_F(HashIndexStateTest, HashCode) {
    auto hash_code1 = hash_index_state1->GetHashCode();
    auto hash_code2 = hash_index_state2->GetHashCode();
    auto same_state = std::make_shared<ledger::HashIndexState>(test_hash1, 100);
    auto same_hash_code = same_state->GetHashCode();
    
    EXPECT_EQ(hash_code1, same_hash_code);
    EXPECT_NE(hash_code1, hash_code2);
}

TEST_F(HashIndexStateTest, IsValid) {
    EXPECT_TRUE(hash_index_state1->IsValid());
    EXPECT_TRUE(hash_index_state2->IsValid());
    EXPECT_FALSE(empty_hash_index_state->IsValid()); // Empty hash should be invalid
}

TEST_F(HashIndexStateTest, Clone) {
    auto cloned_state = hash_index_state1->Clone();
    
    EXPECT_NE(cloned_state.get(), hash_index_state1.get()); // Different objects
    EXPECT_EQ(cloned_state->GetHash(), hash_index_state1->GetHash());
    EXPECT_EQ(cloned_state->GetIndex(), hash_index_state1->GetIndex());
    EXPECT_TRUE(*cloned_state == *hash_index_state1);
}

TEST_F(HashIndexStateTest, Serialization) {
    // Test serialization
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);
    hash_index_state1->Serialize(writer);
    
    // Test deserialization
    stream.seekg(0);
    io::BinaryReader reader(stream);
    auto deserialized_state = ledger::HashIndexState::Deserialize(reader);
    
    EXPECT_EQ(deserialized_state.GetHash(), hash_index_state1->GetHash());
    EXPECT_EQ(deserialized_state.GetIndex(), hash_index_state1->GetIndex());
    EXPECT_TRUE(deserialized_state == *hash_index_state1);
}

TEST_F(HashIndexStateTest, GetSize) {
    size_t size = hash_index_state1->GetSize();
    EXPECT_GT(size, 0);
    
    // Size should be consistent for same object
    size_t size2 = hash_index_state1->GetSize();
    EXPECT_EQ(size, size2);
    
    // Size should account for hash (32 bytes) + index (4 bytes) + overhead
    EXPECT_GE(size, 36);
}

TEST_F(HashIndexStateTest, ToJson) {
    auto json_obj = hash_index_state1->ToJson();
    EXPECT_NE(json_obj, nullptr);
    
    // Should contain hash and index fields
    auto hash_json = json_obj->Get("hash");
    auto index_json = json_obj->Get("index");
    
    EXPECT_NE(hash_json, nullptr);
    EXPECT_NE(index_json, nullptr);
}

TEST_F(HashIndexStateTest, FromJson) {
    auto json_obj = hash_index_state1->ToJson();
    auto state_from_json = ledger::HashIndexState::FromJson(json_obj);
    
    EXPECT_EQ(state_from_json.GetHash(), hash_index_state1->GetHash());
    EXPECT_EQ(state_from_json.GetIndex(), hash_index_state1->GetIndex());
    EXPECT_TRUE(state_from_json == *hash_index_state1);
}

TEST_F(HashIndexStateTest, CompareByIndex) {
    // Test ordering by index
    EXPECT_TRUE(hash_index_state1->GetIndex() < hash_index_state2->GetIndex());
    EXPECT_TRUE(hash_index_state2->GetIndex() < hash_index_state3->GetIndex());
    EXPECT_TRUE(hash_index_state1->GetIndex() < duplicate_hash_state->GetIndex());
}

TEST_F(HashIndexStateTest, CompareByHash) {
    // Test hash comparison
    EXPECT_NE(hash_index_state1->GetHash(), hash_index_state2->GetHash());
    EXPECT_NE(hash_index_state2->GetHash(), hash_index_state3->GetHash());
    EXPECT_EQ(hash_index_state1->GetHash(), duplicate_hash_state->GetHash());
}

TEST_F(HashIndexStateTest, SetInvalidValues) {
    // Test setting empty hash
    hash_index_state1->SetHash(io::UInt256());
    EXPECT_EQ(hash_index_state1->GetHash(), io::UInt256());
    EXPECT_FALSE(hash_index_state1->IsValid());
    
    // Test negative index (if supported)
    hash_index_state1->SetIndex(-1);
    EXPECT_EQ(hash_index_state1->GetIndex(), -1);
}

TEST_F(HashIndexStateTest, ToString) {
    std::string str = hash_index_state1->ToString();
    EXPECT_FALSE(str.empty());
    
    // Should contain both hash and index information
    EXPECT_NE(str.find("100"), std::string::npos); // Should contain index
}

TEST_F(HashIndexStateTest, CopyConstructor) {
    ledger::HashIndexState copied_state(*hash_index_state1);
    
    EXPECT_EQ(copied_state.GetHash(), hash_index_state1->GetHash());
    EXPECT_EQ(copied_state.GetIndex(), hash_index_state1->GetIndex());
    EXPECT_TRUE(copied_state == *hash_index_state1);
}

TEST_F(HashIndexStateTest, AssignmentOperator) {
    ledger::HashIndexState assigned_state;
    assigned_state = *hash_index_state1;
    
    EXPECT_EQ(assigned_state.GetHash(), hash_index_state1->GetHash());
    EXPECT_EQ(assigned_state.GetIndex(), hash_index_state1->GetIndex());
    EXPECT_TRUE(assigned_state == *hash_index_state1);
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_LEDGER_TEST_HASHINDEXSTATE_CPP_H
