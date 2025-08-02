#include <gtest/gtest.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

using namespace neo::io;

TEST(UIntSimpleTest, UInt160BasicTests)
{
    // Test default construction
    UInt160 u1;
    EXPECT_TRUE(u1.IsZero());
    
    // Test parsing
    UInt160 u2 = UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678");
    EXPECT_FALSE(u2.IsZero());
    
    // Test string conversion
    std::string str = u2.ToString();
    EXPECT_FALSE(str.empty());
}

TEST(UIntSimpleTest, UInt256BasicTests)
{
    // Test default construction
    UInt256 u1;
    EXPECT_TRUE(u1.IsZero());
    
    // Test Zero constant
    EXPECT_TRUE(UInt256::Zero().IsZero());
    
    // Test parsing
    UInt256 u2 = UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    EXPECT_FALSE(u2.IsZero());
    
    // Test string conversion
    std::string str = u2.ToString();
    EXPECT_FALSE(str.empty());
}