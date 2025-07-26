// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/ledger/test_witness.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_LEDGER_TEST_WITNESS_CPP_H
#define TESTS_UNIT_LEDGER_TEST_WITNESS_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/ledger/witness.h>

namespace neo {
namespace test {

class WitnessTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for Witness testing
        invocation_script = io::ByteVector::Parse("40414243444546474849505152535455565758596061626364656667686970");
        verification_script = io::ByteVector::Parse("56216c766b00527ac46c766b51527ac46203006c766b00c3616c766b51c3617d65");
        empty_script = io::ByteVector();
        
        // Create witness instances
        test_witness = std::make_shared<ledger::Witness>(invocation_script, verification_script);
        empty_witness = std::make_shared<ledger::Witness>();
        
        // Test data for verification
        test_message = io::ByteVector::Parse("48656c6c6f20576f726c64"); // "Hello World"
        script_hash = cryptography::Hash::Hash160(verification_script);
    }

    void TearDown() override {
        // Clean up test fixtures
        test_witness.reset();
        empty_witness.reset();
    }

    // Helper methods and test data for Witness testing
    io::ByteVector invocation_script;
    io::ByteVector verification_script;
    io::ByteVector empty_script;
    std::shared_ptr<ledger::Witness> test_witness;
    std::shared_ptr<ledger::Witness> empty_witness;
    io::ByteVector test_message;
    io::UInt160 script_hash;
};

// Witness test methods converted from C# UT_Witness.cs functionality

TEST_F(WitnessTest, ConstructorWithScripts) {
    EXPECT_EQ(test_witness->GetInvocationScript(), invocation_script);
    EXPECT_EQ(test_witness->GetVerificationScript(), verification_script);
}

TEST_F(WitnessTest, DefaultConstructor) {
    EXPECT_EQ(empty_witness->GetInvocationScript(), empty_script);
    EXPECT_EQ(empty_witness->GetVerificationScript(), empty_script);
}

TEST_F(WitnessTest, GetScriptHash) {
    auto computed_hash = test_witness->GetScriptHash();
    EXPECT_EQ(computed_hash, script_hash);
    
    // Empty witness should have zero hash
    auto empty_hash = empty_witness->GetScriptHash();
    EXPECT_EQ(empty_hash, cryptography::Hash::Hash160(empty_script));
}

TEST_F(WitnessTest, GetSize) {
    size_t expected_size = invocation_script.Size() + verification_script.Size() + 2; // +2 for length prefixes
    size_t actual_size = test_witness->GetSize();
    EXPECT_EQ(actual_size, expected_size);
    
    // Empty witness should have minimal size
    EXPECT_EQ(empty_witness->GetSize(), 2); // Just the length prefixes
}

TEST_F(WitnessTest, Serialization) {
    // Test serialization
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);
    test_witness->Serialize(writer);
    
    // Test deserialization
    stream.seekg(0);
    io::BinaryReader reader(stream);
    auto deserialized_witness = ledger::Witness::Deserialize(reader);
    
    EXPECT_EQ(deserialized_witness.GetInvocationScript(), invocation_script);
    EXPECT_EQ(deserialized_witness.GetVerificationScript(), verification_script);
    EXPECT_EQ(deserialized_witness.GetScriptHash(), script_hash);
}

TEST_F(WitnessTest, Clone) {
    auto cloned_witness = test_witness->Clone();
    
    EXPECT_NE(cloned_witness.get(), test_witness.get()); // Different objects
    EXPECT_EQ(cloned_witness->GetInvocationScript(), test_witness->GetInvocationScript());
    EXPECT_EQ(cloned_witness->GetVerificationScript(), test_witness->GetVerificationScript());
    EXPECT_EQ(cloned_witness->GetScriptHash(), test_witness->GetScriptHash());
}

TEST_F(WitnessTest, EqualityComparison) {
    auto same_witness = std::make_shared<ledger::Witness>(invocation_script, verification_script);
    auto different_invocation = std::make_shared<ledger::Witness>(empty_script, verification_script);
    auto different_verification = std::make_shared<ledger::Witness>(invocation_script, empty_script);
    
    EXPECT_TRUE(*test_witness == *same_witness);
    EXPECT_FALSE(*test_witness == *different_invocation);
    EXPECT_FALSE(*test_witness == *different_verification);
    EXPECT_FALSE(*test_witness == *empty_witness);
}

TEST_F(WitnessTest, GetHashCode) {
    auto hash1 = test_witness->GetHashCode();
    auto same_witness = std::make_shared<ledger::Witness>(invocation_script, verification_script);
    auto hash2 = same_witness->GetHashCode();
    
    EXPECT_EQ(hash1, hash2); // Same witnesses should have same hash
    
    auto hash3 = empty_witness->GetHashCode();
    EXPECT_NE(hash1, hash3); // Different witnesses should have different hashes
}

TEST_F(WitnessTest, ToJsonObject) {
    auto json_obj = test_witness->ToJson();
    EXPECT_NE(json_obj, nullptr);
    
    // Should contain invocation and verification script fields
    auto invocation_json = json_obj->Get("invocation");
    auto verification_json = json_obj->Get("verification");
    
    EXPECT_NE(invocation_json, nullptr);
    EXPECT_NE(verification_json, nullptr);
}

TEST_F(WitnessTest, FromJsonObject) {
    // Create JSON representation
    auto json_obj = test_witness->ToJson();
    
    // Create witness from JSON
    auto witness_from_json = ledger::Witness::FromJson(json_obj);
    
    EXPECT_EQ(witness_from_json.GetInvocationScript(), invocation_script);
    EXPECT_EQ(witness_from_json.GetVerificationScript(), verification_script);
    EXPECT_EQ(witness_from_json.GetScriptHash(), script_hash);
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_LEDGER_TEST_WITNESS_CPP_H
