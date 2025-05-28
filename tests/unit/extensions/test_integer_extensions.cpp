#include <gtest/gtest.h>
#include <neo/extensions/integer_extensions.h>

namespace neo::extensions::tests
{
    class IntegerExtensionsTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // Test values
        }
    };

    TEST_F(IntegerExtensionsTest, TestGetVarSize)
    {
        // Test variable-length encoding size calculation
        EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<int32_t>(100)));
        EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<int32_t>(252)));
        EXPECT_EQ(3, IntegerExtensions::GetVarSize(static_cast<int32_t>(253)));
        EXPECT_EQ(3, IntegerExtensions::GetVarSize(static_cast<int32_t>(65535)));
        EXPECT_EQ(5, IntegerExtensions::GetVarSize(static_cast<int32_t>(65536)));

        EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<uint16_t>(100)));
        EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<uint32_t>(100)));
        EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<int64_t>(100)));
        EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<uint64_t>(100)));
        EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<size_t>(100)));
    }

    TEST_F(IntegerExtensionsTest, TestToLittleEndianBytes16)
    {
        auto bytes = IntegerExtensions::ToLittleEndianBytes(static_cast<int16_t>(0x1234));
        std::vector<uint8_t> expected = {0x34, 0x12};
        EXPECT_EQ(expected, bytes);

        auto ubytes = IntegerExtensions::ToLittleEndianBytes(static_cast<uint16_t>(0x1234));
        EXPECT_EQ(expected, ubytes);
    }

    TEST_F(IntegerExtensionsTest, TestToLittleEndianBytes32)
    {
        auto bytes = IntegerExtensions::ToLittleEndianBytes(static_cast<int32_t>(0x12345678));
        std::vector<uint8_t> expected = {0x78, 0x56, 0x34, 0x12};
        EXPECT_EQ(expected, bytes);

        auto ubytes = IntegerExtensions::ToLittleEndianBytes(static_cast<uint32_t>(0x12345678));
        EXPECT_EQ(expected, ubytes);
    }

    TEST_F(IntegerExtensionsTest, TestToLittleEndianBytes64)
    {
        auto bytes = IntegerExtensions::ToLittleEndianBytes(static_cast<int64_t>(0x123456789ABCDEF0));
        std::vector<uint8_t> expected = {0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12};
        EXPECT_EQ(expected, bytes);

        auto ubytes = IntegerExtensions::ToLittleEndianBytes(static_cast<uint64_t>(0x123456789ABCDEF0));
        EXPECT_EQ(expected, ubytes);
    }

    TEST_F(IntegerExtensionsTest, TestFromLittleEndianBytes16)
    {
        std::vector<uint8_t> bytes = {0x34, 0x12};

        int16_t result = IntegerExtensions::FromLittleEndianBytes16(bytes);
        EXPECT_EQ(0x1234, result);

        uint16_t uresult = IntegerExtensions::FromLittleEndianBytesU16(bytes);
        EXPECT_EQ(0x1234, uresult);
    }

    TEST_F(IntegerExtensionsTest, TestFromLittleEndianBytes32)
    {
        std::vector<uint8_t> bytes = {0x78, 0x56, 0x34, 0x12};

        int32_t result = IntegerExtensions::FromLittleEndianBytes32(bytes);
        EXPECT_EQ(0x12345678, result);

        uint32_t uresult = IntegerExtensions::FromLittleEndianBytesU32(bytes);
        EXPECT_EQ(0x12345678, uresult);
    }

    TEST_F(IntegerExtensionsTest, TestFromLittleEndianBytes64)
    {
        std::vector<uint8_t> bytes = {0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12};

        int64_t result = IntegerExtensions::FromLittleEndianBytes64(bytes);
        EXPECT_EQ(0x123456789ABCDEF0, result);

        uint64_t uresult = IntegerExtensions::FromLittleEndianBytesU64(bytes);
        EXPECT_EQ(0x123456789ABCDEF0, uresult);
    }

    TEST_F(IntegerExtensionsTest, TestFromLittleEndianBytesWithOffset)
    {
        std::vector<uint8_t> bytes = {0x00, 0x00, 0x78, 0x56, 0x34, 0x12, 0x00, 0x00};

        int32_t result = IntegerExtensions::FromLittleEndianBytes32(bytes, 2);
        EXPECT_EQ(0x12345678, result);
    }

    TEST_F(IntegerExtensionsTest, TestFromLittleEndianBytesOutOfRange)
    {
        std::vector<uint8_t> bytes = {0x78, 0x56};

        EXPECT_THROW(IntegerExtensions::FromLittleEndianBytes32(bytes), std::out_of_range);
        EXPECT_THROW(IntegerExtensions::FromLittleEndianBytes16(bytes, 2), std::out_of_range);
    }

    TEST_F(IntegerExtensionsTest, TestRoundTripConversion16)
    {
        int16_t original = -12345;
        auto bytes = IntegerExtensions::ToLittleEndianBytes(original);
        int16_t converted_back = IntegerExtensions::FromLittleEndianBytes16(bytes);
        EXPECT_EQ(original, converted_back);

        uint16_t uoriginal = 54321;
        auto ubytes = IntegerExtensions::ToLittleEndianBytes(uoriginal);
        uint16_t uconverted_back = IntegerExtensions::FromLittleEndianBytesU16(ubytes);
        EXPECT_EQ(uoriginal, uconverted_back);
    }

    TEST_F(IntegerExtensionsTest, TestRoundTripConversion32)
    {
        int32_t original = -1234567890;
        auto bytes = IntegerExtensions::ToLittleEndianBytes(original);
        int32_t converted_back = IntegerExtensions::FromLittleEndianBytes32(bytes);
        EXPECT_EQ(original, converted_back);

        uint32_t uoriginal = 3234567890U;
        auto ubytes = IntegerExtensions::ToLittleEndianBytes(uoriginal);
        uint32_t uconverted_back = IntegerExtensions::FromLittleEndianBytesU32(ubytes);
        EXPECT_EQ(uoriginal, uconverted_back);
    }

    TEST_F(IntegerExtensionsTest, TestRoundTripConversion64)
    {
        int64_t original = -1234567890123456789LL;
        auto bytes = IntegerExtensions::ToLittleEndianBytes(original);
        int64_t converted_back = IntegerExtensions::FromLittleEndianBytes64(bytes);
        EXPECT_EQ(original, converted_back);

        uint64_t uoriginal = 12345678901234567890ULL;
        auto ubytes = IntegerExtensions::ToLittleEndianBytes(uoriginal);
        uint64_t uconverted_back = IntegerExtensions::FromLittleEndianBytesU64(ubytes);
        EXPECT_EQ(uoriginal, uconverted_back);
    }

    TEST_F(IntegerExtensionsTest, TestIsLittleEndian)
    {
        // This test will pass on little-endian systems (most modern systems)
        // and may fail on big-endian systems, which is expected behavior
        bool is_little = IntegerExtensions::IsLittleEndian();

        // Test that the endianness detection is consistent
        uint16_t test_value = 0x1234;
        auto bytes = IntegerExtensions::ToLittleEndianBytes(test_value);
        uint16_t converted_back = IntegerExtensions::FromLittleEndianBytesU16(bytes);
        EXPECT_EQ(test_value, converted_back);

        // On little-endian systems, the first byte should be 0x34
        // On big-endian systems, the conversion should still work correctly
        if (is_little)
        {
            EXPECT_EQ(0x34, bytes[0]);
            EXPECT_EQ(0x12, bytes[1]);
        }
    }

    TEST_F(IntegerExtensionsTest, TestZeroValues)
    {
        // Test with zero values
        EXPECT_EQ(1, IntegerExtensions::GetVarSize(static_cast<int32_t>(0)));

        auto zero_bytes = IntegerExtensions::ToLittleEndianBytes(static_cast<int32_t>(0));
        std::vector<uint8_t> expected_zero = {0x00, 0x00, 0x00, 0x00};
        EXPECT_EQ(expected_zero, zero_bytes);

        int32_t zero_back = IntegerExtensions::FromLittleEndianBytes32(zero_bytes);
        EXPECT_EQ(0, zero_back);
    }
}
