#include <gtest/gtest.h>
#include <neo/io/uint160.h>

using namespace neo::io;

TEST(UInt160Test, Constructor)
{
    // Default constructor
    UInt160 u1;
    for (size_t i = 0; i < UInt160::Size; i++)
    {
        EXPECT_EQ(u1.Data()[i], 0);
    }

    // ByteSpan constructor
    uint8_t data[UInt160::Size];
    for (size_t i = 0; i < UInt160::Size; i++)
    {
        data[i] = static_cast<uint8_t>(i);
    }
    
    ByteSpan span(data, UInt160::Size);
    UInt160 u2(span);
    
    for (size_t i = 0; i < UInt160::Size; i++)
    {
        EXPECT_EQ(u2.Data()[i], i);
    }
    
    // Invalid size
    uint8_t invalidData[UInt160::Size - 1];
    ByteSpan invalidSpan(invalidData, UInt160::Size - 1);
    EXPECT_THROW(UInt160(invalidSpan), std::invalid_argument);
}

TEST(UInt160Test, AsSpan)
{
    uint8_t data[UInt160::Size];
    for (size_t i = 0; i < UInt160::Size; i++)
    {
        data[i] = static_cast<uint8_t>(i);
    }
    
    UInt160 u(ByteSpan(data, UInt160::Size));
    ByteSpan span = u.AsSpan();
    
    EXPECT_EQ(span.Size(), UInt160::Size);
    for (size_t i = 0; i < UInt160::Size; i++)
    {
        EXPECT_EQ(span[i], i);
    }
}

TEST(UInt160Test, ToHexString)
{
    uint8_t data[UInt160::Size];
    for (size_t i = 0; i < UInt160::Size; i++)
    {
        data[i] = static_cast<uint8_t>(i);
    }
    
    UInt160 u(ByteSpan(data, UInt160::Size));
    std::string hex = u.ToHexString();
    
    EXPECT_EQ(hex, "000102030405060708090a0b0c0d0e0f10111213");
}

TEST(UInt160Test, Parse)
{
    // Normal case
    UInt160 u1 = UInt160::Parse("000102030405060708090a0b0c0d0e0f10111213");
    for (size_t i = 0; i < UInt160::Size; i++)
    {
        EXPECT_EQ(u1.Data()[i], i);
    }
    
    // With 0x prefix
    UInt160 u2 = UInt160::Parse("0x000102030405060708090a0b0c0d0e0f10111213");
    for (size_t i = 0; i < UInt160::Size; i++)
    {
        EXPECT_EQ(u2.Data()[i], i);
    }
    
    // Invalid hex string (wrong length)
    EXPECT_THROW(UInt160::Parse("0001020304"), std::invalid_argument);
    
    // Invalid hex string (non-hex characters)
    EXPECT_THROW(UInt160::Parse("000102030405060708090a0b0c0d0e0f1011121G"), std::invalid_argument);
}

TEST(UInt160Test, TryParse)
{
    UInt160 u;
    
    // Normal case
    EXPECT_TRUE(UInt160::TryParse("000102030405060708090a0b0c0d0e0f10111213", u));
    for (size_t i = 0; i < UInt160::Size; i++)
    {
        EXPECT_EQ(u.Data()[i], i);
    }
    
    // With 0x prefix
    EXPECT_TRUE(UInt160::TryParse("0x000102030405060708090a0b0c0d0e0f10111213", u));
    for (size_t i = 0; i < UInt160::Size; i++)
    {
        EXPECT_EQ(u.Data()[i], i);
    }
    
    // Invalid hex string (wrong length)
    EXPECT_FALSE(UInt160::TryParse("0001020304", u));
    
    // Invalid hex string (non-hex characters)
    EXPECT_FALSE(UInt160::TryParse("000102030405060708090a0b0c0d0e0f1011121G", u));
}

TEST(UInt160Test, Equality)
{
    uint8_t data1[UInt160::Size];
    uint8_t data2[UInt160::Size];
    
    for (size_t i = 0; i < UInt160::Size; i++)
    {
        data1[i] = static_cast<uint8_t>(i);
        data2[i] = static_cast<uint8_t>(i);
    }
    
    UInt160 u1(ByteSpan(data1, UInt160::Size));
    UInt160 u2(ByteSpan(data2, UInt160::Size));
    
    data2[0] = 42;
    UInt160 u3(ByteSpan(data2, UInt160::Size));
    
    EXPECT_TRUE(u1 == u1);
    EXPECT_TRUE(u1 == u2);
    EXPECT_FALSE(u1 == u3);
    
    EXPECT_FALSE(u1 != u1);
    EXPECT_FALSE(u1 != u2);
    EXPECT_TRUE(u1 != u3);
}

TEST(UInt160Test, Comparison)
{
    UInt160 u1 = UInt160::Parse("0000000000000000000000000000000000000000");
    UInt160 u2 = UInt160::Parse("0000000000000000000000000000000000000001");
    UInt160 u3 = UInt160::Parse("0100000000000000000000000000000000000000");
    
    EXPECT_TRUE(u1 < u2);
    EXPECT_TRUE(u1 < u3);
    EXPECT_TRUE(u2 < u3);
    
    EXPECT_FALSE(u2 < u1);
    EXPECT_FALSE(u3 < u1);
    EXPECT_FALSE(u3 < u2);
}

TEST(UInt160Test, Zero)
{
    UInt160 u = UInt160::Zero();
    for (size_t i = 0; i < UInt160::Size; i++)
    {
        EXPECT_EQ(u.Data()[i], 0);
    }
}
