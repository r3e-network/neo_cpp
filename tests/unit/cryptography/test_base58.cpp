#include <gtest/gtest.h>
#include <neo/cryptography/base58.h>
#include <neo/io/byte_vector.h>
#include <string>
#include <vector>
#include <stdexcept>

using namespace neo::cryptography;
using namespace neo::io;

TEST(Base58Test, Encode)
{
    // Test vectors from https://en.bitcoin.it/wiki/Base58Check_encoding
    
    // Test case 1: Empty array
    ByteVector empty;
    std::string emptyEncoded = Base58::Encode(empty.AsSpan());
    EXPECT_EQ(emptyEncoded, "");
    
    // Test case 2: Single byte
    ByteVector singleByte = { 0 };
    std::string singleByteEncoded = Base58::Encode(singleByte.AsSpan());
    EXPECT_EQ(singleByteEncoded, "1");
    
    // Test case 3: Multiple zeros
    ByteVector multipleZeros = { 0, 0, 0, 0 };
    std::string multipleZerosEncoded = Base58::Encode(multipleZeros.AsSpan());
    EXPECT_EQ(multipleZerosEncoded, "1111");
    
    // Test case 4: Bitcoin address
    ByteVector address = ByteVector::Parse("00010966776006953D5567439E5E39F86A0D273BEED61967F6");
    std::string addressEncoded = Base58::Encode(address.AsSpan());
    EXPECT_EQ(addressEncoded, "16UwLL9Risc3QfPqBUvKofHmBQ7wMtjvM");
    
    // Test case 5: Maximum value
    ByteVector maxValue = { 0xFF, 0xFF, 0xFF, 0xFF };
    std::string maxValueEncoded = Base58::Encode(maxValue.AsSpan());
    EXPECT_EQ(maxValueEncoded, "LUv1");
}

TEST(Base58Test, Decode)
{
    // Test case 1: Empty string
    std::string emptyString = "";
    ByteVector emptyDecoded = Base58::Decode(emptyString);
    EXPECT_EQ(emptyDecoded.Size(), 0);
    
    // Test case 2: Single character
    std::string singleChar = "1";
    ByteVector singleCharDecoded = Base58::Decode(singleChar);
    EXPECT_EQ(singleCharDecoded.Size(), 1);
    EXPECT_EQ(singleCharDecoded[0], 0);
    
    // Test case 3: Multiple ones
    std::string multipleOnes = "1111";
    ByteVector multipleOnesDecoded = Base58::Decode(multipleOnes);
    EXPECT_EQ(multipleOnesDecoded.Size(), 4);
    for (size_t i = 0; i < multipleOnesDecoded.Size(); i++)
    {
        EXPECT_EQ(multipleOnesDecoded[i], 0);
    }
    
    // Test case 4: Bitcoin address
    std::string address = "16UwLL9Risc3QfPqBUvKofHmBQ7wMtjvM";
    ByteVector addressDecoded = Base58::Decode(address);
    EXPECT_EQ(addressDecoded.ToHexString(), "00010966776006953d5567439e5e39f86a0d273beed61967f6");
    
    // Test case 5: Maximum value
    std::string maxValue = "LUv1";
    ByteVector maxValueDecoded = Base58::Decode(maxValue);
    EXPECT_EQ(maxValueDecoded.Size(), 4);
    for (size_t i = 0; i < maxValueDecoded.Size(); i++)
    {
        EXPECT_EQ(maxValueDecoded[i], 0xFF);
    }
}

TEST(Base58Test, EncodeWithChecksum)
{
    // Test case 1: Empty array
    ByteVector empty;
    std::string emptyEncoded = Base58::EncodeWithChecksum(empty.AsSpan());
    ByteVector emptyWithChecksum = empty;
    uint32_t emptyChecksum = Hash::Checksum(empty.AsSpan());
    emptyWithChecksum.PushBack((emptyChecksum >> 0) & 0xFF);
    emptyWithChecksum.PushBack((emptyChecksum >> 8) & 0xFF);
    emptyWithChecksum.PushBack((emptyChecksum >> 16) & 0xFF);
    emptyWithChecksum.PushBack((emptyChecksum >> 24) & 0xFF);
    EXPECT_EQ(emptyEncoded, Base58::Encode(emptyWithChecksum.AsSpan()));
    
    // Test case 2: Bitcoin address
    ByteVector address = ByteVector::Parse("00010966776006953D5567439E5E39F86A0D273BEED61967F6");
    std::string addressEncoded = Base58::EncodeWithChecksum(address.AsSpan());
    EXPECT_EQ(addressEncoded, "16UwLL9Risc3QfPqBUvKofHmBQ7wMtjvM");
}

TEST(Base58Test, DecodeWithChecksum)
{
    // Test case 1: Empty string
    std::string emptyString = "";
    EXPECT_THROW(Base58::DecodeWithChecksum(emptyString), std::invalid_argument);
    
    // Test case 2: Invalid checksum
    std::string invalidChecksum = "16UwLL9Risc3QfPqBUvKofHmBQ7wMtjvN"; // Changed last character
    EXPECT_THROW(Base58::DecodeWithChecksum(invalidChecksum), std::invalid_argument);
    
    // Test case 3: Valid checksum
    std::string validChecksum = "16UwLL9Risc3QfPqBUvKofHmBQ7wMtjvM";
    ByteVector validChecksumDecoded = Base58::DecodeWithChecksum(validChecksum);
    EXPECT_EQ(validChecksumDecoded.ToHexString(), "00010966776006953d5567439e5e39f86a0d273beed61967f6");
}

TEST(Base58Test, InvalidCharacters)
{
    // Test case 1: Invalid character 'O'
    std::string invalidChar = "1O1";
    EXPECT_THROW(Base58::Decode(invalidChar), std::invalid_argument);
    
    // Test case 2: Invalid character 'I'
    std::string invalidChar2 = "1I1";
    EXPECT_THROW(Base58::Decode(invalidChar2), std::invalid_argument);
    
    // Test case 3: Invalid character 'l'
    std::string invalidChar3 = "1l1";
    EXPECT_THROW(Base58::Decode(invalidChar3), std::invalid_argument);
    
    // Test case 4: Invalid character '0'
    std::string invalidChar4 = "101";
    EXPECT_THROW(Base58::Decode(invalidChar4), std::invalid_argument);
}

TEST(Base58Test, RoundTrip)
{
    // Test case 1: Random data
    ByteVector data = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0 };
    std::string encoded = Base58::Encode(data.AsSpan());
    ByteVector decoded = Base58::Decode(encoded);
    EXPECT_EQ(data, decoded);
    
    // Test case 2: Random data with checksum
    std::string encodedWithChecksum = Base58::EncodeWithChecksum(data.AsSpan());
    ByteVector decodedWithChecksum = Base58::DecodeWithChecksum(encodedWithChecksum);
    EXPECT_EQ(data, decodedWithChecksum);
}
