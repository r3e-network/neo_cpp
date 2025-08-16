#include <gtest/gtest.h>
#include <neo/cryptography/crypto.h>
#include <neo/wallets/key_pair.h>
#include <neo/io/byte_vector.h>
#include <memory>

using namespace neo::cryptography;
using namespace neo::wallets;
using namespace neo::io;

class CryptoExtendedTest : public ::testing::Test
{
protected:
    std::unique_ptr<KeyPair> key;
    
    void SetUp() override
    {
        // Generate test key
        auto privateKey = Crypto::GenerateRandomBytes(32);
        key = std::make_unique<KeyPair>(privateKey);
    }
};

TEST_F(CryptoExtendedTest, TestVerifySignature)
{
    ByteVector message((uint8_t*)"test message", 12);
    
    // Sign message
    auto signature = Crypto::Sign(message.AsSpan(), key->GetPrivateKey().AsSpan());
    EXPECT_FALSE(signature.IsEmpty());
    EXPECT_EQ(signature.Size(), 64);
    
    // Verify signature
    bool valid = Crypto::VerifySignature(message.AsSpan(), signature.AsSpan(), key->GetPublicKey());
    EXPECT_TRUE(valid);
    
    // Verify with wrong message should fail
    ByteVector wrongMessage((uint8_t*)"wrong message", 13);
    bool invalid = Crypto::VerifySignature(wrongMessage.AsSpan(), signature.AsSpan(), key->GetPublicKey());
    EXPECT_FALSE(invalid);
}

TEST_F(CryptoExtendedTest, TestHashFunctions)
{
    ByteVector data((uint8_t*)"test data", 9);
    
    // Test Hash256 (double SHA256)
    auto hash256 = Crypto::Hash256(data.AsSpan());
    EXPECT_EQ(hash256.size, 32);
    
    // Test Hash160 (SHA256 + RIPEMD160)
    auto hash160 = Crypto::Hash160(data.AsSpan());
    EXPECT_EQ(hash160.size, 20);
}

TEST_F(CryptoExtendedTest, TestRandomBytes)
{
    // Generate random bytes
    auto random1 = Crypto::GenerateRandomBytes(32);
    auto random2 = Crypto::GenerateRandomBytes(32);
    
    EXPECT_EQ(random1.Size(), 32);
    EXPECT_EQ(random2.Size(), 32);
    
    // Should be different
    EXPECT_NE(random1, random2);
}

TEST_F(CryptoExtendedTest, TestKeyPairGeneration)
{
    // Generate new key pair
    auto privateKey = Crypto::GenerateRandomBytes(32);
    KeyPair newKey(privateKey);
    
    EXPECT_EQ(newKey.GetPrivateKey().Size(), 32);
    EXPECT_FALSE(newKey.GetPublicKey().IsInfinity());
    
    // Test address generation
    auto address = newKey.GetAddress();
    EXPECT_FALSE(address.empty());
    EXPECT_EQ(address[0], 'N'); // Neo N3 address prefix
}

TEST_F(CryptoExtendedTest, TestSignatureRedeemScript)
{
    auto redeemScript = Crypto::CreateSignatureRedeemScript(key->GetPublicKey());
    EXPECT_FALSE(redeemScript.IsEmpty());
    EXPECT_GT(redeemScript.Size(), 0u);
}