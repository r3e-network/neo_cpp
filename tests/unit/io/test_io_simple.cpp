/**
 * @file test_io_simple.cpp
 * @brief Simple unit tests for IO module to increase coverage
 */

#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/byte_span.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <sstream>
#include <vector>

using namespace neo::io;

class IOSimpleTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// ByteVector Tests
// ============================================================================

TEST_F(IOSimpleTest, ByteVector_DefaultConstructor) {
    ByteVector vec;
    EXPECT_EQ(vec.Size(), 0u);
    EXPECT_TRUE(vec.IsEmpty());
}

TEST_F(IOSimpleTest, ByteVector_SizeConstructor) {
    ByteVector vec(10);
    EXPECT_EQ(vec.Size(), 10u);
    EXPECT_FALSE(vec.IsEmpty());
}

TEST_F(IOSimpleTest, ByteVector_InitializerList) {
    ByteVector vec = {0x01, 0x02, 0x03, 0x04};
    EXPECT_EQ(vec.Size(), 4u);
    EXPECT_EQ(vec[0], 0x01);
    EXPECT_EQ(vec[3], 0x04);
}

TEST_F(IOSimpleTest, ByteVector_Append) {
    ByteVector vec1 = {0x01, 0x02};
    ByteVector vec2 = {0x03, 0x04};
    
    vec1.Append(ByteSpan(vec2.Data(), vec2.Size()));
    EXPECT_EQ(vec1.Size(), 4u);
    EXPECT_EQ(vec1[2], 0x03);
}

TEST_F(IOSimpleTest, ByteVector_ToHexString) {
    ByteVector vec = {0x01, 0x23, 0xAB, 0xCD, 0xEF};
    std::string hex = vec.ToHexString();
    EXPECT_EQ(hex, "0123abcdef");
}

TEST_F(IOSimpleTest, ByteVector_FromHexString) {
    std::string hex = "0123abcdef";
    ByteVector vec = ByteVector::FromHexString(hex);
    
    EXPECT_EQ(vec.Size(), 5u);
    EXPECT_EQ(vec[0], 0x01);
    EXPECT_EQ(vec[4], 0xEF);
}

// ============================================================================
// ByteSpan Tests
// ============================================================================

TEST_F(IOSimpleTest, ByteSpan_DefaultConstructor) {
    ByteSpan span;
    EXPECT_EQ(span.Size(), 0u);
    EXPECT_TRUE(span.IsEmpty());
    EXPECT_EQ(span.Data(), nullptr);
}

TEST_F(IOSimpleTest, ByteSpan_FromByteVector) {
    ByteVector vec = {0x01, 0x02, 0x03};
    ByteSpan span(vec.Data(), vec.Size());
    
    EXPECT_EQ(span.Size(), vec.Size());
    EXPECT_EQ(span[0], vec[0]);
    EXPECT_EQ(span[2], vec[2]);
}

TEST_F(IOSimpleTest, ByteSpan_FromPointer) {
    uint8_t data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    ByteSpan span(data, sizeof(data));
    
    EXPECT_EQ(span.Size(), 4u);
    EXPECT_EQ(span[0], 0xAA);
    EXPECT_EQ(span[3], 0xDD);
}

// ============================================================================
// UInt256 Tests
// ============================================================================

TEST_F(IOSimpleTest, UInt256_DefaultConstructor) {
    UInt256 val;
    EXPECT_EQ(UInt256::Size, 32u);
    
    // Should be zero-initialized
    for (size_t i = 0; i < 32; ++i) {
        EXPECT_EQ(val.Data()[i], 0);
    }
}

TEST_F(IOSimpleTest, UInt256_Parse) {
    std::string hex = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    UInt256 val = UInt256::Parse(hex);
    
    EXPECT_EQ(val.ToString(), hex);
}

TEST_F(IOSimpleTest, UInt256_Comparison) {
    UInt256 val1, val2;
    std::memset(val1.Data(), 0x00, UInt256::Size);
    std::memset(val2.Data(), 0xFF, UInt256::Size);
    
    EXPECT_NE(val1, val2);
    EXPECT_LT(val1, val2);
}

// ============================================================================
// UInt160 Tests
// ============================================================================

TEST_F(IOSimpleTest, UInt160_DefaultConstructor) {
    UInt160 val;
    EXPECT_EQ(UInt160::Size, 20u);
    
    for (size_t i = 0; i < 20; ++i) {
        EXPECT_EQ(val.Data()[i], 0);
    }
}

TEST_F(IOSimpleTest, UInt160_Parse) {
    std::string hex = "0123456789abcdef0123456789abcdef01234567";
    UInt160 val = UInt160::Parse(hex);
    
    EXPECT_EQ(val.ToString(), hex);
}

// ============================================================================
// BinaryReader Tests
// ============================================================================

TEST_F(IOSimpleTest, BinaryReader_ReadBytes) {
    ByteVector data = {0x01, 0x02, 0x03, 0x04, 0x05};
    ByteSpan span(data.Data(), data.Size());
    BinaryReader reader(span);
    
    auto bytes = reader.ReadBytes(3);
    EXPECT_EQ(bytes.Size(), 3u);
    EXPECT_EQ(bytes[0], 0x01);
    EXPECT_EQ(bytes[2], 0x03);
}

TEST_F(IOSimpleTest, BinaryReader_ReadUInt8) {
    ByteVector data = {0xFF, 0x00, 0x7F};
    ByteSpan span(data.Data(), data.Size());
    BinaryReader reader(span);
    
    EXPECT_EQ(reader.ReadUInt8(), 0xFF);
    EXPECT_EQ(reader.ReadUInt8(), 0x00);
    EXPECT_EQ(reader.ReadUInt8(), 0x7F);
}

TEST_F(IOSimpleTest, BinaryReader_ReadUInt16) {
    ByteVector data = {0x34, 0x12, 0xFF, 0xFF};  // Little-endian
    ByteSpan span(data.Data(), data.Size());
    BinaryReader reader(span);
    
    EXPECT_EQ(reader.ReadUInt16(), 0x1234);
    EXPECT_EQ(reader.ReadUInt16(), 0xFFFF);
}

TEST_F(IOSimpleTest, BinaryReader_ReadUInt32) {
    ByteVector data = {0x78, 0x56, 0x34, 0x12};  // Little-endian
    ByteSpan span(data.Data(), data.Size());
    BinaryReader reader(span);
    
    EXPECT_EQ(reader.ReadUInt32(), 0x12345678u);
}

TEST_F(IOSimpleTest, BinaryReader_ReadBool) {
    ByteVector data = {0x01, 0x00, 0xFF};
    ByteSpan span(data.Data(), data.Size());
    BinaryReader reader(span);
    
    EXPECT_TRUE(reader.ReadBool());
    EXPECT_FALSE(reader.ReadBool());
    EXPECT_TRUE(reader.ReadBool());
}

// ============================================================================
// BinaryWriter Tests
// ============================================================================

TEST_F(IOSimpleTest, BinaryWriter_WriteBytes) {
    std::stringstream ss;
    BinaryWriter writer(ss);
    
    ByteVector data = {0x01, 0x02, 0x03};
    writer.WriteBytes(data);
    
    std::string result = ss.str();
    EXPECT_EQ(result.size(), 3u);
    EXPECT_EQ(uint8_t(result[0]), 0x01);
    EXPECT_EQ(uint8_t(result[2]), 0x03);
}

TEST_F(IOSimpleTest, BinaryWriter_WriteUInt8) {
    std::stringstream ss;
    BinaryWriter writer(ss);
    
    writer.WriteByte(0xFF);
    writer.WriteByte(0x00);
    writer.WriteByte(0x7F);
    
    std::string result = ss.str();
    EXPECT_EQ(result.size(), 3u);
    EXPECT_EQ(uint8_t(result[0]), 0xFF);
    EXPECT_EQ(uint8_t(result[1]), 0x00);
    EXPECT_EQ(uint8_t(result[2]), 0x7F);
}

TEST_F(IOSimpleTest, BinaryWriter_WriteUInt16) {
    std::stringstream ss;
    BinaryWriter writer(ss);
    
    writer.WriteUInt16(0x1234);
    
    std::string result = ss.str();
    EXPECT_EQ(result.size(), 2u);
    EXPECT_EQ(uint8_t(result[0]), 0x34);  // Little-endian
    EXPECT_EQ(uint8_t(result[1]), 0x12);
}

TEST_F(IOSimpleTest, BinaryWriter_WriteUInt32) {
    std::stringstream ss;
    BinaryWriter writer(ss);
    
    writer.Write(uint32_t(0x12345678u));
    
    std::string result = ss.str();
    EXPECT_EQ(result.size(), 4u);
    EXPECT_EQ(uint8_t(result[0]), 0x78);  // Little-endian
    EXPECT_EQ(uint8_t(result[3]), 0x12);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(IOSimpleTest, BinaryReader_ReadPastEnd) {
    ByteVector data = {0x01, 0x02};
    ByteSpan span(data.Data(), data.Size());
    BinaryReader reader(span);
    
    reader.ReadUInt8();
    reader.ReadUInt8();
    
    // Reading past end should throw
    EXPECT_THROW(reader.ReadUInt8(), std::exception);
}

TEST_F(IOSimpleTest, ByteVector_InvalidHexString) {
    // Odd number of characters
    EXPECT_THROW(ByteVector::FromHexString("abc"), std::exception);
}

TEST_F(IOSimpleTest, UInt256_InvalidParse) {
    // Wrong length
    EXPECT_THROW(UInt256::Parse("1234"), std::exception);
}