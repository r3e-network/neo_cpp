// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/ledger/test_storageitem.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_LEDGER_TEST_STORAGEITEM_CPP_H
#define TESTS_UNIT_LEDGER_TEST_STORAGEITEM_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/persistence/storage_item.h>

namespace neo
{
namespace test
{

class StorageItemTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for StorageItem testing
        test_value = io::ByteVector::Parse("48656c6c6f20576f726c64");  // "Hello World"
        empty_value = io::ByteVector();
        large_value = io::ByteVector::Parse("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");

        test_storage_item = std::make_shared<persistence::StorageItem>(test_value);
        empty_storage_item = std::make_shared<persistence::StorageItem>(empty_value);

        // Test StackItem conversion
        stack_item_int = vm::StackItem::CreateInteger(42);
        stack_item_bytes = vm::StackItem::CreateByteArray(test_value);
    }

    void TearDown() override
    {
        // Clean up test fixtures
        test_storage_item.reset();
        empty_storage_item.reset();
        stack_item_int.reset();
        stack_item_bytes.reset();
    }

    // Helper methods and test data for StorageItem testing
    io::ByteVector test_value;
    io::ByteVector empty_value;
    io::ByteVector large_value;
    std::shared_ptr<persistence::StorageItem> test_storage_item;
    std::shared_ptr<persistence::StorageItem> empty_storage_item;
    std::shared_ptr<vm::StackItem> stack_item_int;
    std::shared_ptr<vm::StackItem> stack_item_bytes;
};

// StorageItem test methods converted from C# UT_StorageItem.cs functionality

TEST_F(StorageItemTest, ConstructorWithByteVector)
{
    EXPECT_EQ(test_storage_item->GetValue(), test_value);
    EXPECT_EQ(test_storage_item->GetSize(), test_value.Size());
}

TEST_F(StorageItemTest, ConstructorWithEmptyValue)
{
    EXPECT_EQ(empty_storage_item->GetValue(), empty_value);
    EXPECT_EQ(empty_storage_item->GetSize(), 0);
}

TEST_F(StorageItemTest, ConstructorFromStackItem)
{
    auto item_from_int = persistence::StorageItem::FromStackItem(stack_item_int);
    EXPECT_NE(item_from_int, nullptr);
    EXPECT_GT(item_from_int->GetSize(), 0);

    auto item_from_bytes = persistence::StorageItem::FromStackItem(stack_item_bytes);
    EXPECT_NE(item_from_bytes, nullptr);
    EXPECT_EQ(item_from_bytes->GetValue(), test_value);
}

TEST_F(StorageItemTest, ToStackItem)
{
    auto stack_item = test_storage_item->ToStackItem();
    EXPECT_NE(stack_item, nullptr);

    // Should be able to convert back
    auto back_to_storage = persistence::StorageItem::FromStackItem(stack_item);
    EXPECT_EQ(back_to_storage->GetValue(), test_value);
}

TEST_F(StorageItemTest, Serialization)
{
    // Test serialization
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);
    test_storage_item->Serialize(writer);

    // Test deserialization
    stream.seekg(0);
    io::BinaryReader reader(stream);
    auto deserialized_item = persistence::StorageItem::Deserialize(reader);

    EXPECT_EQ(deserialized_item.GetValue(), test_value);
    EXPECT_EQ(deserialized_item.GetSize(), test_value.Size());
}

TEST_F(StorageItemTest, Clone)
{
    auto cloned_item = test_storage_item->Clone();
    EXPECT_NE(cloned_item.get(), test_storage_item.get());              // Different objects
    EXPECT_EQ(cloned_item->GetValue(), test_storage_item->GetValue());  // Same data
}

TEST_F(StorageItemTest, EqualityComparison)
{
    auto same_item = std::make_shared<persistence::StorageItem>(test_value);
    auto different_item = std::make_shared<persistence::StorageItem>(large_value);

    EXPECT_TRUE(*test_storage_item == *same_item);
    EXPECT_FALSE(*test_storage_item == *different_item);
    EXPECT_FALSE(*test_storage_item == *empty_storage_item);
}

TEST_F(StorageItemTest, GetHashCode)
{
    auto hash1 = test_storage_item->GetHashCode();
    auto same_item = std::make_shared<persistence::StorageItem>(test_value);
    auto hash2 = same_item->GetHashCode();

    EXPECT_EQ(hash1, hash2);  // Same items should have same hash

    auto hash3 = empty_storage_item->GetHashCode();
    EXPECT_NE(hash1, hash3);  // Different items should have different hashes
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_LEDGER_TEST_STORAGEITEM_CPP_H
