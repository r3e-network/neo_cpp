#include <gtest/gtest.h>
#include <neo/io/byte_vector.h>

using namespace neo::io;

TEST(ByteVectorTest, Constructor)
{
    // Default constructor
    ByteVector v1;
    EXPECT_EQ(v1.Size(), 0);

    // Size constructor
    ByteVector v2(5);
    EXPECT_EQ(v2.Size(), 5);

    // ByteSpan constructor
    uint8_t data[] = {1, 2, 3, 4, 5};
    ByteSpan span(data, 5);
    ByteVector v3(span);
    EXPECT_EQ(v3.Size(), 5);
    EXPECT_EQ(v3[0], 1);
    EXPECT_EQ(v3[4], 5);

    // Initializer list constructor
    ByteVector v4 = {1, 2, 3, 4, 5};
    EXPECT_EQ(v4.Size(), 5);
    EXPECT_EQ(v4[0], 1);
    EXPECT_EQ(v4[4], 5);
}

TEST(ByteVectorTest, AccessOperators)
{
    ByteVector v = {1, 2, 3, 4, 5};

    // Const access
    const ByteVector& cv = v;
    EXPECT_EQ(cv[0], 1);
    EXPECT_EQ(cv[4], 5);

    // Non-const access
    v[0] = 10;
    EXPECT_EQ(v[0], 10);
}

TEST(ByteVectorTest, Resize)
{
    ByteVector v = {1, 2, 3};
    EXPECT_EQ(v.Size(), 3);

    v.Resize(5);
    EXPECT_EQ(v.Size(), 5);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[2], 3);

    v.Resize(2);
    EXPECT_EQ(v.Size(), 2);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
}

TEST(ByteVectorTest, Append)
{
    ByteVector v1 = {1, 2, 3};
    ByteVector v2 = {4, 5, 6};

    v1.Append(v2.AsSpan());
    EXPECT_EQ(v1.Size(), 6);
    EXPECT_EQ(v1[0], 1);
    EXPECT_EQ(v1[3], 4);
    EXPECT_EQ(v1[5], 6);
}

TEST(ByteVectorTest, AsSpan)
{
    ByteVector v = {1, 2, 3, 4, 5};
    ByteSpan span = v.AsSpan();

    EXPECT_EQ(span.Size(), 5);
    EXPECT_EQ(span[0], 1);
    EXPECT_EQ(span[4], 5);
}

TEST(ByteVectorTest, ToHexString)
{
    ByteVector v = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    EXPECT_EQ(v.ToHexString(), "0123456789abcdef");
}

TEST(ByteVectorTest, Parse)
{
    // Normal case
    ByteVector v1 = ByteVector::Parse("0123456789abcdef");
    EXPECT_EQ(v1.Size(), 8);
    EXPECT_EQ(v1[0], 0x01);
    EXPECT_EQ(v1[7], 0xEF);

    // With 0x prefix
    ByteVector v2 = ByteVector::Parse("0x0123456789abcdef");
    EXPECT_EQ(v2.Size(), 8);
    EXPECT_EQ(v2[0], 0x01);
    EXPECT_EQ(v2[7], 0xEF);

    // Empty string
    ByteVector v3 = ByteVector::Parse("");
    EXPECT_EQ(v3.Size(), 0);

    // Invalid hex string (odd length)
    EXPECT_THROW(ByteVector::Parse("123"), std::invalid_argument);

    // Invalid hex string (non-hex characters)
    EXPECT_THROW(ByteVector::Parse("123G"), std::invalid_argument);
}

TEST(ByteVectorTest, Equality)
{
    ByteVector v1 = {1, 2, 3};
    ByteVector v2 = {1, 2, 3};
    ByteVector v3 = {1, 2, 4};
    ByteVector v4 = {1, 2};

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 == v3);
    EXPECT_FALSE(v1 == v4);

    EXPECT_FALSE(v1 != v2);
    EXPECT_TRUE(v1 != v3);
    EXPECT_TRUE(v1 != v4);
}
