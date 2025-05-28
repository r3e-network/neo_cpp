#include <iostream>
#include <gtest/gtest.h>
#include <neo/io/byte_vector.h>
#include <neo/io/byte_span.h>
#include <neo/io/byte_string.h>
#include <neo/io/fixed8.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/lru_cache.h>
#include <sstream>

using namespace neo::io;

// Test ByteVector
TEST(IOComprehensiveTest, ByteVector) {
    // Create a ByteVector
    ByteVector vector;
    EXPECT_EQ(vector.Size(), 0);

    // Add data to the ByteVector
    vector.Resize(3);
    vector[0] = 0x01;
    vector[1] = 0x02;
    vector[2] = 0x03;
    EXPECT_EQ(vector.Size(), 3);

    // Check the data
    EXPECT_EQ(vector[0], 0x01);
    EXPECT_EQ(vector[1], 0x02);
    EXPECT_EQ(vector[2], 0x03);

    // Convert to hex string
    EXPECT_EQ(vector.ToHexString(), "010203");

    // Parse from hex string
    ByteVector parsed = ByteVector::Parse("010203");
    EXPECT_EQ(parsed.Size(), 3);
    EXPECT_EQ(parsed[0], 0x01);
    EXPECT_EQ(parsed[1], 0x02);
    EXPECT_EQ(parsed[2], 0x03);

    // Append data
    ByteVector other = { 0x04, 0x05 };
    vector.Append(other.AsSpan());
    EXPECT_EQ(vector.Size(), 5);
    EXPECT_EQ(vector[3], 0x04);
    EXPECT_EQ(vector[4], 0x05);
}

// Test ByteSpan
TEST(IOComprehensiveTest, ByteSpan) {
    // Create a ByteVector
    ByteVector vector = { 0x01, 0x02, 0x03, 0x04, 0x05 };

    // Create a ByteSpan from the ByteVector
    ByteSpan span = vector.AsSpan();
    EXPECT_EQ(span.Size(), 5);

    // Check the data
    EXPECT_EQ(span[0], 0x01);
    EXPECT_EQ(span[1], 0x02);
    EXPECT_EQ(span[2], 0x03);
    EXPECT_EQ(span[3], 0x04);
    EXPECT_EQ(span[4], 0x05);

    // Create a slice
    ByteSpan slice = span.Slice(1, 3);
    EXPECT_EQ(slice.Size(), 3);
    EXPECT_EQ(slice[0], 0x02);
    EXPECT_EQ(slice[1], 0x03);
    EXPECT_EQ(slice[2], 0x04);

    // Convert to hex string
    EXPECT_EQ(slice.ToHexString(), "020304");
}

// Test ByteString
TEST(IOComprehensiveTest, ByteString) {
    // Create a ByteString
    ByteString str;
    EXPECT_EQ(str.Size(), 0);

    // Create a ByteString from a ByteVector
    ByteVector vector = { 0x01, 0x02, 0x03 };
    ByteString str2(vector);
    EXPECT_EQ(str2.Size(), 3);
    EXPECT_EQ(str2[0], 0x01);
    EXPECT_EQ(str2[1], 0x02);
    EXPECT_EQ(str2[2], 0x03);

    // Create a ByteString from a ByteSpan
    ByteSpan span = vector.AsSpan();
    ByteString str3(span);
    EXPECT_EQ(str3.Size(), 3);
    EXPECT_EQ(str3[0], 0x01);
    EXPECT_EQ(str3[1], 0x02);
    EXPECT_EQ(str3[2], 0x03);

    // Create a ByteString from an initializer list
    ByteString str4 = { 0x01, 0x02, 0x03 };
    EXPECT_EQ(str4.Size(), 3);
    EXPECT_EQ(str4[0], 0x01);
    EXPECT_EQ(str4[1], 0x02);
    EXPECT_EQ(str4[2], 0x03);

    // Convert to hex string
    EXPECT_EQ(str4.ToHexString(), "010203");

    // Parse from hex string
    ByteString parsed = ByteString::Parse("010203");
    EXPECT_EQ(parsed.Size(), 3);
    EXPECT_EQ(parsed[0], 0x01);
    EXPECT_EQ(parsed[1], 0x02);
    EXPECT_EQ(parsed[2], 0x03);
}

// Test Fixed8
TEST(IOComprehensiveTest, Fixed8) {
    // Create a Fixed8
    Fixed8 fixed8(123);
    EXPECT_EQ(fixed8.Value(), 123);

    // Create a Fixed8 from a double
    Fixed8 fixed8_2 = Fixed8::FromDouble(1.23);
    EXPECT_EQ(fixed8_2.Value(), 123000000);

    // Add two Fixed8 values
    Fixed8 fixed8_3 = fixed8 + fixed8_2;
    EXPECT_EQ(fixed8_3.Value(), 123000123);

    // Subtract two Fixed8 values
    Fixed8 fixed8_4 = fixed8_3 - fixed8;
    EXPECT_EQ(fixed8_4.Value(), 123000000);

    // Multiply two Fixed8 values
    Fixed8 fixed8_5 = fixed8 * fixed8_2;
    EXPECT_EQ(fixed8_5.Value(), 151);

    // Divide two Fixed8 values
    Fixed8 fixed8_6 = fixed8_2 / fixed8;
    EXPECT_EQ(fixed8_6.Value(), 100000000000000);

    // Convert to string
    EXPECT_EQ(fixed8_2.ToString(), "1.23");

    // Parse from string
    Fixed8 fixed8_7 = Fixed8::Parse("1.23");
    EXPECT_EQ(fixed8_7.Value(), 123000000);
}

// Test UInt160
TEST(IOComprehensiveTest, UInt160) {
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
TEST(IOComprehensiveTest, UInt256) {
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

// Test BinaryReader and BinaryWriter
TEST(IOComprehensiveTest, BinaryIO) {
    // Create a stream
    std::stringstream stream;

    // Create a BinaryWriter
    BinaryWriter writer(stream);

    // Write values
    writer.Write(true);
    writer.Write(static_cast<uint8_t>(123));
    writer.Write(static_cast<uint16_t>(12345));
    writer.Write(static_cast<uint32_t>(1234567890));
    writer.Write(static_cast<uint64_t>(1234567890123456789ULL));
    writer.Write(static_cast<int8_t>(-123));
    writer.Write(static_cast<int16_t>(-12345));
    writer.Write(static_cast<int32_t>(-1234567890));
    writer.Write(static_cast<int64_t>(-1234567890123456789LL));
    writer.Write(ByteVector({ 0x01, 0x02, 0x03 }).AsSpan());
    writer.Write(UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));
    writer.Write(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"));
    writer.Write(Fixed8::FromDouble(1.23));
    writer.WriteVarInt(123);
    writer.WriteVarBytes(ByteVector({ 0x01, 0x02, 0x03 }).AsSpan());
    writer.WriteString("Hello, World!");

    // Reset the stream
    stream.seekg(0);

    // Create a BinaryReader
    BinaryReader reader(stream);

    // Read values
    EXPECT_EQ(reader.ReadBool(), true);
    EXPECT_EQ(reader.ReadUInt8(), 123);
    EXPECT_EQ(reader.ReadUInt16(), 12345);
    EXPECT_EQ(reader.ReadUInt32(), 1234567890);
    EXPECT_EQ(reader.ReadUInt64(), 1234567890123456789ULL);
    EXPECT_EQ(reader.ReadInt8(), -123);
    EXPECT_EQ(reader.ReadInt16(), -12345);
    EXPECT_EQ(reader.ReadInt32(), -1234567890);
    EXPECT_EQ(reader.ReadInt64(), -1234567890123456789LL);
    ByteVector bytes = reader.ReadBytes(3);
    EXPECT_EQ(bytes.ToHexString(), "010203");
    UInt160 uint160 = reader.ReadUInt160();
    EXPECT_EQ(uint160.ToHexString(), "0102030405060708090a0b0c0d0e0f1011121314");
    UInt256 uint256 = reader.ReadUInt256();
    EXPECT_EQ(uint256.ToHexString(), "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    Fixed8 fixed8 = reader.ReadFixed8();
    EXPECT_EQ(fixed8.Value(), 123000000);
    EXPECT_EQ(reader.ReadVarInt(), 123);
    ByteVector varBytes = reader.ReadVarBytes();
    EXPECT_EQ(varBytes.ToHexString(), "010203");
    EXPECT_EQ(reader.ReadString(), "Hello, World!");
}

// Test LRUCache
TEST(IOComprehensiveTest, LRUCache) {
    // Create an LRUCache
    LRUCache<int, std::string> cache(3);

    // Add items
    cache.Add(1, "One");
    cache.Add(2, "Two");
    cache.Add(3, "Three");

    // Check items
    EXPECT_EQ(cache.Count(), 3);
    EXPECT_EQ(cache.TryGet(1).value(), "One");
    EXPECT_EQ(cache.TryGet(2).value(), "Two");
    EXPECT_EQ(cache.TryGet(3).value(), "Three");

    // Add a new item, which should evict the oldest item
    cache.Add(4, "Four");
    EXPECT_EQ(cache.Count(), 3);
    EXPECT_FALSE(cache.TryGet(1).has_value());
    EXPECT_EQ(cache.TryGet(2).value(), "Two");
    EXPECT_EQ(cache.TryGet(3).value(), "Three");
    EXPECT_EQ(cache.TryGet(4).value(), "Four");

    // Access an item, which should move it to the front
    EXPECT_EQ(cache.TryGet(2).value(), "Two");

    // Add a new item, which should evict the oldest item (now 3)
    cache.Add(5, "Five");
    EXPECT_EQ(cache.Count(), 3);
    EXPECT_FALSE(cache.TryGet(1).has_value());
    EXPECT_EQ(cache.TryGet(2).value(), "Two");
    EXPECT_FALSE(cache.TryGet(3).has_value());
    EXPECT_EQ(cache.TryGet(4).value(), "Four");
    EXPECT_EQ(cache.TryGet(5).value(), "Five");

    // Remove an item
    EXPECT_TRUE(cache.Remove(4));
    EXPECT_EQ(cache.Count(), 2);
    EXPECT_FALSE(cache.TryGet(4).has_value());

    // Clear the cache
    cache.Clear();
    EXPECT_EQ(cache.Count(), 0);
}

int main(int argc, char** argv) {
    std::cout << "Running IO comprehensive test..." << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
