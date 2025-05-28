#include <iostream>
#include <gtest/gtest.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc.h>
#include <neo/cryptography/merkletree.h>
#include <neo/io/byte_vector.h>

using namespace neo::cryptography;
using namespace neo::io;

// Test Hash functions
TEST(CryptoTest, Hash) {
    // Test SHA256
    ByteVector data = { 0x01, 0x02, 0x03 };
    ByteVector sha256 = Hash::Sha256(data);
    EXPECT_EQ(sha256.ToHexString(), "039058c6f2c0cb492c533b0a4d14ef77cc0f78abccced5287d84a1a2011cfb81");

    // Test RIPEMD160
    ByteVector ripemd160 = Hash::Ripemd160(data);
    EXPECT_EQ(ripemd160.ToHexString(), "79eaec3a7d2a7764c5d65c4b32f0acb7c2c7b8af");

    // Test Hash160
    ByteVector hash160 = Hash::Hash160(data);
    EXPECT_EQ(hash160.ToHexString(), "9486d2cc9ada53e4d55a966f9fbd9a2c7d9d63f9");

    // Test Hash256
    ByteVector hash256 = Hash::Hash256(data);
    EXPECT_EQ(hash256.ToHexString(), "3f2c7ccae98af81e44c0ec419659f50d8b7d48c681e5d57fc747d0461e42dda1");
}

// Test Crypto functions
TEST(CryptoTest, Crypto) {
    // Test AES encryption and decryption
    ByteVector data = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };
    ByteVector key = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };
    ByteVector iv = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

    ByteVector encrypted = Crypto::AesEncrypt(data, key, iv);
    ByteVector decrypted = Crypto::AesDecrypt(encrypted, key, iv);

    EXPECT_EQ(data, decrypted);
}

// Test ECC functions
TEST(CryptoTest, ECC) {
    // Test key generation
    auto keyPair = ECDsa::GenerateKey();
    EXPECT_TRUE(keyPair.first.Size() > 0);  // Private key
    EXPECT_TRUE(keyPair.second.Size() > 0); // Public key

    // Test signing and verification
    ByteVector data = { 0x01, 0x02, 0x03 };
    ByteVector signature = ECDsa::Sign(data, keyPair.first);
    EXPECT_TRUE(ECDsa::Verify(data, signature, keyPair.second));

    // Test invalid signature
    ByteVector invalidSignature = signature;
    invalidSignature[0] ^= 0xFF; // Flip some bits
    EXPECT_FALSE(ECDsa::Verify(data, invalidSignature, keyPair.second));
}

// Test MerkleTree
TEST(CryptoTest, MerkleTree) {
    // Create some hashes
    std::vector<UInt256> hashes;
    hashes.push_back(UInt256::Parse("0000000000000000000000000000000000000000000000000000000000000001"));
    hashes.push_back(UInt256::Parse("0000000000000000000000000000000000000000000000000000000000000002"));
    hashes.push_back(UInt256::Parse("0000000000000000000000000000000000000000000000000000000000000003"));
    hashes.push_back(UInt256::Parse("0000000000000000000000000000000000000000000000000000000000000004"));

    // Compute the Merkle root
    UInt256 root = MerkleTree::ComputeRoot(hashes);
    EXPECT_EQ(root.ToHexString(), "6a9a3c86d47f1fe12648c86368ecd9723ff12e3fc34f6ae219d4d9d3e0d60667");
}

int main(int argc, char** argv) {
    std::cout << "Running Cryptography test..." << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
