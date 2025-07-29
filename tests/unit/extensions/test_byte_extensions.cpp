#include <gtest/gtest.h>
#include <neo/extensions/byte_extensions.h>

namespace neo::extensions::tests
{
class ByteExtensionsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        test_data = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
        empty_data = {};
        zero_data = {0x00, 0x00, 0x00, 0x00};
    }

    std::vector<uint8_t> test_data;
    std::vector<uint8_t> empty_data;
    std::vector<uint8_t> zero_data;
};

TEST_F(ByteExtensionsTest, TestToHexString)
{
    std::string hex = ByteExtensions::ToHexString(test_data);
    EXPECT_EQ("0123456789abcdef", hex);

    std::string empty_hex = ByteExtensions::ToHexString(empty_data);
    EXPECT_EQ("", empty_hex);
}

TEST_F(ByteExtensionsTest, TestToHexStringReverse)
{
    std::string hex = ByteExtensions::ToHexString(test_data, true);
    EXPECT_EQ("efcdab8967452301", hex);
}

TEST_F(ByteExtensionsTest, TestFromHexString)
{
    auto result = ByteExtensions::FromHexString("0123456789abcdef");
    EXPECT_EQ(test_data, result);

    auto empty_result = ByteExtensions::FromHexString("");
    EXPECT_EQ(empty_data, empty_result);

    // Test uppercase
    auto upper_result = ByteExtensions::FromHexString("0123456789ABCDEF");
    EXPECT_EQ(test_data, upper_result);
}

TEST_F(ByteExtensionsTest, TestFromHexStringInvalid)
{
    EXPECT_THROW(ByteExtensions::FromHexString("123"), std::invalid_argument);   // Odd length
    EXPECT_THROW(ByteExtensions::FromHexString("12GH"), std::invalid_argument);  // Invalid chars
}

TEST_F(ByteExtensionsTest, TestNotZero)
{
    EXPECT_TRUE(ByteExtensions::NotZero(test_data));
    EXPECT_FALSE(ByteExtensions::NotZero(zero_data));
    EXPECT_FALSE(ByteExtensions::NotZero(empty_data));
}

TEST_F(ByteExtensionsTest, TestIsZero)
{
    EXPECT_FALSE(ByteExtensions::IsZero(test_data));
    EXPECT_TRUE(ByteExtensions::IsZero(zero_data));
    EXPECT_TRUE(ByteExtensions::IsZero(empty_data));
}

TEST_F(ByteExtensionsTest, TestReverse)
{
    auto reversed = ByteExtensions::Reverse(test_data);
    std::vector<uint8_t> expected = {0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01};
    EXPECT_EQ(expected, reversed);

    // Original should be unchanged
    EXPECT_EQ(std::vector<uint8_t>({0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF}), test_data);
}

TEST_F(ByteExtensionsTest, TestReverseInPlace)
{
    auto data_copy = test_data;
    ByteExtensions::ReverseInPlace(data_copy);
    std::vector<uint8_t> expected = {0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01};
    EXPECT_EQ(expected, data_copy);
}

TEST_F(ByteExtensionsTest, TestConcat)
{
    std::vector<uint8_t> first = {0x01, 0x02};
    std::vector<uint8_t> second = {0x03, 0x04};

    auto result = ByteExtensions::Concat(first, second);
    std::vector<uint8_t> expected = {0x01, 0x02, 0x03, 0x04};
    EXPECT_EQ(expected, result);
}

TEST_F(ByteExtensionsTest, TestConcatMultiple)
{
    std::vector<std::vector<uint8_t>> arrays = {{0x01, 0x02}, {0x03, 0x04}, {0x05, 0x06}};

    auto result = ByteExtensions::Concat(arrays);
    std::vector<uint8_t> expected = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    EXPECT_EQ(expected, result);
}

TEST_F(ByteExtensionsTest, TestSlice)
{
    auto result = ByteExtensions::Slice(test_data, 2, 4);
    std::vector<uint8_t> expected = {0x45, 0x67, 0x89, 0xAB};
    EXPECT_EQ(expected, result);

    auto result2 = ByteExtensions::Slice(test_data, 4);
    std::vector<uint8_t> expected2 = {0x89, 0xAB, 0xCD, 0xEF};
    EXPECT_EQ(expected2, result2);
}

TEST_F(ByteExtensionsTest, TestSliceOutOfRange)
{
    EXPECT_THROW(ByteExtensions::Slice(test_data, 10, 2), std::out_of_range);
    EXPECT_THROW(ByteExtensions::Slice(test_data, 2, 10), std::out_of_range);
    EXPECT_THROW(ByteExtensions::Slice(test_data, 10), std::out_of_range);
}

TEST_F(ByteExtensionsTest, TestSequenceEqual)
{
    auto data_copy = test_data;
    EXPECT_TRUE(ByteExtensions::SequenceEqual(test_data, data_copy));
    EXPECT_FALSE(ByteExtensions::SequenceEqual(test_data, zero_data));
    EXPECT_FALSE(ByteExtensions::SequenceEqual(test_data, empty_data));
}

TEST_F(ByteExtensionsTest, TestXxHash3_32)
{
    int hash1 = ByteExtensions::XxHash3_32(test_data);
    int hash2 = ByteExtensions::XxHash3_32(test_data);
    EXPECT_EQ(hash1, hash2);  // Same input should produce same hash

    int hash3 = ByteExtensions::XxHash3_32(zero_data);
    EXPECT_NE(hash1, hash3);  // Different input should produce different hash (usually)
}

TEST_F(ByteExtensionsTest, TestRoundTripHexConversion)
{
    // Test round-trip conversion
    std::string hex = ByteExtensions::ToHexString(test_data);
    auto converted_back = ByteExtensions::FromHexString(hex);
    EXPECT_EQ(test_data, converted_back);
}
}  // namespace neo::extensions::tests
