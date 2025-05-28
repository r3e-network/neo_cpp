#include <iostream>
#include <gtest/gtest.h>
#include <neo/io/byte_vector.h>
#include <neo/io/byte_span.h>
#include <neo/io/fixed8.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

using namespace neo::io;

// Test ByteVector
TEST(IOTest, ByteVector) {
    // Create a ByteVector
    ByteVector vector;
    EXPECT_EQ(vector.Size(), 0);
    EXPECT_TRUE(vector.IsEmpty());

    // Add data to the ByteVector
    vector.PushBack(0x01);
    vector.PushBack(0x02);
    vector.PushBack(0x03);
    EXPECT_EQ(vector.Size(), 3);
    EXPECT_FALSE(vector.IsEmpty());

    // Check the data
    EXPECT_EQ(vector[0], 0x01);
    EXPECT_EQ(vector[1], 0x02);
    EXPECT_EQ(vector[2], 0x03);

    // Convert to hex string
    EXPECT_EQ(vector.ToHexString(), "010203");
}

// Test ByteSpan
TEST(IOTest, ByteSpan) {
    // Create a ByteVector
    ByteVector vector;
    vector.PushBack(0x01);
    vector.PushBack(0x02);
    vector.PushBack(0x03);

    // Create a ByteSpan from the ByteVector
    ByteSpan span(vector);
    EXPECT_EQ(span.Size(), 3);

    // Check the data
    EXPECT_EQ(span[0], 0x01);
    EXPECT_EQ(span[1], 0x02);
    EXPECT_EQ(span[2], 0x03);

    // Convert to hex string
    EXPECT_EQ(span.ToHexString(), "010203");
}

// Test Fixed8
TEST(IOTest, Fixed8) {
    // Create a Fixed8
    Fixed8 fixed8(123);
    EXPECT_EQ(fixed8.GetValue(), 123);

    // Create a Fixed8 from a double
    Fixed8 fixed8_2 = Fixed8::FromDouble(1.23);
    EXPECT_EQ(fixed8_2.GetValue(), 123000000);

    // Add two Fixed8 values
    Fixed8 fixed8_3 = fixed8 + fixed8_2;
    EXPECT_EQ(fixed8_3.GetValue(), 123000123);

    // Subtract two Fixed8 values
    Fixed8 fixed8_4 = fixed8_3 - fixed8;
    EXPECT_EQ(fixed8_4.GetValue(), 123000000);

    // Multiply two Fixed8 values
    Fixed8 fixed8_5 = fixed8 * fixed8_2;
    EXPECT_EQ(fixed8_5.GetValue(), 15129);

    // Divide two Fixed8 values
    Fixed8 fixed8_6 = fixed8_2 / fixed8;
    EXPECT_EQ(fixed8_6.GetValue(), 1000000);
}

// Test UInt160
TEST(IOTest, UInt160) {
    // Create a UInt160
    UInt160 uint160 = UInt160::Zero();
    EXPECT_EQ(uint160.ToHexString(), "0000000000000000000000000000000000000000");

    // Parse a UInt160 from a hex string
    UInt160 uint160_2 = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    EXPECT_EQ(uint160_2.ToHexString(), "0102030405060708090a0b0c0d0e0f1011121314");

    // Compare UInt160 values
    EXPECT_TRUE(uint160 != uint160_2);
    EXPECT_TRUE(uint160 < uint160_2);
}

// Test UInt256
TEST(IOTest, UInt256) {
    // Create a UInt256
    UInt256 uint256 = UInt256::Zero();
    EXPECT_EQ(uint256.ToHexString(), "0000000000000000000000000000000000000000000000000000000000000000");

    // Parse a UInt256 from a hex string
    UInt256 uint256_2 = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    EXPECT_EQ(uint256_2.ToHexString(), "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");

    // Compare UInt256 values
    EXPECT_TRUE(uint256 != uint256_2);
    EXPECT_TRUE(uint256 < uint256_2);
}

int main(int argc, char** argv) {
    std::cout << "Running IO test..." << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
