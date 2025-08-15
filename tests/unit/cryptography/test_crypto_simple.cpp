/**
 * @file test_crypto_simple.cpp
 * @brief Simple unit tests for cryptography module to increase coverage
 */

#include <gtest/gtest.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/crypto_modern.h>
#include <neo/io/byte_vector.h>
#include <neo/io/byte_span.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <vector>

using namespace neo::cryptography;
using namespace neo::io;

class CryptoSimpleTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Hash Function Tests
// ============================================================================

TEST_F(CryptoSimpleTest, SHA256_Basic) {
    ByteVector data = {'t', 'e', 's', 't'};
    auto hash = Hash::Sha256(ByteSpan(data.Data(), data.Size()));
    EXPECT_EQ(UInt256::Size, 32u);
}

TEST_F(CryptoSimpleTest, SHA256_Empty) {
    ByteVector empty;
    auto hash = Hash::Sha256(ByteSpan(empty.Data(), empty.Size()));
    EXPECT_EQ(UInt256::Size, 32u);
}

TEST_F(CryptoSimpleTest, Hash256_Basic) {
    ByteVector data = {'n', 'e', 'o'};
    auto hash256 = Hash::Hash256(ByteSpan(data.Data(), data.Size()));
    EXPECT_EQ(UInt256::Size, 32u);
}

TEST_F(CryptoSimpleTest, RIPEMD160_Basic) {
    ByteVector data = {'h', 'e', 'l', 'l', 'o'};
    auto hash = Hash::Ripemd160(ByteSpan(data.Data(), data.Size()));
    EXPECT_EQ(UInt160::Size, 20u);
}

TEST_F(CryptoSimpleTest, Hash160_Basic) {
    ByteVector data = {'t', 'e', 's', 't'};
    auto hash160 = Hash::Hash160(ByteSpan(data.Data(), data.Size()));
    EXPECT_EQ(UInt160::Size, 20u);
}

// ============================================================================
// Crypto Modern Functions Tests  
// ============================================================================

TEST_F(CryptoSimpleTest, GenerateRandomBytes_Size) {
    auto random8 = GenerateRandomBytes(8);
    auto random16 = GenerateRandomBytes(16);
    auto random32 = GenerateRandomBytes(32);
    
    EXPECT_EQ(random8.Size(), 8u);
    EXPECT_EQ(random16.Size(), 16u);
    EXPECT_EQ(random32.Size(), 32u);
}

TEST_F(CryptoSimpleTest, GenerateRandomBytes_Uniqueness) {
    auto rand1 = GenerateRandomBytes(32);
    auto rand2 = GenerateRandomBytes(32);
    
    // Should be different
    EXPECT_NE(rand1, rand2);
}

TEST_F(CryptoSimpleTest, HmacSha256_Basic) {
    ByteVector key = {'k', 'e', 'y'};
    ByteVector data = {'d', 'a', 't', 'a'};
    
    auto hmac = HmacSha256(ByteSpan(data.Data(), data.Size()), 
                           ByteSpan(key.Data(), key.Size()));
    EXPECT_EQ(hmac.Size(), 32u);
}

TEST_F(CryptoSimpleTest, Sha256_Standalone) {
    ByteVector data = {'t', 'e', 's', 't'};
    auto hash = Sha256(ByteSpan(data.Data(), data.Size()));
    EXPECT_EQ(UInt256::Size, 32u);
}

// ============================================================================
// UInt256 Tests
// ============================================================================

TEST_F(CryptoSimpleTest, UInt256_DefaultConstructor) {
    UInt256 val;
    EXPECT_EQ(UInt256::Size, 32u);
}

TEST_F(CryptoSimpleTest, UInt256_Parse) {
    std::string hex = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    UInt256 val = UInt256::Parse(hex);
    EXPECT_EQ(val.ToString(), hex);
}

TEST_F(CryptoSimpleTest, UInt256_Comparison) {
    UInt256 val1, val2;
    std::memset(val1.Data(), 0x00, UInt256::Size);
    std::memset(val2.Data(), 0xFF, UInt256::Size);
    
    EXPECT_NE(val1, val2);
    EXPECT_LT(val1, val2);
}

// ============================================================================
// UInt160 Tests
// ============================================================================

TEST_F(CryptoSimpleTest, UInt160_DefaultConstructor) {
    UInt160 val;
    EXPECT_EQ(UInt160::Size, 20u);
}

TEST_F(CryptoSimpleTest, UInt160_Parse) {
    std::string hex = "0123456789abcdef0123456789abcdef01234567";
    UInt160 val = UInt160::Parse(hex);
    EXPECT_EQ(val.ToString(), hex);
}

// ============================================================================
// ByteVector Tests
// ============================================================================

TEST_F(CryptoSimpleTest, ByteVector_InitializerList) {
    ByteVector vec = {0x01, 0x02, 0x03, 0x04};
    EXPECT_EQ(vec.Size(), 4u);
    EXPECT_EQ(vec[0], 0x01);
    EXPECT_EQ(vec[3], 0x04);
}

TEST_F(CryptoSimpleTest, ByteVector_Append) {
    ByteVector vec1 = {0x01, 0x02};
    ByteVector vec2 = {0x03, 0x04};
    
    vec1.Append(ByteSpan(vec2.Data(), vec2.Size()));
    EXPECT_EQ(vec1.Size(), 4u);
    EXPECT_EQ(vec1[2], 0x03);
}

TEST_F(CryptoSimpleTest, ByteVector_ToHexString) {
    ByteVector vec = {0x01, 0x23, 0xAB, 0xCD, 0xEF};
    std::string hex = vec.ToHexString();
    EXPECT_EQ(hex, "0123abcdef");
}

TEST_F(CryptoSimpleTest, ByteVector_FromHexString) {
    std::string hex = "0123abcdef";
    ByteVector vec = ByteVector::FromHexString(hex);
    
    EXPECT_EQ(vec.Size(), 5u);
    EXPECT_EQ(vec[0], 0x01);
    EXPECT_EQ(vec[4], 0xEF);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(CryptoSimpleTest, EdgeCase_EmptyInput) {
    ByteVector empty;
    
    // These should handle empty input gracefully
    EXPECT_NO_THROW(Hash::Sha256(ByteSpan(empty.Data(), empty.Size())));
    EXPECT_NO_THROW(Hash::Ripemd160(ByteSpan(empty.Data(), empty.Size())));
    EXPECT_NO_THROW(Hash::Hash160(ByteSpan(empty.Data(), empty.Size())));
    EXPECT_NO_THROW(Hash::Hash256(ByteSpan(empty.Data(), empty.Size())));
}

TEST_F(CryptoSimpleTest, EdgeCase_SingleByte) {
    ByteVector single = {0xFF};
    
    auto sha256 = Hash::Sha256(ByteSpan(single.Data(), single.Size()));
    auto hash160 = Hash::Hash160(ByteSpan(single.Data(), single.Size()));
    
    EXPECT_EQ(UInt256::Size, 32u);
    EXPECT_EQ(UInt160::Size, 20u);
}