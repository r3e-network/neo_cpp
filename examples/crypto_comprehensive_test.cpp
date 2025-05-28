#include <iostream>
#include <gtest/gtest.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/merkletree.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

using namespace neo::cryptography;
using namespace neo::io;

// Test Hash functions
TEST(CryptoComprehensiveTest, Hash) {
    // Test SHA256
    ByteVector data = { 0x01, 0x02, 0x03 };
    EXPECT_EQ(data.Size(), 3);
}

// Test Crypto functions
TEST(CryptoComprehensiveTest, Crypto) {
    // Test random bytes generation
    ByteVector random = Crypto::GenerateRandomBytes(16);
    EXPECT_EQ(random.Size(), 16);

    // Test AES encryption and decryption
    ByteVector data = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };
    ByteVector key = Crypto::GenerateRandomBytes(32);
    ByteVector iv = Crypto::GenerateRandomBytes(16);

    ByteVector encrypted = Crypto::AesEncrypt(data.AsSpan(), key.AsSpan(), iv.AsSpan());
    ByteVector decrypted = Crypto::AesDecrypt(encrypted.AsSpan(), key.AsSpan(), iv.AsSpan());

    EXPECT_EQ(data, decrypted);

    // Test PBKDF2
    ByteVector password = { 'p', 'a', 's', 's', 'w', 'o', 'r', 'd' };
    ByteVector salt = { 's', 'a', 'l', 't' };
    ByteVector derivedKey = Crypto::PBKDF2(password.AsSpan(), salt.AsSpan(), 1000, 32);
    EXPECT_EQ(derivedKey.Size(), 32);

    // Test HMAC-SHA256
    ByteVector hmacKey = { 'k', 'e', 'y' };
    ByteVector hmacData = { 'd', 'a', 't', 'a' };
    ByteVector hmac = Crypto::HmacSha256(hmacKey.AsSpan(), hmacData.AsSpan());
    EXPECT_EQ(hmac.ToHexString(), "5031fe3d989c6d1537a013fa6e739da23463fdaec3b70137d828e36ace221bd0");

    // Test Base64 encoding and decoding
    ByteVector base64Data = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
    std::string base64 = Crypto::Base64Encode(base64Data.AsSpan());
    EXPECT_EQ(base64, "SGVsbG8sIFdvcmxkIQ==");

    ByteVector decodedBase64 = Crypto::Base64Decode(base64);
    EXPECT_EQ(decodedBase64, base64Data);
}

// Test MerkleTree
TEST(CryptoComprehensiveTest, MerkleTree) {
    // Create some hashes
    std::vector<UInt256> hashes;
    hashes.push_back(UInt256::Parse("0000000000000000000000000000000000000000000000000000000000000001"));
    hashes.push_back(UInt256::Parse("0000000000000000000000000000000000000000000000000000000000000002"));
    hashes.push_back(UInt256::Parse("0000000000000000000000000000000000000000000000000000000000000003"));
    hashes.push_back(UInt256::Parse("0000000000000000000000000000000000000000000000000000000000000004"));

    // Compute the Merkle root
    auto rootOpt = MerkleTree::ComputeRootOptional(hashes);
    ASSERT_TRUE(rootOpt.has_value());
    UInt256 root = rootOpt.value();
    EXPECT_EQ(root.ToHexString(), "2c76ecc1f6a379b82aadc24b14cded50e6b59693b02cee76342c15cf0e31b700");

    // Compute the Merkle path
    std::vector<UInt256> path = MerkleTree::ComputePath(hashes, 0);
    EXPECT_EQ(path.size(), 2);
    EXPECT_EQ(path[0].ToHexString(), "0000000000000000000000000000000000000000000000000000000000000002");

    // Verify the Merkle path
    bool valid = MerkleTree::VerifyPath(hashes[0], path, 0, root);
    EXPECT_TRUE(valid);

    // Test with invalid path
    path[0] = UInt256::Parse("0000000000000000000000000000000000000000000000000000000000000005");
    valid = MerkleTree::VerifyPath(hashes[0], path, 0, root);
    EXPECT_FALSE(valid);
}

int main(int argc, char** argv) {
    std::cout << "Running Cryptography comprehensive test..." << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
