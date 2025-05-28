#include <gtest/gtest.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/io/fixed8.h>
#include <neo/io/byte_vector.h>
#include <string>

using namespace neo::io;

// Test class that implements IJsonSerializable
class TestObject : public IJsonSerializable
{
public:
    bool boolValue = false;
    uint8_t uint8Value = 0;
    uint16_t uint16Value = 0;
    uint32_t uint32Value = 0;
    uint64_t uint64Value = 0;
    int8_t int8Value = 0;
    int16_t int16Value = 0;
    int32_t int32Value = 0;
    int64_t int64Value = 0;
    std::string stringValue;
    ByteVector bytesValue;
    UInt160 uint160Value;
    UInt256 uint256Value;
    Fixed8 fixed8Value;
    std::vector<TestObject> children;

    void SerializeJson(JsonWriter& writer) const override
    {
        writer.Write("bool", boolValue);
        writer.Write("uint8", uint8Value);
        writer.Write("uint16", uint16Value);
        writer.Write("uint32", uint32Value);
        writer.Write("uint64", uint64Value);
        writer.Write("int8", int8Value);
        writer.Write("int16", int16Value);
        writer.Write("int32", int32Value);
        writer.Write("int64", int64Value);
        writer.Write("string", stringValue);
        writer.Write("bytes", bytesValue.AsSpan());
        writer.Write("uint160", uint160Value);
        writer.Write("uint256", uint256Value);
        writer.Write("fixed8", fixed8Value);
        writer.WriteVector("children", children);
    }

    void DeserializeJson(const JsonReader& reader) override
    {
        boolValue = reader.ReadBool("bool");
        uint8Value = reader.ReadUInt8("uint8");
        uint16Value = reader.ReadUInt16("uint16");
        uint32Value = reader.ReadUInt32("uint32");
        uint64Value = reader.ReadUInt64("uint64");
        int8Value = reader.ReadInt8("int8");
        int16Value = reader.ReadInt16("int16");
        int32Value = reader.ReadInt32("int32");
        int64Value = reader.ReadInt64("int64");
        stringValue = reader.ReadString("string");
        bytesValue = reader.ReadBytes("bytes");
        uint160Value = reader.ReadUInt160("uint160");
        uint256Value = reader.ReadUInt256("uint256");
        fixed8Value = reader.ReadFixed8("fixed8");
        children = reader.ReadVector<TestObject>("children");
    }

    bool operator==(const TestObject& other) const
    {
        return boolValue == other.boolValue &&
               uint8Value == other.uint8Value &&
               uint16Value == other.uint16Value &&
               uint32Value == other.uint32Value &&
               uint64Value == other.uint64Value &&
               int8Value == other.int8Value &&
               int16Value == other.int16Value &&
               int32Value == other.int32Value &&
               int64Value == other.int64Value &&
               stringValue == other.stringValue &&
               bytesValue.AsSpan().ToHexString() == other.bytesValue.AsSpan().ToHexString() &&
               uint160Value == other.uint160Value &&
               uint256Value == other.uint256Value &&
               fixed8Value == other.fixed8Value &&
               children.size() == other.children.size();
    }
};

TEST(JsonSerializationTest, SerializeDeserialize)
{
    // Create a test object
    TestObject obj;
    obj.boolValue = true;
    obj.uint8Value = 123;
    obj.uint16Value = 12345;
    obj.uint32Value = 1234567890;
    obj.uint64Value = 1234567890123456789ULL;
    obj.int8Value = -123;
    obj.int16Value = -12345;
    obj.int32Value = -1234567890;
    obj.int64Value = -1234567890123456789LL;
    obj.stringValue = "Hello, world!";
    obj.bytesValue = ByteVector::Parse("0123456789ABCDEF");
    obj.uint160Value = UInt160::Parse("0123456789ABCDEF0123456789ABCDEF01234567");
    obj.uint256Value = UInt256::Parse("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");
    obj.fixed8Value = Fixed8::FromDouble(123.45678);

    // Add a child object
    TestObject child;
    child.boolValue = false;
    child.uint8Value = 210;
    child.stringValue = "Child object";
    obj.children.push_back(child);

    // Serialize to JSON
    nlohmann::json json = obj.ToJson();

    // Verify JSON values
    EXPECT_EQ(json["bool"], true);
    EXPECT_EQ(json["uint8"], 123);
    EXPECT_EQ(json["uint16"], 12345);
    EXPECT_EQ(json["uint32"], 1234567890);
    EXPECT_EQ(json["uint64"], 1234567890123456789ULL);
    EXPECT_EQ(json["int8"], -123);
    EXPECT_EQ(json["int16"], -12345);
    EXPECT_EQ(json["int32"], -1234567890);
    EXPECT_EQ(json["int64"], -1234567890123456789LL);
    EXPECT_EQ(json["string"], "Hello, world!");
    EXPECT_EQ(json["bytes"], "0123456789ABCDEF");
    EXPECT_EQ(json["uint160"], "0123456789ABCDEF0123456789ABCDEF01234567");
    EXPECT_EQ(json["uint256"], "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");
    EXPECT_EQ(json["fixed8"], "123.45678");
    EXPECT_EQ(json["children"].size(), 1);
    EXPECT_EQ(json["children"][0]["bool"], false);
    EXPECT_EQ(json["children"][0]["uint8"], 210);
    EXPECT_EQ(json["children"][0]["string"], "Child object");

    // Deserialize from JSON
    TestObject deserialized;
    deserialized.DeserializeFromJson(json);

    // Verify deserialized values
    EXPECT_EQ(deserialized.boolValue, true);
    EXPECT_EQ(deserialized.uint8Value, 123);
    EXPECT_EQ(deserialized.uint16Value, 12345);
    EXPECT_EQ(deserialized.uint32Value, 1234567890);
    EXPECT_EQ(deserialized.uint64Value, 1234567890123456789ULL);
    EXPECT_EQ(deserialized.int8Value, -123);
    EXPECT_EQ(deserialized.int16Value, -12345);
    EXPECT_EQ(deserialized.int32Value, -1234567890);
    EXPECT_EQ(deserialized.int64Value, -1234567890123456789LL);
    EXPECT_EQ(deserialized.stringValue, "Hello, world!");
    EXPECT_EQ(deserialized.bytesValue.AsSpan().ToHexString(), "0123456789ABCDEF");
    EXPECT_EQ(deserialized.uint160Value.ToHexString(), "0123456789ABCDEF0123456789ABCDEF01234567");
    EXPECT_EQ(deserialized.uint256Value.ToHexString(), "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");
    EXPECT_EQ(deserialized.fixed8Value.ToString(), "123.45678");
    EXPECT_EQ(deserialized.children.size(), 1);
    EXPECT_EQ(deserialized.children[0].boolValue, false);
    EXPECT_EQ(deserialized.children[0].uint8Value, 210);
    EXPECT_EQ(deserialized.children[0].stringValue, "Child object");

    // Compare objects
    EXPECT_EQ(obj, deserialized);

    // Test JSON string serialization/deserialization
    std::string jsonString = obj.ToJsonString(true);
    TestObject deserializedFromString;
    deserializedFromString.DeserializeFromJsonString(jsonString);
    EXPECT_EQ(obj, deserializedFromString);
}

TEST(JsonSerializationTest, DefaultValues)
{
    // Create an empty JSON object
    nlohmann::json json = nlohmann::json::object();
    
    // Create a JsonReader
    JsonReader reader(json);
    
    // Test default values
    EXPECT_EQ(reader.ReadBool("nonexistent"), false);
    EXPECT_EQ(reader.ReadBool("nonexistent", true), true);
    EXPECT_EQ(reader.ReadUInt8("nonexistent"), 0);
    EXPECT_EQ(reader.ReadUInt8("nonexistent", 123), 123);
    EXPECT_EQ(reader.ReadUInt16("nonexistent"), 0);
    EXPECT_EQ(reader.ReadUInt16("nonexistent", 12345), 12345);
    EXPECT_EQ(reader.ReadUInt32("nonexistent"), 0);
    EXPECT_EQ(reader.ReadUInt32("nonexistent", 1234567890), 1234567890);
    EXPECT_EQ(reader.ReadUInt64("nonexistent"), 0);
    EXPECT_EQ(reader.ReadUInt64("nonexistent", 1234567890123456789ULL), 1234567890123456789ULL);
    EXPECT_EQ(reader.ReadInt8("nonexistent"), 0);
    EXPECT_EQ(reader.ReadInt8("nonexistent", -123), -123);
    EXPECT_EQ(reader.ReadInt16("nonexistent"), 0);
    EXPECT_EQ(reader.ReadInt16("nonexistent", -12345), -12345);
    EXPECT_EQ(reader.ReadInt32("nonexistent"), 0);
    EXPECT_EQ(reader.ReadInt32("nonexistent", -1234567890), -1234567890);
    EXPECT_EQ(reader.ReadInt64("nonexistent"), 0);
    EXPECT_EQ(reader.ReadInt64("nonexistent", -1234567890123456789LL), -1234567890123456789LL);
    EXPECT_EQ(reader.ReadString("nonexistent"), "");
    EXPECT_EQ(reader.ReadString("nonexistent", "default"), "default");
    EXPECT_EQ(reader.ReadBytes("nonexistent").Size(), 0);
    EXPECT_EQ(reader.ReadUInt160("nonexistent"), UInt160::Zero());
    EXPECT_EQ(reader.ReadUInt256("nonexistent"), UInt256::Zero());
    EXPECT_EQ(reader.ReadFixed8("nonexistent"), Fixed8::Zero());
    EXPECT_EQ(reader.ReadObject("nonexistent").size(), 0);
    EXPECT_EQ(reader.ReadArray("nonexistent").size(), 0);
    EXPECT_EQ(reader.ReadVector<TestObject>("nonexistent").size(), 0);
}
