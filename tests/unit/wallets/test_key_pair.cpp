#include <gtest/gtest.h>
#include <neo/wallets/key_pair.h>

namespace neo::wallets::tests
{
    class KeyPairTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            test_private_key = {
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
                0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
            };
        }

        std::vector<uint8_t> test_private_key;
    };

    TEST_F(KeyPairTest, TestConstructorFromSpan)
    {
        KeyPair key_pair(std::span<const uint8_t>(test_private_key));
        
        EXPECT_TRUE(key_pair.IsValid());
        EXPECT_EQ(test_private_key, key_pair.GetPrivateKey());
    }

    TEST_F(KeyPairTest, TestConstructorFromVector)
    {
        KeyPair key_pair(test_private_key);
        
        EXPECT_TRUE(key_pair.IsValid());
        EXPECT_EQ(test_private_key, key_pair.GetPrivateKey());
    }

    TEST_F(KeyPairTest, TestConstructorInvalidKey)
    {
        // Test with wrong size
        std::vector<uint8_t> wrong_size(16, 0x01);
        EXPECT_THROW(KeyPair(wrong_size), std::invalid_argument);
        
        // Test with all zeros
        std::vector<uint8_t> all_zeros(32, 0x00);
        EXPECT_THROW(KeyPair(all_zeros), std::invalid_argument);
    }

    TEST_F(KeyPairTest, TestCopyConstructor)
    {
        KeyPair original(test_private_key);
        KeyPair copy(original);
        
        EXPECT_EQ(original.GetPrivateKey(), copy.GetPrivateKey());
        EXPECT_EQ(original.GetPublicKey(), copy.GetPublicKey());
        EXPECT_EQ(original.GetScriptHash(), copy.GetScriptHash());
    }

    TEST_F(KeyPairTest, TestMoveConstructor)
    {
        KeyPair original(test_private_key);
        auto original_private_key = original.GetPrivateKey();
        auto original_public_key = original.GetPublicKey();
        
        KeyPair moved(std::move(original));
        
        EXPECT_EQ(original_private_key, moved.GetPrivateKey());
        EXPECT_EQ(original_public_key, moved.GetPublicKey());
    }

    TEST_F(KeyPairTest, TestCopyAssignment)
    {
        KeyPair original(test_private_key);
        KeyPair copy = original;
        
        EXPECT_EQ(original.GetPrivateKey(), copy.GetPrivateKey());
        EXPECT_EQ(original.GetPublicKey(), copy.GetPublicKey());
        EXPECT_EQ(original.GetScriptHash(), copy.GetScriptHash());
    }

    TEST_F(KeyPairTest, TestMoveAssignment)
    {
        KeyPair original(test_private_key);
        auto original_private_key = original.GetPrivateKey();
        auto original_public_key = original.GetPublicKey();
        
        KeyPair moved;
        moved = std::move(original);
        
        EXPECT_EQ(original_private_key, moved.GetPrivateKey());
        EXPECT_EQ(original_public_key, moved.GetPublicKey());
    }

    TEST_F(KeyPairTest, TestGetPublicKey)
    {
        KeyPair key_pair(test_private_key);
        auto public_key = key_pair.GetPublicKey();
        
        EXPECT_FALSE(public_key.IsInfinity());
        EXPECT_TRUE(public_key.IsValid());
    }

    TEST_F(KeyPairTest, TestGetScriptHash)
    {
        KeyPair key_pair(test_private_key);
        auto script_hash = key_pair.GetScriptHash();
        
        EXPECT_FALSE(script_hash.IsZero());
    }

    TEST_F(KeyPairTest, TestGetAddress)
    {
        KeyPair key_pair(test_private_key);
        auto address = key_pair.GetAddress();
        
        EXPECT_FALSE(address.empty());
        EXPECT_GT(address.length(), 20); // Should be reasonable length
    }

    TEST_F(KeyPairTest, TestGetAddressWithVersion)
    {
        KeyPair key_pair(test_private_key);
        auto address1 = key_pair.GetAddress(0x35);
        auto address2 = key_pair.GetAddress(0x17);
        
        EXPECT_NE(address1, address2); // Should be different
    }

    TEST_F(KeyPairTest, TestSignAndVerify)
    {
        KeyPair key_pair(test_private_key);
        std::vector<uint8_t> message = {0x01, 0x02, 0x03, 0x04, 0x05};
        
        auto signature = key_pair.Sign(message);
        EXPECT_FALSE(signature.empty());
        
        bool valid = key_pair.VerifySignature(message, signature);
        EXPECT_TRUE(valid);
        
        // Test with wrong message
        std::vector<uint8_t> wrong_message = {0x06, 0x07, 0x08, 0x09, 0x0a};
        bool invalid = key_pair.VerifySignature(wrong_message, signature);
        EXPECT_FALSE(invalid);
    }

    TEST_F(KeyPairTest, TestToWIFAndFromWIF)
    {
        KeyPair original(test_private_key);
        auto wif = original.ToWIF();
        
        EXPECT_FALSE(wif.empty());
        EXPECT_GT(wif.length(), 40); // Should be reasonable length
        
        auto restored = KeyPair::FromWIF(wif);
        EXPECT_EQ(original.GetPrivateKey(), restored.GetPrivateKey());
        EXPECT_EQ(original.GetPublicKey(), restored.GetPublicKey());
        EXPECT_EQ(original.GetScriptHash(), restored.GetScriptHash());
    }

    TEST_F(KeyPairTest, TestFromWIFInvalid)
    {
        EXPECT_THROW(KeyPair::FromWIF("invalid"), std::invalid_argument);
        EXPECT_THROW(KeyPair::FromWIF(""), std::invalid_argument);
        EXPECT_THROW(KeyPair::FromWIF("123456789"), std::invalid_argument);
    }

    TEST_F(KeyPairTest, TestGenerate)
    {
        auto key_pair1 = KeyPair::Generate();
        auto key_pair2 = KeyPair::Generate();
        
        EXPECT_TRUE(key_pair1.IsValid());
        EXPECT_TRUE(key_pair2.IsValid());
        EXPECT_NE(key_pair1.GetPrivateKey(), key_pair2.GetPrivateKey());
        EXPECT_NE(key_pair1.GetPublicKey(), key_pair2.GetPublicKey());
    }

    TEST_F(KeyPairTest, TestFromHexAndToHex)
    {
        std::string hex = "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20";
        auto key_pair = KeyPair::FromHex(hex);
        
        EXPECT_EQ(test_private_key, key_pair.GetPrivateKey());
        EXPECT_EQ(hex, key_pair.ToHex());
    }

    TEST_F(KeyPairTest, TestFromHexInvalid)
    {
        EXPECT_THROW(KeyPair::FromHex("invalid"), std::invalid_argument);
        EXPECT_THROW(KeyPair::FromHex("123"), std::invalid_argument); // Wrong length
        EXPECT_THROW(KeyPair::FromHex("0000000000000000000000000000000000000000000000000000000000000000"), std::invalid_argument); // All zeros
    }

    TEST_F(KeyPairTest, TestIsValid)
    {
        KeyPair key_pair(test_private_key);
        EXPECT_TRUE(key_pair.IsValid());
    }

    TEST_F(KeyPairTest, TestEqualityOperators)
    {
        KeyPair key_pair1(test_private_key);
        KeyPair key_pair2(test_private_key);
        KeyPair key_pair3 = KeyPair::Generate();
        
        EXPECT_EQ(key_pair1, key_pair2);
        EXPECT_NE(key_pair1, key_pair3);
        EXPECT_NE(key_pair2, key_pair3);
    }

    TEST_F(KeyPairTest, TestConsistentResults)
    {
        KeyPair key_pair(test_private_key);
        
        // Multiple calls should return same results
        auto public_key1 = key_pair.GetPublicKey();
        auto public_key2 = key_pair.GetPublicKey();
        EXPECT_EQ(public_key1, public_key2);
        
        auto script_hash1 = key_pair.GetScriptHash();
        auto script_hash2 = key_pair.GetScriptHash();
        EXPECT_EQ(script_hash1, script_hash2);
        
        auto address1 = key_pair.GetAddress();
        auto address2 = key_pair.GetAddress();
        EXPECT_EQ(address1, address2);
    }

    TEST_F(KeyPairTest, TestSignatureDeterministic)
    {
        KeyPair key_pair(test_private_key);
        std::vector<uint8_t> message = {0x01, 0x02, 0x03, 0x04, 0x05};
        
        // Note: ECDSA signatures are typically not deterministic due to random k
        // This test just ensures signing works consistently
        auto signature1 = key_pair.Sign(message);
        auto signature2 = key_pair.Sign(message);
        
        EXPECT_FALSE(signature1.empty());
        EXPECT_FALSE(signature2.empty());
        
        // Both signatures should be valid
        EXPECT_TRUE(key_pair.VerifySignature(message, signature1));
        EXPECT_TRUE(key_pair.VerifySignature(message, signature2));
    }

    TEST_F(KeyPairTest, TestLargeMessage)
    {
        KeyPair key_pair(test_private_key);
        std::vector<uint8_t> large_message(10000);
        std::iota(large_message.begin(), large_message.end(), 0);
        
        auto signature = key_pair.Sign(large_message);
        EXPECT_FALSE(signature.empty());
        
        bool valid = key_pair.VerifySignature(large_message, signature);
        EXPECT_TRUE(valid);
    }

    TEST_F(KeyPairTest, TestEmptyMessage)
    {
        KeyPair key_pair(test_private_key);
        std::vector<uint8_t> empty_message;
        
        auto signature = key_pair.Sign(empty_message);
        EXPECT_FALSE(signature.empty());
        
        bool valid = key_pair.VerifySignature(empty_message, signature);
        EXPECT_TRUE(valid);
    }
}
