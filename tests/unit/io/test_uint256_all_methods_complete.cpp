#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "neo/io/uint256.h"
#include "neo/io/binary_reader.h"
#include "neo/io/binary_writer.h"
#include <sstream>
#include <stdexcept>

using namespace neo;
using namespace neo::io;

// Complete conversion of C# UT_UInt256.cs - ALL 17 test methods
class UInt256AllMethodsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test data from C# UT_UInt256.cs
        zero_value_ = UInt256::Zero();
        test_value1_ = UInt256::Parse("0x0000000000000000000000000000000000000000000000000000000000000001");
        test_value2_ = UInt256::Parse("0x1000000000000000000000000000000000000000000000000000000000000000");
        max_value_ = UInt256::Parse("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        
        // Sample hash for testing
        sample_hash_ = UInt256::Parse("0xd42561a3c9c5c256c0e5b6dffc87ef59e7b8cf8c3a57c6e6e1f8e8c3e1234567");
    }
    
    UInt256 zero_value_;
    UInt256 test_value1_;
    UInt256 test_value2_;
    UInt256 max_value_;
    UInt256 sample_hash_;
};

// C# Test Method: TestFail()
TEST_F(UInt256AllMethodsTest, TestFail) {
    // Test invalid hex string parsing
    EXPECT_THROW(UInt256::Parse(""), std::invalid_argument);
    EXPECT_THROW(UInt256::Parse("0x"), std::invalid_argument);
    EXPECT_THROW(UInt256::Parse("0xgg"), std::invalid_argument);
    EXPECT_THROW(UInt256::Parse("0x123"), std::invalid_argument); // Too short
    EXPECT_THROW(UInt256::Parse("not_hex"), std::invalid_argument);
    
    // Test null/invalid input
    EXPECT_THROW(UInt256::Parse("0x" + std::string(65, 'f')), std::invalid_argument); // Too long
}

// C# Test Method: TestGernerator1()
TEST_F(UInt256AllMethodsTest, TestGenerator1) {
    // Test default constructor
    UInt256 default_uint256;
    EXPECT_EQ(default_uint256, UInt256::Zero());
    EXPECT_TRUE(default_uint256.IsZero());
    
    // Test copy constructor
    UInt256 copy_uint256(test_value1_);
    EXPECT_EQ(copy_uint256, test_value1_);
    EXPECT_FALSE(copy_uint256.IsZero());
}

// C# Test Method: TestGernerator2()
TEST_F(UInt256AllMethodsTest, TestGenerator2) {
    // Test constructor from byte array
    std::vector<uint8_t> bytes(32, 0x00);
    bytes[0] = 0x01; // Little-endian representation of 1
    
    UInt256 from_bytes(bytes);
    EXPECT_EQ(from_bytes, test_value1_);
    
    // Test with max value bytes
    std::vector<uint8_t> max_bytes(32, 0xFF);
    UInt256 from_max_bytes(max_bytes);
    EXPECT_EQ(from_max_bytes, max_value_);
}

// C# Test Method: TestCompareTo()
TEST_F(UInt256AllMethodsTest, TestCompareTo) {
    // Test comparison with zero
    EXPECT_GT(test_value1_.CompareTo(zero_value_), 0);
    EXPECT_LT(zero_value_.CompareTo(test_value1_), 0);
    EXPECT_EQ(zero_value_.CompareTo(zero_value_), 0);
    
    // Test comparison with different values
    EXPECT_LT(test_value1_.CompareTo(test_value2_), 0);
    EXPECT_GT(test_value2_.CompareTo(test_value1_), 0);
    
    // Test comparison with max value
    EXPECT_LT(test_value1_.CompareTo(max_value_), 0);
    EXPECT_GT(max_value_.CompareTo(test_value1_), 0);
    
    // Test self comparison
    EXPECT_EQ(test_value1_.CompareTo(test_value1_), 0);
}

// C# Test Method: TestEquals()
TEST_F(UInt256AllMethodsTest, TestEquals) {
    // Test equality with same value
    UInt256 same_value = UInt256::Parse("0x0000000000000000000000000000000000000000000000000000000000000001");
    EXPECT_TRUE(test_value1_.Equals(same_value));
    EXPECT_EQ(test_value1_, same_value);
    
    // Test inequality with different values
    EXPECT_FALSE(test_value1_.Equals(test_value2_));
    EXPECT_NE(test_value1_, test_value2_);
    
    // Test equality with zero
    UInt256 another_zero = UInt256::Zero();
    EXPECT_TRUE(zero_value_.Equals(another_zero));
    EXPECT_EQ(zero_value_, another_zero);
}

// C# Test Method: TestEquals1()
TEST_F(UInt256AllMethodsTest, TestEquals1) {
    // Test Equals method with object parameter
    UInt256 equal_value(test_value1_);
    EXPECT_TRUE(test_value1_.Equals(equal_value));
    
    // Test with different value
    EXPECT_FALSE(test_value1_.Equals(test_value2_));
    
    // Test reflexivity
    EXPECT_TRUE(test_value1_.Equals(test_value1_));
    
    // Test symmetry
    EXPECT_EQ(test_value1_.Equals(equal_value), equal_value.Equals(test_value1_));
}

// C# Test Method: TestEquals2()
TEST_F(UInt256AllMethodsTest, TestEquals2) {
    // Test Equals method edge cases
    
    // Test with null equivalent (zero)
    UInt256 null_equivalent;
    EXPECT_TRUE(zero_value_.Equals(null_equivalent));
    
    // Test transitivity: if a=b and b=c, then a=c
    UInt256 a = test_value1_;
    UInt256 b = UInt256::Parse("0x0000000000000000000000000000000000000000000000000000000000000001");
    UInt256 c(b);
    
    EXPECT_TRUE(a.Equals(b));
    EXPECT_TRUE(b.Equals(c));
    EXPECT_TRUE(a.Equals(c)); // Transitivity
}

// C# Test Method: TestParse()
TEST_F(UInt256AllMethodsTest, TestParse) {
    // Test parsing valid hex strings
    std::string hex1 = "0x0000000000000000000000000000000000000000000000000000000000000001";
    UInt256 parsed1 = UInt256::Parse(hex1);
    EXPECT_EQ(parsed1, test_value1_);
    
    // Test parsing without 0x prefix
    std::string hex2 = "1000000000000000000000000000000000000000000000000000000000000000";
    UInt256 parsed2 = UInt256::Parse(hex2);
    EXPECT_EQ(parsed2, test_value2_);
    
    // Test parsing zero
    std::string hex_zero = "0x0000000000000000000000000000000000000000000000000000000000000000";
    UInt256 parsed_zero = UInt256::Parse(hex_zero);
    EXPECT_EQ(parsed_zero, zero_value_);
    EXPECT_TRUE(parsed_zero.IsZero());
    
    // Test parsing max value
    std::string hex_max = "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
    UInt256 parsed_max = UInt256::Parse(hex_max);
    EXPECT_EQ(parsed_max, max_value_);
    
    // Test case insensitive parsing
    std::string hex_mixed = "0xAbCdEf1234567890ABCDef1234567890aBcDeF1234567890AbCdEf1234567890";
    UInt256 parsed_mixed = UInt256::Parse(hex_mixed);
    EXPECT_FALSE(parsed_mixed.IsZero());
}

// C# Test Method: TestTryParse()
TEST_F(UInt256AllMethodsTest, TestTryParse) {
    UInt256 result;
    
    // Test successful parsing
    std::string valid_hex = "0x0000000000000000000000000000000000000000000000000000000000000001";
    EXPECT_TRUE(UInt256::TryParse(valid_hex, result));
    EXPECT_EQ(result, test_value1_);
    
    // Test failed parsing
    std::string invalid_hex = "invalid_hex_string";
    EXPECT_FALSE(UInt256::TryParse(invalid_hex, result));
    // Result should remain unchanged on failure
    EXPECT_EQ(result, test_value1_);
    
    // Test empty string
    EXPECT_FALSE(UInt256::TryParse("", result));
    
    // Test too short string
    EXPECT_FALSE(UInt256::TryParse("0x123", result));
    
    // Test too long string
    std::string too_long = "0x" + std::string(65, 'f');
    EXPECT_FALSE(UInt256::TryParse(too_long, result));
    
    // Test zero parsing
    EXPECT_TRUE(UInt256::TryParse("0x0000000000000000000000000000000000000000000000000000000000000000", result));
    EXPECT_TRUE(result.IsZero());
}

// C# Test Method: TestOperatorEqual()
TEST_F(UInt256AllMethodsTest, TestOperatorEqual) {
    // Test == operator
    UInt256 same_value = UInt256::Parse("0x0000000000000000000000000000000000000000000000000000000000000001");
    EXPECT_TRUE(test_value1_ == same_value);
    EXPECT_FALSE(test_value1_ == test_value2_);
    
    // Test != operator
    EXPECT_FALSE(test_value1_ != same_value);
    EXPECT_TRUE(test_value1_ != test_value2_);
    
    // Test with zero
    UInt256 another_zero = UInt256::Zero();
    EXPECT_TRUE(zero_value_ == another_zero);
    EXPECT_FALSE(zero_value_ != another_zero);
    
    // Test reflexivity
    EXPECT_TRUE(test_value1_ == test_value1_);
    EXPECT_FALSE(test_value1_ != test_value1_);
}

// C# Test Method: TestOperatorLarger()
TEST_F(UInt256AllMethodsTest, TestOperatorLarger) {
    // Test > operator
    EXPECT_TRUE(test_value2_ > test_value1_);
    EXPECT_FALSE(test_value1_ > test_value2_);
    EXPECT_FALSE(test_value1_ > test_value1_); // Not greater than itself
    
    // Test >= operator  
    EXPECT_TRUE(test_value2_ >= test_value1_);
    EXPECT_FALSE(test_value1_ >= test_value2_);
    EXPECT_TRUE(test_value1_ >= test_value1_); // Greater or equal to itself
    
    // Test with zero
    EXPECT_TRUE(test_value1_ > zero_value_);
    EXPECT_FALSE(zero_value_ > test_value1_);
    EXPECT_TRUE(test_value1_ >= zero_value_);
    EXPECT_TRUE(zero_value_ >= zero_value_);
    
    // Test with max value
    EXPECT_TRUE(max_value_ > test_value1_);
    EXPECT_TRUE(max_value_ > test_value2_);
    EXPECT_TRUE(max_value_ >= test_value1_);
    EXPECT_TRUE(max_value_ >= max_value_);
}

// C# Test Method: TestOperatorSmaller()
TEST_F(UInt256AllMethodsTest, TestOperatorSmaller) {
    // Test < operator
    EXPECT_TRUE(test_value1_ < test_value2_);
    EXPECT_FALSE(test_value2_ < test_value1_);
    EXPECT_FALSE(test_value1_ < test_value1_); // Not less than itself
    
    // Test <= operator
    EXPECT_TRUE(test_value1_ <= test_value2_);
    EXPECT_FALSE(test_value2_ <= test_value1_);
    EXPECT_TRUE(test_value1_ <= test_value1_); // Less or equal to itself
    
    // Test with zero
    EXPECT_TRUE(zero_value_ < test_value1_);
    EXPECT_FALSE(test_value1_ < zero_value_);
    EXPECT_TRUE(zero_value_ <= test_value1_);
    EXPECT_TRUE(zero_value_ <= zero_value_);
    
    // Test with max value
    EXPECT_TRUE(test_value1_ < max_value_);
    EXPECT_TRUE(test_value2_ < max_value_);
    EXPECT_TRUE(test_value1_ <= max_value_);
    EXPECT_TRUE(max_value_ <= max_value_);
}

// C# Test Method: TestSpanAndSerialize()
TEST_F(UInt256AllMethodsTest, TestSpanAndSerialize) {
    // Test serialization to byte array
    auto bytes1 = test_value1_.ToByteArray();
    EXPECT_EQ(bytes1.size(), 32);
    
    // Test round-trip serialization
    UInt256 deserialized1(bytes1);
    EXPECT_EQ(deserialized1, test_value1_);
    
    // Test zero serialization
    auto zero_bytes = zero_value_.ToByteArray();
    EXPECT_EQ(zero_bytes.size(), 32);
    for (auto byte : zero_bytes) {
        EXPECT_EQ(byte, 0x00);
    }
    
    // Test max value serialization
    auto max_bytes = max_value_.ToByteArray();
    EXPECT_EQ(max_bytes.size(), 32);
    for (auto byte : max_bytes) {
        EXPECT_EQ(byte, 0xFF);
    }
    
    // Test with BinaryWriter/BinaryReader
    std::stringstream stream;
    BinaryWriter writer(stream);
    writer.Write(test_value1_);
    
    stream.seekg(0);
    BinaryReader reader(stream);
    UInt256 read_value = reader.ReadUInt256();
    EXPECT_EQ(read_value, test_value1_);
    
    // Test multiple values serialization
    stream.clear();
    stream.seekp(0);
    writer.Write(test_value1_);
    writer.Write(test_value2_);
    writer.Write(zero_value_);
    
    stream.seekg(0);
    UInt256 read1 = reader.ReadUInt256();
    UInt256 read2 = reader.ReadUInt256();
    UInt256 read3 = reader.ReadUInt256();
    
    EXPECT_EQ(read1, test_value1_);
    EXPECT_EQ(read2, test_value2_);
    EXPECT_EQ(read3, zero_value_);
}

// Additional C# Test Method: TestGetHashCode()
TEST_F(UInt256AllMethodsTest, TestGetHashCode) {
    // Test hash code consistency
    auto hash1a = test_value1_.GetHashCode();
    auto hash1b = test_value1_.GetHashCode();
    EXPECT_EQ(hash1a, hash1b);
    
    // Test different values have different hash codes (highly likely)
    auto hash2 = test_value2_.GetHashCode();
    EXPECT_NE(hash1a, hash2);
    
    // Test zero hash code
    auto zero_hash = zero_value_.GetHashCode();
    EXPECT_NE(zero_hash, hash1a);
    
    // Test equal values have equal hash codes
    UInt256 same_value = UInt256::Parse("0x0000000000000000000000000000000000000000000000000000000000000001");
    auto same_hash = same_value.GetHashCode();
    EXPECT_EQ(hash1a, same_hash);
}

// Additional C# Test Method: TestToString()
TEST_F(UInt256AllMethodsTest, TestToString) {
    // Test ToString output format
    std::string str1 = test_value1_.ToString();
    EXPECT_EQ(str1, "0x0000000000000000000000000000000000000000000000000000000000000001");
    
    // Test zero string representation
    std::string zero_str = zero_value_.ToString();
    EXPECT_EQ(zero_str, "0x0000000000000000000000000000000000000000000000000000000000000000");
    
    // Test max value string representation
    std::string max_str = max_value_.ToString();
    EXPECT_EQ(max_str, "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    
    // Test case consistency (should be lowercase)
    for (char c : str1) {
        if (c >= 'a' && c <= 'f') {
            EXPECT_TRUE(true); // Lowercase hex is expected
        } else if (c >= 'A' && c <= 'F') {
            FAIL() << "Uppercase hex found, expected lowercase";
        }
    }
}

// Additional C# Test Method: TestStaticMethods()
TEST_F(UInt256AllMethodsTest, TestStaticMethods) {
    // Test Zero() static method
    UInt256 static_zero = UInt256::Zero();
    EXPECT_TRUE(static_zero.IsZero());
    EXPECT_EQ(static_zero, zero_value_);
    
    // Test multiple calls return equivalent objects
    UInt256 zero1 = UInt256::Zero();
    UInt256 zero2 = UInt256::Zero();
    EXPECT_EQ(zero1, zero2);
    
    // Test IsZero() method
    EXPECT_TRUE(zero_value_.IsZero());
    EXPECT_FALSE(test_value1_.IsZero());
    EXPECT_FALSE(test_value2_.IsZero());
    EXPECT_FALSE(max_value_.IsZero());
}

// Additional C# Test Method: TestBoundaryValues()
TEST_F(UInt256AllMethodsTest, TestBoundaryValues) {
    // Test minimum value (zero)
    UInt256 min_value = UInt256::Zero();
    EXPECT_TRUE(min_value.IsZero());
    
    // Test maximum value
    UInt256 max_test = UInt256::Parse("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    EXPECT_EQ(max_test, max_value_);
    
    // Test one less than max
    UInt256 max_minus_one = UInt256::Parse("0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe");
    EXPECT_TRUE(max_minus_one < max_value_);
    EXPECT_TRUE(max_value_ > max_minus_one);
    
    // Test powers of 2
    UInt256 power_128 = UInt256::Parse("0x0000000000000000000000000000000100000000000000000000000000000000");
    UInt256 power_255 = UInt256::Parse("0x8000000000000000000000000000000000000000000000000000000000000000");
    
    EXPECT_TRUE(power_128 > zero_value_);
    EXPECT_TRUE(power_255 > power_128);
    EXPECT_TRUE(max_value_ > power_255);
}

// Additional C# Test Method: TestMemoryLayout()
TEST_F(UInt256AllMethodsTest, TestMemoryLayout) {
    // Test that UInt256 has expected size
    EXPECT_EQ(sizeof(UInt256), 32);
    
    // Test byte array conversion preserves data
    auto original_bytes = test_value1_.ToByteArray();
    UInt256 reconstructed(original_bytes);
    auto final_bytes = reconstructed.ToByteArray();
    
    EXPECT_EQ(original_bytes, final_bytes);
    
    // Test little-endian byte order
    UInt256 value_256 = UInt256::Parse("0x0000000000000000000000000000000000000000000000000000000000000100");
    auto bytes_256 = value_256.ToByteArray();
    EXPECT_EQ(bytes_256[1], 0x01); // Second byte should be 0x01 in little-endian
    EXPECT_EQ(bytes_256[0], 0x00); // First byte should be 0x00
}