#include <gtest/gtest.h>
#include <neo/cryptography/hash.h>
#include <neo/io/byte_vector.h>
#include <string>
#include <vector>
#include <stdexcept>

using namespace neo::cryptography;
using namespace neo::io;

TEST(MurmurTest, Murmur32)
{
    // Test case 1: Empty array
    ByteVector empty;
    uint32_t emptyHash = Hash::Murmur32(empty.AsSpan(), 0);
    EXPECT_EQ(emptyHash, 0x00000000);
    
    // Test case 2: Single byte
    ByteVector singleByte = { 0x42 };
    uint32_t singleByteHash = Hash::Murmur32(singleByte.AsSpan(), 0);
    EXPECT_NE(singleByteHash, 0x00000000);
    
    // Test case 3: Multiple bytes
    ByteVector multipleBytes = { 0x01, 0x02, 0x03, 0x04 };
    uint32_t multipleBytesHash = Hash::Murmur32(multipleBytes.AsSpan(), 0);
    EXPECT_NE(multipleBytesHash, 0x00000000);
    
    // Test case 4: Different seeds
    uint32_t hash1 = Hash::Murmur32(multipleBytes.AsSpan(), 0);
    uint32_t hash2 = Hash::Murmur32(multipleBytes.AsSpan(), 1);
    EXPECT_NE(hash1, hash2);
    
    // Test case 5: Same input, same seed, same hash
    uint32_t hash3 = Hash::Murmur32(multipleBytes.AsSpan(), 0);
    EXPECT_EQ(hash1, hash3);
    
    // Test case 6: Different inputs, same seed, different hashes
    ByteVector differentBytes = { 0x01, 0x02, 0x03, 0x05 }; // Last byte different
    uint32_t hash4 = Hash::Murmur32(differentBytes.AsSpan(), 0);
    EXPECT_NE(hash1, hash4);
}

TEST(MurmurTest, Murmur128)
{
    // Test case 1: Empty array
    ByteVector empty;
    ByteVector emptyHash = Hash::Murmur128(empty.AsSpan(), 0);
    EXPECT_EQ(emptyHash.Size(), 16); // 128 bits = 16 bytes
    
    // Test case 2: Single byte
    ByteVector singleByte = { 0x42 };
    ByteVector singleByteHash = Hash::Murmur128(singleByte.AsSpan(), 0);
    EXPECT_EQ(singleByteHash.Size(), 16);
    
    // Test case 3: Multiple bytes
    ByteVector multipleBytes = { 0x01, 0x02, 0x03, 0x04 };
    ByteVector multipleBytesHash = Hash::Murmur128(multipleBytes.AsSpan(), 0);
    EXPECT_EQ(multipleBytesHash.Size(), 16);
    
    // Test case 4: Different seeds
    ByteVector hash1 = Hash::Murmur128(multipleBytes.AsSpan(), 0);
    ByteVector hash2 = Hash::Murmur128(multipleBytes.AsSpan(), 1);
    EXPECT_NE(hash1, hash2);
    
    // Test case 5: Same input, same seed, same hash
    ByteVector hash3 = Hash::Murmur128(multipleBytes.AsSpan(), 0);
    EXPECT_EQ(hash1, hash3);
    
    // Test case 6: Different inputs, same seed, different hashes
    ByteVector differentBytes = { 0x01, 0x02, 0x03, 0x05 }; // Last byte different
    ByteVector hash4 = Hash::Murmur128(differentBytes.AsSpan(), 0);
    EXPECT_NE(hash1, hash4);
}

TEST(MurmurTest, Murmur32_KnownValues)
{
    // Test with known values from the C# implementation
    
    // Test case 1: "hello"
    ByteVector hello(ByteSpan(reinterpret_cast<const uint8_t*>("hello"), 5));
    uint32_t helloHash = Hash::Murmur32(hello.AsSpan(), 0);
    EXPECT_EQ(helloHash, 0x248BFA47);
    
    // Test case 2: "hello" with seed 42
    uint32_t helloHash2 = Hash::Murmur32(hello.AsSpan(), 42);
    EXPECT_EQ(helloHash2, 0x149F5A8C);
    
    // Test case 3: "hello world"
    ByteVector helloWorld(ByteSpan(reinterpret_cast<const uint8_t*>("hello world"), 11));
    uint32_t helloWorldHash = Hash::Murmur32(helloWorld.AsSpan(), 0);
    EXPECT_EQ(helloWorldHash, 0x5E928F0F);
}

TEST(MurmurTest, Murmur128_KnownValues)
{
    // Test with known values from the C# implementation
    
    // Test case 1: "hello"
    ByteVector hello(ByteSpan(reinterpret_cast<const uint8_t*>("hello"), 5));
    ByteVector helloHash = Hash::Murmur128(hello.AsSpan(), 0);
    EXPECT_EQ(helloHash.ToHexString(), "6f02127a7a0c2e3f85a91f5b96a2901a");
    
    // Test case 2: "hello" with seed 42
    ByteVector helloHash2 = Hash::Murmur128(hello.AsSpan(), 42);
    EXPECT_EQ(helloHash2.ToHexString(), "a9e85cdb2c6e4761c90efd95899e8b2e");
    
    // Test case 3: "hello world"
    ByteVector helloWorld(ByteSpan(reinterpret_cast<const uint8_t*>("hello world"), 11));
    ByteVector helloWorldHash = Hash::Murmur128(helloWorld.AsSpan(), 0);
    EXPECT_EQ(helloWorldHash.ToHexString(), "e0a5a2c3a77a7a362c3f6c831da6e391");
}

TEST(MurmurTest, Murmur32_Consistency)
{
    // Test consistency of the hash function
    
    // Generate a large input
    ByteVector largeInput(1000);
    for (size_t i = 0; i < largeInput.Size(); i++)
    {
        largeInput[i] = static_cast<uint8_t>(i % 256);
    }
    
    // Compute hash multiple times
    uint32_t hash1 = Hash::Murmur32(largeInput.AsSpan(), 0);
    uint32_t hash2 = Hash::Murmur32(largeInput.AsSpan(), 0);
    uint32_t hash3 = Hash::Murmur32(largeInput.AsSpan(), 0);
    
    // All hashes should be the same
    EXPECT_EQ(hash1, hash2);
    EXPECT_EQ(hash2, hash3);
}

TEST(MurmurTest, Murmur128_Consistency)
{
    // Test consistency of the hash function
    
    // Generate a large input
    ByteVector largeInput(1000);
    for (size_t i = 0; i < largeInput.Size(); i++)
    {
        largeInput[i] = static_cast<uint8_t>(i % 256);
    }
    
    // Compute hash multiple times
    ByteVector hash1 = Hash::Murmur128(largeInput.AsSpan(), 0);
    ByteVector hash2 = Hash::Murmur128(largeInput.AsSpan(), 0);
    ByteVector hash3 = Hash::Murmur128(largeInput.AsSpan(), 0);
    
    // All hashes should be the same
    EXPECT_EQ(hash1, hash2);
    EXPECT_EQ(hash2, hash3);
}
