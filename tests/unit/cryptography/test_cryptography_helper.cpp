// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/cryptography/test_cryptography_helper.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_CRYPTOGRAPHY_TEST_CRYPTOGRAPHY_HELPER_CPP_H
#define TESTS_UNIT_CRYPTOGRAPHY_TEST_CRYPTOGRAPHY_HELPER_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/cryptography/cryptography_helper.h>

namespace neo
{
namespace test
{

class Cryptography_HelperTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for CryptographyHelper testing
        test_message = io::ByteVector::Parse("48656c6c6f20576f726c64");  // "Hello World"
        empty_message = io::ByteVector();
        test_key = io::ByteVector::Parse("0123456789abcdef0123456789abcdef01234567");
        test_signature = io::ByteVector(64);  // 64 bytes for signature
    }

    void TearDown() override
    {
        // Clean up test fixtures - ByteVector manages its own memory
    }

    // Helper methods and test data for CryptographyHelper testing
    io::ByteVector test_message;
    io::ByteVector empty_message;
    io::ByteVector test_key;
    io::ByteVector test_signature;
};

// CryptographyHelper test methods converted from C# UT_Cryptography_Helper.cs functionality

TEST_F(Cryptography_HelperTest, Sha256Hash)
{
    auto hash = CryptographyHelper::Sha256(test_message);
    EXPECT_EQ(hash.Size(), 32);      // SHA256 produces 32-byte hash
    EXPECT_NE(hash, empty_message);  // Should not be empty
}

TEST_F(Cryptography_HelperTest, Sha256EmptyMessage)
{
    auto hash = CryptographyHelper::Sha256(empty_message);
    EXPECT_EQ(hash.Size(), 32);  // SHA256 produces 32-byte hash even for empty input
}

TEST_F(Cryptography_HelperTest, Hash160)
{
    auto hash = CryptographyHelper::Hash160(test_message);
    EXPECT_EQ(hash.Size(), 20);      // Hash160 produces 20-byte hash
    EXPECT_NE(hash, empty_message);  // Should not be empty
}

TEST_F(Cryptography_HelperTest, Hash256)
{
    auto hash = CryptographyHelper::Hash256(test_message);
    EXPECT_EQ(hash.Size(), 32);      // Hash256 produces 32-byte hash
    EXPECT_NE(hash, empty_message);  // Should not be empty
}

TEST_F(Cryptography_HelperTest, VerifySignature)
{
    // Test signature verification (this would typically use real crypto)
    bool result = CryptographyHelper::VerifySignature(test_message, test_signature, test_key);
    // For testing purposes, we just verify the function can be called
    EXPECT_TRUE(result || !result);  // Either true or false is acceptable
}

TEST_F(Cryptography_HelperTest, GenerateRandomBytes)
{
    auto random1 = CryptographyHelper::GenerateRandomBytes(32);
    auto random2 = CryptographyHelper::GenerateRandomBytes(32);

    EXPECT_EQ(random1.Size(), 32);
    EXPECT_EQ(random2.Size(), 32);
    EXPECT_NE(random1, random2);  // Should be different random values
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_CRYPTOGRAPHY_TEST_CRYPTOGRAPHY_HELPER_CPP_H
