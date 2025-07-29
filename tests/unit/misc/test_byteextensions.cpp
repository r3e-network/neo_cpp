#include <gtest/gtest.h>
#include <memory>
#include <neo/extensions/byte_extensions.h>
#include <span>
#include <string>
#include <vector>

using namespace neo::extensions;

class ByteExtensionsTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Initialize test data
        emptyBytes_ = {};
        singleByte_ = {0x42};
        testBytes_ = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
        zeroBytes_ = {0x00, 0x00, 0x00, 0x00};
        nonZeroBytes_ = {0x01, 0x00, 0x02, 0x00};
    }

    void TearDown() override
    {
        // Clean up test environment
    }

    std::vector<uint8_t> emptyBytes_;
    std::vector<uint8_t> singleByte_;
    std::vector<uint8_t> testBytes_;
    std::vector<uint8_t> zeroBytes_;
    std::vector<uint8_t> nonZeroBytes_;
};

TEST_F(ByteExtensionsTest, ToHexString_BasicConversion)
{
    // Test basic hex string conversion
    std::string hex = ByteExtensions::ToHexString(testBytes_);
    EXPECT_EQ(hex, "0123456789abcdef");

    // Test single byte
    std::string singleHex = ByteExtensions::ToHexString(singleByte_);
    EXPECT_EQ(singleHex, "42");

    // Test empty array
    std::string emptyHex = ByteExtensions::ToHexString(emptyBytes_);
    EXPECT_EQ(emptyHex, "");
}

TEST_F(ByteExtensionsTest, ToHexString_WithReverse)
{
    // Test hex string conversion with reverse
    std::string hex = ByteExtensions::ToHexString(testBytes_, true);
    EXPECT_EQ(hex, "efcdab8967452301");

    // Test single byte with reverse (should be same)
    std::string singleHex = ByteExtensions::ToHexString(singleByte_, true);
    EXPECT_EQ(singleHex, "42");
}

TEST_F(ByteExtensionsTest, ToHexString_Span)
{
    // Test hex string conversion with span
    std::span<const uint8_t> span(testBytes_);
    std::string hex = ByteExtensions::ToHexString(span);
    EXPECT_EQ(hex, "0123456789abcdef");

    // Test with reverse
    std::string hexReverse = ByteExtensions::ToHexString(span, true);
    EXPECT_EQ(hexReverse, "efcdab8967452301");
}

TEST_F(ByteExtensionsTest, FromHexString_BasicConversion)
{
    // Test hex string to bytes conversion
    auto bytes = ByteExtensions::FromHexString("0123456789abcdef");
    EXPECT_EQ(bytes, testBytes_);

    // Test uppercase hex
    auto bytesUpper = ByteExtensions::FromHexString("0123456789ABCDEF");
    EXPECT_EQ(bytesUpper, testBytes_);

    // Test empty string
    auto emptyBytes = ByteExtensions::FromHexString("");
    EXPECT_TRUE(emptyBytes.empty());
}

TEST_F(ByteExtensionsTest, FromHexString_InvalidInput)
{
    // Test invalid hex string
    EXPECT_THROW(ByteExtensions::FromHexString("xyz"), std::invalid_argument);

    // Test odd length hex string
    EXPECT_THROW(ByteExtensions::FromHexString("123"), std::invalid_argument);
}

TEST_F(ByteExtensionsTest, HexRoundTrip)
{
    // Test round trip conversion
    auto hex = ByteExtensions::ToHexString(testBytes_);
    auto bytes = ByteExtensions::FromHexString(hex);
    EXPECT_EQ(bytes, testBytes_);
}

TEST_F(ByteExtensionsTest, IsZero_NotZero)
{
    // Test zero detection
    EXPECT_TRUE(ByteExtensions::IsZero(zeroBytes_));
    EXPECT_FALSE(ByteExtensions::IsZero(nonZeroBytes_));
    EXPECT_TRUE(ByteExtensions::IsZero(emptyBytes_));  // Empty is considered zero

    // Test NotZero
    EXPECT_FALSE(ByteExtensions::NotZero(zeroBytes_));
    EXPECT_TRUE(ByteExtensions::NotZero(nonZeroBytes_));
    EXPECT_FALSE(ByteExtensions::NotZero(emptyBytes_));
}

TEST_F(ByteExtensionsTest, IsZero_NotZero_Span)
{
    // Test zero detection with spans
    std::span<const uint8_t> zeroSpan(zeroBytes_);
    std::span<const uint8_t> nonZeroSpan(nonZeroBytes_);

    EXPECT_TRUE(ByteExtensions::IsZero(zeroSpan));
    EXPECT_FALSE(ByteExtensions::IsZero(nonZeroSpan));

    EXPECT_FALSE(ByteExtensions::NotZero(zeroSpan));
    EXPECT_TRUE(ByteExtensions::NotZero(nonZeroSpan));
}

TEST_F(ByteExtensionsTest, Reverse)
{
    // Test reverse functionality
    auto reversed = ByteExtensions::Reverse(testBytes_);
    std::vector<uint8_t> expected = {0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01};
    EXPECT_EQ(reversed, expected);

    // Test empty array
    auto emptyReversed = ByteExtensions::Reverse(emptyBytes_);
    EXPECT_TRUE(emptyReversed.empty());
}

TEST_F(ByteExtensionsTest, ReverseInPlace)
{
    // Test in-place reverse
    auto bytes = testBytes_;
    ByteExtensions::ReverseInPlace(bytes);
    std::vector<uint8_t> expected = {0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01};
    EXPECT_EQ(bytes, expected);
}

TEST_F(ByteExtensionsTest, Concat_TwoArrays)
{
    // Test concatenation of two arrays
    std::vector<uint8_t> first = {0x01, 0x02};
    std::vector<uint8_t> second = {0x03, 0x04};
    auto result = ByteExtensions::Concat(first, second);
    std::vector<uint8_t> expected = {0x01, 0x02, 0x03, 0x04};
    EXPECT_EQ(result, expected);
}

TEST_F(ByteExtensionsTest, Concat_MultipleArrays)
{
    // Test concatenation of multiple arrays
    std::vector<std::vector<uint8_t>> arrays = {{0x01, 0x02}, {0x03}, {0x04, 0x05, 0x06}};
    auto result = ByteExtensions::Concat(arrays);
    std::vector<uint8_t> expected = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    EXPECT_EQ(result, expected);
}

TEST_F(ByteExtensionsTest, Slice)
{
    // Test slicing functionality
    auto slice = ByteExtensions::Slice(testBytes_, 2, 3);
    std::vector<uint8_t> expected = {0x45, 0x67, 0x89};
    EXPECT_EQ(slice, expected);

    // Test slice from start to end
    auto sliceFromStart = ByteExtensions::Slice(testBytes_, 3);
    std::vector<uint8_t> expectedFromStart = {0x67, 0x89, 0xAB, 0xCD, 0xEF};
    EXPECT_EQ(sliceFromStart, expectedFromStart);
}

TEST_F(ByteExtensionsTest, Slice_EdgeCases)
{
    // Test slice edge cases
    EXPECT_THROW(ByteExtensions::Slice(testBytes_, 10, 2), std::out_of_range);
    EXPECT_THROW(ByteExtensions::Slice(testBytes_, 2, 10), std::out_of_range);

    // Test empty slice
    auto emptySlice = ByteExtensions::Slice(testBytes_, 2, 0);
    EXPECT_TRUE(emptySlice.empty());
}

TEST_F(ByteExtensionsTest, SequenceEqual)
{
    // Test sequence equality
    EXPECT_TRUE(ByteExtensions::SequenceEqual(testBytes_, testBytes_));
    EXPECT_FALSE(ByteExtensions::SequenceEqual(testBytes_, zeroBytes_));
    EXPECT_TRUE(ByteExtensions::SequenceEqual(emptyBytes_, emptyBytes_));

    // Test with spans
    std::span<const uint8_t> span1(testBytes_);
    std::span<const uint8_t> span2(testBytes_);
    std::span<const uint8_t> span3(zeroBytes_);

    EXPECT_TRUE(ByteExtensions::SequenceEqual(span1, span2));
    EXPECT_FALSE(ByteExtensions::SequenceEqual(span1, span3));
}

TEST_F(ByteExtensionsTest, XxHash3_32)
{
    // Test hash function
    int hash1 = ByteExtensions::XxHash3_32(testBytes_);
    int hash2 = ByteExtensions::XxHash3_32(testBytes_);
    EXPECT_EQ(hash1, hash2);  // Same input should produce same hash

    // Different inputs should produce different hashes (high probability)
    int hash3 = ByteExtensions::XxHash3_32(zeroBytes_);
    EXPECT_NE(hash1, hash3);

    // Test with custom seed
    int hashWithSeed = ByteExtensions::XxHash3_32(testBytes_, 12345);
    EXPECT_NE(hash1, hashWithSeed);  // Different seed should produce different hash

    // Test with span
    std::span<const uint8_t> span(testBytes_);
    int hashSpan = ByteExtensions::XxHash3_32(span);
    EXPECT_EQ(hash1, hashSpan);  // Vector and span should produce same hash
}
