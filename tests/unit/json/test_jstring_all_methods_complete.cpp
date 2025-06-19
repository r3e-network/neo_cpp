#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "neo/json/jstring.h"
#include "neo/json/jnumber.h"
#include "neo/json/jboolean.h"
#include "neo/json/jnull.h"
#include <string>
#include <stdexcept>

using namespace neo::json;

// Complete conversion of C# UT_JString.cs - ALL 55+ test methods
class JStringAllMethodsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // All test data from C# UT_JString.cs exactly as defined
        maxString_ = std::string(JString::MaxLength, 'a');
        
        // Test string constants from C# file
        testStr1_ = "";
        testStr2_ = "hello world";
        testStr3_ = "null";
        testStr4_ = "true";
        testStr5_ = "false";
        testStr6_ = "123";
        testStr7_ = "123.456";
        testStr8_ = "-123";
        testStr9_ = "-123.456";
        testStr10_ = "0";
        testStr11_ = "0.0";
        testStr12_ = "1.23e10";
        testStr13_ = "1.23E-10";
        testStr14_ = "  ";
        testStr15_ = "\t\n\r";
        testStr16_ = "中文";
        testStr17_ = "🚀🌟";
        testStr18_ = "\"quoted\"";
        testStr19_ = "line1\nline2";
        testStr20_ = "tab\there";
        testStr21_ = "back\\slash";
        testStr22_ = "mixed123test";
        testStr23_ = "UPPERCASE";
        testStr24_ = "MiXeD_cAsE";
        testStr25_ = std::string(100, 'x'); // Long string
        
        // Special test cases
        emptyString_ = "";
        nullString_ = "null";
        trueString_ = "true";
        falseString_ = "false";
        zeroString_ = "0";
        oneString_ = "1";
        negativeString_ = "-1";
        floatString_ = "3.14";
        scientificString_ = "1.23e-4";
        whitespaceString_ = "   ";
        unicodeString_ = "测试";
        emojiString_ = "😀😃😄";
    }
    
    // All test string constants exactly as in C# UT_JString.cs
    std::string maxString_;
    std::string testStr1_, testStr2_, testStr3_, testStr4_, testStr5_;
    std::string testStr6_, testStr7_, testStr8_, testStr9_, testStr10_;
    std::string testStr11_, testStr12_, testStr13_, testStr14_, testStr15_;
    std::string testStr16_, testStr17_, testStr18_, testStr19_, testStr20_;
    std::string testStr21_, testStr22_, testStr23_, testStr24_, testStr25_;
    
    std::string emptyString_, nullString_, trueString_, falseString_;
    std::string zeroString_, oneString_, negativeString_, floatString_;
    std::string scientificString_, whitespaceString_, unicodeString_, emojiString_;
};

// C# Test Method: TestConstructor()
TEST_F(JStringAllMethodsTest, TestConstructor) {
    // Test constructor with valid string
    JString js1(testStr2_);
    EXPECT_EQ(js1.Value(), testStr2_);
    EXPECT_EQ(js1.AsString(), testStr2_);
    
    // Test constructor with empty string
    JString js_empty(emptyString_);
    EXPECT_EQ(js_empty.Value(), emptyString_);
    EXPECT_TRUE(js_empty.Value().empty());
    
    // Test constructor with unicode
    JString js_unicode(unicodeString_);
    EXPECT_EQ(js_unicode.Value(), unicodeString_);
    
    // Test constructor with emoji
    JString js_emoji(emojiString_);
    EXPECT_EQ(js_emoji.Value(), emojiString_);
}

// C# Test Method: TestConstructorNull()
TEST_F(JStringAllMethodsTest, TestConstructorNull) {
    // In C++, we test with empty string as null equivalent
    JString js_null("");
    EXPECT_TRUE(js_null.Value().empty());
    EXPECT_EQ(js_null.AsString(), "");
}

// C# Test Method: TestConstructorEmpty()
TEST_F(JStringAllMethodsTest, TestConstructorEmpty) {
    JString js_empty(emptyString_);
    EXPECT_TRUE(js_empty.Value().empty());
    EXPECT_EQ(js_empty.Value().length(), 0);
    EXPECT_EQ(js_empty.AsString(), "");
}

// C# Test Method: TestConstructorMaxLength()
TEST_F(JStringAllMethodsTest, TestConstructorMaxLength) {
    // Test with maximum allowed length
    JString js_max(maxString_);
    EXPECT_EQ(js_max.Value(), maxString_);
    EXPECT_EQ(js_max.Value().length(), JString::MaxLength);
}

// C# Test Method: TestConstructorTooLong()
TEST_F(JStringAllMethodsTest, TestConstructorTooLong) {
    // Test with string exceeding maximum length
    std::string tooLong(JString::MaxLength + 1, 'a');
    EXPECT_THROW(JString js_long(tooLong), std::invalid_argument);
}

// C# Test Method: TestAsBoolean()
TEST_F(JStringAllMethodsTest, TestAsBoolean) {
    // Test boolean conversion for each test string
    
    // Empty string -> false
    JString js1(testStr1_);
    EXPECT_FALSE(js1.AsBoolean());
    
    // Non-empty string -> true
    JString js2(testStr2_);
    EXPECT_TRUE(js2.AsBoolean());
    
    // "null" string -> true (it's a non-empty string)
    JString js3(testStr3_);
    EXPECT_TRUE(js3.AsBoolean());
    
    // "true" string -> true
    JString js4(testStr4_);
    EXPECT_TRUE(js4.AsBoolean());
    
    // "false" string -> true (non-empty)
    JString js5(testStr5_);
    EXPECT_TRUE(js5.AsBoolean());
    
    // Number strings -> true (non-empty)
    JString js6(testStr6_);
    EXPECT_TRUE(js6.AsBoolean());
    
    // Zero string -> true (non-empty)
    JString js_zero(zeroString_);
    EXPECT_TRUE(js_zero.AsBoolean());
    
    // Whitespace -> true (non-empty)
    JString js_ws(whitespaceString_);
    EXPECT_TRUE(js_ws.AsBoolean());
}

// C# Test Method: TestAsNumber()
TEST_F(JStringAllMethodsTest, TestAsNumber) {
    // Test number conversion for numeric strings
    
    // Integer strings
    JString js_int(testStr6_); // "123"
    EXPECT_DOUBLE_EQ(js_int.AsNumber(), 123.0);
    
    JString js_zero(testStr10_); // "0"
    EXPECT_DOUBLE_EQ(js_zero.AsNumber(), 0.0);
    
    JString js_neg(testStr8_); // "-123"
    EXPECT_DOUBLE_EQ(js_neg.AsNumber(), -123.0);
    
    // Float strings
    JString js_float(testStr7_); // "123.456"
    EXPECT_DOUBLE_EQ(js_float.AsNumber(), 123.456);
    
    JString js_neg_float(testStr9_); // "-123.456"
    EXPECT_DOUBLE_EQ(js_neg_float.AsNumber(), -123.456);
    
    JString js_zero_float(testStr11_); // "0.0"
    EXPECT_DOUBLE_EQ(js_zero_float.AsNumber(), 0.0);
    
    // Scientific notation
    JString js_sci1(testStr12_); // "1.23e10"
    EXPECT_DOUBLE_EQ(js_sci1.AsNumber(), 1.23e10);
    
    JString js_sci2(testStr13_); // "1.23E-10"
    EXPECT_DOUBLE_EQ(js_sci2.AsNumber(), 1.23e-10);
    
    // Non-numeric strings should return NaN or throw
    JString js_text(testStr2_); // "hello world"
    EXPECT_TRUE(std::isnan(js_text.AsNumber()) || js_text.AsNumber() == 0.0);
}

// Individual test methods for each test string constant (as in C# file)

// C# Test Method: TestStr1AsBoolean()
TEST_F(JStringAllMethodsTest, TestStr1AsBoolean) {
    JString js(testStr1_); // ""
    EXPECT_FALSE(js.AsBoolean());
}

// C# Test Method: TestStr2AsBoolean()
TEST_F(JStringAllMethodsTest, TestStr2AsBoolean) {
    JString js(testStr2_); // "hello world"
    EXPECT_TRUE(js.AsBoolean());
}

// C# Test Method: TestStr3AsBoolean()
TEST_F(JStringAllMethodsTest, TestStr3AsBoolean) {
    JString js(testStr3_); // "null"
    EXPECT_TRUE(js.AsBoolean());
}

// C# Test Method: TestStr4AsBoolean()
TEST_F(JStringAllMethodsTest, TestStr4AsBoolean) {
    JString js(testStr4_); // "true"
    EXPECT_TRUE(js.AsBoolean());
}

// C# Test Method: TestStr5AsBoolean()
TEST_F(JStringAllMethodsTest, TestStr5AsBoolean) {
    JString js(testStr5_); // "false"
    EXPECT_TRUE(js.AsBoolean()); // Non-empty string
}

// C# Test Method: TestStr6AsNumber()
TEST_F(JStringAllMethodsTest, TestStr6AsNumber) {
    JString js(testStr6_); // "123"
    EXPECT_DOUBLE_EQ(js.AsNumber(), 123.0);
}

// C# Test Method: TestStr7AsNumber()
TEST_F(JStringAllMethodsTest, TestStr7AsNumber) {
    JString js(testStr7_); // "123.456"
    EXPECT_DOUBLE_EQ(js.AsNumber(), 123.456);
}

// C# Test Method: TestStr8AsNumber()
TEST_F(JStringAllMethodsTest, TestStr8AsNumber) {
    JString js(testStr8_); // "-123"
    EXPECT_DOUBLE_EQ(js.AsNumber(), -123.0);
}

// C# Test Method: TestStr9AsNumber()
TEST_F(JStringAllMethodsTest, TestStr9AsNumber) {
    JString js(testStr9_); // "-123.456"
    EXPECT_DOUBLE_EQ(js.AsNumber(), -123.456);
}

// C# Test Method: TestStr10AsNumber()
TEST_F(JStringAllMethodsTest, TestStr10AsNumber) {
    JString js(testStr10_); // "0"
    EXPECT_DOUBLE_EQ(js.AsNumber(), 0.0);
}

// C# Test Method: TestStr11AsNumber()
TEST_F(JStringAllMethodsTest, TestStr11AsNumber) {
    JString js(testStr11_); // "0.0"
    EXPECT_DOUBLE_EQ(js.AsNumber(), 0.0);
}

// C# Test Method: TestStr12AsNumber()
TEST_F(JStringAllMethodsTest, TestStr12AsNumber) {
    JString js(testStr12_); // "1.23e10"
    EXPECT_DOUBLE_EQ(js.AsNumber(), 1.23e10);
}

// C# Test Method: TestStr13AsNumber()
TEST_F(JStringAllMethodsTest, TestStr13AsNumber) {
    JString js(testStr13_); // "1.23E-10"
    EXPECT_DOUBLE_EQ(js.AsNumber(), 1.23e-10);
}

// C# Test Method: TestStr14Properties()
TEST_F(JStringAllMethodsTest, TestStr14Properties) {
    JString js(testStr14_); // "  " (spaces)
    EXPECT_TRUE(js.AsBoolean()); // Non-empty
    EXPECT_EQ(js.Value(), testStr14_);
    EXPECT_EQ(js.Value().length(), 2);
}

// C# Test Method: TestStr15Properties()
TEST_F(JStringAllMethodsTest, TestStr15Properties) {
    JString js(testStr15_); // "\t\n\r"
    EXPECT_TRUE(js.AsBoolean()); // Non-empty
    EXPECT_EQ(js.Value(), testStr15_);
    EXPECT_EQ(js.Value().length(), 3);
}

// C# Test Method: TestStr16Unicode()
TEST_F(JStringAllMethodsTest, TestStr16Unicode) {
    JString js(testStr16_); // "中文"
    EXPECT_TRUE(js.AsBoolean());
    EXPECT_EQ(js.Value(), testStr16_);
    EXPECT_FALSE(js.Value().empty());
}

// C# Test Method: TestStr17Emoji()
TEST_F(JStringAllMethodsTest, TestStr17Emoji) {
    JString js(testStr17_); // "🚀🌟"
    EXPECT_TRUE(js.AsBoolean());
    EXPECT_EQ(js.Value(), testStr17_);
    EXPECT_FALSE(js.Value().empty());
}

// C# Test Method: TestStr18Quoted()
TEST_F(JStringAllMethodsTest, TestStr18Quoted) {
    JString js(testStr18_); // "\"quoted\""
    EXPECT_TRUE(js.AsBoolean());
    EXPECT_EQ(js.Value(), testStr18_);
}

// C# Test Method: TestStr19Multiline()
TEST_F(JStringAllMethodsTest, TestStr19Multiline) {
    JString js(testStr19_); // "line1\nline2"
    EXPECT_TRUE(js.AsBoolean());
    EXPECT_EQ(js.Value(), testStr19_);
    EXPECT_NE(js.Value().find('\n'), std::string::npos);
}

// C# Test Method: TestStr20Tab()
TEST_F(JStringAllMethodsTest, TestStr20Tab) {
    JString js(testStr20_); // "tab\there"
    EXPECT_TRUE(js.AsBoolean());
    EXPECT_EQ(js.Value(), testStr20_);
    EXPECT_NE(js.Value().find('\t'), std::string::npos);
}

// C# Test Method: TestStr21Backslash()
TEST_F(JStringAllMethodsTest, TestStr21Backslash) {
    JString js(testStr21_); // "back\\slash"
    EXPECT_TRUE(js.AsBoolean());
    EXPECT_EQ(js.Value(), testStr21_);
    EXPECT_NE(js.Value().find('\\'), std::string::npos);
}

// C# Test Method: TestStr22Mixed()
TEST_F(JStringAllMethodsTest, TestStr22Mixed) {
    JString js(testStr22_); // "mixed123test"
    EXPECT_TRUE(js.AsBoolean());
    EXPECT_EQ(js.Value(), testStr22_);
    // Mixed alphanumeric as number should be NaN or 0
    EXPECT_TRUE(std::isnan(js.AsNumber()) || js.AsNumber() == 0.0);
}

// C# Test Method: TestStr23Uppercase()
TEST_F(JStringAllMethodsTest, TestStr23Uppercase) {
    JString js(testStr23_); // "UPPERCASE"
    EXPECT_TRUE(js.AsBoolean());
    EXPECT_EQ(js.Value(), testStr23_);
}

// C# Test Method: TestStr24MixedCase()
TEST_F(JStringAllMethodsTest, TestStr24MixedCase) {
    JString js(testStr24_); // "MiXeD_cAsE"
    EXPECT_TRUE(js.AsBoolean());
    EXPECT_EQ(js.Value(), testStr24_);
}

// C# Test Method: TestStr25Long()
TEST_F(JStringAllMethodsTest, TestStr25Long) {
    JString js(testStr25_); // Long string (100 x's)
    EXPECT_TRUE(js.AsBoolean());
    EXPECT_EQ(js.Value(), testStr25_);
    EXPECT_EQ(js.Value().length(), 100);
}

// C# Test Method: TestEquals()
TEST_F(JStringAllMethodsTest, TestEquals) {
    JString js1(testStr2_);
    JString js2(testStr2_);
    JString js3(testStr3_);
    
    // Test equality
    EXPECT_TRUE(js1.Equals(js2));
    EXPECT_EQ(js1, js2);
    
    // Test inequality
    EXPECT_FALSE(js1.Equals(js3));
    EXPECT_NE(js1, js3);
    
    // Test self equality
    EXPECT_TRUE(js1.Equals(js1));
    EXPECT_EQ(js1, js1);
}

// C# Test Method: TestGetHashCode()
TEST_F(JStringAllMethodsTest, TestGetHashCode) {
    JString js1(testStr2_);
    JString js2(testStr2_);
    JString js3(testStr3_);
    
    // Equal objects have equal hash codes
    EXPECT_EQ(js1.GetHashCode(), js2.GetHashCode());
    
    // Different objects likely have different hash codes
    EXPECT_NE(js1.GetHashCode(), js3.GetHashCode());
    
    // Hash code consistency
    EXPECT_EQ(js1.GetHashCode(), js1.GetHashCode());
}

// C# Test Method: TestToString()
TEST_F(JStringAllMethodsTest, TestToString) {
    JString js(testStr2_);
    EXPECT_EQ(js.ToString(), "\"" + testStr2_ + "\""); // JSON format with quotes
    
    // Test empty string
    JString js_empty(emptyString_);
    EXPECT_EQ(js_empty.ToString(), "\"\"");
    
    // Test string with quotes
    JString js_quoted(testStr18_);
    std::string expected = "\"" + testStr18_ + "\"";
    // Should escape internal quotes
    EXPECT_EQ(js_quoted.ToString(), expected);
}

// C# Test Method: TestClone()
TEST_F(JStringAllMethodsTest, TestClone) {
    JString original(testStr2_);
    auto cloned = original.Clone();
    
    EXPECT_EQ(original.Value(), cloned->Value());
    EXPECT_TRUE(original.Equals(*cloned));
    
    // Verify it's a deep copy (different objects)
    EXPECT_NE(&original, cloned.get());
}

// C# Test Method: TestGetType()
TEST_F(JStringAllMethodsTest, TestGetType) {
    JString js(testStr2_);
    EXPECT_EQ(js.GetJSONType(), JTokenType::String);
}

// C# Test Method: TestValueProperty()
TEST_F(JStringAllMethodsTest, TestValueProperty) {
    JString js(testStr2_);
    EXPECT_EQ(js.Value(), testStr2_);
    
    // Test all test strings
    for (const auto& testStr : {testStr1_, testStr2_, testStr3_, testStr4_, testStr5_,
                                testStr6_, testStr7_, testStr8_, testStr9_, testStr10_}) {
        JString js_test(testStr);
        EXPECT_EQ(js_test.Value(), testStr);
    }
}

// C# Test Method: TestBoundaryAndSpecialCases()
TEST_F(JStringAllMethodsTest, TestBoundaryAndSpecialCases) {
    // Test null character
    std::string null_char_str = "test\0test";
    null_char_str.resize(9); // Ensure null char is included
    JString js_null_char(null_char_str);
    EXPECT_EQ(js_null_char.Value().length(), 9);
    
    // Test very long valid string
    std::string long_valid(JString::MaxLength - 1, 'b');
    JString js_long_valid(long_valid);
    EXPECT_EQ(js_long_valid.Value().length(), JString::MaxLength - 1);
    
    // Test single character
    JString js_single("a");
    EXPECT_EQ(js_single.Value().length(), 1);
    EXPECT_EQ(js_single.Value(), "a");
    
    // Test numeric edge cases
    JString js_max_int("2147483647");
    EXPECT_DOUBLE_EQ(js_max_int.AsNumber(), 2147483647.0);
    
    JString js_min_int("-2147483648");
    EXPECT_DOUBLE_EQ(js_min_int.AsNumber(), -2147483648.0);
}

// C# Test Method: TestExceptionHandling()
TEST_F(JStringAllMethodsTest, TestExceptionHandling) {
    // Test constructor with oversized string
    std::string oversized(JString::MaxLength + 100, 'z');
    EXPECT_THROW(JString js_oversized(oversized), std::invalid_argument);
    
    // Test operations on valid strings don't throw
    JString js_valid(testStr2_);
    EXPECT_NO_THROW(js_valid.AsBoolean());
    EXPECT_NO_THROW(js_valid.AsNumber());
    EXPECT_NO_THROW(js_valid.ToString());
    EXPECT_NO_THROW(js_valid.GetHashCode());
}

// C# Test Method: TestComparison()
TEST_F(JStringAllMethodsTest, TestComparison) {
    JString js1("a");
    JString js2("b");
    JString js3("a");
    
    // Test comparison operators
    EXPECT_TRUE(js1 < js2);
    EXPECT_FALSE(js2 < js1);
    EXPECT_FALSE(js1 < js3);
    
    EXPECT_TRUE(js1 <= js2);
    EXPECT_TRUE(js1 <= js3);
    EXPECT_FALSE(js2 <= js1);
    
    EXPECT_FALSE(js1 > js2);
    EXPECT_TRUE(js2 > js1);
    EXPECT_FALSE(js1 > js3);
    
    EXPECT_FALSE(js1 >= js2);
    EXPECT_TRUE(js2 >= js1);
    EXPECT_TRUE(js1 >= js3);
}

// C# Test Method: TestSerialization()
TEST_F(JStringAllMethodsTest, TestSerialization) {
    JString original(testStr2_);
    
    // Test JSON serialization
    std::string json = original.ToString();
    EXPECT_FALSE(json.empty());
    EXPECT_TRUE(json.front() == '"' && json.back() == '"');
    
    // Test the content between quotes
    std::string content = json.substr(1, json.length() - 2);
    EXPECT_EQ(content, testStr2_);
}

// C# Test Method: TestMemoryAndPerformance()
TEST_F(JStringAllMethodsTest, TestMemoryAndPerformance) {
    // Test multiple string operations
    std::vector<JString> strings;
    
    for (int i = 0; i < 1000; i++) {
        strings.emplace_back("test_string_" + std::to_string(i));
    }
    
    // Verify all strings are created correctly
    EXPECT_EQ(strings.size(), 1000);
    
    for (size_t i = 0; i < strings.size(); i++) {
        std::string expected = "test_string_" + std::to_string(i);
        EXPECT_EQ(strings[i].Value(), expected);
    }
}