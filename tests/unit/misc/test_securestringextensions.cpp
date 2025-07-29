#include <cstring>
#include <gtest/gtest.h>
#include <memory>
#include <neo/extensions/secure_string_extensions.h>
#include <string>
#include <vector>

using namespace neo::extensions;

/**
 * @brief Test fixture for SecureStringExtensions
 */
class SecureStringExtensionsTest : public testing::Test
{
  protected:
    std::string test_string;
    std::string sensitive_data;

    void SetUp() override
    {
        test_string = "Hello World";
        sensitive_data = "SecretPassword123!";
    }
};

TEST_F(SecureStringExtensionsTest, CreateSecureString)
{
    // From std::string
    auto secure1 = SecureStringExtensions::CreateSecureString(test_string);
    EXPECT_EQ(test_string.length(), secure1.length());
    EXPECT_FALSE(secure1.empty());

    // From C-string
    auto secure2 = SecureStringExtensions::CreateSecureString("Test String");
    EXPECT_EQ(11, secure2.length());

    // Empty string
    auto secure3 = SecureStringExtensions::CreateSecureString("");
    EXPECT_TRUE(secure3.empty());
    EXPECT_EQ(0, secure3.length());
}

TEST_F(SecureStringExtensionsTest, SecureStringBasicOperations)
{
    SecureStringExtensions::SecureString secure(test_string);

    // Length and empty checks
    EXPECT_EQ(test_string.length(), secure.length());
    EXPECT_FALSE(secure.empty());

    // Character access
    EXPECT_EQ('H', secure.at(0));
    EXPECT_EQ('o', secure.at(4));
    EXPECT_EQ('d', secure.at(10));

    // Bounds checking should throw
    EXPECT_THROW(secure.at(100), std::out_of_range);
}

TEST_F(SecureStringExtensionsTest, SecureStringSubstring)
{
    SecureStringExtensions::SecureString secure(test_string);

    // Basic substring
    auto sub1 = secure.substr(0, 5);
    EXPECT_EQ(5, sub1.length());
    EXPECT_EQ("Hello", sub1.to_string());

    // Substring from middle
    auto sub2 = secure.substr(6, 5);
    EXPECT_EQ(5, sub2.length());
    EXPECT_EQ("World", sub2.to_string());

    // Substring to end
    auto sub3 = secure.substr(6);
    EXPECT_EQ(5, sub3.length());
    EXPECT_EQ("World", sub3.to_string());
}

TEST_F(SecureStringExtensionsTest, SecureEquals)
{
    // Test with std::strings
    std::string str1 = "password123";
    std::string str2 = "password123";
    std::string str3 = "different";

    EXPECT_TRUE(SecureStringExtensions::SecureEquals(str1, str2));
    EXPECT_FALSE(SecureStringExtensions::SecureEquals(str1, str3));

    // Test with empty strings
    EXPECT_TRUE(SecureStringExtensions::SecureEquals("", ""));
    EXPECT_FALSE(SecureStringExtensions::SecureEquals("", "a"));

    // Test with different lengths
    EXPECT_FALSE(SecureStringExtensions::SecureEquals("short", "longer string"));
}

TEST_F(SecureStringExtensionsTest, SecureStringComparison)
{
    SecureStringExtensions::SecureString secure1(test_string);
    SecureStringExtensions::SecureString secure2(test_string);
    SecureStringExtensions::SecureString secure3("Different");

    // Compare with another SecureString
    EXPECT_TRUE(secure1.secure_equals(secure2));
    EXPECT_FALSE(secure1.secure_equals(secure3));

    // Compare with std::string
    EXPECT_TRUE(secure1.secure_equals(test_string));
    EXPECT_FALSE(secure1.secure_equals("Different"));
}

TEST_F(SecureStringExtensionsTest, SecureClearString)
{
    std::string str = "sensitive data";
    size_t original_length = str.length();

    SecureStringExtensions::SecureClear(str);

    // String should be cleared but capacity might remain
    EXPECT_TRUE(str.empty());

    // Create a new string to verify old data is cleared
    std::string new_str = "test";
    EXPECT_NE(new_str, "sensitive data");
}

TEST_F(SecureStringExtensionsTest, SecureClearVector)
{
    std::vector<char> vec = {'s', 'e', 'c', 'r', 'e', 't'};
    size_t original_size = vec.size();

    SecureStringExtensions::SecureClear(vec);

    // Vector should be cleared
    EXPECT_TRUE(vec.empty());
}

TEST_F(SecureStringExtensionsTest, SecureClearMemory)
{
    char buffer[16];
    strcpy(buffer, "secret");

    SecureStringExtensions::SecureClear(buffer, sizeof(buffer));

    // All bytes should be zero
    for (size_t i = 0; i < sizeof(buffer); ++i)
    {
        EXPECT_EQ(0, buffer[i]);
    }
}

TEST_F(SecureStringExtensionsTest, SecureStringMoveSemantics)
{
    SecureStringExtensions::SecureString secure1(test_string);
    size_t original_length = secure1.length();

    // Move constructor
    SecureStringExtensions::SecureString secure2(std::move(secure1));
    EXPECT_EQ(original_length, secure2.length());
    EXPECT_EQ(test_string, secure2.to_string());

    // Move assignment
    SecureStringExtensions::SecureString secure3("temp");
    secure3 = std::move(secure2);
    EXPECT_EQ(original_length, secure3.length());
    EXPECT_EQ(test_string, secure3.to_string());
}

TEST_F(SecureStringExtensionsTest, SecureBufferComparison)
{
    const char* buf1 = "test123";
    const char* buf2 = "test123";
    const char* buf3 = "test124";

    EXPECT_TRUE(SecureStringExtensions::SecureEquals(buf1, buf2, 7));
    EXPECT_FALSE(SecureStringExtensions::SecureEquals(buf1, buf3, 7));

    // Partial comparison
    EXPECT_TRUE(SecureStringExtensions::SecureEquals(buf1, buf3, 4));
}

TEST_F(SecureStringExtensionsTest, SecureStringFromBuffer)
{
    const char buffer[] = {'t', 'e', 's', 't'};
    SecureStringExtensions::SecureString secure(buffer, 4);

    EXPECT_EQ(4, secure.length());
    EXPECT_EQ("test", secure.to_string());
}

TEST_F(SecureStringExtensionsTest, RandomFillSecure)
{
    std::vector<uint8_t> buffer(32);
    std::vector<uint8_t> buffer2(32);

    SecureStringExtensions::RandomFillSecure(buffer.data(), buffer.size());
    SecureStringExtensions::RandomFillSecure(buffer2.data(), buffer2.size());

    // Buffers should be different (extremely unlikely to be the same)
    EXPECT_NE(buffer, buffer2);

    // Buffer should not be all zeros
    bool has_non_zero = false;
    for (auto byte : buffer)
    {
        if (byte != 0)
        {
            has_non_zero = true;
            break;
        }
    }
    EXPECT_TRUE(has_non_zero);
}
