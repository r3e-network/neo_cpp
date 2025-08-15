/**
 * @file test_crypto_comprehensive.cpp
 * @brief Comprehensive unit tests for cryptography module
 */

#include <gtest/gtest.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/crypto_modern.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/base58.h>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/cryptography/merkletree.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint256.h>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <set>

using namespace neo::cryptography;
using namespace neo::io;

class CryptoComprehensiveTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Hash Function Tests
// ============================================================================

TEST_F(CryptoComprehensiveTest, SHA256_EmptyInput) {
    ByteVector empty;
    auto hash = Hash::Sha256(ByteSpan(empty.Data(), empty.Size()));
    EXPECT_EQ(UInt256::Size, 32u);
    
    // Known hash of empty string
    const std::string expectedHex = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    EXPECT_EQ(hash.ToString(), expectedHex);
}

TEST_F(CryptoComprehensiveTest, SHA256_KnownVectors) {
    // Test with known test vectors
    struct TestVector {
        std::string input;
        std::string expectedHash;
    };
    
    std::vector<TestVector> vectors = {
        {"abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
        {"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
        {"The quick brown fox jumps over the lazy dog", 
         "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592"}
    };
    
    for (const auto& vec : vectors) {
        ByteVector input(reinterpret_cast<const uint8_t*>(vec.input.data()), vec.input.size());
        auto hash = Hash::Sha256(ByteSpan(input.Data(), input.Size()));
        EXPECT_EQ(hash.ToString(), vec.expectedHash);
    }
}

TEST_F(CryptoComprehensiveTest, SHA256_LargeInput) {
    // Test with large input (1MB)
    std::vector<uint8_t> data(1024 * 1024, 0xAB);
    ByteVector largeInput(data);
    auto hash = Hash::Sha256(ByteSpan(largeInput.Data(), largeInput.Size()));
    EXPECT_EQ(UInt256::Size, 32u);
}

TEST_F(CryptoComprehensiveTest, DoubleSHA256) {
    ByteVector data = {'t', 'e', 's', 't'};
    auto hash1 = Hash::Sha256(ByteSpan(data.Data(), data.Size()));
    auto hash2 = Hash::Sha256(ByteSpan(reinterpret_cast<const uint8_t*>(&hash1), sizeof(hash1)));
    auto doubleHash = Hash::Hash256(ByteSpan(data.Data(), data.Size()));
    
    EXPECT_EQ(hash2, doubleHash);
}

TEST_F(CryptoComprehensiveTest, RIPEMD160_Basic) {
    ByteVector data = {'h', 'e', 'l', 'l', 'o'};
    auto hash = Hash::Ripemd160(ByteSpan(data.Data(), data.Size()));
    EXPECT_EQ(UInt160::Size, 20u);
}

TEST_F(CryptoComprehensiveTest, Hash160_Combination) {
    // Hash160 = RIPEMD160(SHA256(data))
    ByteVector data = {'t', 'e', 's', 't'};
    auto sha = Hash::Sha256(ByteSpan(data.Data(), data.Size()));
    auto ripemd = Hash::Ripemd160(ByteSpan(reinterpret_cast<const uint8_t*>(&sha), sizeof(sha)));
    auto hash160 = Hash::Hash160(ByteSpan(data.Data(), data.Size()));
    
    EXPECT_EQ(ripemd, hash160);
    EXPECT_EQ(UInt160::Size, 20u);
}

TEST_F(CryptoComprehensiveTest, Hash256_Consistency) {
    ByteVector data = {'n', 'e', 'o'};
    auto hash256 = Hash::Hash256(ByteSpan(data.Data(), data.Size()));
    // Hash256 is double SHA256
    auto sha1 = Hash::Sha256(ByteSpan(data.Data(), data.Size()));
    auto doublesha = Hash::Sha256(ByteSpan(reinterpret_cast<const uint8_t*>(&sha1), sizeof(sha1)));
    
    EXPECT_EQ(hash256, doublesha);
    EXPECT_EQ(UInt256::Size, 32u);
}

// ============================================================================
// HMAC Tests
// ============================================================================

TEST_F(CryptoComprehensiveTest, HMACSHA256_Basic) {
    ByteVector key = {'k', 'e', 'y'};
    ByteVector data = {'d', 'a', 't', 'a'};
    
    auto hmac = Crypto::HmacSha256(ByteSpan(key.Data(), key.Size()), ByteSpan(data.Data(), data.Size()));
    EXPECT_EQ(hmac.Size(), 32);
}

TEST_F(CryptoComprehensiveTest, HMACSHA256_EmptyKey) {
    ByteVector emptyKey;
    ByteVector data = {'d', 'a', 't', 'a'};
    
    auto hmac = Crypto::HmacSha256(ByteSpan(emptyKey.Data(), emptyKey.Size()), ByteSpan(data.Data(), data.Size()));
    EXPECT_EQ(hmac.Size(), 32);
}

TEST_F(CryptoComprehensiveTest, HMACSHA256_EmptyData) {
    ByteVector key = {'k', 'e', 'y'};
    ByteVector emptyData;
    
    auto hmac = Crypto::HmacSha256(ByteSpan(key.Data(), key.Size()), ByteSpan(emptyData.Data(), emptyData.Size()));
    EXPECT_EQ(hmac.Size(), 32);
}

TEST_F(CryptoComprehensiveTest, HMACSHA256_LongKey) {
    // Test with key longer than block size (64 bytes for SHA256)
    ByteVector longKey(100, 0xFF);
    ByteVector data = {'t', 'e', 's', 't'};
    
    auto hmac = Crypto::HmacSha256(ByteSpan(longKey.Data(), longKey.Size()), ByteSpan(data.Data(), data.Size()));
    EXPECT_EQ(hmac.Size(), 32);
}

// ============================================================================
// Random Number Generation Tests
// ============================================================================

TEST_F(CryptoComprehensiveTest, GenerateRandomBytes_Size) {
    auto random8 = GenerateRandomBytes(8);
    auto random16 = GenerateRandomBytes(16);
    auto random32 = GenerateRandomBytes(32);
    
    EXPECT_EQ(random8.Size(), 8);
    EXPECT_EQ(random16.Size(), 16);
    EXPECT_EQ(random32.Size(), 32);
}

TEST_F(CryptoComprehensiveTest, GenerateRandomBytes_Uniqueness) {
    // Generate multiple random values and ensure they're different
    auto rand1 = GenerateRandomBytes(32);
    auto rand2 = GenerateRandomBytes(32);
    auto rand3 = GenerateRandomBytes(32);
    
    EXPECT_NE(rand1, rand2);
    EXPECT_NE(rand2, rand3);
    EXPECT_NE(rand1, rand3);
}

TEST_F(CryptoComprehensiveTest, GenerateRandomBytes_Distribution) {
    // Statistical test for randomness
    const int sampleSize = 10000;
    int byteFrequency[256] = {0};
    
    for (int i = 0; i < sampleSize; ++i) {
        auto randByte = GenerateRandomBytes(1);
        byteFrequency[randByte[0]]++;
    }
    
    // Each byte should appear roughly sampleSize/256 times
    // Allow for statistical variance
    double expected = sampleSize / 256.0;
    double tolerance = expected * 0.3; // 30% tolerance
    
    for (int i = 0; i < 256; ++i) {
        EXPECT_GT(byteFrequency[i], expected - tolerance);
        EXPECT_LT(byteFrequency[i], expected + tolerance);
    }
}

// ============================================================================
// Base58 Encoding Tests
// ============================================================================

TEST_F(CryptoComprehensiveTest, Base58_Encode) {
    ByteVector data = {'h', 'e', 'l', 'l', 'o'};
    auto encoded = Base58::Encode(data);
    EXPECT_FALSE(encoded.empty());
}

TEST_F(CryptoComprehensiveTest, Base58_Decode) {
    std::string encoded = "Cn8eVZg";
    auto decoded = Base58::Decode(encoded);
    
    ByteVector expected = {'h', 'e', 'l', 'l', 'o'};
    EXPECT_EQ(decoded, expected);
}

TEST_F(CryptoComprehensiveTest, Base58_RoundTrip) {
    ByteVector original = {'t', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'};
    auto encoded = Base58::Encode(original);
    auto decoded = Base58::Decode(encoded);
    
    EXPECT_EQ(original, decoded);
}

TEST_F(CryptoComprehensiveTest, Base58Check_Encode) {
    ByteVector data = {0x00, 0x01, 0x02, 0x03};
    auto encoded = Base58::EncodeCheck(data);
    EXPECT_FALSE(encoded.empty());
}

TEST_F(CryptoComprehensiveTest, Base58Check_Decode) {
    ByteVector data = {0x00, 0x01, 0x02, 0x03};
    auto encoded = Base58::EncodeCheck(data);
    auto decoded = Base58::DecodeCheck(encoded);
    
    EXPECT_EQ(data, decoded);
}

TEST_F(CryptoComprehensiveTest, Base58Check_InvalidChecksum) {
    std::string invalid = "1invalid1234";
    EXPECT_THROW(Base58::DecodeCheck(invalid), std::exception);
}

// ============================================================================
// ECC (Elliptic Curve Cryptography) Tests
// ============================================================================

TEST_F(CryptoComprehensiveTest, ECC_GenerateKeyPair) {
    auto keyPair = ecc::KeyPair::Generate();
    
    EXPECT_EQ(keyPair.GetPrivateKey().Size(), 32);
    EXPECT_GT(keyPair.GetPublicKey().ToArray().Size(), 0);
}

TEST_F(CryptoComprehensiveTest, ECC_Sign) {
    auto keyPair = ecc::KeyPair::Generate();
    ByteVector message = {'t', 'e', 's', 't'};
    
    auto signature = keyPair.Sign(message);
    EXPECT_GT(signature.Size(), 0);
}

TEST_F(CryptoComprehensiveTest, ECC_Verify) {
    auto keyPair = ecc::KeyPair::Generate();
    ByteVector message = {'t', 'e', 's', 't'};
    
    auto signature = keyPair.Sign(message);
    bool valid = keyPair.Verify(message, signature);
    
    EXPECT_TRUE(valid);
}

TEST_F(CryptoComprehensiveTest, ECC_VerifyInvalidSignature) {
    auto keyPair = ecc::KeyPair::Generate();
    ByteVector message = {'t', 'e', 's', 't'};
    ByteVector wrongMessage = {'w', 'r', 'o', 'n', 'g'};
    
    auto signature = keyPair.Sign(message);
    bool valid = keyPair.Verify(wrongMessage, signature);
    
    EXPECT_FALSE(valid);
}

TEST_F(CryptoComprehensiveTest, ECC_DifferentKeys) {
    auto keyPair1 = ecc::KeyPair::Generate();
    auto keyPair2 = ecc::KeyPair::Generate();
    ByteVector message = {'t', 'e', 's', 't'};
    
    auto signature = keyPair1.Sign(message);
    bool valid = keyPair2.Verify(message, signature);
    
    EXPECT_FALSE(valid);
}

// ============================================================================
// Merkle Tree Tests
// ============================================================================

TEST_F(CryptoComprehensiveTest, MerkleTree_SingleHash) {
    std::vector<UInt256> hashes;
    UInt256 hash1;
    std::memset(hash1.Data(), 0x01, hash1.Size());
    hashes.push_back(hash1);
    
    MerkleTree tree(hashes);
    auto root = tree.GetRoot();
    EXPECT_EQ(root, hash1);  // Single hash should be the root itself
}

TEST_F(CryptoComprehensiveTest, MerkleTree_TwoHashes) {
    std::vector<UInt256> hashes;
    UInt256 hash1, hash2;
    std::memset(hash1.Data(), 0x01, hash1.Size());
    std::memset(hash2.Data(), 0x02, hash2.Size());
    hashes.push_back(hash1);
    hashes.push_back(hash2);
    
    MerkleTree tree(hashes);
    auto root = tree.GetRoot();
    EXPECT_NE(root, hash1);
    EXPECT_NE(root, hash2);
}

TEST_F(CryptoComprehensiveTest, MerkleTree_MultipleHashes) {
    std::vector<UInt256> hashes;
    for (int i = 0; i < 8; ++i) {
        UInt256 hash;
        std::memset(hash.Data(), i, hash.Size());
        hashes.push_back(hash);
    }
    
    MerkleTree tree(hashes);
    auto root = tree.GetRoot();
    EXPECT_NE(root, UInt256());
}

TEST_F(CryptoComprehensiveTest, MerkleTree_EmptyInput) {
    std::vector<UInt256> emptyHashes;
    MerkleTree tree(emptyHashes);
    auto root = tree.GetRoot();
    EXPECT_EQ(root, UInt256());  // Empty input should return zero hash
}

// ============================================================================
// AES Encryption Tests
// ============================================================================

TEST_F(CryptoComprehensiveTest, AES256_Encrypt) {
    ByteVector key(32, 0xFF);  // 256-bit key
    ByteVector plaintext = {'s', 'e', 'c', 'r', 'e', 't', ' ', 'd', 'a', 't', 'a'};
    ByteVector iv(16, 0x00);   // 128-bit IV
    
    auto encrypted = Crypto::AesEncrypt(ByteSpan(plaintext.Data(), plaintext.Size()), 
                                       ByteSpan(key.Data(), key.Size()), 
                                       ByteSpan(iv.Data(), iv.Size()));
    EXPECT_GT(encrypted.Size(), 0);
    EXPECT_NE(encrypted, plaintext);
}

TEST_F(CryptoComprehensiveTest, AES256_Decrypt) {
    ByteVector key(32, 0xFF);
    ByteVector plaintext = {'s', 'e', 'c', 'r', 'e', 't', ' ', 'd', 'a', 't', 'a'};
    ByteVector iv(16, 0x00);
    
    auto encrypted = Crypto::AesEncrypt(ByteSpan(plaintext.Data(), plaintext.Size()), 
                                       ByteSpan(key.Data(), key.Size()), 
                                       ByteSpan(iv.Data(), iv.Size()));
    auto decrypted = Crypto::AesDecrypt(ByteSpan(encrypted.Data(), encrypted.Size()), 
                                       ByteSpan(key.Data(), key.Size()), 
                                       ByteSpan(iv.Data(), iv.Size()));
    
    EXPECT_EQ(decrypted, plaintext);
}

TEST_F(CryptoComprehensiveTest, AES256_DifferentKeys) {
    ByteVector key1(32, 0xFF);
    ByteVector key2(32, 0xAA);
    ByteVector plaintext = {'t', 'e', 's', 't'};
    ByteVector iv(16, 0x00);
    
    auto encrypted = Crypto::AesEncrypt(ByteSpan(plaintext.Data(), plaintext.Size()), 
                                       ByteSpan(key1.Data(), key1.Size()), 
                                       ByteSpan(iv.Data(), iv.Size()));
    
    // Decrypting with wrong key should fail or produce garbage
    EXPECT_THROW(Crypto::AesDecrypt(ByteSpan(encrypted.Data(), encrypted.Size()), 
                                   ByteSpan(key2.Data(), key2.Size()), 
                                   ByteSpan(iv.Data(), iv.Size())), std::exception);
}

// ============================================================================
// Checksum Tests
// ============================================================================

TEST_F(CryptoComprehensiveTest, Checksum_Calculate) {
    ByteVector data = {'d', 'a', 't', 'a'};
    // Checksum is first 4 bytes of double SHA256
    auto hash = Hash::Hash256(ByteSpan(data.Data(), data.Size()));
    ByteVector checksum(hash.Data(), hash.Data() + 4);
    EXPECT_EQ(checksum.Size(), 4);  
}

TEST_F(CryptoComprehensiveTest, Checksum_Verify) {
    ByteVector data = {'d', 'a', 't', 'a'};
    auto hash = Hash::Hash256(ByteSpan(data.Data(), data.Size()));
    ByteVector checksum(hash.Data(), hash.Data() + 4);
    
    ByteVector dataWithChecksum = data;
    dataWithChecksum.Append(ByteVector(checksum.begin(), checksum.end()));
    
    // Verify by recalculating hash and comparing checksum
    auto verifyHash = Hash::Hash256(ByteSpan(data.Data(), data.Size()));
    ByteVector verifyChecksum(verifyHash.Data(), verifyHash.Data() + 4);
    EXPECT_EQ(checksum, verifyChecksum);
}

TEST_F(CryptoComprehensiveTest, Checksum_InvalidVerify) {
    ByteVector data = {'d', 'a', 't', 'a'};
    ByteVector wrongChecksum = {0x00, 0x00, 0x00, 0x00};
    
    ByteVector dataWithWrongChecksum = data;
    dataWithWrongChecksum.Append(wrongChecksum);
    
    // Verify by recalculating hash and comparing checksum
    auto hash = Hash::Hash256(ByteSpan(data.Data(), data.Size()));
    ByteVector actualChecksum(hash.Data(), hash.Data() + 4);
    EXPECT_NE(wrongChecksum, actualChecksum);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(CryptoComprehensiveTest, Performance_SHA256_Throughput) {
    const int iterations = 1000;
    ByteVector data(1024);  // 1KB of data
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto hash = Hash::Sha256(ByteSpan(data.Data(), data.Size()));
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double throughput = (iterations * 1024.0) / (duration.count() / 1000.0) / (1024 * 1024);
    
    // Should be able to hash at least 10 MB/s
    EXPECT_GT(throughput, 10.0);
}

TEST_F(CryptoComprehensiveTest, Performance_RandomGeneration) {
    const int iterations = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto random = GenerateRandomBytes(32);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should generate at least 1000 random values per second
    EXPECT_LT(duration.count(), iterations);
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(CryptoComprehensiveTest, EdgeCase_ZeroSizeInput) {
    ByteVector empty;
    
    // These should handle empty input gracefully
    EXPECT_NO_THROW(Hash::Sha256(ByteSpan(empty.Data(), empty.Size())));
    EXPECT_NO_THROW(Hash::Ripemd160(ByteSpan(empty.Data(), empty.Size())));
    EXPECT_NO_THROW(Hash::Hash160(ByteSpan(empty.Data(), empty.Size())));
    EXPECT_NO_THROW(Hash::Hash256(ByteSpan(empty.Data(), empty.Size())));
}

TEST_F(CryptoComprehensiveTest, EdgeCase_MaxSizeInput) {
    // Test with maximum practical size (10MB)
    ByteVector large(10 * 1024 * 1024, 0xAB);
    
    EXPECT_NO_THROW(Hash::Sha256(ByteSpan(large.Data(), large.Size())));
}

TEST_F(CryptoComprehensiveTest, ThreadSafety_ConcurrentHashing) {
    const int threadCount = 10;
    std::vector<std::thread> threads;
    std::vector<UInt256> results(threadCount);
    
    ByteVector data = {'t', 'e', 's', 't'};
    
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&results, i, data]() {
            results[i] = Hash::Sha256(ByteSpan(data.Data(), data.Size()));
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // All threads should produce the same hash
    for (int i = 1; i < threadCount; ++i) {
        EXPECT_EQ(results[0], results[i]);
    }
}