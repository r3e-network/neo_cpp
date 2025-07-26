// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/ledger/test_storagekey.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_LEDGER_TEST_STORAGEKEY_CPP_H
#define TESTS_UNIT_LEDGER_TEST_STORAGEKEY_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/ledger/storage_key.h>

namespace neo {
namespace test {

class StorageKeyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for StorageKey testing
        contract_id = 123;
        test_key_data = io::ByteVector::Parse("48656c6c6f576f726c64"); // "HelloWorld"
        empty_key_data = io::ByteVector();
        large_key_data = io::ByteVector::Parse("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        
        test_storage_key = std::make_shared<ledger::StorageKey>(contract_id, test_key_data);
        empty_storage_key = std::make_shared<ledger::StorageKey>(contract_id, empty_key_data);
    }

    void TearDown() override {
        // Clean up test fixtures
        test_storage_key.reset();
        empty_storage_key.reset();
    }

    // Helper methods and test data for StorageKey testing
    int32_t contract_id;
    io::ByteVector test_key_data;
    io::ByteVector empty_key_data;
    io::ByteVector large_key_data;
    std::shared_ptr<ledger::StorageKey> test_storage_key;
    std::shared_ptr<ledger::StorageKey> empty_storage_key;
};

// StorageKey test methods converted from C# UT_StorageKey.cs functionality

TEST_F(StorageKeyTest, ConstructorWithContractIdAndKey) {
    EXPECT_EQ(test_storage_key->GetContractId(), contract_id);
    EXPECT_EQ(test_storage_key->GetKey(), test_key_data);
}

TEST_F(StorageKeyTest, ConstructorWithEmptyKey) {
    EXPECT_EQ(empty_storage_key->GetContractId(), contract_id);
    EXPECT_EQ(empty_storage_key->GetKey(), empty_key_data);
    EXPECT_EQ(empty_storage_key->GetKey().Size(), 0);
}

TEST_F(StorageKeyTest, GetKeySize) {
    EXPECT_EQ(test_storage_key->GetKeySize(), test_key_data.Size());
    EXPECT_EQ(empty_storage_key->GetKeySize(), 0);
}

TEST_F(StorageKeyTest, EqualityComparison) {
    auto same_key = std::make_shared<ledger::StorageKey>(contract_id, test_key_data);
    auto different_contract = std::make_shared<ledger::StorageKey>(456, test_key_data);
    auto different_key = std::make_shared<ledger::StorageKey>(contract_id, large_key_data);
    
    EXPECT_TRUE(*test_storage_key == *same_key);
    EXPECT_FALSE(*test_storage_key == *different_contract);
    EXPECT_FALSE(*test_storage_key == *different_key);
}

TEST_F(StorageKeyTest, Serialization) {
    // Test serialization
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);
    test_storage_key->Serialize(writer);
    
    // Test deserialization
    stream.seekg(0);
    io::BinaryReader reader(stream);
    auto deserialized_key = ledger::StorageKey::Deserialize(reader);
    
    EXPECT_EQ(deserialized_key.GetContractId(), contract_id);
    EXPECT_EQ(deserialized_key.GetKey(), test_key_data);
}

TEST_F(StorageKeyTest, ToByteArray) {
    auto bytes = test_storage_key->ToByteArray();
    EXPECT_GT(bytes.Size(), 0);
    EXPECT_GE(bytes.Size(), test_key_data.Size()); // Should include contract ID
}

TEST_F(StorageKeyTest, GetHashCode) {
    auto hash1 = test_storage_key->GetHashCode();
    auto same_key = std::make_shared<ledger::StorageKey>(contract_id, test_key_data);
    auto hash2 = same_key->GetHashCode();
    
    EXPECT_EQ(hash1, hash2); // Same keys should have same hash
    
    auto different_key = std::make_shared<ledger::StorageKey>(contract_id, large_key_data);
    auto hash3 = different_key->GetHashCode();
    EXPECT_NE(hash1, hash3); // Different keys should have different hashes
}

TEST_F(StorageKeyTest, ToString) {
    std::string str_repr = test_storage_key->ToString();
    EXPECT_GT(str_repr.length(), 0);
    EXPECT_NE(str_repr.find(std::to_string(contract_id)), std::string::npos);
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_LEDGER_TEST_STORAGEKEY_CPP_H
