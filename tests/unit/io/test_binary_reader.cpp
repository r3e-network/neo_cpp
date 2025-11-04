/**
 * @file test_binary_reader.cpp
 * @brief Binary reader test suite
 */

#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>

namespace neo::io::tests {

class BinaryReaderTest : public ::testing::Test {
protected:
};

TEST_F(BinaryReaderTest, ReadByte) {
    std::vector<uint8_t> data = {0x42};
    BinaryReader reader(data);
    uint8_t value = reader.ReadByte();
    EXPECT_EQ(value, 0x42);
}

TEST_F(BinaryReaderTest, ReadInt16) {
    std::vector<uint8_t> data = {0x34, 0x12}; // Little endian
    BinaryReader reader(data);
    int16_t value = reader.ReadInt16();
    EXPECT_EQ(value, 0x1234);
}

TEST_F(BinaryReaderTest, ReadInt32) {
    std::vector<uint8_t> data = {0x78, 0x56, 0x34, 0x12}; // Little endian
    BinaryReader reader(data);
    int32_t value = reader.ReadInt32();
    EXPECT_EQ(value, 0x12345678);
}

TEST_F(BinaryReaderTest, ReadInt64) {
    std::vector<uint8_t> data = {0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01};
    BinaryReader reader(data);
    int64_t value = reader.ReadInt64();
    EXPECT_EQ(value, 0x0123456789ABCDEF);
}

TEST_F(BinaryReaderTest, ReadBoolean) {
    std::vector<uint8_t> data = {0x01, 0x00};
    BinaryReader reader(data);
    bool value1 = reader.ReadBoolean();
    bool value2 = reader.ReadBoolean();
    EXPECT_TRUE(value1);
    EXPECT_FALSE(value2);
}

TEST_F(BinaryReaderTest, ReadBytes) {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
    BinaryReader reader(data);
    auto bytes = reader.ReadBytes(3);
    EXPECT_EQ(bytes.size(), 3);
    EXPECT_EQ(bytes[0], 0x01);
    EXPECT_EQ(bytes[1], 0x02);
    EXPECT_EQ(bytes[2], 0x03);
}

TEST_F(BinaryReaderTest, ReadVarInt) {
    std::vector<uint8_t> data = {0xFC}; // VarInt for 252
    BinaryReader reader(data);
    uint64_t value = reader.ReadVarInt();
    EXPECT_EQ(value, 252);
}

TEST_F(BinaryReaderTest, ReadString) {
    std::string test_string = "Hello";
    std::vector<uint8_t> data;
    data.push_back(test_string.size()); // Length prefix
    for (char c : test_string) {
        data.push_back(c);
    }

    BinaryReader reader(data);
    std::string value = reader.ReadVarString();
    EXPECT_EQ(value, test_string);
}

TEST_F(BinaryReaderTest, EndOfStream) {
    std::vector<uint8_t> data = {0x42};
    BinaryReader reader(data);
    reader.ReadByte(); // Read the only byte

    EXPECT_THROW(reader.ReadByte(), std::out_of_range);
}

} // namespace neo::io::tests
