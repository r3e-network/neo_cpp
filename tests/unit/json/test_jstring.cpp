#include <gtest/gtest.h>
#include <neo/json/jstring.h>

namespace neo::json::tests
{
class JStringTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        simple_string = std::make_shared<JString>("hello world");
        empty_string = std::make_shared<JString>("");
        special_chars = std::make_shared<JString>("\"\\\/\b\f\n\r\t");
    }

    std::shared_ptr<JString> simple_string;
    std::shared_ptr<JString> empty_string;
    std::shared_ptr<JString> special_chars;
};

TEST_F(JStringTest, TestGetType)
{
    EXPECT_EQ(JTokenType::String, simple_string->GetType());
}

TEST_F(JStringTest, TestAsString)
{
    EXPECT_EQ("hello world", simple_string->AsString());
    EXPECT_EQ("", empty_string->AsString());
}

TEST_F(JStringTest, TestGetString)
{
    EXPECT_EQ("hello world", simple_string->GetString());
    EXPECT_EQ("", empty_string->GetString());
}

TEST_F(JStringTest, TestToString)
{
    EXPECT_EQ("\"hello world\"", simple_string->ToString());
    EXPECT_EQ("\"\"", empty_string->ToString());

    // Test that special characters are properly escaped
    std::string escaped = special_chars->ToString();
    EXPECT_NE(escaped.find("\\\""), std::string::npos);  // Quote should be escaped
    EXPECT_NE(escaped.find("\\\\"), std::string::npos);  // Backslash should be escaped
    EXPECT_NE(escaped.find("\\n"), std::string::npos);   // Newline should be escaped
}

TEST_F(JStringTest, TestClone)
{
    auto cloned = std::dynamic_pointer_cast<JString>(simple_string->Clone());
    EXPECT_NE(simple_string.get(), cloned.get());
    EXPECT_EQ(simple_string->GetString(), cloned->GetString());
}

TEST_F(JStringTest, TestEquals)
{
    auto same_string = std::make_shared<JString>("hello world");
    auto different_string = std::make_shared<JString>("goodbye world");

    EXPECT_TRUE(simple_string->Equals(*same_string));
    EXPECT_FALSE(simple_string->Equals(*different_string));
    EXPECT_FALSE(simple_string->Equals(*empty_string));
}

TEST_F(JStringTest, TestImplicitConversion)
{
    std::string converted = *simple_string;
    EXPECT_EQ("hello world", converted);
}

TEST_F(JStringTest, TestGetValue)
{
    const std::string& value = simple_string->GetValue();
    EXPECT_EQ("hello world", value);
}

TEST_F(JStringTest, TestMoveConstructor)
{
    std::string original = "test string";
    auto string_token = std::make_shared<JString>(std::move(original));
    EXPECT_EQ("test string", string_token->GetString());
    // original should be moved from (empty or unspecified state)
}

TEST_F(JStringTest, TestSpecialCharacterEscaping)
{
    // Test various special characters
    auto quote_string = std::make_shared<JString>("He said \"Hello\"");
    std::string json = quote_string->ToString();
    EXPECT_NE(json.find("\\\""), std::string::npos);

    auto newline_string = std::make_shared<JString>("Line 1\nLine 2");
    json = newline_string->ToString();
    EXPECT_NE(json.find("\\n"), std::string::npos);

    auto tab_string = std::make_shared<JString>("Column1\tColumn2");
    json = tab_string->ToString();
    EXPECT_NE(json.find("\\t"), std::string::npos);
}

TEST_F(JStringTest, TestUnicodeHandling)
{
    // Test Unicode characters
    auto unicode_string = std::make_shared<JString>("Hello 世界");
    EXPECT_EQ("Hello 世界", unicode_string->GetString());

    std::string json = unicode_string->ToString();
    EXPECT_NE(json.find("Hello 世界"), std::string::npos);
}
}  // namespace neo::json::tests
