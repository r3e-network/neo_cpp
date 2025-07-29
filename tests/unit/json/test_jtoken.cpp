#include <gtest/gtest.h>
#include <neo/json/jarray.h>
#include <neo/json/jboolean.h>
#include <neo/json/jnumber.h>
#include <neo/json/jobject.h>
#include <neo/json/jstring.h>
#include <neo/json/jtoken.h>

namespace neo::json::tests
{
class JTokenTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create various token types for testing
        string_token = std::make_shared<JString>("test string");
        number_token = std::make_shared<JNumber>(42.5);
        boolean_token = std::make_shared<JBoolean>(true);
        array_token = std::make_shared<JArray>();
        object_token = std::make_shared<JObject>();
    }

    std::shared_ptr<JToken> string_token;
    std::shared_ptr<JToken> number_token;
    std::shared_ptr<JToken> boolean_token;
    std::shared_ptr<JToken> array_token;
    std::shared_ptr<JToken> object_token;
};

TEST_F(JTokenTest, TestNull)
{
    EXPECT_EQ(JToken::Null, nullptr);
}

TEST_F(JTokenTest, TestParseNull)
{
    auto null_token = JToken::Parse("null");
    EXPECT_EQ(null_token, nullptr);
}

TEST_F(JTokenTest, TestParseBoolean)
{
    auto true_token = JToken::Parse("true");
    EXPECT_NE(true_token, nullptr);
    EXPECT_EQ(JTokenType::Boolean, true_token->GetType());
    EXPECT_TRUE(true_token->AsBoolean());

    auto false_token = JToken::Parse("false");
    EXPECT_NE(false_token, nullptr);
    EXPECT_EQ(JTokenType::Boolean, false_token->GetType());
    EXPECT_FALSE(false_token->AsBoolean());
}

TEST_F(JTokenTest, TestParseNumber)
{
    auto int_token = JToken::Parse("42");
    EXPECT_NE(int_token, nullptr);
    EXPECT_EQ(JTokenType::Number, int_token->GetType());
    EXPECT_EQ(42.0, int_token->AsNumber());

    auto float_token = JToken::Parse("3.14159");
    EXPECT_NE(float_token, nullptr);
    EXPECT_EQ(JTokenType::Number, float_token->GetType());
    EXPECT_DOUBLE_EQ(3.14159, float_token->AsNumber());

    auto negative_token = JToken::Parse("-123.456");
    EXPECT_NE(negative_token, nullptr);
    EXPECT_EQ(JTokenType::Number, negative_token->GetType());
    EXPECT_DOUBLE_EQ(-123.456, negative_token->AsNumber());
}

TEST_F(JTokenTest, TestParseString)
{
    auto simple_string = JToken::Parse("\"hello world\"");
    EXPECT_NE(simple_string, nullptr);
    EXPECT_EQ(JTokenType::String, simple_string->GetType());
    EXPECT_EQ("hello world", simple_string->AsString());

    auto empty_string = JToken::Parse("\"\"");
    EXPECT_NE(empty_string, nullptr);
    EXPECT_EQ(JTokenType::String, empty_string->GetType());
    EXPECT_EQ("", empty_string->AsString());

    auto escaped_string = JToken::Parse("\"\\\"Hello\\nWorld\\\"\"");
    EXPECT_NE(escaped_string, nullptr);
    EXPECT_EQ(JTokenType::String, escaped_string->GetType());
    EXPECT_EQ("\"Hello\nWorld\"", escaped_string->AsString());
}

TEST_F(JTokenTest, TestParseArray)
{
    auto empty_array = JToken::Parse("[]");
    EXPECT_NE(empty_array, nullptr);
    EXPECT_EQ(JTokenType::Array, empty_array->GetType());

    auto simple_array = JToken::Parse("[1, 2, 3]");
    EXPECT_NE(simple_array, nullptr);
    EXPECT_EQ(JTokenType::Array, simple_array->GetType());

    auto mixed_array = JToken::Parse("[\"hello\", 42, true, null]");
    EXPECT_NE(mixed_array, nullptr);
    EXPECT_EQ(JTokenType::Array, mixed_array->GetType());

    // Test array access
    auto item0 = (*mixed_array)[0];
    EXPECT_NE(item0, nullptr);
    EXPECT_EQ("hello", item0->AsString());

    auto item1 = (*mixed_array)[1];
    EXPECT_NE(item1, nullptr);
    EXPECT_EQ(42.0, item1->AsNumber());

    auto item2 = (*mixed_array)[2];
    EXPECT_NE(item2, nullptr);
    EXPECT_TRUE(item2->AsBoolean());

    auto item3 = (*mixed_array)[3];
    EXPECT_EQ(item3, nullptr);
}

TEST_F(JTokenTest, TestParseObject)
{
    auto empty_object = JToken::Parse("{}");
    EXPECT_NE(empty_object, nullptr);
    EXPECT_EQ(JTokenType::Object, empty_object->GetType());

    auto simple_object = JToken::Parse("{\"name\": \"John\", \"age\": 30}");
    EXPECT_NE(simple_object, nullptr);
    EXPECT_EQ(JTokenType::Object, simple_object->GetType());

    // Test object access
    auto name = (*simple_object)["name"];
    EXPECT_NE(name, nullptr);
    EXPECT_EQ("John", name->AsString());

    auto age = (*simple_object)["age"];
    EXPECT_NE(age, nullptr);
    EXPECT_EQ(30.0, age->AsNumber());

    auto nonexistent = (*simple_object)["nonexistent"];
    EXPECT_EQ(nonexistent, nullptr);
}

TEST_F(JTokenTest, TestParseNestedStructures)
{
    std::string complex_json = R"({
            "users": [
                {"name": "Alice", "age": 25, "active": true},
                {"name": "Bob", "age": 30, "active": false}
            ],
            "count": 2,
            "metadata": {
                "version": "1.0",
                "created": "2023-01-01"
            }
        })";

    auto complex_object = JToken::Parse(complex_json);
    EXPECT_NE(complex_object, nullptr);
    EXPECT_EQ(JTokenType::Object, complex_object->GetType());

    // Test nested array access
    auto users = (*complex_object)["users"];
    EXPECT_NE(users, nullptr);
    EXPECT_EQ(JTokenType::Array, users->GetType());

    auto first_user = (*users)[0];
    EXPECT_NE(first_user, nullptr);
    EXPECT_EQ(JTokenType::Object, first_user->GetType());

    auto alice_name = (*first_user)["name"];
    EXPECT_NE(alice_name, nullptr);
    EXPECT_EQ("Alice", alice_name->AsString());

    // Test nested object access
    auto metadata = (*complex_object)["metadata"];
    EXPECT_NE(metadata, nullptr);
    EXPECT_EQ(JTokenType::Object, metadata->GetType());

    auto version = (*metadata)["version"];
    EXPECT_NE(version, nullptr);
    EXPECT_EQ("1.0", version->AsString());
}

TEST_F(JTokenTest, TestParseErrors)
{
    // Test various invalid JSON strings
    EXPECT_THROW(JToken::Parse(""), std::runtime_error);
    EXPECT_THROW(JToken::Parse("invalid"), std::runtime_error);
    EXPECT_THROW(JToken::Parse("{invalid}"), std::runtime_error);
    EXPECT_THROW(JToken::Parse("[1, 2,]"), std::runtime_error);
    EXPECT_THROW(JToken::Parse("{\"key\": }"), std::runtime_error);
    EXPECT_THROW(JToken::Parse("\"unterminated string"), std::runtime_error);
}

TEST_F(JTokenTest, TestGetInt32)
{
    auto int_token = JToken::Parse("42");
    EXPECT_EQ(42, int_token->GetInt32());

    auto float_token = JToken::Parse("42.0");
    EXPECT_EQ(42, float_token->GetInt32());

    // Test overflow
    auto large_token = JToken::Parse("9999999999999999999");
    EXPECT_THROW(large_token->GetInt32(), std::overflow_error);

    // Test non-integer
    auto non_int_token = JToken::Parse("42.5");
    EXPECT_THROW(non_int_token->GetInt32(), std::invalid_argument);

    // Test non-number
    EXPECT_THROW(string_token->GetInt32(), std::invalid_argument);
}

TEST_F(JTokenTest, TestImplicitConversions)
{
    // Test bool conversion
    bool bool_val = *boolean_token;
    EXPECT_TRUE(bool_val);

    // Test string conversion
    std::string str_val = *string_token;
    EXPECT_EQ("test string", str_val);

    // Test int conversion
    int int_val = *number_token;
    EXPECT_EQ(42, int_val);

    // Test double conversion
    double double_val = *number_token;
    EXPECT_DOUBLE_EQ(42.5, double_val);
}

TEST_F(JTokenTest, TestToStringFormatting)
{
    // Test compact formatting
    std::string compact = object_token->ToString(false);
    EXPECT_EQ(compact.find('\n'), std::string::npos);

    // Test indented formatting
    auto test_object = JToken::Parse("{\"key\": \"value\", \"nested\": {\"inner\": 123}}");
    std::string indented = test_object->ToString(true);
    EXPECT_NE(indented.find('\n'), std::string::npos);
    EXPECT_NE(indented.find("  "), std::string::npos);  // Should contain indentation
}
}  // namespace neo::json::tests
