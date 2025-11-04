#include <gtest/gtest.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/base58.h>
#include <neo/cryptography/bloom_filter.h>
#include <neo/cryptography/merkle_tree.h>
#include <neo/cryptography/murmur32.h>
#include <neo/cryptography/ripemd160.h>
#include <neo/cryptography/scrypt.h>
#include <neo/wallets/key_pair.h>
#include <neo/io/byte_vector.h>

#include <memory>
#include <random>

using namespace neo::cryptography;
using namespace neo::wallets;
using namespace neo::io;

class CryptoExtendedTest : public ::testing::Test
{
protected:
    std::unique_ptr<KeyPair> key;

    void SetUp() override { key = std::make_unique<KeyPair>(ByteVector::GenerateRandom(32)); }
};

TEST_F(CryptoExtendedTest, TestVerifySignature)
{
    ByteVector message = ByteVector::FromString("HelloWorld");
    auto signature = Crypto::Sign(message.AsSpan(), key->GetPrivateKey().AsSpan());

    // Valid signature should verify
    EXPECT_TRUE(Crypto::VerifySignature(message.AsSpan(), signature.AsSpan(), key->GetPublicKey()));

    // Wrong key should fail
    KeyPair wrongKey(ByteVector::GenerateRandom(32));
    EXPECT_FALSE(Crypto::VerifySignature(message.AsSpan(), signature.AsSpan(), wrongKey.GetPublicKey()));
}

TEST_F(CryptoExtendedTest, TestHashFunction)
{
    ByteVector data = ByteVector::FromString("test data");

    // Test Hash256 (double SHA256)
    auto hash256 = Crypto::Hash256(data.AsSpan());
    EXPECT_EQ(hash256.AsSpan().Size(), UInt256::Size);

    // Test Hash160 (SHA256 + RIPEMD160)
    auto hash160 = Crypto::Hash160(data.AsSpan());
    EXPECT_EQ(hash160.AsSpan().Size(), UInt160::SIZE);
}

TEST_F(CryptoExtendedTest, TestBase58Encoding)
{
    ByteVector data = ByteVector::Parse("00112233445566778899aabbccddeeff");

    // Encode to Base58
    std::string encoded = Base58::Encode(data.AsSpan());
    EXPECT_FALSE(encoded.empty());

    // Decode from Base58
    ByteVector decoded = Base58::Decode(encoded);
    EXPECT_EQ(decoded, data);

    // Test with check
    std::string encodedCheck = Base58::EncodeCheck(data.AsSpan());
    ByteVector decodedCheck = Base58::DecodeCheckToByteVector(encodedCheck);
    EXPECT_EQ(decodedCheck, data);
}

TEST_F(CryptoExtendedTest, TestBloomFilter)
{
    BloomFilter filter(100, 0.01);

    // Add elements
    ByteVector element1 = ByteVector::FromString("element1");
    ByteVector element2 = ByteVector::FromString("element2");
    ByteVector element3 = ByteVector::FromString("element3");

    filter.Add(element1);
    filter.Add(element2);

    // Test contains
    EXPECT_TRUE(filter.Contains(element1));
    EXPECT_TRUE(filter.Contains(element2));
    EXPECT_FALSE(filter.Contains(element3));
}

TEST_F(CryptoExtendedTest, TestMerkleTree)
{
    std::vector<UInt256> hashes;
    hashes.push_back(Crypto::Hash256(ByteVector::FromString("tx1").AsSpan()));
    hashes.push_back(Crypto::Hash256(ByteVector::FromString("tx2").AsSpan()));
    hashes.push_back(Crypto::Hash256(ByteVector::FromString("tx3").AsSpan()));
    hashes.push_back(Crypto::Hash256(ByteVector::FromString("tx4").AsSpan()));

    auto root = MerkleTree::ComputeRoot(hashes);
    EXPECT_EQ(root.AsSpan().Size(), UInt256::Size);

    auto proof = MerkleTree::GetProof(hashes, 1);
    EXPECT_TRUE(MerkleTree::VerifyPath(hashes[1], proof, 1, root));
}

TEST_F(CryptoExtendedTest, TestMurmur32)
{
    ByteVector data = ByteVector::FromString("test data");
    uint32_t seed = 0x12345678;

    uint32_t hash = Murmur32::Hash(data.AsSpan(), seed);
    EXPECT_NE(hash, 0);

    // Same data and seed should produce same hash
    uint32_t hash2 = Murmur32::Hash(data.AsSpan(), seed);
    EXPECT_EQ(hash, hash2);

    // Different seed should produce different hash
    uint32_t hash3 = Murmur32::Hash(data.AsSpan(), seed + 1);
    EXPECT_NE(hash, hash3);
}

TEST_F(CryptoExtendedTest, TestRIPEMD160)
{
    ByteVector data = ByteVector::FromString("test data");

    auto hash = Ripemd160::Hash(data.AsSpan());
    EXPECT_EQ(hash.AsSpan().Size(), UInt160::Size);

    // Known test vector
    ByteVector testData = ByteVector::FromString("abc");
    auto testHash = Ripemd160::Hash(testData.AsSpan());
    EXPECT_EQ(testHash.ToHexString(), "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc");
}

TEST_F(CryptoExtendedTest, TestSCrypt)
{
    ByteVector password = ByteVector::FromString("password");
    ByteVector salt = ByteVector::FromString("salt");
    const uint32_t N = 16384;
    const uint32_t r = 8;
    const uint32_t p = 1;
    const uint32_t dkLen = 32;

    auto derivedKey = Scrypt::DeriveKey(password.GetVector(), salt.GetVector(), N, r, p, dkLen);
    EXPECT_EQ(derivedKey.size(), static_cast<size_t>(dkLen));

    // Same parameters should produce same key
    auto derivedKey2 = Scrypt::DeriveKey(password.GetVector(), salt.GetVector(), N, r, p, dkLen);
    EXPECT_EQ(derivedKey, derivedKey2);

    // Different salt should produce different key
    ByteVector salt2 = ByteVector::FromString("salt2");
    auto derivedKey3 = Scrypt::DeriveKey(password.GetVector(), salt2.GetVector(), N, r, p, dkLen);
    EXPECT_NE(derivedKey, derivedKey3);
}
