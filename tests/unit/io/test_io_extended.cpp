#include <gtest/gtest.h>
#include <neo/io/io_helper.h>
#include <neo/io/memory_reader.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/caching/cache.h>

using namespace neo::io;

class IOExtendedTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(IOExtendedTest, TestGetVarSizeInt)
{
    // Test various integer sizes
    EXPECT_EQ(IOHelper::GetVarSize(0), 1);
    EXPECT_EQ(IOHelper::GetVarSize(0xFC), 1);
    EXPECT_EQ(IOHelper::GetVarSize(0xFD), 3);
    EXPECT_EQ(IOHelper::GetVarSize(0xFFFF), 3);
    EXPECT_EQ(IOHelper::GetVarSize(0x10000), 5);
    EXPECT_EQ(IOHelper::GetVarSize(0xFFFFFFFF), 5);
    EXPECT_EQ(IOHelper::GetVarSize(0x100000000ULL), 9);
}

TEST_F(IOExtendedTest, TestGetVarSizeGeneric)
{
    // Test string
    std::string str = "Hello World";
    size_t strSize = IOHelper::GetVarSize(str);
    EXPECT_EQ(strSize, 1 + str.length()); // 1 byte for length + string data
    
    // Test vector
    std::vector<uint8_t> vec(100);
    size_t vecSize = IOHelper::GetVarSize(vec);
    EXPECT_EQ(vecSize, 1 + vec.size()); // 1 byte for count + data
    
    // Test large vector
    std::vector<uint8_t> largeVec(300);
    size_t largeVecSize = IOHelper::GetVarSize(largeVec);
    EXPECT_EQ(largeVecSize, 3 + largeVec.size()); // 3 bytes for count + data
}

TEST_F(IOExtendedTest, TestMemoryReader)
{
    ByteVector data;
    data.WriteUInt32(0x12345678);
    data.WriteUInt64(0x123456789ABCDEF0ULL);
    data.WriteString("test");
    
    MemoryReader reader(data);
    
    EXPECT_EQ(reader.ReadUInt32(), 0x12345678);
    EXPECT_EQ(reader.ReadUInt64(), 0x123456789ABCDEF0ULL);
    EXPECT_EQ(reader.ReadString(), "test");
    EXPECT_TRUE(reader.IsEnd());
}

TEST_F(IOExtendedTest, TestCaching)
{
    // Create a simple cache
    Cache<int, std::string> cache(100);
    
    // Add items
    cache.Add(1, "one");
    cache.Add(2, "two");
    cache.Add(3, "three");
    
    // Test contains
    EXPECT_TRUE(cache.Contains(1));
    EXPECT_TRUE(cache.Contains(2));
    EXPECT_TRUE(cache.Contains(3));
    EXPECT_FALSE(cache.Contains(4));
    
    // Test get
    EXPECT_EQ(cache.Get(1), "one");
    EXPECT_EQ(cache.Get(2), "two");
    EXPECT_EQ(cache.Get(3), "three");
    
    // Test remove
    cache.Remove(2);
    EXPECT_FALSE(cache.Contains(2));
}

TEST_F(IOExtendedTest, TestByteVector)
{
    ByteVector vec1 = ByteVector::Parse("0102030405");
    EXPECT_EQ(vec1.Size(), 5);
    EXPECT_EQ(vec1[0], 0x01);
    EXPECT_EQ(vec1[4], 0x05);
    
    ByteVector vec2 = ByteVector::FromString("Hello");
    EXPECT_EQ(vec2.Size(), 5);
    EXPECT_EQ(vec2.ToString(), "Hello");
    
    // Test concatenation
    ByteVector vec3 = vec1 + vec2;
    EXPECT_EQ(vec3.Size(), 10);
    
    // Test comparison
    ByteVector vec4 = ByteVector::Parse("0102030405");
    EXPECT_EQ(vec1, vec4);
    EXPECT_NE(vec1, vec2);
}

TEST_F(IOExtendedTest, TestBinaryReader)
{
    ByteVector data;
    BinaryWriter writer(data);
    writer.Write((uint8_t)0x01);
    writer.Write((uint16_t)0x0203);
    writer.Write((uint32_t)0x04050607);
    writer.Write((uint64_t)0x08090A0B0C0D0E0FULL);
    writer.Write("test string");
    writer.WriteVarInt(1000);
    
    BinaryReader reader(data);
    EXPECT_EQ(reader.ReadByte(), 0x01);
    EXPECT_EQ(reader.ReadUInt16(), 0x0203);
    EXPECT_EQ(reader.ReadUInt32(), 0x04050607);
    EXPECT_EQ(reader.ReadUInt64(), 0x08090A0B0C0D0E0FULL);
    EXPECT_EQ(reader.ReadString(), "test string");
    EXPECT_EQ(reader.ReadVarInt(), 1000);
}

TEST_F(IOExtendedTest, TestBinaryWriter)
{
    ByteVector buffer;
    BinaryWriter writer(buffer);
    
    // Write various types
    writer.Write(true);
    writer.Write((int8_t)-128);
    writer.Write((uint8_t)255);
    writer.Write((int16_t)-32768);
    writer.Write((uint16_t)65535);
    writer.Write((int32_t)-2147483648);
    writer.Write((uint32_t)4294967295U);
    writer.Write((int64_t)-9223372036854775808LL);
    writer.Write((uint64_t)18446744073709551615ULL);
    writer.Write(3.14159f);
    writer.Write(2.71828);
    
    // Verify buffer size
    size_t expectedSize = 1 + 1 + 1 + 2 + 2 + 4 + 4 + 8 + 8 + 4 + 8;
    EXPECT_EQ(buffer.Size(), expectedSize);
    
    // Read back and verify
    BinaryReader reader(buffer);
    EXPECT_TRUE(reader.ReadBoolean());
    EXPECT_EQ(reader.ReadInt8(), -128);
    EXPECT_EQ(reader.ReadByte(), 255);
    EXPECT_EQ(reader.ReadInt16(), -32768);
    EXPECT_EQ(reader.ReadUInt16(), 65535);
    EXPECT_EQ(reader.ReadInt32(), -2147483648);
    EXPECT_EQ(reader.ReadUInt32(), 4294967295U);
    EXPECT_EQ(reader.ReadInt64(), -9223372036854775808LL);
    EXPECT_EQ(reader.ReadUInt64(), 18446744073709551615ULL);
    EXPECT_FLOAT_EQ(reader.ReadFloat(), 3.14159f);
    EXPECT_DOUBLE_EQ(reader.ReadDouble(), 2.71828);
}
