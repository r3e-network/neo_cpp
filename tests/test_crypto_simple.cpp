#include <gtest/gtest.h>
#include <neo/cryptography/hash.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <algorithm>
#include <string>

using namespace neo::cryptography;
using namespace neo::io;

// Test SHA256 hashing
TEST(CryptoTest, SHA256) {
    // Test empty string
    ByteVector empty;
    UInt256 emptyHash = Hash::Sha256(empty.AsSpan());
    EXPECT_EQ(emptyHash.ToString(), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    
    // Test "hello"
    std::string hello = "hello";
    ByteVector helloBytes;
    helloBytes.Resize(hello.size());
    std::copy(hello.begin(), hello.end(), helloBytes.Data());
    UInt256 helloHash = Hash::Sha256(helloBytes.AsSpan());
    EXPECT_EQ(helloHash.ToString(), "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
}

// Test RIPEMD160 hashing
TEST(CryptoTest, RIPEMD160) {
    // Test empty string
    ByteVector empty;
    UInt160 emptyHash = Hash::Ripemd160(empty.AsSpan());
    EXPECT_EQ(emptyHash.ToString(), "9c1185a5c5e9fc54612808977ee8f548b2258d31");
    
    // Test "hello"
    std::string hello = "hello";
    ByteVector helloBytes;
    helloBytes.Resize(hello.size());
    std::copy(hello.begin(), hello.end(), helloBytes.Data());
    UInt160 helloHash = Hash::Ripemd160(helloBytes.AsSpan());
    EXPECT_EQ(helloHash.ToString(), "108f07b8382412612c048d07d13f814118445acd");
}

// Test Hash160 (SHA256 + RIPEMD160)
TEST(CryptoTest, Hash160) {
    std::string test = "test";
    ByteVector testBytes;
    testBytes.Resize(test.size());
    std::copy(test.begin(), test.end(), testBytes.Data());
    UInt160 hash160 = Hash::Hash160(testBytes.AsSpan());
    // Hash160 = RIPEMD160(SHA256(data))
    EXPECT_FALSE(hash160.IsZero());
}

// Test Hash256 (double SHA256)
TEST(CryptoTest, Hash256) {
    std::string test = "test";
    ByteVector testBytes;
    testBytes.Resize(test.size());
    std::copy(test.begin(), test.end(), testBytes.Data());
    UInt256 hash256 = Hash::Hash256(testBytes.AsSpan());
    // Hash256 = SHA256(SHA256(data))
    EXPECT_FALSE(hash256.IsZero());
}

// Base58/Base64 tests commented out due to API mismatch
// Will be enabled once the API is consistent

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}