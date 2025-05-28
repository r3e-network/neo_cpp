#include <gtest/gtest.h>
#include <neo/cli/type_converters.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

namespace neo::cli::tests
{
    class TypeConvertersTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // Test setup
        }
    };

    TEST_F(TypeConvertersTest, TestStringToInt)
    {
        // Test valid integers
        EXPECT_EQ(123, TypeConverters::ToInt("123"));
        EXPECT_EQ(-456, TypeConverters::ToInt("-456"));
        EXPECT_EQ(0, TypeConverters::ToInt("0"));
        
        // Test invalid integers
        EXPECT_THROW(TypeConverters::ToInt("abc"), std::invalid_argument);
        EXPECT_THROW(TypeConverters::ToInt("123.45"), std::invalid_argument);
        EXPECT_THROW(TypeConverters::ToInt(""), std::invalid_argument);
    }

    TEST_F(TypeConvertersTest, TestStringToUInt)
    {
        // Test valid unsigned integers
        EXPECT_EQ(123u, TypeConverters::ToUInt("123"));
        EXPECT_EQ(0u, TypeConverters::ToUInt("0"));
        EXPECT_EQ(4294967295u, TypeConverters::ToUInt("4294967295"));
        
        // Test invalid unsigned integers
        EXPECT_THROW(TypeConverters::ToUInt("-123"), std::invalid_argument);
        EXPECT_THROW(TypeConverters::ToUInt("abc"), std::invalid_argument);
        EXPECT_THROW(TypeConverters::ToUInt(""), std::invalid_argument);
    }

    TEST_F(TypeConvertersTest, TestStringToLong)
    {
        // Test valid long integers
        EXPECT_EQ(123456789L, TypeConverters::ToLong("123456789"));
        EXPECT_EQ(-987654321L, TypeConverters::ToLong("-987654321"));
        EXPECT_EQ(0L, TypeConverters::ToLong("0"));
        
        // Test invalid long integers
        EXPECT_THROW(TypeConverters::ToLong("abc"), std::invalid_argument);
        EXPECT_THROW(TypeConverters::ToLong("123.45"), std::invalid_argument);
        EXPECT_THROW(TypeConverters::ToLong(""), std::invalid_argument);
    }

    TEST_F(TypeConvertersTest, TestStringToDouble)
    {
        // Test valid doubles
        EXPECT_DOUBLE_EQ(123.45, TypeConverters::ToDouble("123.45"));
        EXPECT_DOUBLE_EQ(-67.89, TypeConverters::ToDouble("-67.89"));
        EXPECT_DOUBLE_EQ(0.0, TypeConverters::ToDouble("0"));
        EXPECT_DOUBLE_EQ(0.0, TypeConverters::ToDouble("0.0"));
        
        // Test invalid doubles
        EXPECT_THROW(TypeConverters::ToDouble("abc"), std::invalid_argument);
        EXPECT_THROW(TypeConverters::ToDouble(""), std::invalid_argument);
    }

    TEST_F(TypeConvertersTest, TestStringToBool)
    {
        // Test valid booleans
        EXPECT_TRUE(TypeConverters::ToBool("true"));
        EXPECT_TRUE(TypeConverters::ToBool("True"));
        EXPECT_TRUE(TypeConverters::ToBool("TRUE"));
        EXPECT_TRUE(TypeConverters::ToBool("1"));
        
        EXPECT_FALSE(TypeConverters::ToBool("false"));
        EXPECT_FALSE(TypeConverters::ToBool("False"));
        EXPECT_FALSE(TypeConverters::ToBool("FALSE"));
        EXPECT_FALSE(TypeConverters::ToBool("0"));
        
        // Test invalid booleans
        EXPECT_THROW(TypeConverters::ToBool("abc"), std::invalid_argument);
        EXPECT_THROW(TypeConverters::ToBool("2"), std::invalid_argument);
        EXPECT_THROW(TypeConverters::ToBool(""), std::invalid_argument);
    }

    TEST_F(TypeConvertersTest, TestStringToUInt160)
    {
        // Test valid UInt160
        std::string valid_hash = "0x1234567890123456789012345678901234567890";
        auto uint160 = TypeConverters::ToUInt160(valid_hash);
        EXPECT_FALSE(uint160.IsZero());
        
        // Test invalid UInt160
        EXPECT_THROW(TypeConverters::ToUInt160("invalid"), std::invalid_argument);
        EXPECT_THROW(TypeConverters::ToUInt160("0x123"), std::invalid_argument); // Too short
        EXPECT_THROW(TypeConverters::ToUInt160(""), std::invalid_argument);
    }

    TEST_F(TypeConvertersTest, TestStringToUInt256)
    {
        // Test valid UInt256
        std::string valid_hash = "0x1234567890123456789012345678901234567890123456789012345678901234";
        auto uint256 = TypeConverters::ToUInt256(valid_hash);
        EXPECT_FALSE(uint256.IsZero());
        
        // Test invalid UInt256
        EXPECT_THROW(TypeConverters::ToUInt256("invalid"), std::invalid_argument);
        EXPECT_THROW(TypeConverters::ToUInt256("0x123"), std::invalid_argument); // Too short
        EXPECT_THROW(TypeConverters::ToUInt256(""), std::invalid_argument);
    }

    TEST_F(TypeConvertersTest, TestHexStringToBytes)
    {
        // Test valid hex strings
        auto bytes1 = TypeConverters::HexToBytes("0123456789abcdef");
        std::vector<uint8_t> expected1 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
        EXPECT_EQ(expected1, bytes1);
        
        auto bytes2 = TypeConverters::HexToBytes("0x0123456789abcdef");
        EXPECT_EQ(expected1, bytes2);
        
        // Test empty hex string
        auto bytes3 = TypeConverters::HexToBytes("");
        EXPECT_TRUE(bytes3.empty());
        
        // Test invalid hex strings
        EXPECT_THROW(TypeConverters::HexToBytes("xyz"), std::invalid_argument);
        EXPECT_THROW(TypeConverters::HexToBytes("123"), std::invalid_argument); // Odd length
    }

    TEST_F(TypeConvertersTest, TestBytesToHexString)
    {
        // Test valid byte arrays
        std::vector<uint8_t> bytes1 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
        std::string hex1 = TypeConverters::BytesToHex(bytes1);
        EXPECT_EQ("0123456789abcdef", hex1);
        
        // Test empty byte array
        std::vector<uint8_t> empty_bytes;
        std::string empty_hex = TypeConverters::BytesToHex(empty_bytes);
        EXPECT_TRUE(empty_hex.empty());
        
        // Test single byte
        std::vector<uint8_t> single_byte = {0xff};
        std::string single_hex = TypeConverters::BytesToHex(single_byte);
        EXPECT_EQ("ff", single_hex);
    }

    TEST_F(TypeConvertersTest, TestHexRoundTrip)
    {
        // Test round-trip conversion
        std::vector<uint8_t> original = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
        
        std::string hex = TypeConverters::BytesToHex(original);
        auto converted = TypeConverters::HexToBytes(hex);
        
        EXPECT_EQ(original, converted);
    }

    TEST_F(TypeConvertersTest, TestStringToAddress)
    {
        // Test valid Neo address (this would depend on the actual address format)
        std::string valid_address = "NZNos2WqwVfNUXNj5VEqvvPzAqze3RXyP3";
        
        // This test assumes there's an address validation function
        EXPECT_NO_THROW({
            auto script_hash = TypeConverters::AddressToScriptHash(valid_address);
            EXPECT_FALSE(script_hash.IsZero());
        });
        
        // Test invalid address
        EXPECT_THROW(TypeConverters::AddressToScriptHash("invalid_address"), std::invalid_argument);
        EXPECT_THROW(TypeConverters::AddressToScriptHash(""), std::invalid_argument);
    }

    TEST_F(TypeConvertersTest, TestScriptHashToAddress)
    {
        // Test valid script hash
        auto script_hash = io::UInt160::Parse("0x1234567890123456789012345678901234567890");
        
        std::string address = TypeConverters::ScriptHashToAddress(script_hash);
        EXPECT_FALSE(address.empty());
        EXPECT_GT(address.length(), 20); // Should be reasonable length
        
        // Test round-trip
        auto converted_hash = TypeConverters::AddressToScriptHash(address);
        EXPECT_EQ(script_hash, converted_hash);
    }

    TEST_F(TypeConvertersTest, TestNumberFormats)
    {
        // Test different number formats
        EXPECT_EQ(255, TypeConverters::ToInt("255"));
        EXPECT_EQ(255, TypeConverters::ToInt("0xff", 16)); // Hex
        EXPECT_EQ(255, TypeConverters::ToInt("0377", 8));  // Octal
        EXPECT_EQ(255, TypeConverters::ToInt("11111111", 2)); // Binary
    }

    TEST_F(TypeConvertersTest, TestLargeNumbers)
    {
        // Test large numbers
        EXPECT_EQ(9223372036854775807LL, TypeConverters::ToLong("9223372036854775807"));
        EXPECT_EQ(18446744073709551615ULL, TypeConverters::ToULong("18446744073709551615"));
        
        // Test overflow
        EXPECT_THROW(TypeConverters::ToInt("999999999999999999999"), std::out_of_range);
    }

    TEST_F(TypeConvertersTest, TestFloatingPointPrecision)
    {
        // Test floating point precision
        EXPECT_DOUBLE_EQ(3.141592653589793, TypeConverters::ToDouble("3.141592653589793"));
        EXPECT_FLOAT_EQ(3.14159f, TypeConverters::ToFloat("3.14159"));
        
        // Test scientific notation
        EXPECT_DOUBLE_EQ(1.23e10, TypeConverters::ToDouble("1.23e10"));
        EXPECT_DOUBLE_EQ(1.23e-10, TypeConverters::ToDouble("1.23e-10"));
    }

    TEST_F(TypeConvertersTest, TestWhitespaceHandling)
    {
        // Test strings with whitespace
        EXPECT_EQ(123, TypeConverters::ToInt("  123  "));
        EXPECT_DOUBLE_EQ(45.67, TypeConverters::ToDouble("  45.67  "));
        EXPECT_TRUE(TypeConverters::ToBool("  true  "));
        
        // Test hex with whitespace
        auto bytes = TypeConverters::HexToBytes("  0123456789abcdef  ");
        std::vector<uint8_t> expected = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
        EXPECT_EQ(expected, bytes);
    }

    TEST_F(TypeConvertersTest, TestCaseInsensitivity)
    {
        // Test case insensitive hex
        auto bytes1 = TypeConverters::HexToBytes("0123456789ABCDEF");
        auto bytes2 = TypeConverters::HexToBytes("0123456789abcdef");
        EXPECT_EQ(bytes1, bytes2);
        
        // Test case insensitive boolean
        EXPECT_TRUE(TypeConverters::ToBool("TRUE"));
        EXPECT_TRUE(TypeConverters::ToBool("true"));
        EXPECT_TRUE(TypeConverters::ToBool("True"));
        
        EXPECT_FALSE(TypeConverters::ToBool("FALSE"));
        EXPECT_FALSE(TypeConverters::ToBool("false"));
        EXPECT_FALSE(TypeConverters::ToBool("False"));
    }
}
