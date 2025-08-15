/**
 * @file test_io_comprehensive.cpp
 * @brief Comprehensive unit tests for IO module
 */

#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/byte_span.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/json.h>
#include <sstream>
#include <vector>
#include <limits>
#include <chrono>

using namespace neo::io;

class IOComprehensiveTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// ByteVector Tests
// ============================================================================

TEST_F(IOComprehensiveTest, ByteVector_DefaultConstructor) {
    ByteVector vec;
    EXPECT_EQ(vec.Size(), 0);
    EXPECT_TRUE(vec.IsEmpty());
}

TEST_F(IOComprehensiveTest, ByteVector_SizeConstructor) {
    ByteVector vec(10);
    EXPECT_EQ(vec.Size(), 10);
    EXPECT_FALSE(vec.IsEmpty());
    
    // All bytes should be zero-initialized
    for (size_t i = 0; i < vec.Size(); ++i) {
        EXPECT_EQ(vec[i], 0);
    }
}

TEST_F(IOComprehensiveTest, ByteVector_FillConstructor) {
    ByteVector vec(5, 0xFF);
    EXPECT_EQ(vec.Size(), 5);
    
    for (size_t i = 0; i < vec.Size(); ++i) {
        EXPECT_EQ(vec[i], 0xFF);
    }
}

TEST_F(IOComprehensiveTest, ByteVector_InitializerList) {
    ByteVector vec = {0x01, 0x02, 0x03, 0x04};
    EXPECT_EQ(vec.Size(), 4);
    EXPECT_EQ(vec[0], 0x01);
    EXPECT_EQ(vec[1], 0x02);
    EXPECT_EQ(vec[2], 0x03);
    EXPECT_EQ(vec[3], 0x04);
}

TEST_F(IOComprehensiveTest, ByteVector_CopyConstructor) {
    ByteVector original = {0xAA, 0xBB, 0xCC};
    ByteVector copy(original);
    
    EXPECT_EQ(copy.Size(), original.Size());
    EXPECT_EQ(copy, original);
    
    // Modify copy shouldn't affect original
    copy[0] = 0xFF;
    EXPECT_NE(copy[0], original[0]);
}

TEST_F(IOComprehensiveTest, ByteVector_MoveConstructor) {
    ByteVector original = {0xAA, 0xBB, 0xCC};
    size_t originalSize = original.Size();
    ByteVector moved(std::move(original));
    
    EXPECT_EQ(moved.Size(), originalSize);
    EXPECT_EQ(moved[0], 0xAA);
}

TEST_F(IOComprehensiveTest, ByteVector_Append) {
    ByteVector vec1 = {0x01, 0x02};
    ByteVector vec2 = {0x03, 0x04};
    
    vec1.Append(vec2);
    EXPECT_EQ(vec1.Size(), 4);
    EXPECT_EQ(vec1[2], 0x03);
    EXPECT_EQ(vec1[3], 0x04);
}

TEST_F(IOComprehensiveTest, ByteVector_Slice) {
    ByteVector vec = {0x00, 0x11, 0x22, 0x33, 0x44};
    
    auto slice1 = vec.Slice(1, 3);  // Get elements [1, 3)
    EXPECT_EQ(slice1.Size(), 2);
    EXPECT_EQ(slice1[0], 0x11);
    EXPECT_EQ(slice1[1], 0x22);
    
    auto slice2 = vec.Slice(2);  // Get from index 2 to end
    EXPECT_EQ(slice2.Size(), 3);
    EXPECT_EQ(slice2[0], 0x22);
}

TEST_F(IOComprehensiveTest, ByteVector_ToHexString) {
    ByteVector vec = {0x01, 0x23, 0xAB, 0xCD, 0xEF};
    std::string hex = vec.ToHexString();
    EXPECT_EQ(hex, "0123abcdef");
}

TEST_F(IOComprehensiveTest, ByteVector_FromHexString) {
    std::string hex = "0123abcdef";
    ByteVector vec = ByteVector::FromHexString(hex);
    
    EXPECT_EQ(vec.Size(), 5);
    EXPECT_EQ(vec[0], 0x01);
    EXPECT_EQ(vec[1], 0x23);
    EXPECT_EQ(vec[2], 0xAB);
    EXPECT_EQ(vec[3], 0xCD);
    EXPECT_EQ(vec[4], 0xEF);
}

TEST_F(IOComprehensiveTest, ByteVector_Comparison) {
    ByteVector vec1 = {0x01, 0x02, 0x03};
    ByteVector vec2 = {0x01, 0x02, 0x03};
    ByteVector vec3 = {0x01, 0x02, 0x04};
    
    EXPECT_EQ(vec1, vec2);
    EXPECT_NE(vec1, vec3);
    EXPECT_LT(vec1, vec3);
    EXPECT_GT(vec3, vec1);
}

// ============================================================================
// ByteSpan Tests
// ============================================================================

TEST_F(IOComprehensiveTest, ByteSpan_DefaultConstructor) {
    ByteSpan span;
    EXPECT_EQ(span.Size(), 0);
    EXPECT_TRUE(span.IsEmpty());
    EXPECT_EQ(span.Data(), nullptr);
}

TEST_F(IOComprehensiveTest, ByteSpan_FromByteVector) {
    ByteVector vec = {0x01, 0x02, 0x03};
    ByteSpan span(vec);
    
    EXPECT_EQ(span.Size(), vec.Size());
    EXPECT_EQ(span[0], vec[0]);
    EXPECT_EQ(span[1], vec[1]);
    EXPECT_EQ(span[2], vec[2]);
}

TEST_F(IOComprehensiveTest, ByteSpan_FromPointer) {
    uint8_t data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    ByteSpan span(data, sizeof(data));
    
    EXPECT_EQ(span.Size(), 4);
    EXPECT_EQ(span[0], 0xAA);
    EXPECT_EQ(span[3], 0xDD);
}

TEST_F(IOComprehensiveTest, ByteSpan_Subspan) {
    uint8_t data[] = {0x00, 0x11, 0x22, 0x33, 0x44};
    ByteSpan span(data, sizeof(data));
    
    auto sub = span.Subspan(1, 3);
    EXPECT_EQ(sub.Size(), 3);
    EXPECT_EQ(sub[0], 0x11);
    EXPECT_EQ(sub[2], 0x33);
}

// ============================================================================
// UInt256 Tests
// ============================================================================

TEST_F(IOComprehensiveTest, UInt256_DefaultConstructor) {
    UInt256 val;
    EXPECT_EQ(val.Size(), 32);
    
    // Should be zero-initialized
    for (size_t i = 0; i < 32; ++i) {
        EXPECT_EQ(val.Data()[i], 0);
    }
}

TEST_F(IOComprehensiveTest, UInt256_Parse) {
    std::string hex = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    UInt256 val = UInt256::Parse(hex);
    
    EXPECT_EQ(val.ToString(), hex);
}

TEST_F(IOComprehensiveTest, UInt256_Comparison) {
    UInt256 val1, val2, val3;
    val1.Fill(0x00);
    val2.Fill(0x00);
    val3.Fill(0xFF);
    
    EXPECT_EQ(val1, val2);
    EXPECT_NE(val1, val3);
    EXPECT_LT(val1, val3);
}

TEST_F(IOComprehensiveTest, UInt256_Hash) {
    UInt256 val;
    val.Fill(0xAB);
    
    std::hash<UInt256> hasher;
    size_t hash1 = hasher(val);
    
    UInt256 val2;
    val2.Fill(0xAB);
    size_t hash2 = hasher(val2);
    
    EXPECT_EQ(hash1, hash2);
}

// ============================================================================
// UInt160 Tests
// ============================================================================

TEST_F(IOComprehensiveTest, UInt160_DefaultConstructor) {
    UInt160 val;
    EXPECT_EQ(val.Size(), 20);
    
    for (size_t i = 0; i < 20; ++i) {
        EXPECT_EQ(val.Data()[i], 0);
    }
}

TEST_F(IOComprehensiveTest, UInt160_Parse) {
    std::string hex = "0123456789abcdef0123456789abcdef01234567";
    UInt160 val = UInt160::Parse(hex);
    
    EXPECT_EQ(val.ToString(), hex);
}

// ============================================================================
// BinaryReader Tests
// ============================================================================

TEST_F(IOComprehensiveTest, BinaryReader_ReadBytes) {
    ByteVector data = {0x01, 0x02, 0x03, 0x04, 0x05};
    ByteSpan span(data);
    BinaryReader reader(span);
    
    auto bytes = reader.ReadBytes(3);
    EXPECT_EQ(bytes.Size(), 3);
    EXPECT_EQ(bytes[0], 0x01);
    EXPECT_EQ(bytes[1], 0x02);
    EXPECT_EQ(bytes[2], 0x03);
}

TEST_F(IOComprehensiveTest, BinaryReader_ReadUInt8) {
    ByteVector data = {0xFF, 0x00, 0x7F};
    ByteSpan span(data);
    BinaryReader reader(span);
    
    EXPECT_EQ(reader.ReadUInt8(), 0xFF);
    EXPECT_EQ(reader.ReadUInt8(), 0x00);
    EXPECT_EQ(reader.ReadUInt8(), 0x7F);
}

TEST_F(IOComprehensiveTest, BinaryReader_ReadUInt16) {
    ByteVector data = {0x34, 0x12, 0xFF, 0xFF};  // Little-endian
    ByteSpan span(data);
    BinaryReader reader(span);
    
    EXPECT_EQ(reader.ReadUInt16(), 0x1234);
    EXPECT_EQ(reader.ReadUInt16(), 0xFFFF);
}

TEST_F(IOComprehensiveTest, BinaryReader_ReadUInt32) {
    ByteVector data = {0x78, 0x56, 0x34, 0x12};  // Little-endian
    ByteSpan span(data);
    BinaryReader reader(span);
    
    EXPECT_EQ(reader.ReadUInt32(), 0x12345678);
}

TEST_F(IOComprehensiveTest, BinaryReader_ReadUInt64) {
    ByteVector data = {0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01};  // Little-endian
    ByteSpan span(data);
    BinaryReader reader(span);
    
    EXPECT_EQ(reader.ReadUInt64(), 0x0123456789ABCDEF);
}

TEST_F(IOComprehensiveTest, BinaryReader_ReadString) {
    ByteVector data = {0x05, 'h', 'e', 'l', 'l', 'o'};  // Length-prefixed string
    ByteSpan span(data);
    BinaryReader reader(span);
    
    std::string str = reader.ReadString();
    EXPECT_EQ(str, "hello");
}

TEST_F(IOComprehensiveTest, BinaryReader_ReadVarInt) {
    // Test various VarInt encodings
    {
        ByteVector data = {0xFC};  // Value < 0xFD
        ByteSpan span(data);
        BinaryReader reader(span);
        EXPECT_EQ(reader.ReadVarInt(), 0xFC);
    }
    
    {
        ByteVector data = {0xFD, 0x34, 0x12};  // 0xFD prefix for 2 bytes
        ByteSpan span(data);
        BinaryReader reader(span);
        EXPECT_EQ(reader.ReadVarInt(), 0x1234);
    }
    
    {
        ByteVector data = {0xFE, 0x78, 0x56, 0x34, 0x12};  // 0xFE prefix for 4 bytes
        ByteSpan span(data);
        BinaryReader reader(span);
        EXPECT_EQ(reader.ReadVarInt(), 0x12345678);
    }
}

TEST_F(IOComprehensiveTest, BinaryReader_ReadBool) {
    ByteVector data = {0x01, 0x00, 0xFF};
    ByteSpan span(data);
    BinaryReader reader(span);
    
    EXPECT_TRUE(reader.ReadBool());
    EXPECT_FALSE(reader.ReadBool());
    EXPECT_TRUE(reader.ReadBool());
}

// ============================================================================
// BinaryWriter Tests
// ============================================================================

TEST_F(IOComprehensiveTest, BinaryWriter_WriteBytes) {
    std::stringstream ss;
    BinaryWriter writer(ss);
    
    ByteVector data = {0x01, 0x02, 0x03};
    writer.WriteBytes(data);
    
    std::string result = ss.str();
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], 0x01);
    EXPECT_EQ(result[1], 0x02);
    EXPECT_EQ(result[2], 0x03);
}

TEST_F(IOComprehensiveTest, BinaryWriter_WriteUInt8) {
    std::stringstream ss;
    BinaryWriter writer(ss);
    
    writer.WriteUInt8(0xFF);
    writer.WriteUInt8(0x00);
    writer.WriteUInt8(0x7F);
    
    std::string result = ss.str();
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(uint8_t(result[0]), 0xFF);
    EXPECT_EQ(uint8_t(result[1]), 0x00);
    EXPECT_EQ(uint8_t(result[2]), 0x7F);
}

TEST_F(IOComprehensiveTest, BinaryWriter_WriteUInt16) {
    std::stringstream ss;
    BinaryWriter writer(ss);
    
    writer.WriteUInt16(0x1234);
    
    std::string result = ss.str();
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(uint8_t(result[0]), 0x34);  // Little-endian
    EXPECT_EQ(uint8_t(result[1]), 0x12);
}

TEST_F(IOComprehensiveTest, BinaryWriter_WriteUInt32) {
    std::stringstream ss;
    BinaryWriter writer(ss);
    
    writer.WriteUInt32(0x12345678);
    
    std::string result = ss.str();
    EXPECT_EQ(result.size(), 4);
    EXPECT_EQ(uint8_t(result[0]), 0x78);  // Little-endian
    EXPECT_EQ(uint8_t(result[1]), 0x56);
    EXPECT_EQ(uint8_t(result[2]), 0x34);
    EXPECT_EQ(uint8_t(result[3]), 0x12);
}

TEST_F(IOComprehensiveTest, BinaryWriter_WriteString) {
    std::stringstream ss;
    BinaryWriter writer(ss);
    
    writer.WriteString("hello");
    
    std::string result = ss.str();
    EXPECT_EQ(result.size(), 6);  // 1 byte length + 5 bytes string
    EXPECT_EQ(uint8_t(result[0]), 0x05);  // Length
    EXPECT_EQ(result.substr(1), "hello");
}

TEST_F(IOComprehensiveTest, BinaryWriter_WriteVarInt) {
    // Value < 0xFD
    {
        std::stringstream ss;
        BinaryWriter writer(ss);
        writer.WriteVarInt(0xFC);
        
        std::string result = ss.str();
        EXPECT_EQ(result.size(), 1);
        EXPECT_EQ(uint8_t(result[0]), 0xFC);
    }
    
    // Value requiring 2 bytes
    {
        std::stringstream ss;
        BinaryWriter writer(ss);
        writer.WriteVarInt(0x1234);
        
        std::string result = ss.str();
        EXPECT_EQ(result.size(), 3);
        EXPECT_EQ(uint8_t(result[0]), 0xFD);
        EXPECT_EQ(uint8_t(result[1]), 0x34);
        EXPECT_EQ(uint8_t(result[2]), 0x12);
    }
}

// ============================================================================
// JSON Tests
// ============================================================================

TEST_F(IOComprehensiveTest, JsonValue_CreateObject) {
    JsonValue obj = JsonValue::CreateObject();
    EXPECT_TRUE(obj.IsObject());
    EXPECT_FALSE(obj.IsArray());
    EXPECT_FALSE(obj.IsNull());
}

TEST_F(IOComprehensiveTest, JsonValue_CreateArray) {
    JsonValue arr = JsonValue::CreateArray();
    EXPECT_TRUE(arr.IsArray());
    EXPECT_FALSE(arr.IsObject());
}

TEST_F(IOComprehensiveTest, JsonValue_AddMember) {
    JsonValue obj = JsonValue::CreateObject();
    obj.AddMember("name", "value");
    obj.AddMember("number", 42);
    obj.AddMember("flag", true);
    
    EXPECT_EQ(obj["name"].GetString(), "value");
    EXPECT_EQ(obj["number"].GetInt(), 42);
    EXPECT_TRUE(obj["flag"].GetBool());
}

TEST_F(IOComprehensiveTest, JsonValue_ArrayOperations) {
    JsonValue arr = JsonValue::CreateArray();
    arr.PushBack(1);
    arr.PushBack("two");
    arr.PushBack(true);
    
    EXPECT_EQ(arr.Size(), 3);
    EXPECT_EQ(arr[0].GetInt(), 1);
    EXPECT_EQ(arr[1].GetString(), "two");
    EXPECT_TRUE(arr[2].GetBool());
}

TEST_F(IOComprehensiveTest, JsonValue_NestedStructures) {
    JsonValue root = JsonValue::CreateObject();
    JsonValue nested = JsonValue::CreateObject();
    nested.AddMember("key", "value");
    
    JsonValue array = JsonValue::CreateArray();
    array.PushBack(1);
    array.PushBack(2);
    
    root.AddMember("nested", nested);
    root.AddMember("array", array);
    
    EXPECT_EQ(root["nested"]["key"].GetString(), "value");
    EXPECT_EQ(root["array"][0].GetInt(), 1);
    EXPECT_EQ(root["array"][1].GetInt(), 2);
}

TEST_F(IOComprehensiveTest, JsonReader_ParseObject) {
    std::string json = R"({"name": "test", "value": 123, "active": true})";
    JsonReader reader;
    JsonValue value = reader.Parse(json);
    
    EXPECT_TRUE(value.IsObject());
    EXPECT_EQ(value["name"].GetString(), "test");
    EXPECT_EQ(value["value"].GetInt(), 123);
    EXPECT_TRUE(value["active"].GetBool());
}

TEST_F(IOComprehensiveTest, JsonReader_ParseArray) {
    std::string json = R"([1, "two", true, null])";
    JsonReader reader;
    JsonValue value = reader.Parse(json);
    
    EXPECT_TRUE(value.IsArray());
    EXPECT_EQ(value.Size(), 4);
    EXPECT_EQ(value[0].GetInt(), 1);
    EXPECT_EQ(value[1].GetString(), "two");
    EXPECT_TRUE(value[2].GetBool());
    EXPECT_TRUE(value[3].IsNull());
}

TEST_F(IOComprehensiveTest, JsonWriter_WriteObject) {
    JsonValue obj = JsonValue::CreateObject();
    obj.AddMember("test", 123);
    
    JsonWriter writer;
    std::string json = writer.Write(obj);
    
    EXPECT_NE(json.find("\"test\""), std::string::npos);
    EXPECT_NE(json.find("123"), std::string::npos);
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(IOComprehensiveTest, BinaryReader_ReadPastEnd) {
    ByteVector data = {0x01, 0x02};
    ByteSpan span(data);
    BinaryReader reader(span);
    
    reader.ReadUInt8();
    reader.ReadUInt8();
    
    // Reading past end should throw
    EXPECT_THROW(reader.ReadUInt8(), std::exception);
}

TEST_F(IOComprehensiveTest, ByteVector_InvalidHexString) {
    // Invalid hex characters
    EXPECT_THROW(ByteVector::FromHexString("invalid"), std::exception);
    
    // Odd number of characters
    EXPECT_THROW(ByteVector::FromHexString("abc"), std::exception);
}

TEST_F(IOComprehensiveTest, UInt256_InvalidParse) {
    // Wrong length
    EXPECT_THROW(UInt256::Parse("1234"), std::exception);
    
    // Invalid characters
    EXPECT_THROW(UInt256::Parse("zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"), 
                 std::exception);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(IOComprehensiveTest, Performance_ByteVectorAppend) {
    ByteVector vec;
    const int iterations = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        vec.push_back(i & 0xFF);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should be able to append at least 1000 bytes per millisecond
    EXPECT_LT(duration.count(), iterations * 1000);
}

TEST_F(IOComprehensiveTest, Performance_BinaryReaderWriter) {
    const int iterations = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        std::stringstream ss;
        BinaryWriter writer(ss);
        
        writer.WriteUInt32(0x12345678);
        writer.WriteString("test");
        writer.WriteBytes({0x01, 0x02, 0x03});
        
        std::string data = ss.str();
        ByteVector vec(data.begin(), data.end());
        ByteSpan span(vec);
        BinaryReader reader(span);
        
        auto val = reader.ReadUInt32();
        auto str = reader.ReadString();
        auto bytes = reader.ReadBytes(3);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in reasonable time
    EXPECT_LT(duration.count(), iterations);
}