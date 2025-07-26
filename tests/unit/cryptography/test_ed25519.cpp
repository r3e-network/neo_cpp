// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/cryptography/test_ed25519.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_CRYPTOGRAPHY_TEST_ED25519_CPP_H
#define TESTS_UNIT_CRYPTOGRAPHY_TEST_ED25519_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/cryptography/ed25519.h>

namespace neo {
namespace test {

class Ed25519Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for Ed25519 testing
        test_message = io::ByteVector::Parse("48656c6c6f20576f726c64"); // "Hello World"
        empty_message = io::ByteVector();
        long_message = io::ByteVector::Parse("4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e73656374657475722061646970697363696e6720656c6974"); // Lorem ipsum
        
        // Test key pairs (32-byte private keys, 32-byte public keys)
        private_key1 = io::ByteVector::Random(32);
        private_key2 = io::ByteVector::Random(32);
        
        // Generate corresponding public keys
        public_key1 = cryptography::Ed25519::GeneratePublicKey(private_key1);
        public_key2 = cryptography::Ed25519::GeneratePublicKey(private_key2);
        
        // Generate test signatures
        signature1 = cryptography::Ed25519::Sign(test_message, private_key1);
        signature2 = cryptography::Ed25519::Sign(test_message, private_key2);
        
        // Invalid key/signature data for negative testing
        invalid_private_key = io::ByteVector::Random(31); // Wrong size
        invalid_public_key = io::ByteVector::Random(31);  // Wrong size
        invalid_signature = io::ByteVector::Random(63);   // Wrong size
    }

    void TearDown() override {
        // Clean up test fixtures - ByteVector handles its own memory
    }

    // Helper methods and test data for Ed25519 testing
    io::ByteVector test_message;
    io::ByteVector empty_message;
    io::ByteVector long_message;
    io::ByteVector private_key1;
    io::ByteVector private_key2;
    io::ByteVector public_key1;
    io::ByteVector public_key2;
    io::ByteVector signature1;
    io::ByteVector signature2;
    io::ByteVector invalid_private_key;
    io::ByteVector invalid_public_key;
    io::ByteVector invalid_signature;
};

// Ed25519 test methods converted from C# UT_Ed25519.cs functionality

TEST_F(Ed25519Test, GenerateKeyPair) {
    auto key_pair = cryptography::Ed25519::GenerateKeyPair();
    
    EXPECT_EQ(key_pair.private_key.Size(), 32);
    EXPECT_EQ(key_pair.public_key.Size(), 32);
    EXPECT_NE(key_pair.private_key, io::ByteVector());
    EXPECT_NE(key_pair.public_key, io::ByteVector());
}

TEST_F(Ed25519Test, GeneratePublicKeyFromPrivate) {
    auto generated_public = cryptography::Ed25519::GeneratePublicKey(private_key1);
    
    EXPECT_EQ(generated_public.Size(), 32);
    EXPECT_EQ(generated_public, public_key1);
}

TEST_F(Ed25519Test, SignMessage) {
    auto signature = cryptography::Ed25519::Sign(test_message, private_key1);
    
    EXPECT_EQ(signature.Size(), 64); // Ed25519 signatures are 64 bytes
    EXPECT_NE(signature, io::ByteVector());
}

TEST_F(Ed25519Test, SignEmptyMessage) {
    auto signature = cryptography::Ed25519::Sign(empty_message, private_key1);
    
    EXPECT_EQ(signature.Size(), 64);
    EXPECT_NE(signature, io::ByteVector());
}

TEST_F(Ed25519Test, SignLongMessage) {
    auto signature = cryptography::Ed25519::Sign(long_message, private_key1);
    
    EXPECT_EQ(signature.Size(), 64);
    EXPECT_NE(signature, io::ByteVector());
}

TEST_F(Ed25519Test, VerifyValidSignature) {
    bool is_valid = cryptography::Ed25519::Verify(test_message, signature1, public_key1);
    EXPECT_TRUE(is_valid);
}

TEST_F(Ed25519Test, VerifySignatureWithWrongKey) {
    // Signature made with private_key1, but verifying with public_key2
    bool is_valid = cryptography::Ed25519::Verify(test_message, signature1, public_key2);
    EXPECT_FALSE(is_valid);
}

TEST_F(Ed25519Test, VerifySignatureWithWrongMessage) {
    // Signature made for test_message, but verifying different message
    bool is_valid = cryptography::Ed25519::Verify(long_message, signature1, public_key1);
    EXPECT_FALSE(is_valid);
}

TEST_F(Ed25519Test, VerifySignatureWithCorruptedSignature) {
    auto corrupted_signature = signature1;
    corrupted_signature[0] ^= 0xFF; // Flip bits in first byte
    
    bool is_valid = cryptography::Ed25519::Verify(test_message, corrupted_signature, public_key1);
    EXPECT_FALSE(is_valid);
}

TEST_F(Ed25519Test, SignaturesDifferentForDifferentKeys) {
    auto sig1 = cryptography::Ed25519::Sign(test_message, private_key1);
    auto sig2 = cryptography::Ed25519::Sign(test_message, private_key2);
    
    EXPECT_NE(sig1, sig2);
}

TEST_F(Ed25519Test, SignaturesDifferentForDifferentMessages) {
    auto sig1 = cryptography::Ed25519::Sign(test_message, private_key1);
    auto sig2 = cryptography::Ed25519::Sign(long_message, private_key1);
    
    EXPECT_NE(sig1, sig2);
}

TEST_F(Ed25519Test, SignatureIsConsistent) {
    auto sig1 = cryptography::Ed25519::Sign(test_message, private_key1);
    auto sig2 = cryptography::Ed25519::Sign(test_message, private_key1);
    
    // Ed25519 is deterministic, so same message + key should produce same signature
    EXPECT_EQ(sig1, sig2);
}

TEST_F(Ed25519Test, InvalidPrivateKeySize) {
    EXPECT_THROW(
        cryptography::Ed25519::GeneratePublicKey(invalid_private_key),
        std::invalid_argument
    );
    
    EXPECT_THROW(
        cryptography::Ed25519::Sign(test_message, invalid_private_key),
        std::invalid_argument
    );
}

TEST_F(Ed25519Test, InvalidPublicKeySize) {
    EXPECT_THROW(
        cryptography::Ed25519::Verify(test_message, signature1, invalid_public_key),
        std::invalid_argument
    );
}

TEST_F(Ed25519Test, InvalidSignatureSize) {
    EXPECT_THROW(
        cryptography::Ed25519::Verify(test_message, invalid_signature, public_key1),
        std::invalid_argument
    );
}

TEST_F(Ed25519Test, VerifyBatchSignatures) {
    // Test batch verification if supported
    std::vector<io::ByteVector> messages = {test_message, long_message, empty_message};
    std::vector<io::ByteVector> signatures;
    std::vector<io::ByteVector> public_keys;
    
    // Generate signatures for each message
    for (const auto& msg : messages) {
        signatures.push_back(cryptography::Ed25519::Sign(msg, private_key1));
        public_keys.push_back(public_key1);
    }
    
    // Verify all signatures individually
    for (size_t i = 0; i < messages.size(); ++i) {
        bool is_valid = cryptography::Ed25519::Verify(messages[i], signatures[i], public_keys[i]);
        EXPECT_TRUE(is_valid);
    }
}

TEST_F(Ed25519Test, PublicKeyFromPrivateKeyIsConsistent) {
    auto pub1 = cryptography::Ed25519::GeneratePublicKey(private_key1);
    auto pub2 = cryptography::Ed25519::GeneratePublicKey(private_key1);
    
    EXPECT_EQ(pub1, pub2);
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_CRYPTOGRAPHY_TEST_ED25519_CPP_H
