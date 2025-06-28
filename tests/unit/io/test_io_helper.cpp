#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

namespace neo::io::tests
{
    class IOHelperTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // Test setup
        }
    };

    TEST_F(IOHelperTest, TestBinaryReaderWriter)
    {
        ByteVector buffer;
        BinaryWriter writer(buffer);
        
        // Test writing various types
        writer.Write(static_cast<uint8_t>(0x12));
        writer.Write(static_cast<uint16_t>(0x3456));
        writer.Write(static_cast<uint32_t>(0x789ABCDE));
        writer.Write(static_cast<uint64_t>(0x123456789ABCDEF0));
        
        std::string test_string = "Hello, World!";
        writer.WriteVarString(test_string);
        
        std::vector<uint8_t> test_bytes = {0x01, 0x02, 0x03, 0x04};
        writer.WriteVarBytes(test_bytes);
        
        // Test reading back
        BinaryReader reader(buffer.AsSpan());
        
        EXPECT_EQ(0x12, reader.ReadByte());
        EXPECT_EQ(0x3456, reader.ReadUInt16());
        EXPECT_EQ(0x789ABCDE, reader.ReadUInt32());
        EXPECT_EQ(0x123456789ABCDEF0, reader.ReadUInt64());
        
        std::string read_string = reader.ReadVarString();
        EXPECT_EQ(test_string, read_string);
        
        auto read_bytes = reader.ReadVarBytes();
        EXPECT_EQ(test_bytes, read_bytes);
    }

    TEST_F(IOHelperTest, TestUInt160Serialization)
    {
        UInt160 original = UInt160::Parse("0x1234567890123456789012345678901234567890");
        
        ByteVector buffer;
        BinaryWriter writer(buffer);
        writer.Write(original);
        
        BinaryReader reader(buffer.AsSpan());
        UInt160 deserialized = reader.Read<UInt160>();
        
        EXPECT_EQ(original, deserialized);
    }

    TEST_F(IOHelperTest, TestUInt256Serialization)
    {
        UInt256 original = UInt256::Parse("0x1234567890123456789012345678901234567890123456789012345678901234");
        
        ByteVector buffer;
        BinaryWriter writer(buffer);
        writer.Write(original);
        
        BinaryReader reader(buffer.AsSpan());
        UInt256 deserialized = reader.Read<UInt256>();
        
        EXPECT_EQ(original, deserialized);
    }

    TEST_F(IOHelperTest, TestVarIntSerialization)
    {
        ByteVector buffer;
        BinaryWriter writer(buffer);
        
        // Test various VarInt values (must be within int64_t range for WriteVarInt)
        std::vector<int64_t> test_values = {
            0, 1, 252, 253, 254, 255, 256,
            65535, 65536, 4294967295LL, 4294967296LL,
            9223372036854775807LL // INT64_MAX instead of UINT64_MAX
        };
        
        for (int64_t value : test_values)
        {
            writer.WriteVarInt(value);
        }
        
        BinaryReader reader(buffer.AsSpan());
        for (int64_t expected : test_values)
        {
            int64_t actual = reader.ReadVarInt();
            EXPECT_EQ(expected, actual) << "Failed for value: " << expected;
        }
    }

    TEST_F(IOHelperTest, TestStringEncoding)
    {
        ByteVector buffer;
        BinaryWriter writer(buffer);
        
        // Test various strings
        std::vector<std::string> test_strings = {
            "",
            "Hello",
            "Hello, World!",
            "Unicode: 世界",
            "Special chars: !@#$%^&*()",
            std::string(1000, 'A') // Long string
        };
        
        for (const auto& str : test_strings)
        {
            writer.WriteVarString(str);
        }
        
        BinaryReader reader(buffer.AsSpan());
        for (const auto& expected : test_strings)
        {
            std::string actual = reader.ReadVarString();
            EXPECT_EQ(expected, actual);
        }
    }

    TEST_F(IOHelperTest, TestByteArraySerialization)
    {
        ByteVector buffer;
        BinaryWriter writer(buffer);
        
        // Test various byte arrays
        std::vector<std::vector<uint8_t>> test_arrays = {
            {},
            {0x01},
            {0x01, 0x02, 0x03},
            {0xFF, 0xFE, 0xFD, 0xFC},
            std::vector<uint8_t>(1000, 0xAA) // Large array
        };
        
        for (const auto& arr : test_arrays)
        {
            writer.WriteVarBytes(arr);
        }
        
        BinaryReader reader(buffer.AsSpan());
        for (const auto& expected : test_arrays)
        {
            auto actual = reader.ReadVarBytes();
            EXPECT_EQ(expected, actual);
        }
    }

    TEST_F(IOHelperTest, TestEndianness)
    {
        ByteVector buffer;
        BinaryWriter writer(buffer);
        
        // Test little-endian encoding
        writer.Write(static_cast<uint32_t>(0x12345678));
        
        // Verify bytes are in little-endian order
        EXPECT_EQ(4, buffer.Size());
        EXPECT_EQ(0x78, buffer[0]);
        EXPECT_EQ(0x56, buffer[1]);
        EXPECT_EQ(0x34, buffer[2]);
        EXPECT_EQ(0x12, buffer[3]);
        
        // Test reading back
        BinaryReader reader(buffer.AsSpan());
        uint32_t value = reader.ReadUInt32();
        EXPECT_EQ(0x12345678, value);
    }

    TEST_F(IOHelperTest, TestBoundaryConditions)
    {
        ByteVector buffer;
        BinaryWriter writer(buffer);
        
        // Test maximum values
        writer.Write(std::numeric_limits<uint8_t>::max());
        writer.Write(std::numeric_limits<uint16_t>::max());
        writer.Write(std::numeric_limits<uint32_t>::max());
        writer.Write(std::numeric_limits<uint64_t>::max());
        
        BinaryReader reader(buffer.AsSpan());
        EXPECT_EQ(std::numeric_limits<uint8_t>::max(), reader.ReadByte());
        EXPECT_EQ(std::numeric_limits<uint16_t>::max(), reader.ReadUInt16());
        EXPECT_EQ(std::numeric_limits<uint32_t>::max(), reader.ReadUInt32());
        EXPECT_EQ(std::numeric_limits<uint64_t>::max(), reader.ReadUInt64());
    }

    TEST_F(IOHelperTest, TestErrorConditions)
    {
        // Test reading from empty buffer
        ByteVector empty_buffer;
        BinaryReader reader(empty_buffer.AsSpan());
        
        EXPECT_THROW(reader.ReadByte(), std::out_of_range);
        EXPECT_THROW(reader.ReadUInt16(), std::out_of_range);
        EXPECT_THROW(reader.ReadUInt32(), std::out_of_range);
        EXPECT_THROW(reader.ReadUInt64(), std::out_of_range);
        
        // Test reading beyond buffer
        ByteVector small_buffer = {0x01, 0x02};
        BinaryReader small_reader(small_buffer.AsSpan());
        
        small_reader.ReadByte(); // Should succeed
        small_reader.ReadByte(); // Should succeed
        EXPECT_THROW(small_reader.ReadByte(), std::out_of_range); // Should fail
    }

    TEST_F(IOHelperTest, TestMemoryReader)
    {
        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
        BinaryReader reader(data);
        
        // Test position tracking
        EXPECT_EQ(0, reader.GetPosition());
        
        reader.ReadByte();
        EXPECT_EQ(1, reader.GetPosition());
        
        reader.ReadUInt16();
        EXPECT_EQ(3, reader.GetPosition());
        
        // Test available bytes
        EXPECT_EQ(2, reader.Available());
        
        // Test seeking (if implemented)
        // reader.Seek(0);
        // EXPECT_EQ(0, reader.GetPosition());
    }

    TEST_F(IOHelperTest, TestRoundTripSerialization)
    {
        // Test complex round-trip serialization
        struct TestData
        {
            uint8_t byte_val = 0x12;
            uint16_t short_val = 0x3456;
            uint32_t int_val = 0x789ABCDE;
            uint64_t long_val = 0x123456789ABCDEF0;
            std::string string_val = "Test String";
            std::vector<uint8_t> bytes_val = {0xAA, 0xBB, 0xCC, 0xDD};
        };
        
        TestData original;
        
        // Serialize
        ByteVector buffer;
        BinaryWriter writer(buffer);
        writer.Write(original.byte_val);
        writer.Write(original.short_val);
        writer.Write(original.int_val);
        writer.Write(original.long_val);
        writer.WriteVarString(original.string_val);
        writer.WriteVarBytes(original.bytes_val);
        
        // Deserialize
        BinaryReader reader(buffer.AsSpan());
        TestData deserialized;
        deserialized.byte_val = reader.ReadByte();
        deserialized.short_val = reader.ReadUInt16();
        deserialized.int_val = reader.ReadUInt32();
        deserialized.long_val = reader.ReadUInt64();
        deserialized.string_val = reader.ReadVarString();
        deserialized.bytes_val = reader.ReadVarBytes();
        
        // Verify
        EXPECT_EQ(original.byte_val, deserialized.byte_val);
        EXPECT_EQ(original.short_val, deserialized.short_val);
        EXPECT_EQ(original.int_val, deserialized.int_val);
        EXPECT_EQ(original.long_val, deserialized.long_val);
        EXPECT_EQ(original.string_val, deserialized.string_val);
        EXPECT_EQ(original.bytes_val, deserialized.bytes_val);
    }
}
