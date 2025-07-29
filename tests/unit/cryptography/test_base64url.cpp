#include <gtest/gtest.h>
#include <neo/cryptography/base64url.h>
#include <neo/io/byte_vector.h>
#include <stdexcept>
#include <string>
#include <vector>

using namespace neo::cryptography;
using namespace neo::io;

TEST(Base64URLTest, Encode)
{
    // Test case 1: Empty array
    ByteVector empty;
    std::string emptyEncoded = Base64URL::Encode(empty.AsSpan());
    EXPECT_EQ(emptyEncoded, "");

    // Test case 2: Single byte
    ByteVector singleByte = {0x00};
    std::string singleByteEncoded = Base64URL::Encode(singleByte.AsSpan());
    EXPECT_EQ(singleByteEncoded, "AA");

    // Test case 3: Two bytes
    ByteVector twoBytes = {0x00, 0x01};
    std::string twoBytesEncoded = Base64URL::Encode(twoBytes.AsSpan());
    EXPECT_EQ(twoBytesEncoded, "AAE");

    // Test case 4: Three bytes
    ByteVector threeBytes = {0x00, 0x01, 0x02};
    std::string threeBytesEncoded = Base64URL::Encode(threeBytes.AsSpan());
    EXPECT_EQ(threeBytesEncoded, "AAEC");

    // Test case 5: Four bytes
    ByteVector fourBytes = {0x00, 0x01, 0x02, 0x03};
    std::string fourBytesEncoded = Base64URL::Encode(fourBytes.AsSpan());
    EXPECT_EQ(fourBytesEncoded, "AAECAw");

    // Test case 6: Special characters
    ByteVector specialChars = {0xFF, 0xFF, 0xFF};
    std::string specialCharsEncoded = Base64URL::Encode(specialChars.AsSpan());
    EXPECT_EQ(specialCharsEncoded, "_____w");

    // Test case 7: URL-safe characters
    ByteVector urlSafeChars = {0xFB, 0xEF, 0xBE};
    std::string urlSafeEncoded = Base64URL::Encode(urlSafeChars.AsSpan());
    EXPECT_EQ(urlSafeEncoded, "--_-");
}

TEST(Base64URLTest, Decode)
{
    // Test case 1: Empty string
    std::string emptyString = "";
    ByteVector emptyDecoded = Base64URL::Decode(emptyString);
    EXPECT_EQ(emptyDecoded.Size(), 0);

    // Test case 2: Single character (invalid)
    std::string singleChar = "A";
    EXPECT_THROW(Base64URL::Decode(singleChar), std::invalid_argument);

    // Test case 3: Two characters
    std::string twoChars = "AA";
    ByteVector twoCharsDecoded = Base64URL::Decode(twoChars);
    EXPECT_EQ(twoCharsDecoded.Size(), 1);
    EXPECT_EQ(twoCharsDecoded[0], 0x00);

    // Test case 4: Three characters
    std::string threeChars = "AAE";
    ByteVector threeCharsDecoded = Base64URL::Decode(threeChars);
    EXPECT_EQ(threeCharsDecoded.Size(), 2);
    EXPECT_EQ(threeCharsDecoded[0], 0x00);
    EXPECT_EQ(threeCharsDecoded[1], 0x01);

    // Test case 5: Four characters
    std::string fourChars = "AAEC";
    ByteVector fourCharsDecoded = Base64URL::Decode(fourChars);
    EXPECT_EQ(fourCharsDecoded.Size(), 3);
    EXPECT_EQ(fourCharsDecoded[0], 0x00);
    EXPECT_EQ(fourCharsDecoded[1], 0x01);
    EXPECT_EQ(fourCharsDecoded[2], 0x02);

    // Test case 6: Special characters
    std::string specialChars = "_____w";
    ByteVector specialCharsDecoded = Base64URL::Decode(specialChars);
    EXPECT_EQ(specialCharsDecoded.Size(), 3);
    EXPECT_EQ(specialCharsDecoded[0], 0xFF);
    EXPECT_EQ(specialCharsDecoded[1], 0xFF);
    EXPECT_EQ(specialCharsDecoded[2], 0xFF);

    // Test case 7: URL-safe characters
    std::string urlSafeChars = "--_-";
    ByteVector urlSafeDecoded = Base64URL::Decode(urlSafeChars);
    EXPECT_EQ(urlSafeDecoded.Size(), 3);
    EXPECT_EQ(urlSafeDecoded[0], 0xFB);
    EXPECT_EQ(urlSafeDecoded[1], 0xEF);
    EXPECT_EQ(urlSafeDecoded[2], 0xBE);
}

TEST(Base64URLTest, InvalidCharacters)
{
    // Test case 1: Invalid character '+'
    std::string invalidChar = "AA+AAA";
    EXPECT_THROW(Base64URL::Decode(invalidChar), std::invalid_argument);

    // Test case 2: Invalid character '/'
    std::string invalidChar2 = "AA/AAA";
    EXPECT_THROW(Base64URL::Decode(invalidChar2), std::invalid_argument);

    // Test case 3: Invalid character '='
    std::string invalidChar3 = "AA=AAA";
    EXPECT_THROW(Base64URL::Decode(invalidChar3), std::invalid_argument);

    // Test case 4: Invalid character '$'
    std::string invalidChar4 = "AA$AAA";
    EXPECT_THROW(Base64URL::Decode(invalidChar4), std::invalid_argument);
}

TEST(Base64URLTest, RoundTrip)
{
    // Test case 1: Random data
    ByteVector data = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    std::string encoded = Base64URL::Encode(data.AsSpan());
    ByteVector decoded = Base64URL::Decode(encoded);
    EXPECT_EQ(data, decoded);

    // Test case 2: Empty data
    ByteVector emptyData;
    std::string emptyEncoded = Base64URL::Encode(emptyData.AsSpan());
    ByteVector emptyDecoded = Base64URL::Decode(emptyEncoded);
    EXPECT_EQ(emptyData, emptyDecoded);

    // Test case 3: Single byte
    ByteVector singleByte = {0x42};
    std::string singleByteEncoded = Base64URL::Encode(singleByte.AsSpan());
    ByteVector singleByteDecoded = Base64URL::Decode(singleByteEncoded);
    EXPECT_EQ(singleByte, singleByteDecoded);

    // Test case 4: Two bytes
    ByteVector twoBytes = {0x42, 0x43};
    std::string twoBytesEncoded = Base64URL::Encode(twoBytes.AsSpan());
    ByteVector twoBytesDecoded = Base64URL::Decode(twoBytesEncoded);
    EXPECT_EQ(twoBytes, twoBytesDecoded);

    // Test case 5: Three bytes
    ByteVector threeBytes = {0x42, 0x43, 0x44};
    std::string threeBytesEncoded = Base64URL::Encode(threeBytes.AsSpan());
    ByteVector threeBytesDecoded = Base64URL::Decode(threeBytesEncoded);
    EXPECT_EQ(threeBytes, threeBytesDecoded);
}
