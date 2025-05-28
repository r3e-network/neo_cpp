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
#include <stdexcept>

using namespace neo::io;

// Test ByteVector
TEST(IOComprehensiveTest, ByteVector) {
    // Default constructor
    ByteVector v1;
    EXPECT_EQ(v1.Size(), 0);

    // Size constructor
    ByteVector v2(5);
    EXPECT_EQ(v2.Size(), 5);

    // Add data to the ByteVector
    v1.Resize(3);
    v1[0] = 0x01;
    v1[1] = 0x02;
    v1[2] = 0x03;
    EXPECT_EQ(v1.Size(), 3);

    // Check the data
    EXPECT_EQ(v1[0], 0x01);
    EXPECT_EQ(v1[1], 0x02);
    EXPECT_EQ(v1[2], 0x03);

    // Convert to hex string
    EXPECT_EQ(v1.ToHexString(), "010203");

    // Parse from hex string
    ByteVector parsed = ByteVector::Parse("010203");
    EXPECT_EQ(parsed.Size(), 3);
    EXPECT_EQ(parsed[0], 0x01);
    EXPECT_EQ(parsed[1], 0x02);
    EXPECT_EQ(parsed[2], 0x03);

    // Parse with 0x prefix
    ByteVector parsed2 = ByteVector::Parse("0x010203");
    EXPECT_EQ(parsed2.Size(), 3);
    EXPECT_EQ(parsed2[0], 0x01);
    EXPECT_EQ(parsed2[1], 0x02);
    EXPECT_EQ(parsed2[2], 0x03);

    // Empty string
    ByteVector parsed3 = ByteVector::Parse("");
    EXPECT_EQ(parsed3.Size(), 0);

    // Invalid hex string (odd length)
    EXPECT_THROW(ByteVector::Parse("123"), std::invalid_argument);

    // Invalid hex string (non-hex characters)
    EXPECT_THROW(ByteVector::Parse("123G"), std::invalid_argument);

    // Append data
    ByteVector other = { 0x04, 0x05 };
    v1.Append(other.AsSpan());
    EXPECT_EQ(v1.Size(), 5);
    EXPECT_EQ(v1[3], 0x04);
    EXPECT_EQ(v1[4], 0x05);

    // Equality
    ByteVector v3 = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    ByteVector v4 = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    ByteVector v5 = { 0x01, 0x02, 0x03, 0x04, 0x06 };
    ByteVector v6 = { 0x01, 0x02, 0x03, 0x04 };

    EXPECT_TRUE(v3 == v4);
    EXPECT_FALSE(v3 == v5);
    EXPECT_FALSE(v3 == v6);

    EXPECT_FALSE(v3 != v4);
    EXPECT_TRUE(v3 != v5);
    EXPECT_TRUE(v3 != v6);
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

    // Test empty span
    ByteVector emptyVector;
    ByteSpan emptySpan = emptyVector.AsSpan();
    EXPECT_EQ(emptySpan.Size(), 0);

    // Test equality
    ByteVector vector2 = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    ByteSpan span2 = vector2.AsSpan();

    EXPECT_TRUE(span == span2);
    EXPECT_FALSE(span != span2);

    ByteVector vector3 = { 0x01, 0x02, 0x03, 0x04, 0x06 };
    ByteSpan span3 = vector3.AsSpan();

    EXPECT_FALSE(span == span3);
    EXPECT_TRUE(span != span3);
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

    // Test equality
    ByteString str5 = { 0x01, 0x02, 0x03 };
    EXPECT_TRUE(str4 == str5);
    EXPECT_FALSE(str4 != str5);

    ByteString str6 = { 0x01, 0x02, 0x04 };
    EXPECT_FALSE(str4 == str6);
    EXPECT_TRUE(str4 != str6);
}

// Test UInt160
TEST(IOComprehensiveTest, UInt160) {
    // Create a UInt160
    UInt160 uint160 = UInt160::Zero();
    EXPECT_EQ(uint160.ToHexString(), "0000000000000000000000000000000000000000");

    // Parse a UInt160 from a hex string
    UInt160 uint160_2 = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    EXPECT_EQ(uint160_2.ToHexString(), "0102030405060708090a0b0c0d0e0f1011121314");

    // Parse with 0x prefix
    UInt160 uint160_3 = UInt160::Parse("0x0102030405060708090a0b0c0d0e0f1011121314");
    EXPECT_EQ(uint160_3.ToHexString(), "0102030405060708090a0b0c0d0e0f1011121314");

    // Invalid hex string (wrong length)
    EXPECT_THROW(UInt160::Parse("0001020304"), std::invalid_argument);

    // Invalid hex string (non-hex characters)
    EXPECT_THROW(UInt160::Parse("0102030405060708090a0b0c0d0e0f101112131G"), std::invalid_argument);

    // Try parse
    UInt160 uint160_4;
    EXPECT_TRUE(UInt160::TryParse("0102030405060708090a0b0c0d0e0f1011121314", uint160_4));
    EXPECT_EQ(uint160_4.ToHexString(), "0102030405060708090a0b0c0d0e0f1011121314");

    EXPECT_FALSE(UInt160::TryParse("0001020304", uint160_4));
    EXPECT_FALSE(UInt160::TryParse("0102030405060708090a0b0c0d0e0f101112131G", uint160_4));

    // Compare UInt160 values
    UInt160 uint160_5 = UInt160::Parse("0000000000000000000000000000000000000000");
    UInt160 uint160_6 = UInt160::Parse("0000000000000000000000000000000000000001");
    UInt160 uint160_7 = UInt160::Parse("0100000000000000000000000000000000000000");

    EXPECT_TRUE(uint160_5 < uint160_6);
    EXPECT_TRUE(uint160_5 < uint160_7);
    EXPECT_TRUE(uint160_6 < uint160_7);

    EXPECT_FALSE(uint160_6 < uint160_5);
    EXPECT_FALSE(uint160_7 < uint160_5);
    EXPECT_FALSE(uint160_7 < uint160_6);

    EXPECT_TRUE(uint160_5 == UInt160::Zero());
    EXPECT_FALSE(uint160_6 == UInt160::Zero());
}

// Test UInt256
TEST(IOComprehensiveTest, UInt256) {
    // Create a UInt256
    UInt256 uint256 = UInt256::Zero();
    EXPECT_EQ(uint256.ToHexString(), "0000000000000000000000000000000000000000000000000000000000000000");

    // Parse a UInt256 from a hex string
    UInt256 uint256_2 = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    EXPECT_EQ(uint256_2.ToHexString(), "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");

    // Parse with 0x prefix
    UInt256 uint256_3 = UInt256::Parse("0x0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    EXPECT_EQ(uint256_3.ToHexString(), "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");

    // Invalid hex string (wrong length)
    EXPECT_THROW(UInt256::Parse("0001020304"), std::invalid_argument);

    // Invalid hex string (non-hex characters)
    EXPECT_THROW(UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f2G"), std::invalid_argument);

    // Try parse
    UInt256 uint256_4;
    EXPECT_TRUE(UInt256::TryParse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20", uint256_4));
    EXPECT_EQ(uint256_4.ToHexString(), "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");

    EXPECT_FALSE(UInt256::TryParse("0001020304", uint256_4));
    EXPECT_FALSE(UInt256::TryParse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f2G", uint256_4));

    // Compare UInt256 values
    UInt256 uint256_5 = UInt256::Parse("0000000000000000000000000000000000000000000000000000000000000000");
    UInt256 uint256_6 = UInt256::Parse("0000000000000000000000000000000000000000000000000000000000000001");
    UInt256 uint256_7 = UInt256::Parse("0100000000000000000000000000000000000000000000000000000000000000");

    EXPECT_TRUE(uint256_5 < uint256_6);
    EXPECT_TRUE(uint256_5 < uint256_7);
    EXPECT_TRUE(uint256_6 < uint256_7);

    EXPECT_FALSE(uint256_6 < uint256_5);
    EXPECT_FALSE(uint256_7 < uint256_5);
    EXPECT_FALSE(uint256_7 < uint256_6);

    EXPECT_TRUE(uint256_5 == UInt256::Zero());
    EXPECT_FALSE(uint256_6 == UInt256::Zero());
}

// Test Fixed8
TEST(IOComprehensiveTest, Fixed8) {
    // Create a Fixed8
    Fixed8 fixed8(123);
    EXPECT_EQ(fixed8.Value(), 123);

    // Create a Fixed8 from a double
    Fixed8 fixed8_2 = Fixed8::FromDouble(1.23);
    EXPECT_EQ(fixed8_2.Value(), 123000000);

    // Create a Fixed8 from a decimal
    Fixed8 fixed8_3 = Fixed8::FromDecimal(1.23);
    EXPECT_EQ(fixed8_3.Value(), 123000000);

    // Parse from string
    Fixed8 fixed8_4 = Fixed8::Parse("1.23");
    EXPECT_EQ(fixed8_4.Value(), 123000000);

    // Invalid string
    EXPECT_THROW(Fixed8::Parse("invalid"), std::invalid_argument);

    // Add two Fixed8 values
    Fixed8 fixed8_5 = fixed8 + fixed8_2;
    EXPECT_EQ(fixed8_5.Value(), 123000123);

    // Subtract two Fixed8 values
    Fixed8 fixed8_6 = fixed8_5 - fixed8;
    EXPECT_EQ(fixed8_6.Value(), 123000000);

    // Multiply two Fixed8 values
    Fixed8 fixed8_7 = fixed8 * fixed8_2;
    EXPECT_EQ(fixed8_7.Value(), 151);

    // Divide two Fixed8 values
    Fixed8 fixed8_8 = fixed8_2 / fixed8;
    EXPECT_EQ(fixed8_8.Value(), 100000000000000);

    // Convert to string
    EXPECT_EQ(fixed8_2.ToString(), "1.23");

    // Compare Fixed8 values
    Fixed8 fixed8_9 = Fixed8::FromDecimal(1.23);
    Fixed8 fixed8_10 = Fixed8::FromDecimal(4.56);

    EXPECT_TRUE(fixed8_9 < fixed8_10);
    EXPECT_FALSE(fixed8_10 < fixed8_9);

    EXPECT_TRUE(fixed8_9 == fixed8_2);
    EXPECT_FALSE(fixed8_9 == fixed8_10);

    // Test zero
    Fixed8 fixed8_zero = Fixed8::Zero();
    EXPECT_EQ(fixed8_zero.Value(), 0);

    // Test min and max
    Fixed8 fixed8_min = Fixed8::MinValue();
    Fixed8 fixed8_max = Fixed8::MaxValue();
    EXPECT_LT(fixed8_min, fixed8_zero);
    EXPECT_GT(fixed8_max, fixed8_zero);
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

    // Test VarInt edge cases
    std::stringstream stream2;
    BinaryWriter writer2(stream2);

    // Write VarInts
    writer2.WriteVarInt(0);
    writer2.WriteVarInt(1);
    writer2.WriteVarInt(0xFC);
    writer2.WriteVarInt(0xFD);
    writer2.WriteVarInt(0xFFFF);
    writer2.WriteVarInt(0x10000);
    writer2.WriteVarInt(0xFFFFFFFF);
    writer2.WriteVarInt(0x100000000);

    // Read VarInts
    stream2.seekg(0);
    BinaryReader reader2(stream2);

    EXPECT_EQ(reader2.ReadVarInt(), 0);
    EXPECT_EQ(reader2.ReadVarInt(), 1);
    EXPECT_EQ(reader2.ReadVarInt(), 0xFC);
    EXPECT_EQ(reader2.ReadVarInt(), 0xFD);
    EXPECT_EQ(reader2.ReadVarInt(), 0xFFFF);
    EXPECT_EQ(reader2.ReadVarInt(), 0x10000);
    EXPECT_EQ(reader2.ReadVarInt(), 0xFFFFFFFF);
    EXPECT_EQ(reader2.ReadVarInt(), 0x100000000);
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

    // Try to remove a non-existent item
    EXPECT_FALSE(cache.Remove(1));

    // Clear the cache
    cache.Clear();
    EXPECT_EQ(cache.Count(), 0);
    EXPECT_FALSE(cache.TryGet(2).has_value());
    EXPECT_FALSE(cache.TryGet(5).has_value());

    // Test with minimum capacity
    LRUCache<int, std::string> minCache(1);
    EXPECT_EQ(minCache.Capacity(), 1);
}

// Test serializable object
class TestSerializable
{
public:
    int IntValue;
    std::string StringValue;
    ByteVector BytesValue;

    TestSerializable() : IntValue(0) {}

    TestSerializable(int intValue, const std::string& stringValue, const ByteVector& bytesValue)
        : IntValue(intValue), StringValue(stringValue), BytesValue(bytesValue) {}

    void Serialize(BinaryWriter& writer) const
    {
        writer.Write(IntValue);
        writer.WriteString(StringValue);
        writer.WriteVarBytes(BytesValue.AsSpan());
    }

    void Deserialize(BinaryReader& reader)
    {
        IntValue = reader.ReadInt32();
        StringValue = reader.ReadString();
        BytesValue = reader.ReadVarBytes();
    }

    bool operator==(const TestSerializable& other) const
    {
        return IntValue == other.IntValue &&
               StringValue == other.StringValue &&
               BytesValue == other.BytesValue;
    }
};

// Test serialization
TEST(IOComprehensiveTest, Serialization) {
    // Create test data
    TestSerializable original(42, "Hello, World!", ByteVector{1, 2, 3, 4, 5});

    // Serialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    original.Serialize(writer);

    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    TestSerializable deserialized;
    deserialized.Deserialize(reader);

    // Verify
    EXPECT_EQ(deserialized.IntValue, original.IntValue);
    EXPECT_EQ(deserialized.StringValue, original.StringValue);
    EXPECT_EQ(deserialized.BytesValue, original.BytesValue);
    EXPECT_TRUE(deserialized == original);
}

int main(int argc, char** argv) {
    std::cout << "Running IO comprehensive test..." << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
