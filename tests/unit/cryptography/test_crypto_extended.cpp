#include <gtest/gtest.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc/ecc_curve.h>
#include <neo/cryptography/base58.h>
#include <neo/cryptography/bloom_filter.h>
#include <neo/cryptography/merkle_tree.h>
#include <neo/cryptography/murmur32.h>
#include <neo/cryptography/ripemd160.h>
#include <neo/cryptography/scrypt.h>
#include <neo/wallets/key_pair.h>
#include <neo/io/byte_vector.h>

using namespace neo::cryptography;
using namespace neo::wallets;
using namespace neo::io;

class CryptoExtendedTest : public ::testing::Test
{
protected:
    KeyPair key;
    
    void SetUp() override
    {
        // Generate test key
        auto privateKey = ByteVector::GenerateRandom(32);
        key = KeyPair(privateKey);
    }
};

TEST_F(CryptoExtendedTest, TestVerifySignature)
{
    ByteVector message = ByteVector::FromString("HelloWorld");
    auto signature = Crypto::Sign(message.AsSpan(), key.GetPrivateKey());
    
    // Valid signature should verify
    EXPECT_TRUE(Crypto::VerifySignature(message.AsSpan(), signature, key.GetPublicKey()));
    
    // Wrong key should fail
    KeyPair wrongKey(ByteVector::GenerateRandom(32));
    EXPECT_FALSE(Crypto::VerifySignature(message.AsSpan(), signature, wrongKey.GetPublicKey()));
    
    // Modified message should fail
    ByteVector wrongMessage = ByteVector::FromString("WrongMessage");
    EXPECT_FALSE(Crypto::VerifySignature(wrongMessage.AsSpan(), signature, key.GetPublicKey()));
}

TEST_F(CryptoExtendedTest, TestSecp256k1)
{
    ByteVector privkey = ByteVector::Parse("7177f0d04c79fa0b8c91fe90c1cf1d44772d1fba6e5eb9b281a22cd3aafb51fe");
    ByteVector message = ByteVector::Parse("2d46a712699bae19a634563d74d04cc2da497b841456da270dccb75ac2f7c4e7");
    
    auto signature = Crypto::Sign(message.AsSpan(), privkey.AsSpan(), ECCurve::Secp256k1);
    
    KeyPair keyPair(privkey, ECCurve::Secp256k1);
    EXPECT_TRUE(Crypto::VerifySignature(message.AsSpan(), signature, keyPair.GetPublicKey(), ECCurve::Secp256k1));
}

TEST_F(CryptoExtendedTest, TestSecp256r1)
{
    ByteVector privkey = ByteVector::GenerateRandom(32);
    ByteVector message = ByteVector::GenerateRandom(32);
    
    auto signature = Crypto::Sign(message.AsSpan(), privkey.AsSpan(), ECCurve::Secp256r1);
    
    KeyPair keyPair(privkey, ECCurve::Secp256r1);
    EXPECT_TRUE(Crypto::VerifySignature(message.AsSpan(), signature, keyPair.GetPublicKey(), ECCurve::Secp256r1));
}

TEST_F(CryptoExtendedTest, TestSignatureRecover)
{
    ByteVector message = ByteVector::GenerateRandom(32);
    auto signature = Crypto::Sign(message.AsSpan(), key.GetPrivateKey());
    
    // Should be able to recover public key from signature
    auto recoveredKey = Crypto::RecoverPublicKey(message.AsSpan(), signature);
    EXPECT_EQ(recoveredKey, key.GetPublicKey());
}

TEST_F(CryptoExtendedTest, TestHashFunction)
{
    ByteVector data = ByteVector::FromString("test data");
    
    // Note: SHA256 and RIPEMD160 are not directly exposed, using Hash256 and Hash160 instead
    
    // Test Hash256 (double SHA256)
    auto hash256 = Crypto::Hash256(data.AsSpan());
    EXPECT_EQ(hash256.Size(), 32);
    
    // Test Hash160 (SHA256 + RIPEMD160)
    auto hash160 = Crypto::Hash160(data.AsSpan());
    EXPECT_EQ(hash160.Size(), 20);
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
    std::string encodedCheck = Base58::EncodeWithCheckSum(data.AsSpan());
    ByteVector decodedCheck = Base58::DecodeWithCheckSum(encodedCheck);
    EXPECT_EQ(decodedCheck, data);
}

TEST_F(CryptoExtendedTest, TestBloomFilter)
{
    BloomFilter filter(100, 3, ByteVector::GenerateRandom(4).ToUInt32());
    
    // Add elements
    ByteVector element1 = ByteVector::FromString("element1");
    ByteVector element2 = ByteVector::FromString("element2");
    ByteVector element3 = ByteVector::FromString("element3");
    
    filter.Add(element1.AsSpan());
    filter.Add(element2.AsSpan());
    
    // Test contains
    EXPECT_TRUE(filter.Contains(element1.AsSpan()));
    EXPECT_TRUE(filter.Contains(element2.AsSpan()));
    EXPECT_FALSE(filter.Contains(element3.AsSpan()));
}

TEST_F(CryptoExtendedTest, TestMerkleTree)
{
    std::vector<ByteVector> hashes;
    hashes.push_back(Crypto::Hash256(ByteVector::FromString("tx1").AsSpan()));
    hashes.push_back(Crypto::Hash256(ByteVector::FromString("tx2").AsSpan()));
    hashes.push_back(Crypto::Hash256(ByteVector::FromString("tx3").AsSpan()));
    hashes.push_back(Crypto::Hash256(ByteVector::FromString("tx4").AsSpan()));
    
    MerkleTree tree(hashes);
    auto root = tree.GetRoot();
    
    EXPECT_FALSE(root.IsEmpty());
    EXPECT_EQ(root.Size(), 32);
    
    // Verify proof
    auto proof = tree.GetProof(1);
    EXPECT_TRUE(tree.VerifyProof(hashes[1], proof, root));
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
    
    auto hash = RIPEMD160::Hash(data.AsSpan());
    EXPECT_EQ(hash.Size(), 20);
    
    // Known test vector
    ByteVector testData = ByteVector::FromString("abc");
    auto testHash = RIPEMD160::Hash(testData.AsSpan());
    EXPECT_EQ(testHash.ToHexString(), "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc");
}

TEST_F(CryptoExtendedTest, TestSCrypt)
{
    ByteVector password = ByteVector::FromString("password");
    ByteVector salt = ByteVector::FromString("salt");
    int N = 16384;
    int r = 8;
    int p = 1;
    int dkLen = 32;
    
    auto key = SCrypt::Generate(password.AsSpan(), salt.AsSpan(), N, r, p, dkLen);
    EXPECT_EQ(key.Size(), dkLen);
    
    // Same parameters should produce same key
    auto key2 = SCrypt::Generate(password.AsSpan(), salt.AsSpan(), N, r, p, dkLen);
    EXPECT_EQ(key, key2);
    
    // Different salt should produce different key
    ByteVector salt2 = ByteVector::FromString("salt2");
    auto key3 = SCrypt::Generate(password.AsSpan(), salt2.AsSpan(), N, r, p, dkLen);
    EXPECT_NE(key, key3);
}
