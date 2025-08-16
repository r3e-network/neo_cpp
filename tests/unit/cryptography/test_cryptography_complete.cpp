/**
 * @file test_cryptography_complete.cpp
 * @brief Complete cryptography test suite
 */

#include <gtest/gtest.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/base58.h>
#include <neo/cryptography/base64.h>

namespace neo::cryptography::tests {

class CryptographyCompleteTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(CryptographyCompleteTest, SHA256_BasicHash) {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    auto hash = Hash::SHA256(data);
    EXPECT_EQ(hash.size(), 32);
}

TEST_F(CryptographyCompleteTest, SHA256_EmptyInput) {
    std::vector<uint8_t> data;
    auto hash = Hash::SHA256(data);
    EXPECT_EQ(hash.size(), 32);
}

TEST_F(CryptographyCompleteTest, RIPEMD160_BasicHash) {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    auto hash = Hash::RIPEMD160(data);
    EXPECT_EQ(hash.size(), 20);
}

TEST_F(CryptographyCompleteTest, Base58_Encode) {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    auto encoded = Base58::Encode(data);
    EXPECT_FALSE(encoded.empty());
}

TEST_F(CryptographyCompleteTest, Base58_Decode) {
    std::string encoded = "Ldp";
    auto decoded = Base58::Decode(encoded);
    EXPECT_FALSE(decoded.empty());
}

TEST_F(CryptographyCompleteTest, Base58_RoundTrip) {
    std::vector<uint8_t> original = {0x01, 0x02, 0x03, 0x04};
    auto encoded = Base58::Encode(original);
    auto decoded = Base58::Decode(encoded);
    EXPECT_EQ(original, decoded);
}

TEST_F(CryptographyCompleteTest, Base64_Encode) {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    auto encoded = Base64::Encode(data);
    EXPECT_FALSE(encoded.empty());
}

TEST_F(CryptographyCompleteTest, Base64_Decode) {
    std::string encoded = "AQID";
    auto decoded = Base64::Decode(encoded);
    EXPECT_FALSE(decoded.empty());
}

TEST_F(CryptographyCompleteTest, Hash256_DoubleHash) {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    auto hash = Hash::Hash256(data);
    EXPECT_EQ(hash.size(), 32);
}

TEST_F(CryptographyCompleteTest, Hash160_CompositeHash) {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    auto hash = Hash::Hash160(data);
    EXPECT_EQ(hash.size(), 20);
}

// Additional comprehensive tests
TEST_F(CryptographyCompleteTest, LargeDataHash) {
    std::vector<uint8_t> data(1024 * 1024, 0xFF); // 1MB of data
    auto hash = Hash::SHA256(data);
    EXPECT_EQ(hash.size(), 32);
}

TEST_F(CryptographyCompleteTest, RandomDataValidation) {
    for (int i = 0; i < 10; ++i) {
        std::vector<uint8_t> data(32);
        for (auto& byte : data) {
            byte = rand() % 256;
        }
        auto hash = Hash::SHA256(data);
        EXPECT_EQ(hash.size(), 32);
    }
}

} // namespace neo::cryptography::tests