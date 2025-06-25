#include <gtest/gtest.h>
#include <neo/extensions/string_extensions.h>
#include <neo/extensions/byte_extensions.h>

namespace neo::extensions::tests
{
    class StringExtensionsTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            test_string = "Hello, World!";
            hex_string = "48656c6c6f";
            invalid_hex = "48656c6c6";
            utf8_string = "Hello 世界";
        }

        std::string test_string;
        std::string hex_string;
        std::string invalid_hex;
        std::string utf8_string;
    };

    TEST_F(StringExtensionsTest, TestToStrictUtf8Bytes)
    {
        auto bytes = StringExtensions::ToStrictUtf8Bytes(test_string);
        EXPECT_EQ(test_string.size(), bytes.size());
        
        // Convert back and verify
        std::string converted_back = StringExtensions::ToStrictUtf8String(bytes);
        EXPECT_EQ(test_string, converted_back);
    }

    TEST_F(StringExtensionsTest, TestGetStrictUtf8ByteCount)
    {
        size_t count = StringExtensions::GetStrictUtf8ByteCount(test_string);
        EXPECT_EQ(test_string.size(), count);
        
        // UTF-8 string should have more bytes than characters
        size_t utf8_count = StringExtensions::GetStrictUtf8ByteCount(utf8_string);
        EXPECT_GT(utf8_count, 8); // "Hello 世界" has more than 8 bytes
    }

    TEST_F(StringExtensionsTest, TestIsHex)
    {
        EXPECT_TRUE(StringExtensions::IsHex(""));
        EXPECT_TRUE(StringExtensions::IsHex("48656c6c6f"));
        EXPECT_TRUE(StringExtensions::IsHex("48656C6C6F")); // Uppercase
        EXPECT_TRUE(StringExtensions::IsHex("0123456789abcdefABCDEF"));
        
        EXPECT_FALSE(StringExtensions::IsHex("48656c6c6")); // Odd length
        EXPECT_FALSE(StringExtensions::IsHex("48656g6c6f")); // Invalid character
        EXPECT_FALSE(StringExtensions::IsHex("Hello"));
    }

    TEST_F(StringExtensionsTest, TestHexToBytes)
    {
        auto bytes = StringExtensions::HexToBytes(hex_string);
        std::vector<uint8_t> expected = {0x48, 0x65, 0x6c, 0x6c, 0x6f};
        EXPECT_EQ(expected, bytes);
        
        auto empty_bytes = StringExtensions::HexToBytes("");
        EXPECT_TRUE(empty_bytes.empty());
    }

    TEST_F(StringExtensionsTest, TestHexToBytesInvalid)
    {
        EXPECT_THROW(StringExtensions::HexToBytes(invalid_hex), std::invalid_argument);
        EXPECT_THROW(StringExtensions::HexToBytes("48656g6c6f"), std::invalid_argument);
    }

    TEST_F(StringExtensionsTest, TestHexToBytesReversed)
    {
        auto bytes = StringExtensions::HexToBytesReversed(hex_string);
        std::vector<uint8_t> expected = {0x6f, 0x6c, 0x6c, 0x65, 0x48};
        EXPECT_EQ(expected, bytes);
    }

    TEST_F(StringExtensionsTest, TestTrim)
    {
        EXPECT_EQ("hello", StringExtensions::Trim("  hello  "));
        EXPECT_EQ("hello", StringExtensions::Trim("hello"));
        EXPECT_EQ("", StringExtensions::Trim("   "));
        EXPECT_EQ("", StringExtensions::Trim(""));
        EXPECT_EQ("hello world", StringExtensions::Trim("\t\n hello world \r\n"));
    }

    TEST_F(StringExtensionsTest, TestTrimStart)
    {
        EXPECT_EQ("hello  ", StringExtensions::TrimStart("  hello  "));
        EXPECT_EQ("hello", StringExtensions::TrimStart("hello"));
        EXPECT_EQ("", StringExtensions::TrimStart("   "));
    }

    TEST_F(StringExtensionsTest, TestTrimEnd)
    {
        EXPECT_EQ("  hello", StringExtensions::TrimEnd("  hello  "));
        EXPECT_EQ("hello", StringExtensions::TrimEnd("hello"));
        EXPECT_EQ("", StringExtensions::TrimEnd("   "));
    }

    TEST_F(StringExtensionsTest, TestToLowerUpper)
    {
        EXPECT_EQ("hello world", StringExtensions::ToLower("Hello World"));
        EXPECT_EQ("HELLO WORLD", StringExtensions::ToUpper("Hello World"));
        EXPECT_EQ("", StringExtensions::ToLower(""));
        EXPECT_EQ("", StringExtensions::ToUpper(""));
    }

    TEST_F(StringExtensionsTest, TestStartsWithEndsWith)
    {
        EXPECT_TRUE(StringExtensions::StartsWith("Hello World", "Hello"));
        EXPECT_TRUE(StringExtensions::StartsWith("Hello World", ""));
        EXPECT_FALSE(StringExtensions::StartsWith("Hello World", "World"));
        EXPECT_FALSE(StringExtensions::StartsWith("Hi", "Hello"));
        
        EXPECT_TRUE(StringExtensions::EndsWith("Hello World", "World"));
        EXPECT_TRUE(StringExtensions::EndsWith("Hello World", ""));
        EXPECT_FALSE(StringExtensions::EndsWith("Hello World", "Hello"));
        EXPECT_FALSE(StringExtensions::EndsWith("Hi", "World"));
    }

    TEST_F(StringExtensionsTest, TestSplit)
    {
        auto result = StringExtensions::Split("a,b,c,d", ',');
        std::vector<std::string> expected = {"a", "b", "c", "d"};
        EXPECT_EQ(expected, result);
        
        auto single_result = StringExtensions::Split("hello", ',');
        std::vector<std::string> single_expected = {"hello"};
        EXPECT_EQ(single_expected, single_result);
        
        auto empty_result = StringExtensions::Split("", ',');
        std::vector<std::string> empty_expected = {""};
        EXPECT_EQ(empty_expected, empty_result);
    }

    TEST_F(StringExtensionsTest, TestJoin)
    {
        std::vector<std::string> parts = {"a", "b", "c", "d"};
        std::string result = StringExtensions::Join(parts, ",");
        EXPECT_EQ("a,b,c,d", result);
        
        std::vector<std::string> single = {"hello"};
        std::string single_result = StringExtensions::Join(single, ",");
        EXPECT_EQ("hello", single_result);
        
        std::vector<std::string> empty;
        std::string empty_result = StringExtensions::Join(empty, ",");
        EXPECT_EQ("", empty_result);
    }

    TEST_F(StringExtensionsTest, TestGetVarSize)
    {
        // Test variable-length encoding size calculation
        std::string short_string = "Hi";
        EXPECT_EQ(3, StringExtensions::GetVarSize(short_string)); // 1 byte length + 2 bytes data
        
        std::string medium_string(300, 'x');
        EXPECT_EQ(303, StringExtensions::GetVarSize(medium_string)); // 3 bytes length + 300 bytes data
    }

    TEST_F(StringExtensionsTest, TestTryToStrictUtf8String)
    {
        std::vector<uint8_t> valid_utf8 = {0x48, 0x65, 0x6c, 0x6c, 0x6f}; // "Hello"
        std::string result;
        
        EXPECT_TRUE(StringExtensions::TryToStrictUtf8String(valid_utf8, result));
        EXPECT_EQ("Hello", result);
        
        // Test with invalid UTF-8 (this is a simplified test)
        std::vector<uint8_t> invalid_utf8 = {0xFF, 0xFE, 0xFD};
        EXPECT_FALSE(StringExtensions::TryToStrictUtf8String(invalid_utf8, result));
    }

    TEST_F(StringExtensionsTest, TestRoundTripConversion)
    {
        // Test round-trip string to bytes to string
        auto bytes = StringExtensions::ToStrictUtf8Bytes(test_string);
        std::string converted_back = StringExtensions::ToStrictUtf8String(bytes);
        EXPECT_EQ(test_string, converted_back);
        
        // Test round-trip hex conversion
        auto hex_bytes = StringExtensions::HexToBytes(hex_string);
        std::string hex_back = ByteExtensions::ToHexString(hex_bytes);
        EXPECT_EQ(hex_string, hex_back);
    }
}
