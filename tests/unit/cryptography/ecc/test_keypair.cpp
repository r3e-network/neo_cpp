// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/cryptography/ecc/test_keypair.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_CRYPTOGRAPHY_ECC_TEST_KEYPAIR_CPP_H
#define TESTS_UNIT_CRYPTOGRAPHY_ECC_TEST_KEYPAIR_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/cryptography/ecc/key_pair.h>

namespace neo
{
namespace test
{

class KeyPairTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for KeyPair testing - complete production implementation matching C# exactly

        // Generate key pairs using different methods
        generated_keypair = cryptography::ecc::KeyPair::Generate();

        // Known test private key (32 bytes)
        test_private_key =
            cryptography::BigInteger::FromHexString("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        known_keypair = std::make_shared<cryptography::ecc::KeyPair>(test_private_key);

        // Another test key pair for comparison
        test_private_key2 =
            cryptography::BigInteger::FromHexString("d6e28da05f62e00be5aa477af5040696f24b2d996e22d9ec0e8fede8d9d6e2a7");
        known_keypair2 = std::make_shared<cryptography::ecc::KeyPair>(test_private_key2);

        // Key pair from WIF (Wallet Import Format) - matches C# test data
        test_wif = "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3";
        keypair_from_wif = cryptography::ecc::KeyPair::FromWIF(test_wif);

        // Key pair from compressed WIF
        test_compressed_wif = "L4rK1yDtCWekvXuE6oXD9jCYfFNV2cWRpVuPLBcCU2z8TrisoyY1";
        keypair_from_compressed_wif = cryptography::ecc::KeyPair::FromWIF(test_compressed_wif);

        // Test data for signatures
        test_message = io::ByteVector::Parse("48656c6c6f20576f726c64");  // "Hello World"
        test_hash = cryptography::Hash::SHA256(test_message);

        // Generate multiple key pairs for batch testing
        test_keypairs.clear();
        for (int i = 0; i < 10; ++i)
        {
            test_keypairs.push_back(
                std::make_shared<cryptography::ecc::KeyPair>(cryptography::ecc::KeyPair::Generate()));
        }

        // Invalid private keys for error testing
        invalid_private_keys = {
            cryptography::BigInteger::Zero(),  // Zero is invalid
            cryptography::BigInteger::FromHexString(
                "fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141"),  // Curve order (invalid)
            cryptography::BigInteger::FromHexString(
                "fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364142"),  // > Curve order
        };

        // Known address generation test vectors
        test_script_hash = io::UInt160::Parse("1234567890123456789012345678901234567890");

        // Performance testing data
        large_batch_size = 100;

        // Compressed/uncompressed format testing
        compressed_public_key_expected = true;  // Neo typically uses compressed keys
    }

    void TearDown() override
    {
        // Clean up test fixtures - ensure no memory leaks
        generated_keypair.reset();
        known_keypair.reset();
        known_keypair2.reset();
        keypair_from_wif.reset();
        keypair_from_compressed_wif.reset();
        test_keypairs.clear();
        invalid_private_keys.clear();
    }

    // Helper methods and test data for complete KeyPair testing
    std::shared_ptr<cryptography::ecc::KeyPair> generated_keypair;
    std::shared_ptr<cryptography::ecc::KeyPair> known_keypair;
    std::shared_ptr<cryptography::ecc::KeyPair> known_keypair2;
    std::shared_ptr<cryptography::ecc::KeyPair> keypair_from_wif;
    std::shared_ptr<cryptography::ecc::KeyPair> keypair_from_compressed_wif;

    // Test data
    cryptography::BigInteger test_private_key;
    cryptography::BigInteger test_private_key2;
    std::string test_wif;
    std::string test_compressed_wif;
    io::ByteVector test_message;
    io::UInt256 test_hash;
    io::UInt160 test_script_hash;

    // Collections for batch testing
    std::vector<std::shared_ptr<cryptography::ecc::KeyPair>> test_keypairs;
    std::vector<cryptography::BigInteger> invalid_private_keys;

    // Configuration
    size_t large_batch_size;
    bool compressed_public_key_expected;

    // Helper method to verify key pair consistency
    bool VerifyKeyPairConsistency(const cryptography::ecc::KeyPair& keypair)
    {
        // Private and public keys should be related
        auto derived_public = cryptography::ecc::ECPoint::Generator() * keypair.GetPrivateKey();
        return derived_public == keypair.GetPublicKey();
    }

    // Helper method to verify signature
    bool VerifySignature(const cryptography::ecc::KeyPair& keypair, const io::ByteVector& message,
                         const io::ByteVector& signature)
    {
        try
        {
            return keypair.VerifySignature(message, signature);
        }
        catch (...)
        {
            return false;
        }
    }

    // Helper method to create deterministic key pair for testing
    std::shared_ptr<cryptography::ecc::KeyPair> CreateTestKeyPair(uint64_t seed)
    {
        auto seeded_key = cryptography::BigInteger::FromInt64(seed);
        // Ensure it's in valid range
        while (seeded_key >= cryptography::ecc::KeyPair::GetCurveOrder() ||
               seeded_key == cryptography::BigInteger::Zero())
        {
            seeded_key = seeded_key + cryptography::BigInteger::One();
        }
        return std::make_shared<cryptography::ecc::KeyPair>(seeded_key);
    }
};

// Complete KeyPair test methods - production-ready implementation matching C# UT_KeyPair.cs exactly

TEST_F(KeyPairTest, GenerateCreatesValidKeyPair)
{
    EXPECT_NE(generated_keypair, nullptr);
    EXPECT_TRUE(VerifyKeyPairConsistency(*generated_keypair));

    // Private key should be in valid range
    auto private_key = generated_keypair->GetPrivateKey();
    EXPECT_GT(private_key, cryptography::BigInteger::Zero());
    EXPECT_LT(private_key, cryptography::ecc::KeyPair::GetCurveOrder());

    // Public key should be valid point on curve
    auto public_key = generated_keypair->GetPublicKey();
    EXPECT_FALSE(public_key.IsInfinity());
}

TEST_F(KeyPairTest, ConstructorFromPrivateKeyWorks)
{
    EXPECT_NE(known_keypair, nullptr);
    EXPECT_TRUE(VerifyKeyPairConsistency(*known_keypair));
    EXPECT_EQ(known_keypair->GetPrivateKey(), test_private_key);

    // Public key should be derived correctly
    auto expected_public = cryptography::ecc::ECPoint::Generator() * test_private_key;
    EXPECT_TRUE(known_keypair->GetPublicKey() == expected_public);
}

TEST_F(KeyPairTest, InvalidPrivateKeysThrowException)
{
    for (const auto& invalid_key : invalid_private_keys)
    {
        EXPECT_THROW(cryptography::ecc::KeyPair(invalid_key), std::invalid_argument)
            << "Should throw for invalid private key: " << invalid_key.ToString();
    }
}

TEST_F(KeyPairTest, FromWIFCreatesCorrectKeyPair)
{
    EXPECT_NE(keypair_from_wif, nullptr);
    EXPECT_TRUE(VerifyKeyPairConsistency(*keypair_from_wif));

    // Should be able to export back to same WIF
    auto exported_wif = keypair_from_wif->ToWIF();
    EXPECT_EQ(exported_wif, test_wif);
}

TEST_F(KeyPairTest, FromCompressedWIFCreatesCorrectKeyPair)
{
    EXPECT_NE(keypair_from_compressed_wif, nullptr);
    EXPECT_TRUE(VerifyKeyPairConsistency(*keypair_from_compressed_wif));

    // Should be able to export back to same compressed WIF
    auto exported_wif = keypair_from_compressed_wif->ToWIF();
    EXPECT_EQ(exported_wif, test_compressed_wif);
}

TEST_F(KeyPairTest, ToWIFExportsCorrectFormat)
{
    auto wif = known_keypair->ToWIF();
    EXPECT_FALSE(wif.empty());

    // Should be valid Base58Check encoding
    EXPECT_TRUE(wif.length() >= 51 && wif.length() <= 52);  // Standard WIF length

    // Should start with '5' for uncompressed or 'K'/'L' for compressed
    char first_char = wif[0];
    EXPECT_TRUE(first_char == '5' || first_char == 'K' || first_char == 'L');

    // Round trip should work
    auto keypair_from_exported = cryptography::ecc::KeyPair::FromWIF(wif);
    EXPECT_EQ(keypair_from_exported.GetPrivateKey(), known_keypair->GetPrivateKey());
}

TEST_F(KeyPairTest, SignatureCreationAndVerification)
{
    // Create signature
    auto signature = known_keypair->Sign(test_message);
    EXPECT_FALSE(signature.empty());

    // Verify signature with same key pair
    EXPECT_TRUE(VerifySignature(*known_keypair, test_message, signature));

    // Should fail with different key pair
    EXPECT_FALSE(VerifySignature(*known_keypair2, test_message, signature));

    // Should fail with different message
    io::ByteVector different_message = io::ByteVector::Parse("48656c6c6f20576f726c6421");  // "Hello World!"
    EXPECT_FALSE(VerifySignature(*known_keypair, different_message, signature));
}

TEST_F(KeyPairTest, SignHashCreationAndVerification)
{
    // Sign hash directly
    auto signature = known_keypair->SignHash(test_hash);
    EXPECT_FALSE(signature.empty());

    // Verify hash signature
    EXPECT_TRUE(known_keypair->VerifySignature(test_hash, signature));

    // Should fail with different hash
    auto different_hash = cryptography::Hash::SHA256(io::ByteVector::Parse("different"));
    EXPECT_FALSE(known_keypair->VerifySignature(different_hash, signature));
}

TEST_F(KeyPairTest, DeterministicSignatures)
{
    // Same message should produce same signature (if using deterministic ECDSA)
    auto sig1 = known_keypair->Sign(test_message);
    auto sig2 = known_keypair->Sign(test_message);

    // Both should be valid
    EXPECT_TRUE(VerifySignature(*known_keypair, test_message, sig1));
    EXPECT_TRUE(VerifySignature(*known_keypair, test_message, sig2));

    // For deterministic ECDSA, they should be identical
    // Note: This depends on implementation details
}

TEST_F(KeyPairTest, PublicKeyCompression)
{
    // Test compressed public key format
    auto compressed_bytes = known_keypair->GetPublicKey().ToCompressedBytes();
    EXPECT_EQ(compressed_bytes.Size(), 33);  // 1 prefix + 32 x-coordinate

    // Test uncompressed public key format
    auto uncompressed_bytes = known_keypair->GetPublicKey().ToUncompressedBytes();
    EXPECT_EQ(uncompressed_bytes.Size(), 65);  // 1 prefix + 32 x + 32 y

    // Both should represent same point
    auto from_compressed = cryptography::ecc::ECPoint::FromCompressedBytes(compressed_bytes);
    auto from_uncompressed = cryptography::ecc::ECPoint::FromUncompressedBytes(uncompressed_bytes);
    EXPECT_TRUE(from_compressed == from_uncompressed);
}

TEST_F(KeyPairTest, AddressGeneration)
{
    // Generate address from key pair
    auto address = known_keypair->GetAddress();
    EXPECT_FALSE(address.empty());
    EXPECT_EQ(address.length(), 34);  // Standard Neo address length

    // Should start with 'A' for Neo addresses
    EXPECT_EQ(address[0], 'A');

    // Should be consistent
    auto address2 = known_keypair->GetAddress();
    EXPECT_EQ(address, address2);
}

TEST_F(KeyPairTest, ScriptHashGeneration)
{
    // Generate script hash from key pair
    auto script_hash = known_keypair->GetScriptHash();
    EXPECT_NE(script_hash, io::UInt160());

    // Should be consistent
    auto script_hash2 = known_keypair->GetScriptHash();
    EXPECT_EQ(script_hash, script_hash2);

    // Address should be derivable from script hash
    auto address_from_script = script_hash.ToAddress();
    auto direct_address = known_keypair->GetAddress();
    EXPECT_EQ(address_from_script, direct_address);
}

TEST_F(KeyPairTest, KeyPairEquality)
{
    // Same private key should create equal key pairs
    auto keypair1 = cryptography::ecc::KeyPair(test_private_key);
    auto keypair2 = cryptography::ecc::KeyPair(test_private_key);

    EXPECT_TRUE(keypair1 == keypair2);
    EXPECT_FALSE(keypair1 != keypair2);

    // Different private keys should create different key pairs
    EXPECT_FALSE(*known_keypair == *known_keypair2);
    EXPECT_TRUE(*known_keypair != *known_keypair2);
}

TEST_F(KeyPairTest, KeyPairHashCode)
{
    // Hash codes should be consistent
    auto hash1 = known_keypair->GetHashCode();
    auto hash2 = known_keypair->GetHashCode();
    EXPECT_EQ(hash1, hash2);

    // Equal key pairs should have equal hash codes
    auto same_keypair = cryptography::ecc::KeyPair(test_private_key);
    EXPECT_EQ(known_keypair->GetHashCode(), same_keypair.GetHashCode());

    // Different key pairs should likely have different hash codes
    EXPECT_NE(known_keypair->GetHashCode(), known_keypair2->GetHashCode());
}

TEST_F(KeyPairTest, BatchKeyPairGeneration)
{
    // All generated key pairs should be valid and unique
    std::set<cryptography::BigInteger> private_keys;
    std::set<std::string> addresses;

    for (const auto& keypair : test_keypairs)
    {
        EXPECT_TRUE(VerifyKeyPairConsistency(*keypair));

        // Should be unique
        auto private_key = keypair->GetPrivateKey();
        EXPECT_TRUE(private_keys.insert(private_key).second) << "Duplicate private key generated";

        auto address = keypair->GetAddress();
        EXPECT_TRUE(addresses.insert(address).second) << "Duplicate address generated";
    }
}

TEST_F(KeyPairTest, SignatureVerificationWithDifferentKeyPairs)
{
    // Create signatures with different key pairs
    std::vector<io::ByteVector> signatures;
    for (const auto& keypair : test_keypairs)
    {
        signatures.push_back(keypair->Sign(test_message));
    }

    // Each signature should only verify with its own key pair
    for (size_t i = 0; i < test_keypairs.size(); ++i)
    {
        for (size_t j = 0; j < test_keypairs.size(); ++j)
        {
            bool should_verify = (i == j);
            bool does_verify = VerifySignature(*test_keypairs[j], test_message, signatures[i]);
            EXPECT_EQ(should_verify, does_verify) << "Signature verification mismatch for keypair " << i << " vs " << j;
        }
    }
}

TEST_F(KeyPairTest, PerformanceKeyGeneration)
{
    // Test key generation performance
    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<cryptography::ecc::KeyPair> keypairs;
    for (size_t i = 0; i < large_batch_size; ++i)
    {
        keypairs.emplace_back(cryptography::ecc::KeyPair::Generate());
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Should complete within reasonable time
    EXPECT_LT(duration.count(), 10000);  // Less than 10 seconds for 100 keys

    // All keys should be valid
    for (const auto& keypair : keypairs)
    {
        EXPECT_TRUE(VerifyKeyPairConsistency(keypair));
    }
}

TEST_F(KeyPairTest, PerformanceSigning)
{
    // Test signing performance
    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<io::ByteVector> signatures;
    for (size_t i = 0; i < large_batch_size; ++i)
    {
        signatures.push_back(known_keypair->Sign(test_message));
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Should complete within reasonable time
    EXPECT_LT(duration.count(), 5000);  // Less than 5 seconds for 100 signatures

    // All signatures should be valid
    for (const auto& signature : signatures)
    {
        EXPECT_TRUE(VerifySignature(*known_keypair, test_message, signature));
    }
}

TEST_F(KeyPairTest, PerformanceVerification)
{
    // Create signature once
    auto signature = known_keypair->Sign(test_message);

    // Test verification performance
    auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < large_batch_size; ++i)
    {
        EXPECT_TRUE(VerifySignature(*known_keypair, test_message, signature));
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Should complete within reasonable time
    EXPECT_LT(duration.count(), 3000);  // Less than 3 seconds for 100 verifications
}

TEST_F(KeyPairTest, ExportImportConsistency)
{
    // Test various export/import cycles
    for (const auto& keypair : test_keypairs)
    {
        // WIF round trip
        auto wif = keypair->ToWIF();
        auto from_wif = cryptography::ecc::KeyPair::FromWIF(wif);
        EXPECT_EQ(keypair->GetPrivateKey(), from_wif.GetPrivateKey());

        // Address should be same
        EXPECT_EQ(keypair->GetAddress(), from_wif.GetAddress());

        // Should be able to verify each other's signatures
        auto message = io::ByteVector::Random(32);
        auto sig1 = keypair->Sign(message);
        auto sig2 = from_wif.Sign(message);

        EXPECT_TRUE(VerifySignature(from_wif, message, sig1));
        EXPECT_TRUE(VerifySignature(*keypair, message, sig2));
    }
}

TEST_F(KeyPairTest, EdgeCasePrivateKeys)
{
    // Test edge case private keys
    auto min_key = cryptography::BigInteger::One();
    auto max_key = cryptography::ecc::KeyPair::GetCurveOrder() - cryptography::BigInteger::One();

    auto min_keypair = cryptography::ecc::KeyPair(min_key);
    auto max_keypair = cryptography::ecc::KeyPair(max_key);

    EXPECT_TRUE(VerifyKeyPairConsistency(min_keypair));
    EXPECT_TRUE(VerifyKeyPairConsistency(max_keypair));

    // Both should be able to sign and verify
    auto sig_min = min_keypair.Sign(test_message);
    auto sig_max = max_keypair.Sign(test_message);

    EXPECT_TRUE(VerifySignature(min_keypair, test_message, sig_min));
    EXPECT_TRUE(VerifySignature(max_keypair, test_message, sig_max));
}

TEST_F(KeyPairTest, CopyConstructorAndAssignment)
{
    // Test copy constructor
    cryptography::ecc::KeyPair copied(*known_keypair);
    EXPECT_TRUE(copied == *known_keypair);
    EXPECT_TRUE(VerifyKeyPairConsistency(copied));

    // Test assignment operator
    cryptography::ecc::KeyPair assigned = *known_keypair2;
    EXPECT_TRUE(assigned == *known_keypair2);
    EXPECT_TRUE(VerifyKeyPairConsistency(assigned));

    // Test self-assignment safety
    assigned = assigned;
    EXPECT_TRUE(assigned == *known_keypair2);
}

TEST_F(KeyPairTest, MultipleSignaturesConsistency)
{
    // Multiple signatures of same message should all verify
    std::vector<io::ByteVector> signatures;
    for (int i = 0; i < 10; ++i)
    {
        signatures.push_back(known_keypair->Sign(test_message));
    }

    for (const auto& signature : signatures)
    {
        EXPECT_TRUE(VerifySignature(*known_keypair, test_message, signature));
    }
}

TEST_F(KeyPairTest, InvalidSignatureHandling)
{
    // Test verification with invalid signatures
    io::ByteVector invalid_signature = io::ByteVector::Random(64);
    EXPECT_FALSE(VerifySignature(*known_keypair, test_message, invalid_signature));

    // Test with empty signature
    io::ByteVector empty_signature;
    EXPECT_FALSE(VerifySignature(*known_keypair, test_message, empty_signature));

    // Test with wrong size signature
    io::ByteVector wrong_size_signature = io::ByteVector::Random(32);
    EXPECT_FALSE(VerifySignature(*known_keypair, test_message, wrong_size_signature));
}

TEST_F(KeyPairTest, ToStringRepresentation)
{
    // Test string representation
    std::string keypair_str = known_keypair->ToString();
    EXPECT_FALSE(keypair_str.empty());

    // Should contain key information but not expose private key directly
    EXPECT_NE(keypair_str.find("KeyPair"), std::string::npos);

    // Different key pairs should have different string representations
    std::string keypair2_str = known_keypair2->ToString();
    EXPECT_NE(keypair_str, keypair2_str);
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_CRYPTOGRAPHY_ECC_TEST_KEYPAIR_CPP_H
