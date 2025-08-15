#include <gtest/gtest.h>
#include <neo/security/input_validator.h>

using namespace neo::security;

class InputValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test environment
    }
};

TEST_F(InputValidationTest, ValidateAddress) {
    // Valid Neo address (34 characters)
    EXPECT_TRUE(InputValidator::ValidateAddress("AQVh2pG732YvtNaxEGkQUei3YA4cvo2dCD"));
    
    // Invalid addresses
    EXPECT_FALSE(InputValidator::ValidateAddress(""));
    EXPECT_FALSE(InputValidator::ValidateAddress("invalid"));
    EXPECT_FALSE(InputValidator::ValidateAddress("AQVh2pG732YvtNaxEGkQUei3YA4cvo2d"));  // Too short
    EXPECT_FALSE(InputValidator::ValidateAddress("AQVh2pG732YvtNaxEGkQUei3YA4cvo2dCDE")); // Too long
}

TEST_F(InputValidationTest, ValidateTransactionHash) {
    // Valid transaction hash
    EXPECT_TRUE(InputValidator::ValidateTransactionHash("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"));
    
    // Invalid hashes
    EXPECT_FALSE(InputValidator::ValidateTransactionHash(""));
    EXPECT_FALSE(InputValidator::ValidateTransactionHash("0x123"));
    EXPECT_FALSE(InputValidator::ValidateTransactionHash("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef")); // Missing 0x
}

TEST_F(InputValidationTest, ValidateBlockHeight) {
    EXPECT_TRUE(InputValidator::ValidateBlockHeight(0));
    EXPECT_TRUE(InputValidator::ValidateBlockHeight(1000000));
    EXPECT_TRUE(InputValidator::ValidateBlockHeight(99999999));
    
    EXPECT_FALSE(InputValidator::ValidateBlockHeight(-1));
    EXPECT_FALSE(InputValidator::ValidateBlockHeight(100000000));
}

TEST_F(InputValidationTest, ValidateAmount) {
    EXPECT_TRUE(InputValidator::ValidateAmount("0"));
    EXPECT_TRUE(InputValidator::ValidateAmount("100.50"));
    EXPECT_TRUE(InputValidator::ValidateAmount("999999999"));
    
    EXPECT_FALSE(InputValidator::ValidateAmount("-100"));
    EXPECT_FALSE(InputValidator::ValidateAmount("1000000001"));
    EXPECT_FALSE(InputValidator::ValidateAmount("not_a_number"));
}

TEST_F(InputValidationTest, ValidateRPCMethod) {
    // Valid RPC methods
    EXPECT_TRUE(InputValidator::ValidateRPCMethod("getblock"));
    EXPECT_TRUE(InputValidator::ValidateRPCMethod("getblockcount"));
    EXPECT_TRUE(InputValidator::ValidateRPCMethod("getbalance"));
    
    // Invalid methods
    EXPECT_FALSE(InputValidator::ValidateRPCMethod(""));
    EXPECT_FALSE(InputValidator::ValidateRPCMethod("invalid_method"));
    EXPECT_FALSE(InputValidator::ValidateRPCMethod("eval")); // Dangerous method
}

TEST_F(InputValidationTest, ValidatePath) {
    EXPECT_TRUE(InputValidator::ValidatePath("/home/user/file.txt"));
    EXPECT_TRUE(InputValidator::ValidatePath("data/blockchain.db"));
    
    // Path traversal attempts
    EXPECT_FALSE(InputValidator::ValidatePath("../../../etc/passwd"));
    EXPECT_FALSE(InputValidator::ValidatePath("/home/../../../etc/passwd"));
    EXPECT_FALSE(InputValidator::ValidatePath("..\\..\\windows\\system32"));
}

TEST_F(InputValidationTest, SanitizeString) {
    EXPECT_EQ(InputValidator::SanitizeString("Hello World"), "Hello World");
    EXPECT_EQ(InputValidator::SanitizeString("<script>alert('XSS')</script>"), "scriptalert(XSS)script");
    EXPECT_EQ(InputValidator::SanitizeString("user@example.com"), "user@example.com");
}

TEST_F(InputValidationTest, SanitizeHTML) {
    EXPECT_EQ(InputValidator::SanitizeHTML("<div>Hello</div>"), "&lt;div&gt;Hello&lt;&#x2F;div&gt;");
    EXPECT_EQ(InputValidator::SanitizeHTML("Hello & Goodbye"), "Hello &amp; Goodbye");
    EXPECT_EQ(InputValidator::SanitizeHTML("\"quoted\""), "&quot;quoted&quot;");
}

TEST_F(InputValidationTest, SanitizeSQL) {
    EXPECT_EQ(InputValidator::SanitizeSQL("normal text"), "normal text");
    EXPECT_EQ(InputValidator::SanitizeSQL("O'Neill"), "O''Neill");
    EXPECT_EQ(InputValidator::SanitizeSQL("1'; DROP TABLE users--"), "1''; DROP TABLE users--");
}

TEST_F(InputValidationTest, ContainsInjectionPattern) {
    // SQL injection patterns
    EXPECT_TRUE(InputValidator::ContainsInjectionPattern("SELECT * FROM users"));
    EXPECT_TRUE(InputValidator::ContainsInjectionPattern("1' OR '1'='1"));
    EXPECT_TRUE(InputValidator::ContainsInjectionPattern("'; DROP TABLE users--"));
    
    // XSS patterns
    EXPECT_TRUE(InputValidator::ContainsInjectionPattern("<script>alert(1)</script>"));
    EXPECT_TRUE(InputValidator::ContainsInjectionPattern("javascript:alert(1)"));
    EXPECT_TRUE(InputValidator::ContainsInjectionPattern("onerror=alert(1)"));
    
    // Command injection patterns
    EXPECT_TRUE(InputValidator::ContainsInjectionPattern("ls; rm -rf /"));
    EXPECT_TRUE(InputValidator::ContainsInjectionPattern("| cat /etc/passwd"));
    EXPECT_TRUE(InputValidator::ContainsInjectionPattern("$(whoami)"));
    
    // Path traversal
    EXPECT_TRUE(InputValidator::ContainsInjectionPattern("../../../etc/passwd"));
    
    // Safe strings
    EXPECT_FALSE(InputValidator::ContainsInjectionPattern("Hello World"));
    EXPECT_FALSE(InputValidator::ContainsInjectionPattern("user@example.com"));
}

TEST_F(InputValidationTest, IsValidUTF8) {
    EXPECT_TRUE(InputValidator::IsValidUTF8("Hello World"));
    EXPECT_TRUE(InputValidator::IsValidUTF8("你好世界")); // Chinese
    EXPECT_TRUE(InputValidator::IsValidUTF8("🚀🔒")); // Emojis
    
    // Invalid UTF-8 sequences
    std::string invalid = "Hello";
    invalid.push_back(0xFF);
    invalid.push_back(0xFE);
    EXPECT_FALSE(InputValidator::IsValidUTF8(invalid));
}

TEST_F(InputValidationTest, IsSafeLength) {
    std::string short_str = "Hello";
    std::string long_str(20000, 'A');
    
    EXPECT_TRUE(InputValidator::IsSafeLength(short_str));
    EXPECT_TRUE(InputValidator::IsSafeLength(short_str, 100));
    EXPECT_FALSE(InputValidator::IsSafeLength(long_str));
    EXPECT_FALSE(InputValidator::IsSafeLength(long_str, 10000));
}