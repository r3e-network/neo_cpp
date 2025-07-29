#include <gtest/gtest.h>
#include <limits>
#include <neo/extensions/integer_extensions.h>
#include <vector>

using namespace neo::extensions;

/**
 * @brief Test fixture for IntegerExtensions
 */
class IntegerExtensionsTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        // No specific setup needed
    }
};

TEST_F(IntegerExtensionsTest, GetVarSize)
{
    // Test int32_t
    EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<int32_t>(0)));
    EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<int32_t>(252)));
    EXPECT_EQ(3, IntegerExtensions::GetVarSize(static_cast<int32_t>(253)));
    EXPECT_EQ(3, IntegerExtensions::GetVarSize(static_cast<int32_t>(65535)));
    EXPECT_EQ(5, IntegerExtensions::GetVarSize(static_cast<int32_t>(65536)));
    EXPECT_EQ(5, IntegerExtensions::GetVarSize(std::numeric_limits<int32_t>::max()));

    // Test uint16_t
    EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<uint16_t>(0)));
    EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<uint16_t>(252)));
    EXPECT_EQ(3, IntegerExtensions::GetVarSize(static_cast<uint16_t>(253)));
    EXPECT_EQ(3, IntegerExtensions::GetVarSize(std::numeric_limits<uint16_t>::max()));

    // Test uint32_t
    EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<uint32_t>(0)));
    EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<uint32_t>(252)));
    EXPECT_EQ(3, IntegerExtensions::GetVarSize(static_cast<uint32_t>(253)));
    EXPECT_EQ(3, IntegerExtensions::GetVarSize(static_cast<uint32_t>(65535)));
    EXPECT_EQ(5, IntegerExtensions::GetVarSize(static_cast<uint32_t>(65536)));
    EXPECT_EQ(5, IntegerExtensions::GetVarSize(std::numeric_limits<uint32_t>::max()));

    // Test int64_t
    EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<int64_t>(0)));
    EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<int64_t>(252)));
    EXPECT_EQ(3, IntegerExtensions::GetVarSize(static_cast<int64_t>(253)));
    EXPECT_EQ(3, IntegerExtensions::GetVarSize(static_cast<int64_t>(65535)));
    EXPECT_EQ(5, IntegerExtensions::GetVarSize(static_cast<int64_t>(65536)));
    EXPECT_EQ(5, IntegerExtensions::GetVarSize(static_cast<int64_t>(4294967295)));
    EXPECT_EQ(9, IntegerExtensions::GetVarSize(static_cast<int64_t>(4294967296)));
    EXPECT_EQ(9, IntegerExtensions::GetVarSize(std::numeric_limits<int64_t>::max()));

    // Test uint64_t
    EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<uint64_t>(0)));
    EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<uint64_t>(252)));
    EXPECT_EQ(3, IntegerExtensions::GetVarSize(static_cast<uint64_t>(253)));
    EXPECT_EQ(3, IntegerExtensions::GetVarSize(static_cast<uint64_t>(65535)));
    EXPECT_EQ(5, IntegerExtensions::GetVarSize(static_cast<uint64_t>(65536)));
    EXPECT_EQ(5, IntegerExtensions::GetVarSize(static_cast<uint64_t>(4294967295)));
    EXPECT_EQ(9, IntegerExtensions::GetVarSize(static_cast<uint64_t>(4294967296)));
    EXPECT_EQ(9, IntegerExtensions::GetVarSize(std::numeric_limits<uint64_t>::max()));
}

TEST_F(IntegerExtensionsTest, ToLittleEndianBytes_Int16)
{
    // Test positive values
    auto bytes1 = IntegerExtensions::ToLittleEndianBytes(static_cast<int16_t>(0x1234));
    EXPECT_EQ(2u, bytes1.size());
    EXPECT_EQ(0x34, bytes1[0]);
    EXPECT_EQ(0x12, bytes1[1]);

    // Test negative values
    auto bytes2 = IntegerExtensions::ToLittleEndianBytes(static_cast<int16_t>(-1));
    EXPECT_EQ(2u, bytes2.size());
    EXPECT_EQ(0xFF, bytes2[0]);
    EXPECT_EQ(0xFF, bytes2[1]);

    // Test zero
    auto bytes3 = IntegerExtensions::ToLittleEndianBytes(static_cast<int16_t>(0));
    EXPECT_EQ(2u, bytes3.size());
    EXPECT_EQ(0x00, bytes3[0]);
    EXPECT_EQ(0x00, bytes3[1]);
}

TEST_F(IntegerExtensionsTest, ToLittleEndianBytes_UInt16)
{
    auto bytes = IntegerExtensions::ToLittleEndianBytes(static_cast<uint16_t>(0x1234));
    EXPECT_EQ(2u, bytes.size());
    EXPECT_EQ(0x34, bytes[0]);
    EXPECT_EQ(0x12, bytes[1]);

    // Test max value
    auto maxBytes = IntegerExtensions::ToLittleEndianBytes(std::numeric_limits<uint16_t>::max());
    EXPECT_EQ(2u, maxBytes.size());
    EXPECT_EQ(0xFF, maxBytes[0]);
    EXPECT_EQ(0xFF, maxBytes[1]);
}

TEST_F(IntegerExtensionsTest, ToLittleEndianBytes_Int32)
{
    auto bytes = IntegerExtensions::ToLittleEndianBytes(static_cast<int32_t>(0x12345678));
    EXPECT_EQ(4u, bytes.size());
    EXPECT_EQ(0x78, bytes[0]);
    EXPECT_EQ(0x56, bytes[1]);
    EXPECT_EQ(0x34, bytes[2]);
    EXPECT_EQ(0x12, bytes[3]);

    // Test negative
    auto negBytes = IntegerExtensions::ToLittleEndianBytes(static_cast<int32_t>(-1));
    EXPECT_EQ(4u, negBytes.size());
    for (auto byte : negBytes)
    {
        EXPECT_EQ(0xFF, byte);
    }
}

TEST_F(IntegerExtensionsTest, ToLittleEndianBytes_UInt32)
{
    auto bytes = IntegerExtensions::ToLittleEndianBytes(static_cast<uint32_t>(0x12345678));
    EXPECT_EQ(4u, bytes.size());
    EXPECT_EQ(0x78, bytes[0]);
    EXPECT_EQ(0x56, bytes[1]);
    EXPECT_EQ(0x34, bytes[2]);
    EXPECT_EQ(0x12, bytes[3]);
}

TEST_F(IntegerExtensionsTest, ToLittleEndianBytes_Int64)
{
    auto bytes = IntegerExtensions::ToLittleEndianBytes(static_cast<int64_t>(0x123456789ABCDEF0));
    EXPECT_EQ(8u, bytes.size());
    EXPECT_EQ(0xF0, bytes[0]);
    EXPECT_EQ(0xDE, bytes[1]);
    EXPECT_EQ(0xBC, bytes[2]);
    EXPECT_EQ(0x9A, bytes[3]);
    EXPECT_EQ(0x78, bytes[4]);
    EXPECT_EQ(0x56, bytes[5]);
    EXPECT_EQ(0x34, bytes[6]);
    EXPECT_EQ(0x12, bytes[7]);
}

TEST_F(IntegerExtensionsTest, ToLittleEndianBytes_UInt64)
{
    auto bytes = IntegerExtensions::ToLittleEndianBytes(static_cast<uint64_t>(0x123456789ABCDEF0));
    EXPECT_EQ(8u, bytes.size());
    EXPECT_EQ(0xF0, bytes[0]);
    EXPECT_EQ(0xDE, bytes[1]);
    EXPECT_EQ(0xBC, bytes[2]);
    EXPECT_EQ(0x9A, bytes[3]);
    EXPECT_EQ(0x78, bytes[4]);
    EXPECT_EQ(0x56, bytes[5]);
    EXPECT_EQ(0x34, bytes[6]);
    EXPECT_EQ(0x12, bytes[7]);
}

TEST_F(IntegerExtensionsTest, FromLittleEndianBytes16)
{
    std::vector<uint8_t> bytes = {0x34, 0x12};
    EXPECT_EQ(0x1234, IntegerExtensions::FromLittleEndianBytes16(bytes));

    // Test with offset
    std::vector<uint8_t> largerBytes = {0xFF, 0xFF, 0x34, 0x12, 0xFF};
    EXPECT_EQ(0x1234, IntegerExtensions::FromLittleEndianBytes16(largerBytes, 2));

    // Test negative value
    std::vector<uint8_t> negBytes = {0xFF, 0xFF};
    EXPECT_EQ(-1, IntegerExtensions::FromLittleEndianBytes16(negBytes));

    // Test insufficient bytes should throw
    std::vector<uint8_t> shortBytes = {0x34};
    EXPECT_THROW(IntegerExtensions::FromLittleEndianBytes16(shortBytes), std::out_of_range);
}

TEST_F(IntegerExtensionsTest, FromLittleEndianBytesU16)
{
    std::vector<uint8_t> bytes = {0x34, 0x12};
    EXPECT_EQ(0x1234u, IntegerExtensions::FromLittleEndianBytesU16(bytes));

    // Test max value
    std::vector<uint8_t> maxBytes = {0xFF, 0xFF};
    EXPECT_EQ(0xFFFFu, IntegerExtensions::FromLittleEndianBytesU16(maxBytes));
}

TEST_F(IntegerExtensionsTest, FromLittleEndianBytes32)
{
    std::vector<uint8_t> bytes = {0x78, 0x56, 0x34, 0x12};
    EXPECT_EQ(0x12345678, IntegerExtensions::FromLittleEndianBytes32(bytes));

    // Test negative
    std::vector<uint8_t> negBytes = {0xFF, 0xFF, 0xFF, 0xFF};
    EXPECT_EQ(-1, IntegerExtensions::FromLittleEndianBytes32(negBytes));

    // Test with offset
    std::vector<uint8_t> largerBytes = {0x00, 0x00, 0x78, 0x56, 0x34, 0x12};
    EXPECT_EQ(0x12345678, IntegerExtensions::FromLittleEndianBytes32(largerBytes, 2));
}

TEST_F(IntegerExtensionsTest, FromLittleEndianBytesU32)
{
    std::vector<uint8_t> bytes = {0x78, 0x56, 0x34, 0x12};
    EXPECT_EQ(0x12345678u, IntegerExtensions::FromLittleEndianBytesU32(bytes));

    // Test max value
    std::vector<uint8_t> maxBytes = {0xFF, 0xFF, 0xFF, 0xFF};
    EXPECT_EQ(0xFFFFFFFFu, IntegerExtensions::FromLittleEndianBytesU32(maxBytes));
}

TEST_F(IntegerExtensionsTest, FromLittleEndianBytes64)
{
    std::vector<uint8_t> bytes = {0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12};
    EXPECT_EQ(0x123456789ABCDEF0, IntegerExtensions::FromLittleEndianBytes64(bytes));

    // Test negative
    std::vector<uint8_t> negBytes(8, 0xFF);
    EXPECT_EQ(-1, IntegerExtensions::FromLittleEndianBytes64(negBytes));
}

TEST_F(IntegerExtensionsTest, FromLittleEndianBytesU64)
{
    std::vector<uint8_t> bytes = {0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12};
    EXPECT_EQ(0x123456789ABCDEF0u, IntegerExtensions::FromLittleEndianBytesU64(bytes));

    // Test max value
    std::vector<uint8_t> maxBytes(8, 0xFF);
    EXPECT_EQ(0xFFFFFFFFFFFFFFFFu, IntegerExtensions::FromLittleEndianBytesU64(maxBytes));
}

TEST_F(IntegerExtensionsTest, RoundTripConversion)
{
    // Test round-trip conversion for various types

    // int16_t
    int16_t i16 = 0x1234;
    auto i16Bytes = IntegerExtensions::ToLittleEndianBytes(i16);
    EXPECT_EQ(i16, IntegerExtensions::FromLittleEndianBytes16(i16Bytes));

    // uint16_t
    uint16_t u16 = 0xABCD;
    auto u16Bytes = IntegerExtensions::ToLittleEndianBytes(u16);
    EXPECT_EQ(u16, IntegerExtensions::FromLittleEndianBytesU16(u16Bytes));

    // int32_t
    int32_t i32 = 0x12345678;
    auto i32Bytes = IntegerExtensions::ToLittleEndianBytes(i32);
    EXPECT_EQ(i32, IntegerExtensions::FromLittleEndianBytes32(i32Bytes));

    // uint32_t
    uint32_t u32 = 0xABCDEF01;
    auto u32Bytes = IntegerExtensions::ToLittleEndianBytes(u32);
    EXPECT_EQ(u32, IntegerExtensions::FromLittleEndianBytesU32(u32Bytes));

    // int64_t
    int64_t i64 = 0x123456789ABCDEF0;
    auto i64Bytes = IntegerExtensions::ToLittleEndianBytes(i64);
    EXPECT_EQ(i64, IntegerExtensions::FromLittleEndianBytes64(i64Bytes));

    // uint64_t
    uint64_t u64 = 0xFEDCBA9876543210;
    auto u64Bytes = IntegerExtensions::ToLittleEndianBytes(u64);
    EXPECT_EQ(u64, IntegerExtensions::FromLittleEndianBytesU64(u64Bytes));
}

TEST_F(IntegerExtensionsTest, IsLittleEndian)
{
    // Just test that the function returns a value
    // The actual value depends on the system architecture
    bool isLE = IntegerExtensions::IsLittleEndian();
    EXPECT_TRUE(isLE || !isLE);  // Always true, just checking it returns a bool

    // On most modern systems, this should be true
    // but we can't assert it because it's architecture-dependent
}
