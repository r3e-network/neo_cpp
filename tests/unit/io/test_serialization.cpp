#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/iserializable.h>
#include <sstream>

using namespace neo::io;

// Test serializable class
class TestSerializable : public ISerializable
{
  public:
    uint32_t IntValue;
    std::string StringValue;
    ByteVector BytesValue;

    TestSerializable() : IntValue(0) {}

    TestSerializable(uint32_t intValue, const std::string& stringValue, const ByteVector& bytesValue)
        : IntValue(intValue), StringValue(stringValue), BytesValue(bytesValue)
    {
    }

    void Serialize(BinaryWriter& writer) const override
    {
        writer.Write(IntValue);
        writer.WriteString(StringValue);
        writer.WriteVarBytes(BytesValue.AsSpan());
    }

    void Deserialize(BinaryReader& reader) override
    {
        IntValue = reader.ReadUInt32();
        StringValue = reader.ReadString();
        BytesValue = reader.ReadVarBytes();
    }

    bool operator==(const TestSerializable& other) const
    {
        return IntValue == other.IntValue && StringValue == other.StringValue && BytesValue == other.BytesValue;
    }
};

TEST(SerializationTest, BinaryWriterReader)
{
    // Create test data
    TestSerializable original(42, "Hello, World!", ByteVector{1, 2, 3, 4, 5});

    // Serialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    original.Serialize(writer);

    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    TestSerializable deserialized;
    deserialized.Deserialize(reader);

    // Verify
    EXPECT_EQ(deserialized.IntValue, original.IntValue);
    EXPECT_EQ(deserialized.StringValue, original.StringValue);
    EXPECT_EQ(deserialized.BytesValue, original.BytesValue);
}

TEST(SerializationTest, ToArrayFromArray)
{
    // Create test data
    TestSerializable original(42, "Hello, World!", ByteVector{1, 2, 3, 4, 5});

    // Serialize to array
    ByteVector data = original.ToArray();

    // Deserialize from array
    TestSerializable deserialized = ISerializable::FromArray<TestSerializable>(data.AsSpan());

    // Verify
    EXPECT_EQ(deserialized, original);
}

TEST(SerializationTest, VarInt)
{
    std::stringstream stream;
    BinaryWriter writer(stream);

    // Write VarInts
    writer.WriteVarInt(0);
    writer.WriteVarInt(1);
    writer.WriteVarInt(0xFC);
    writer.WriteVarInt(0xFD);
    writer.WriteVarInt(0xFFFF);
    writer.WriteVarInt(0x10000);
    writer.WriteVarInt(0xFFFFFFFF);
    writer.WriteVarInt(0x100000000);

    // Read VarInts
    stream.seekg(0);
    BinaryReader reader(stream);

    EXPECT_EQ(reader.ReadVarInt(), 0);
    EXPECT_EQ(reader.ReadVarInt(), 1);
    EXPECT_EQ(reader.ReadVarInt(), 0xFC);
    EXPECT_EQ(reader.ReadVarInt(), 0xFD);
    EXPECT_EQ(reader.ReadVarInt(), 0xFFFF);
    EXPECT_EQ(reader.ReadVarInt(), 0x10000);
    EXPECT_EQ(reader.ReadVarInt(), 0xFFFFFFFF);
    EXPECT_EQ(reader.ReadVarInt(), 0x100000000);
}

TEST(SerializationTest, VarBytes)
{
    std::stringstream stream;
    BinaryWriter writer(stream);

    // Write VarBytes
    writer.WriteVarBytes(ByteVector{}.AsSpan());
    writer.WriteVarBytes(ByteVector{1}.AsSpan());
    writer.WriteVarBytes(ByteVector{1, 2, 3, 4, 5}.AsSpan());

    // Read VarBytes
    stream.seekg(0);
    BinaryReader reader(stream);

    ByteVector empty = reader.ReadVarBytes();
    EXPECT_EQ(empty.Size(), 0);

    ByteVector single = reader.ReadVarBytes();
    EXPECT_EQ(single.Size(), 1);
    EXPECT_EQ(single[0], 1);

    ByteVector multiple = reader.ReadVarBytes();
    EXPECT_EQ(multiple.Size(), 5);
    EXPECT_EQ(multiple[0], 1);
    EXPECT_EQ(multiple[4], 5);
}

TEST(SerializationTest, String)
{
    std::stringstream stream;
    BinaryWriter writer(stream);

    // Write strings
    writer.WriteString("");
    writer.WriteString("Hello");
    writer.WriteString("Hello, World!");

    // Read strings
    stream.seekg(0);
    BinaryReader reader(stream);

    EXPECT_EQ(reader.ReadString(), "");
    EXPECT_EQ(reader.ReadString(), "Hello");
    EXPECT_EQ(reader.ReadString(), "Hello, World!");
}
