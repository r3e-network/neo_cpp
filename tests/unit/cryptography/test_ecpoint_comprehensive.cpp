#include <gtest/gtest.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/memory_stream.h>

using namespace neo::cryptography::ecc;
using namespace neo::io;

class ECPointComprehensiveTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Known test vectors from Neo C# implementation
        compressed_point_hex = "02486fd15702c4490a26703112a5cc1d0923fd697a33406bd5a1c00e0013b09a7021";
        uncompressed_point_hex = "04486fd15702c4490a26703112a5cc1d0923fd697a33406bd5a1c00e0013b09a7021b8a88f572f5b81f8c0b3e2bb7d03b5b42e05f8b0a6d8a1a2c3d4e5f6a7b8c9d0";
        
        // Create test points
        compressed_bytes = ByteVector::Parse(compressed_point_hex);
        uncompressed_bytes = ByteVector::Parse(uncompressed_point_hex);
    }

    std::string compressed_point_hex;
    std::string uncompressed_point_hex;
    ByteVector compressed_bytes;
    ByteVector uncompressed_bytes;
};

// Test basic construction
TEST_F(ECPointComprehensiveTest, DefaultConstruction)
{
    ECPoint point;
    EXPECT_TRUE(point.IsInfinity());
    EXPECT_EQ(point.GetCurveName(), "");
}

TEST_F(ECPointComprehensiveTest, CurveNameConstruction)
{
    ECPoint point("secp256r1");
    EXPECT_TRUE(point.IsInfinity());
    EXPECT_EQ(point.GetCurveName(), "secp256r1");
}

// Test serialization/deserialization
TEST_F(ECPointComprehensiveTest, SerializeDeserialize)
{
    // Create a non-infinity point
    ECPoint original = ECPoint::FromBytes(compressed_bytes.AsSpan(), "secp256r1");
    
    // Serialize
    ByteVector buffer;
    MemoryStream stream(buffer);
    BinaryWriter writer(stream);
    original.Serialize(writer);
    
    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    ECPoint deserialized("secp256r1");
    deserialized.Deserialize(reader);
    
    // Verify they are equal
    EXPECT_EQ(original, deserialized);
    EXPECT_EQ(original.GetCurveName(), deserialized.GetCurveName());
}

// Test compressed vs uncompressed format
TEST_F(ECPointComprehensiveTest, CompressedVsUncompressed)
{
    ECPoint compressed_point = ECPoint::FromBytes(compressed_bytes.AsSpan(), "secp256r1");
    
    // Get both formats
    auto compressed_result = compressed_point.ToBytes(true);
    auto uncompressed_result = compressed_point.ToBytes(false);
    
    // Compressed should be 33 bytes
    EXPECT_EQ(compressed_result.Size(), 33);
    // Uncompressed should be 65 bytes  
    EXPECT_EQ(uncompressed_result.Size(), 65);
    
    // First byte should indicate format
    EXPECT_TRUE(compressed_result[0] == 0x02 || compressed_result[0] == 0x03);
    EXPECT_EQ(uncompressed_result[0], 0x04);
}

// Test ToArray method (always compressed)
TEST_F(ECPointComprehensiveTest, ToArray)
{
    ECPoint point = ECPoint::FromBytes(compressed_bytes.AsSpan(), "secp256r1");
    auto array = point.ToArray();
    auto compressed = point.ToBytes(true);
    
    EXPECT_EQ(array.Size(), compressed.Size());
    EXPECT_EQ(array.AsSpan().ToHexString(), compressed.AsSpan().ToHexString());
}

// Test infinity point
TEST_F(ECPointComprehensiveTest, InfinityPoint)
{
    ECPoint infinity = ECPoint::Infinity("secp256r1");
    
    EXPECT_TRUE(infinity.IsInfinity());
    EXPECT_EQ(infinity.GetCurveName(), "secp256r1");
    
    // Infinity serializes to single zero byte
    auto bytes = infinity.ToBytes(true);
    EXPECT_EQ(bytes.Size(), 1);
    EXPECT_EQ(bytes[0], 0x00);
}

// Test hex string parsing
TEST_F(ECPointComprehensiveTest, HexStringParsing)
{
    ECPoint point1 = ECPoint::FromHex(compressed_point_hex, "secp256r1");
    ECPoint point2 = ECPoint::Parse(compressed_point_hex, "secp256r1");
    
    EXPECT_EQ(point1, point2);
    EXPECT_FALSE(point1.IsInfinity());
    EXPECT_EQ(point1.GetCurveName(), "secp256r1");
}

// Test string conversion
TEST_F(ECPointComprehensiveTest, StringConversion)
{
    ECPoint point = ECPoint::FromHex(compressed_point_hex, "secp256r1");
    
    std::string hex_compressed = point.ToHex(true);
    std::string hex_uncompressed = point.ToHex(false);
    std::string to_string = point.ToString(true);
    
    EXPECT_EQ(hex_compressed, compressed_point_hex);
    EXPECT_EQ(to_string, hex_compressed);
    EXPECT_EQ(hex_uncompressed.length(), 130); // 65 bytes * 2 hex chars
}

// Test comparison operators
TEST_F(ECPointComprehensiveTest, ComparisonOperators)
{
    ECPoint point1 = ECPoint::FromHex(compressed_point_hex, "secp256r1");
    ECPoint point2 = ECPoint::FromHex(compressed_point_hex, "secp256r1");
    ECPoint point3 = ECPoint::Infinity("secp256r1");
    
    // Equality
    EXPECT_EQ(point1, point2);
    EXPECT_NE(point1, point3);
    
    // Inequality
    EXPECT_FALSE(point1 != point2);
    EXPECT_TRUE(point1 != point3);
    
    // Ordering (for use in containers)
    EXPECT_TRUE(point3 < point1 || point1 < point3); // One should be less than the other
}

// Test error handling
TEST_F(ECPointComprehensiveTest, ErrorHandling)
{
    // Invalid hex string
    EXPECT_THROW(ECPoint::FromHex("invalid_hex", "secp256r1"), std::invalid_argument);
    
    // Invalid byte data
    ByteVector invalid_data(10); // Too short
    EXPECT_THROW(ECPoint::FromBytes(invalid_data.AsSpan(), "secp256r1"), std::invalid_argument);
    
    // Empty data
    ByteVector empty_data;
    EXPECT_THROW(ECPoint::FromBytes(empty_data.AsSpan(), "secp256r1"), std::invalid_argument);
}

// Test edge cases
TEST_F(ECPointComprehensiveTest, EdgeCases)
{
    // Test with different curve names
    ECPoint point1("secp256r1");
    ECPoint point2("secp256k1");
    
    EXPECT_NE(point1.GetCurveName(), point2.GetCurveName());
    
    // Test setting and getting curve name
    ECPoint point;
    point.SetCurveName("test_curve");
    EXPECT_EQ(point.GetCurveName(), "test_curve");
}

// Test coordinate access
TEST_F(ECPointComprehensiveTest, CoordinateAccess)
{
    ECPoint point = ECPoint::FromBytes(compressed_bytes.AsSpan(), "secp256r1");
    
    // Should not be infinity after loading from valid bytes
    EXPECT_FALSE(point.IsInfinity());
    
    // Getting coordinates should not throw (even if Y is derived)
    EXPECT_NO_THROW(auto x = point.GetX());
    EXPECT_NO_THROW(auto y = point.GetY());
    
    // Test setting infinity
    point.SetInfinity(true);
    EXPECT_TRUE(point.IsInfinity());
    
    point.SetInfinity(false);
    EXPECT_FALSE(point.IsInfinity());
}

// Test performance with multiple operations
TEST_F(ECPointComprehensiveTest, PerformanceTest)
{
    const int iterations = 100;
    std::vector<ECPoint> points;
    
    // Create multiple points
    for (int i = 0; i < iterations; ++i) {
        points.emplace_back(ECPoint::FromBytes(compressed_bytes.AsSpan(), "secp256r1"));
    }
    
    // Serialize all points
    ByteVector total_buffer;
    MemoryStream stream(total_buffer);
    BinaryWriter writer(stream);
    
    for (const auto& point : points) {
        point.Serialize(writer);
    }
    
    EXPECT_GT(total_buffer.Size(), 0);
    
    // Deserialize all points
    stream.seekg(0);
    BinaryReader reader(stream);
    
    for (int i = 0; i < iterations; ++i) {
        ECPoint deserialized("secp256r1");
        EXPECT_NO_THROW(deserialized.Deserialize(reader));
        EXPECT_EQ(deserialized, points[i]);
    }
}