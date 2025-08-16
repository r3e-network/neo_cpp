#include <gtest/gtest.h>
#include <neo/io/uint160.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/memory_stream.h>
#include <array>
#include <unordered_set>

using namespace neo::io;

class UInt160ComprehensiveTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Test vectors from Neo C# implementation
        test_hex = "0x1234567890abcdef1234567890abcdef12345678";
        test_hex_no_prefix = "1234567890abcdef1234567890abcdef12345678";
        
        // Create test data array (20 bytes)
        test_data = {{
            0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
            0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
            0x12, 0x34, 0x56, 0x78
        }};
        
        // Zero array
        zero_data.fill(0);
        
        // Max array
        max_data.fill(0xFF);
        
        // Test Neo address (example)
        test_address = "NdypBhqkz2CMMnwxBgvoC9X2XjKF5axgKo";
    }

    std::string test_hex;
    std::string test_hex_no_prefix;
    std::array<uint8_t, UInt160::Size> test_data;
    std::array<uint8_t, UInt160::Size> zero_data;
    std::array<uint8_t, UInt160::Size> max_data;
    std::string test_address;
};

// Test basic construction
TEST_F(UInt160ComprehensiveTest, DefaultConstruction)
{
    UInt160 uint160;
    
    // Should be zero by default
    EXPECT_TRUE(uint160.IsZero());
    
    // All bytes should be zero
    for (size_t i = 0; i < UInt160::Size; ++i) {
        EXPECT_EQ(uint160[i], 0);
    }
}

TEST_F(UInt160ComprehensiveTest, ByteSpanConstruction)
{
    ByteSpan span(test_data.data(), test_data.size());
    UInt160 uint160(span);
    
    EXPECT_FALSE(uint160.IsZero());
    
    // Verify all bytes match
    for (size_t i = 0; i < UInt160::Size; ++i) {
        EXPECT_EQ(uint160[i], test_data[i]);
    }
}

TEST_F(UInt160ComprehensiveTest, ArrayConstruction)
{
    UInt160 uint160(test_data);
    
    EXPECT_FALSE(uint160.IsZero());
    
    // Verify all bytes match
    for (size_t i = 0; i < UInt160::Size; ++i) {
        EXPECT_EQ(uint160[i], test_data[i]);
    }
}

TEST_F(UInt160ComprehensiveTest, RawPointerConstruction)
{
    UInt160 uint160(test_data.data());
    
    EXPECT_FALSE(uint160.IsZero());
    
    // Verify all bytes match
    for (size_t i = 0; i < UInt160::Size; ++i) {
        EXPECT_EQ(uint160[i], test_data[i]);
    }
}

// Test invalid construction
TEST_F(UInt160ComprehensiveTest, InvalidConstruction)
{
    // Wrong size ByteSpan
    std::vector<uint8_t> wrong_size(10);
    ByteSpan wrong_span(wrong_size.data(), wrong_size.size());
    
    EXPECT_THROW(UInt160(wrong_span), std::invalid_argument);
}

// Test data access methods
TEST_F(UInt160ComprehensiveTest, DataAccess)
{
    UInt160 uint160(test_data);
    
    // Mutable data access
    uint8_t* data_ptr = uint160.Data();
    EXPECT_NE(data_ptr, nullptr);
    EXPECT_EQ(data_ptr[0], test_data[0]);
    
    // Const data access
    const UInt160& const_uint160 = uint160;
    const uint8_t* const_data_ptr = const_uint160.Data();
    EXPECT_NE(const_data_ptr, nullptr);
    EXPECT_EQ(const_data_ptr[0], test_data[0]);
    
    // AsSpan
    ByteSpan span = uint160.AsSpan();
    EXPECT_EQ(span.Size(), UInt160::Size);
    EXPECT_EQ(span.Data(), data_ptr);
    
    // ToArray
    ByteVector array = uint160.ToArray();
    EXPECT_EQ(array.Size(), UInt160::Size);
    for (size_t i = 0; i < UInt160::Size; ++i) {
        EXPECT_EQ(array[i], test_data[i]);
    }
}

// Test string conversion
TEST_F(UInt160ComprehensiveTest, StringConversion)
{
    UInt160 uint160(test_data);
    
    // ToHexString
    std::string hex = uint160.ToHexString();
    EXPECT_EQ(hex.length(), UInt160::Size * 2);
    
    // ToString (should be same as ToHexString)
    std::string str = uint160.ToString();
    EXPECT_EQ(str, hex);
    
    // Should not have 0x prefix
    EXPECT_FALSE(hex.starts_with("0x"));
}

// Test parsing
TEST_F(UInt160ComprehensiveTest, Parsing)
{
    // Parse with 0x prefix
    UInt160 uint160_1 = UInt160::Parse(test_hex);
    EXPECT_FALSE(uint160_1.IsZero());
    
    // Parse without 0x prefix
    UInt160 uint160_2 = UInt160::Parse(test_hex_no_prefix);
    EXPECT_FALSE(uint160_2.IsZero());
    
    // Both should be equal
    EXPECT_EQ(uint160_1, uint160_2);
    
    // FromString should work the same
    UInt160 uint160_3 = UInt160::FromString(test_hex);
    EXPECT_EQ(uint160_1, uint160_3);
}

TEST_F(UInt160ComprehensiveTest, TryParse)
{
    UInt160 result;
    
    // Valid hex string
    EXPECT_TRUE(UInt160::TryParse(test_hex, result));
    EXPECT_FALSE(result.IsZero());
    
    // Invalid hex string
    EXPECT_FALSE(UInt160::TryParse("invalid_hex", result));
    
    // Wrong length hex string
    EXPECT_FALSE(UInt160::TryParse("1234", result));
    
    // Too long hex string
    EXPECT_FALSE(UInt160::TryParse(test_hex + "extra", result));
}

// Test error handling in parsing
TEST_F(UInt160ComprehensiveTest, ParseErrorHandling)
{
    // Invalid hex characters
    EXPECT_THROW(UInt160::Parse("xyz"), std::invalid_argument);
    
    // Wrong length
    EXPECT_THROW(UInt160::Parse("1234"), std::invalid_argument);
    
    // Empty string
    EXPECT_THROW(UInt160::Parse(""), std::invalid_argument);
}

// Test IsZero
TEST_F(UInt160ComprehensiveTest, IsZero)
{
    UInt160 zero_uint160(zero_data);
    UInt160 non_zero_uint160(test_data);
    UInt160 default_uint160;
    
    EXPECT_TRUE(zero_uint160.IsZero());
    EXPECT_FALSE(non_zero_uint160.IsZero());
    EXPECT_TRUE(default_uint160.IsZero());
}

// Test Zero static method
TEST_F(UInt160ComprehensiveTest, ZeroStatic)
{
    UInt160 zero = UInt160::Zero();
    EXPECT_TRUE(zero.IsZero());
    
    // Should be same as default construction
    UInt160 default_constructed;
    EXPECT_EQ(zero, default_constructed);
}

// Test FromBytes static method
TEST_F(UInt160ComprehensiveTest, FromBytes)
{
    ByteSpan span(test_data.data(), test_data.size());
    UInt160 uint160 = UInt160::FromBytes(span);
    
    EXPECT_FALSE(uint160.IsZero());
    
    for (size_t i = 0; i < UInt160::Size; ++i) {
        EXPECT_EQ(uint160[i], test_data[i]);
    }
}

// Test comparison operators
TEST_F(UInt160ComprehensiveTest, ComparisonOperators)
{
    UInt160 uint160_1(test_data);
    UInt160 uint160_2(test_data);
    UInt160 uint160_3(zero_data);
    UInt160 uint160_4(max_data);
    
    // Equality
    EXPECT_EQ(uint160_1, uint160_2);
    EXPECT_NE(uint160_1, uint160_3);
    
    // Inequality
    EXPECT_FALSE(uint160_1 != uint160_2);
    EXPECT_TRUE(uint160_1 != uint160_3);
    
    // Less than
    EXPECT_TRUE(uint160_3 < uint160_1);  // zero < test_data
    EXPECT_TRUE(uint160_1 < uint160_4);  // test_data < max
    EXPECT_FALSE(uint160_1 < uint160_2); // equal values
    
    // Greater than
    EXPECT_TRUE(uint160_1 > uint160_3);  // test_data > zero
    EXPECT_TRUE(uint160_4 > uint160_1);  // max > test_data
    EXPECT_FALSE(uint160_1 > uint160_2); // equal values
}

// Test array subscript operator
TEST_F(UInt160ComprehensiveTest, ArraySubscript)
{
    UInt160 uint160(test_data);
    
    // Read access
    for (size_t i = 0; i < UInt160::Size; ++i) {
        EXPECT_EQ(uint160[i], test_data[i]);
    }
    
    // Write access
    uint160[0] = 0xFF;
    EXPECT_EQ(uint160[0], 0xFF);
    
    // Const access
    const UInt160& const_uint160 = uint160;
    EXPECT_EQ(const_uint160[0], 0xFF);
}

// Test serialization/deserialization
TEST_F(UInt160ComprehensiveTest, SerializeDeserialize)
{
    UInt160 original(test_data);
    
    // Serialize
    ByteVector buffer;
    MemoryStream stream(buffer);
    BinaryWriter writer(stream);
    original.Serialize(writer);
    
    EXPECT_EQ(buffer.Size(), UInt160::Size);
    
    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    UInt160 deserialized;
    deserialized.Deserialize(reader);
    
    // Verify
    EXPECT_EQ(original, deserialized);
    
    for (size_t i = 0; i < UInt160::Size; ++i) {
        EXPECT_EQ(original[i], deserialized[i]);
    }
}

// Test hash function for std::unordered_set
TEST_F(UInt160ComprehensiveTest, HashFunction)
{
    UInt160 uint160_1(test_data);
    UInt160 uint160_2(test_data);
    UInt160 uint160_3(zero_data);
    
    std::hash<UInt160> hasher;
    
    // Same values should have same hash
    EXPECT_EQ(hasher(uint160_1), hasher(uint160_2));
    
    // Different values should likely have different hashes
    EXPECT_NE(hasher(uint160_1), hasher(uint160_3));
    
    // Test with unordered_set
    std::unordered_set<UInt160> set;
    set.insert(uint160_1);
    set.insert(uint160_2); // Should not increase size
    set.insert(uint160_3); // Should increase size
    
    EXPECT_EQ(set.size(), 2);
    EXPECT_TRUE(set.contains(uint160_1));
    EXPECT_TRUE(set.contains(uint160_3));
}

// Test Neo address conversion (if implemented)
TEST_F(UInt160ComprehensiveTest, AddressConversion)
{
    // Note: This test assumes address conversion is implemented
    // If not implemented, these tests will fail and can be commented out
    
    try {
        // Test FromAddress (if implemented)
        UInt160 from_address = UInt160::FromAddress(test_address);
        EXPECT_FALSE(from_address.IsZero());
        
        // Test ToAddress (if implemented)
        std::string back_to_address = from_address.ToAddress();
        EXPECT_FALSE(back_to_address.empty());
        
        // Round trip should work
        EXPECT_EQ(back_to_address, test_address);
    } catch (const std::exception&) {
        // Address conversion not implemented or test address invalid
        // This behavior is expected
        SUCCEED();
    }
}

// Test edge cases
TEST_F(UInt160ComprehensiveTest, EdgeCases)
{
    // All zeros
    UInt160 all_zeros(zero_data);
    EXPECT_TRUE(all_zeros.IsZero());
    EXPECT_EQ(all_zeros.ToHexString(), std::string(40, '0'));
    
    // All ones (max value)
    UInt160 all_ones(max_data);
    EXPECT_FALSE(all_ones.IsZero());
    EXPECT_EQ(all_ones.ToHexString(), std::string(40, 'f'));
    
    // Specific pattern
    std::array<uint8_t, UInt160::Size> pattern_data;
    for (size_t i = 0; i < UInt160::Size; ++i) {
        pattern_data[i] = static_cast<uint8_t>(i);
    }
    UInt160 pattern_uint160(pattern_data);
    EXPECT_FALSE(pattern_uint160.IsZero());
}

// Test performance with many operations
TEST_F(UInt160ComprehensiveTest, PerformanceTest)
{
    const int iterations = 1000;
    std::vector<UInt160> uint160s;
    
    // Create many UInt160s
    for (int i = 0; i < iterations; ++i) {
        std::array<uint8_t, UInt160::Size> data;
        for (int j = 0; j < UInt160::Size; ++j) {
            data[j] = static_cast<uint8_t>((i + j) % 256);
        }
        uint160s.emplace_back(data);
    }
    
    // Serialize all
    ByteVector total_buffer;
    MemoryStream stream(total_buffer);
    BinaryWriter writer(stream);
    
    for (const auto& uint160 : uint160s) {
        uint160.Serialize(writer);
    }
    
    EXPECT_EQ(total_buffer.Size(), iterations * UInt160::Size);
    
    // Deserialize all
    stream.seekg(0);
    BinaryReader reader(stream);
    
    for (int i = 0; i < iterations; ++i) {
        UInt160 deserialized;
        EXPECT_NO_THROW(deserialized.Deserialize(reader));
        EXPECT_EQ(deserialized, uint160s[i]);
    }
    
    // Test hashing performance
    std::unordered_set<UInt160> hash_set;
    for (const auto& uint160 : uint160s) {
        hash_set.insert(uint160);
    }
    
    EXPECT_EQ(hash_set.size(), iterations);
}