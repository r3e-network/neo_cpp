#include <gtest/gtest.h>
#include <memory>
#include <neo/json/jstring.h>
#include <neo/json/jtoken.h>
#include <string>

using namespace neo::json;

class JStringTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Initialize test environment
    }

    void TearDown() override
    {
        // Clean up test environment
    }
};

TEST_F(JStringTest, Construction)
{
    // Test construction with string literal
    EXPECT_NO_THROW({
        JString str1("hello");
        EXPECT_EQ(str1.GetValue(), "hello");
    });

    // Test construction with std::string
    std::string value = "world";
    EXPECT_NO_THROW({
        JString str2(value);
        EXPECT_EQ(str2.GetValue(), "world");
    });

    // Test construction with move semantics
    std::string moveValue = "moved";
    EXPECT_NO_THROW({
        JString str3(std::move(moveValue));
        EXPECT_EQ(str3.GetValue(), "moved");
    });
}

TEST_F(JStringTest, GetType)
{
    // Test that GetType returns String
    JString str("test");
    EXPECT_EQ(str.GetType(), JTokenType::String);
}

TEST_F(JStringTest, AsString_GetString)
{
    // Test string conversion methods
    JString str("hello world");

    EXPECT_EQ(str.AsString(), "hello world");
    EXPECT_EQ(str.GetString(), "hello world");
    EXPECT_EQ(str.GetValue(), "hello world");
}

TEST_F(JStringTest, ToString_JsonRepresentation)
{
    // Test JSON string representation
    JString str("hello");
    std::string jsonStr = str.ToString();
    EXPECT_EQ(jsonStr, "\"hello\"");  // Should be quoted

    // Test with special characters that need escaping
    JString specialStr("hello\"world\n");
    std::string specialJson = specialStr.ToString();
    // Should properly escape quotes and newlines
    EXPECT_TRUE(specialJson.find("\\\"") != std::string::npos);  // Escaped quote
    EXPECT_TRUE(specialJson.find("\\n") != std::string::npos);   // Escaped newline
}

TEST_F(JStringTest, Clone)
{
    // Test cloning functionality
    JString original("original");
    auto clone = original.Clone();

    ASSERT_NE(clone, nullptr);
    EXPECT_EQ(clone->GetType(), JTokenType::String);

    auto clonedString = std::dynamic_pointer_cast<JString>(clone);
    ASSERT_NE(clonedString, nullptr);
    EXPECT_EQ(clonedString->GetValue(), "original");

    // Verify they are different objects
    EXPECT_NE(&original, clonedString.get());
}

TEST_F(JStringTest, Equals)
{
    // Test equality comparison
    JString str1("hello");
    JString str2("hello");
    JString str3("world");

    EXPECT_TRUE(str1.Equals(str2));
    EXPECT_FALSE(str1.Equals(str3));

    // Test equality with self
    EXPECT_TRUE(str1.Equals(str1));
}

TEST_F(JStringTest, ImplicitStringConversion)
{
    // Test implicit conversion to string
    JString str("test value");
    std::string converted = str;
    EXPECT_EQ(converted, "test value");
}

TEST_F(JStringTest, EmptyString)
{
    // Test with empty string
    JString emptyStr("");
    EXPECT_EQ(emptyStr.GetValue(), "");
    EXPECT_EQ(emptyStr.AsString(), "");
    EXPECT_EQ(emptyStr.ToString(), "\"\"");  // Empty string in JSON is ""
}

TEST_F(JStringTest, LongString)
{
    // Test with very long string
    std::string longValue(1000, 'a');
    JString longStr(longValue);
    EXPECT_EQ(longStr.GetValue(), longValue);
    EXPECT_EQ(longStr.AsString(), longValue);
}

TEST_F(JStringTest, UnicodeString)
{
    // Test with Unicode characters
    std::string unicodeValue = "Hello 世界 🌍";
    JString unicodeStr(unicodeValue);
    EXPECT_EQ(unicodeStr.GetValue(), unicodeValue);
    EXPECT_EQ(unicodeStr.AsString(), unicodeValue);
}

TEST_F(JStringTest, SpecialCharacters)
{
    // Test with various special characters
    std::string specialChars = "\t\r\n\"\\";
    JString specialStr(specialChars);
    EXPECT_EQ(specialStr.GetValue(), specialChars);

    // Verify ToString properly escapes these characters
    std::string jsonRepr = specialStr.ToString();
    EXPECT_TRUE(jsonRepr.find("\\t") != std::string::npos);
    EXPECT_TRUE(jsonRepr.find("\\r") != std::string::npos);
    EXPECT_TRUE(jsonRepr.find("\\n") != std::string::npos);
    EXPECT_TRUE(jsonRepr.find("\\\"") != std::string::npos);
    EXPECT_TRUE(jsonRepr.find("\\\\") != std::string::npos);
}
