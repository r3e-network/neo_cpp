#include <gtest/gtest.h>
#include <neo/wallets/helper.h>
#include <neo/cryptography/ecc/ec_point.h>

namespace neo::wallets::tests
{
    class HelperTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // Test data
            test_script_hash = io::UInt160::Parse("0x1234567890123456789012345678901234567890");
            test_private_key = {
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
                0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
            };
        }

        io::UInt160 test_script_hash;
        std::vector<uint8_t> test_private_key;
    };

    TEST_F(HelperTest, TestToAddressAndBack)
    {
        // Convert script hash to address
        auto address = Helper::ToAddress(test_script_hash);
        EXPECT_FALSE(address.empty());
        
        // Convert address back to script hash
        auto converted_hash = Helper::ToScriptHash(address);
        EXPECT_EQ(test_script_hash, converted_hash);
    }

    TEST_F(HelperTest, TestIsValidAddress)
    {
        auto address = Helper::ToAddress(test_script_hash);
        EXPECT_TRUE(Helper::IsValidAddress(address));
        
        // Test invalid addresses
        EXPECT_FALSE(Helper::IsValidAddress(""));
        EXPECT_FALSE(Helper::IsValidAddress("invalid"));
        EXPECT_FALSE(Helper::IsValidAddress("1234567890"));
    }

    TEST_F(HelperTest, TestCreateSignatureScript)
    {
        auto public_key = Helper::GetPublicKey(test_private_key);
        auto script = Helper::CreateSignatureScript(public_key);
        
        EXPECT_FALSE(script.empty());
        EXPECT_GT(script.size(), 30); // Should be reasonable size
    }

    TEST_F(HelperTest, TestCreateMultiSigScript)
    {
        // Create multiple public keys
        std::vector<cryptography::ecc::ECPoint> public_keys;
        for (int i = 0; i < 3; ++i)
        {
            auto private_key = test_private_key;
            private_key[31] += i; // Modify last byte to create different keys
            public_keys.push_back(Helper::GetPublicKey(private_key));
        }
        
        auto script = Helper::CreateMultiSigScript(2, public_keys);
        EXPECT_FALSE(script.empty());
        EXPECT_GT(script.size(), 100); // Should be reasonable size for 3 keys
    }

    TEST_F(HelperTest, TestCreateMultiSigScriptInvalidParams)
    {
        std::vector<cryptography::ecc::ECPoint> public_keys;
        public_keys.push_back(Helper::GetPublicKey(test_private_key));
        
        // m = 0 should fail
        EXPECT_THROW(Helper::CreateMultiSigScript(0, public_keys), std::invalid_argument);
        
        // m > n should fail
        EXPECT_THROW(Helper::CreateMultiSigScript(2, public_keys), std::invalid_argument);
        
        // Empty public keys should fail
        std::vector<cryptography::ecc::ECPoint> empty_keys;
        EXPECT_THROW(Helper::CreateMultiSigScript(1, empty_keys), std::invalid_argument);
    }

    TEST_F(HelperTest, TestToScriptHashFromScript)
    {
        auto public_key = Helper::GetPublicKey(test_private_key);
        auto script = Helper::CreateSignatureScript(public_key);
        auto script_hash = Helper::ToScriptHash(script);
        
        EXPECT_FALSE(script_hash.IsZero());
    }

    TEST_F(HelperTest, TestSignAndVerify)
    {
        std::vector<uint8_t> message = {0x01, 0x02, 0x03, 0x04, 0x05};
        auto public_key = Helper::GetPublicKey(test_private_key);
        
        auto signature = Helper::Sign(message, test_private_key);
        EXPECT_FALSE(signature.empty());
        
        bool valid = Helper::VerifySignature(message, signature, public_key);
        EXPECT_TRUE(valid);
        
        // Test with wrong message
        std::vector<uint8_t> wrong_message = {0x06, 0x07, 0x08, 0x09, 0x0a};
        bool invalid = Helper::VerifySignature(wrong_message, signature, public_key);
        EXPECT_FALSE(invalid);
    }

    TEST_F(HelperTest, TestGeneratePrivateKey)
    {
        auto private_key1 = Helper::GeneratePrivateKey();
        auto private_key2 = Helper::GeneratePrivateKey();
        
        EXPECT_EQ(32, private_key1.size());
        EXPECT_EQ(32, private_key2.size());
        EXPECT_NE(private_key1, private_key2); // Should be different
    }

    TEST_F(HelperTest, TestGetPublicKey)
    {
        auto public_key = Helper::GetPublicKey(test_private_key);
        EXPECT_FALSE(public_key.IsInfinity());
        EXPECT_TRUE(public_key.IsValid());
    }

    TEST_F(HelperTest, TestGetScriptHash)
    {
        auto public_key = Helper::GetPublicKey(test_private_key);
        auto script_hash = Helper::GetScriptHash(public_key);
        
        EXPECT_FALSE(script_hash.IsZero());
    }

    TEST_F(HelperTest, TestToHexString)
    {
        std::vector<uint8_t> data = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
        
        auto hex = Helper::ToHexString(data);
        EXPECT_EQ("0123456789abcdef", hex);
        
        auto hex_reversed = Helper::ToHexString(data, true);
        EXPECT_EQ("efcdab8967452301", hex_reversed);
    }

    TEST_F(HelperTest, TestFromHexString)
    {
        std::string hex = "0123456789abcdef";
        auto data = Helper::FromHexString(hex);
        
        std::vector<uint8_t> expected = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
        EXPECT_EQ(expected, data);
        
        auto data_reversed = Helper::FromHexString(hex, true);
        std::vector<uint8_t> expected_reversed = {0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01};
        EXPECT_EQ(expected_reversed, data_reversed);
    }

    TEST_F(HelperTest, TestFromHexStringInvalid)
    {
        EXPECT_THROW(Helper::FromHexString("123"), std::invalid_argument); // Odd length
        EXPECT_THROW(Helper::FromHexString("12zz"), std::invalid_argument); // Invalid hex
    }

    TEST_F(HelperTest, TestCalculateChecksum)
    {
        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
        auto checksum = Helper::CalculateChecksum(data);
        
        EXPECT_EQ(4, checksum.size());
        EXPECT_FALSE(std::all_of(checksum.begin(), checksum.end(), [](uint8_t b) { return b == 0; }));
    }

    TEST_F(HelperTest, TestBase58EncodeAndDecode)
    {
        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
        
        auto encoded = Helper::Base58Encode(data);
        EXPECT_FALSE(encoded.empty());
        
        auto decoded = Helper::Base58Decode(encoded);
        EXPECT_EQ(data, decoded);
    }

    TEST_F(HelperTest, TestBase58CheckEncodeAndDecode)
    {
        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
        
        auto encoded = Helper::Base58CheckEncode(data);
        EXPECT_FALSE(encoded.empty());
        
        auto decoded = Helper::Base58CheckDecode(encoded);
        EXPECT_EQ(data, decoded);
    }

    TEST_F(HelperTest, TestBase58CheckDecodeInvalid)
    {
        // Test with invalid checksum
        EXPECT_THROW(Helper::Base58CheckDecode("invalid"), std::invalid_argument);
        
        // Test with too short data
        EXPECT_THROW(Helper::Base58CheckDecode("123"), std::invalid_argument);
    }

    TEST_F(HelperTest, TestAddressVersions)
    {
        uint8_t custom_version = 0x17; // Different version
        
        auto address1 = Helper::ToAddress(test_script_hash, 0x35); // Default
        auto address2 = Helper::ToAddress(test_script_hash, custom_version);
        
        EXPECT_NE(address1, address2); // Should be different
        
        // Verify both can be converted back
        auto hash1 = Helper::ToScriptHash(address1, 0x35);
        auto hash2 = Helper::ToScriptHash(address2, custom_version);
        
        EXPECT_EQ(test_script_hash, hash1);
        EXPECT_EQ(test_script_hash, hash2);
        
        // Verify wrong version fails
        EXPECT_THROW(Helper::ToScriptHash(address1, custom_version), std::invalid_argument);
        EXPECT_THROW(Helper::ToScriptHash(address2, 0x35), std::invalid_argument);
    }

    TEST_F(HelperTest, TestEmptyData)
    {
        std::vector<uint8_t> empty_data;
        
        auto hex = Helper::ToHexString(empty_data);
        EXPECT_TRUE(hex.empty());
        
        auto encoded = Helper::Base58Encode(empty_data);
        EXPECT_TRUE(encoded.empty());
    }

    TEST_F(HelperTest, TestLargeData)
    {
        // Test with large data
        std::vector<uint8_t> large_data(1000);
        std::iota(large_data.begin(), large_data.end(), 0);
        
        auto hex = Helper::ToHexString(large_data);
        EXPECT_EQ(2000, hex.length()); // 2 chars per byte
        
        auto decoded = Helper::FromHexString(hex);
        EXPECT_EQ(large_data, decoded);
        
        auto encoded = Helper::Base58Encode(large_data);
        EXPECT_FALSE(encoded.empty());
        
        auto base58_decoded = Helper::Base58Decode(encoded);
        EXPECT_EQ(large_data, base58_decoded);
    }
}
