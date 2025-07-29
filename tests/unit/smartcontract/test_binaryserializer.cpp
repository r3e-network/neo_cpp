#include <gtest/gtest.h>
#include <memory>
#include <neo/io/byte_vector.h>
#include <neo/smartcontract/binary_serializer.h>
#include <neo/vm/array_stack_item.h>
#include <neo/vm/boolean_stack_item.h>
#include <neo/vm/byte_string_stack_item.h>
#include <neo/vm/integer_stack_item.h>
#include <neo/vm/map_stack_item.h>
#include <neo/vm/null_stack_item.h>
#include <neo/vm/stack_item.h>
#include <string>
#include <vector>

using namespace neo::smartcontract;
using namespace neo::vm;
using namespace neo::io;

/**
 * @brief Test fixture for BinarySerializer
 */
class BinarySerializerTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Test setup
    }

    // Helper method to create test items
    std::shared_ptr<IntegerStackItem> CreateIntegerItem(int64_t value)
    {
        return std::make_shared<IntegerStackItem>(value);
    }

    std::shared_ptr<BooleanStackItem> CreateBooleanItem(bool value)
    {
        return std::make_shared<BooleanStackItem>(value);
    }

    std::shared_ptr<ByteStringStackItem> CreateByteStringItem(const std::string& str)
    {
        ByteVector bytes(reinterpret_cast<const uint8_t*>(str.data()), str.size());
        return std::make_shared<ByteStringStackItem>(bytes);
    }

    std::shared_ptr<NullStackItem> CreateNullItem()
    {
        return std::make_shared<NullStackItem>();
    }
};

TEST_F(BinarySerializerTest, SerializeDeserialize_Integer)
{
    auto original = CreateIntegerItem(12345);

    // Serialize
    ByteVector serialized = BinarySerializer::Serialize(original);
    EXPECT_FALSE(serialized.empty());

    // Deserialize
    auto deserialized = BinarySerializer::Deserialize(serialized.AsSpan());
    ASSERT_NE(nullptr, deserialized);

    // Verify type and value
    EXPECT_EQ(StackItemType::Integer, deserialized->GetType());
    auto intItem = std::dynamic_pointer_cast<IntegerStackItem>(deserialized);
    ASSERT_NE(nullptr, intItem);
    EXPECT_EQ(12345, intItem->GetValue());
}

TEST_F(BinarySerializerTest, SerializeDeserialize_Boolean_True)
{
    auto original = CreateBooleanItem(true);

    // Serialize
    ByteVector serialized = BinarySerializer::Serialize(original);
    EXPECT_FALSE(serialized.empty());

    // Deserialize
    auto deserialized = BinarySerializer::Deserialize(serialized.AsSpan());
    ASSERT_NE(nullptr, deserialized);

    // Verify type and value
    EXPECT_EQ(StackItemType::Boolean, deserialized->GetType());
    auto boolItem = std::dynamic_pointer_cast<BooleanStackItem>(deserialized);
    ASSERT_NE(nullptr, boolItem);
    EXPECT_TRUE(boolItem->GetValue());
}

TEST_F(BinarySerializerTest, SerializeDeserialize_Boolean_False)
{
    auto original = CreateBooleanItem(false);

    // Serialize
    ByteVector serialized = BinarySerializer::Serialize(original);
    EXPECT_FALSE(serialized.empty());

    // Deserialize
    auto deserialized = BinarySerializer::Deserialize(serialized.AsSpan());
    ASSERT_NE(nullptr, deserialized);

    // Verify type and value
    EXPECT_EQ(StackItemType::Boolean, deserialized->GetType());
    auto boolItem = std::dynamic_pointer_cast<BooleanStackItem>(deserialized);
    ASSERT_NE(nullptr, boolItem);
    EXPECT_FALSE(boolItem->GetValue());
}

TEST_F(BinarySerializerTest, SerializeDeserialize_ByteString)
{
    std::string testString = "Hello, Neo!";
    auto original = CreateByteStringItem(testString);

    // Serialize
    ByteVector serialized = BinarySerializer::Serialize(original);
    EXPECT_FALSE(serialized.empty());

    // Deserialize
    auto deserialized = BinarySerializer::Deserialize(serialized.AsSpan());
    ASSERT_NE(nullptr, deserialized);

    // Verify type and value
    EXPECT_EQ(StackItemType::ByteString, deserialized->GetType());
    auto stringItem = std::dynamic_pointer_cast<ByteStringStackItem>(deserialized);
    ASSERT_NE(nullptr, stringItem);

    ByteVector expectedBytes(reinterpret_cast<const uint8_t*>(testString.data()), testString.size());
    EXPECT_EQ(expectedBytes, stringItem->GetValue());
}

TEST_F(BinarySerializerTest, SerializeDeserialize_Null)
{
    auto original = CreateNullItem();

    // Serialize
    ByteVector serialized = BinarySerializer::Serialize(original);
    EXPECT_FALSE(serialized.empty());

    // Deserialize
    auto deserialized = BinarySerializer::Deserialize(serialized.AsSpan());
    ASSERT_NE(nullptr, deserialized);

    // Verify type
    EXPECT_EQ(StackItemType::Null, deserialized->GetType());
}

TEST_F(BinarySerializerTest, SerializeDeserialize_Array)
{
    auto array = std::make_shared<ArrayStackItem>();
    array->Add(CreateIntegerItem(1));
    array->Add(CreateIntegerItem(2));
    array->Add(CreateBooleanItem(true));

    // Serialize
    ByteVector serialized = BinarySerializer::Serialize(array);
    EXPECT_FALSE(serialized.empty());

    // Deserialize
    auto deserialized = BinarySerializer::Deserialize(serialized.AsSpan());
    ASSERT_NE(nullptr, deserialized);

    // Verify type and contents
    EXPECT_EQ(StackItemType::Array, deserialized->GetType());
    auto arrayItem = std::dynamic_pointer_cast<ArrayStackItem>(deserialized);
    ASSERT_NE(nullptr, arrayItem);
    EXPECT_EQ(3u, arrayItem->Count());

    // Verify elements
    auto item0 = std::dynamic_pointer_cast<IntegerStackItem>(arrayItem->At(0));
    ASSERT_NE(nullptr, item0);
    EXPECT_EQ(1, item0->GetValue());

    auto item1 = std::dynamic_pointer_cast<IntegerStackItem>(arrayItem->At(1));
    ASSERT_NE(nullptr, item1);
    EXPECT_EQ(2, item1->GetValue());

    auto item2 = std::dynamic_pointer_cast<BooleanStackItem>(arrayItem->At(2));
    ASSERT_NE(nullptr, item2);
    EXPECT_TRUE(item2->GetValue());
}

TEST_F(BinarySerializerTest, SerializeDeserialize_Map)
{
    auto map = std::make_shared<MapStackItem>();
    map->Put(CreateByteStringItem("key1"), CreateIntegerItem(100));
    map->Put(CreateByteStringItem("key2"), CreateBooleanItem(false));

    // Serialize
    ByteVector serialized = BinarySerializer::Serialize(map);
    EXPECT_FALSE(serialized.empty());

    // Deserialize
    auto deserialized = BinarySerializer::Deserialize(serialized.AsSpan());
    ASSERT_NE(nullptr, deserialized);

    // Verify type and contents
    EXPECT_EQ(StackItemType::Map, deserialized->GetType());
    auto mapItem = std::dynamic_pointer_cast<MapStackItem>(deserialized);
    ASSERT_NE(nullptr, mapItem);
    EXPECT_EQ(2u, mapItem->Count());
}

TEST_F(BinarySerializerTest, SerializeDeserialize_NestedStructures)
{
    // Create nested array with map inside
    auto outerArray = std::make_shared<ArrayStackItem>();
    outerArray->Add(CreateIntegerItem(42));

    auto innerMap = std::make_shared<MapStackItem>();
    innerMap->Put(CreateByteStringItem("nested"), CreateBooleanItem(true));
    outerArray->Add(innerMap);

    auto innerArray = std::make_shared<ArrayStackItem>();
    innerArray->Add(CreateIntegerItem(1));
    innerArray->Add(CreateIntegerItem(2));
    outerArray->Add(innerArray);

    // Serialize
    ByteVector serialized = BinarySerializer::Serialize(outerArray);
    EXPECT_FALSE(serialized.empty());

    // Deserialize
    auto deserialized = BinarySerializer::Deserialize(serialized.AsSpan());
    ASSERT_NE(nullptr, deserialized);

    // Verify structure
    EXPECT_EQ(StackItemType::Array, deserialized->GetType());
    auto arrayItem = std::dynamic_pointer_cast<ArrayStackItem>(deserialized);
    ASSERT_NE(nullptr, arrayItem);
    EXPECT_EQ(3u, arrayItem->Count());
}

TEST_F(BinarySerializerTest, MaxSizeLimit)
{
    auto item = CreateByteStringItem("small");

    // Should succeed with adequate size limit
    EXPECT_NO_THROW({
        ByteVector result = BinarySerializer::Serialize(item, 1000);
        EXPECT_FALSE(result.empty());
    });

    // Should fail with very small size limit
    EXPECT_THROW({ BinarySerializer::Serialize(item, 1); }, std::exception);
}

TEST_F(BinarySerializerTest, MaxItemsLimit)
{
    auto array = std::make_shared<ArrayStackItem>();

    // Add many items
    for (int i = 0; i < 10; ++i)
    {
        array->Add(CreateIntegerItem(i));
    }

    // Should succeed with adequate item limit
    EXPECT_NO_THROW({
        ByteVector result = BinarySerializer::Serialize(array, 2048, 20);
        EXPECT_FALSE(result.empty());
    });

    // Should fail with very small item limit
    EXPECT_THROW({ BinarySerializer::Serialize(array, 2048, 5); }, std::exception);
}

TEST_F(BinarySerializerTest, EmptyArray)
{
    auto emptyArray = std::make_shared<ArrayStackItem>();

    // Serialize
    ByteVector serialized = BinarySerializer::Serialize(emptyArray);
    EXPECT_FALSE(serialized.empty());

    // Deserialize
    auto deserialized = BinarySerializer::Deserialize(serialized.AsSpan());
    ASSERT_NE(nullptr, deserialized);

    // Verify
    EXPECT_EQ(StackItemType::Array, deserialized->GetType());
    auto arrayItem = std::dynamic_pointer_cast<ArrayStackItem>(deserialized);
    ASSERT_NE(nullptr, arrayItem);
    EXPECT_EQ(0u, arrayItem->Count());
}

TEST_F(BinarySerializerTest, EmptyMap)
{
    auto emptyMap = std::make_shared<MapStackItem>();

    // Serialize
    ByteVector serialized = BinarySerializer::Serialize(emptyMap);
    EXPECT_FALSE(serialized.empty());

    // Deserialize
    auto deserialized = BinarySerializer::Deserialize(serialized.AsSpan());
    ASSERT_NE(nullptr, deserialized);

    // Verify
    EXPECT_EQ(StackItemType::Map, deserialized->GetType());
    auto mapItem = std::dynamic_pointer_cast<MapStackItem>(deserialized);
    ASSERT_NE(nullptr, mapItem);
    EXPECT_EQ(0u, mapItem->Count());
}

TEST_F(BinarySerializerTest, LargeInteger)
{
    // Test with large integer values
    std::vector<int64_t> testValues = {0, 1, -1, INT32_MAX, INT32_MIN, INT64_MAX, INT64_MIN};

    for (int64_t value : testValues)
    {
        auto original = CreateIntegerItem(value);

        // Serialize
        ByteVector serialized = BinarySerializer::Serialize(original);
        EXPECT_FALSE(serialized.empty());

        // Deserialize
        auto deserialized = BinarySerializer::Deserialize(serialized.AsSpan());
        ASSERT_NE(nullptr, deserialized);

        // Verify
        auto intItem = std::dynamic_pointer_cast<IntegerStackItem>(deserialized);
        ASSERT_NE(nullptr, intItem);
        EXPECT_EQ(value, intItem->GetValue()) << "Failed for value: " << value;
    }
}

TEST_F(BinarySerializerTest, UTF8ByteString)
{
    // Test with UTF-8 encoded strings
    std::vector<std::string> testStrings = {"", "ASCII", "Hello, 世界!", "🚀 Neo blockchain", "Тест на кириллице"};

    for (const auto& str : testStrings)
    {
        auto original = CreateByteStringItem(str);

        // Serialize
        ByteVector serialized = BinarySerializer::Serialize(original);
        EXPECT_FALSE(serialized.empty());

        // Deserialize
        auto deserialized = BinarySerializer::Deserialize(serialized.AsSpan());
        ASSERT_NE(nullptr, deserialized);

        // Verify
        auto stringItem = std::dynamic_pointer_cast<ByteStringStackItem>(deserialized);
        ASSERT_NE(nullptr, stringItem);

        ByteVector expectedBytes(reinterpret_cast<const uint8_t*>(str.data()), str.size());
        EXPECT_EQ(expectedBytes, stringItem->GetValue()) << "Failed for string: " << str;
    }
}

TEST_F(BinarySerializerTest, RoundTripMultipleTimes)
{
    auto original = CreateIntegerItem(999);
    std::shared_ptr<StackItem> current = original;

    // Perform multiple serialize/deserialize cycles
    for (int i = 0; i < 5; ++i)
    {
        ByteVector serialized = BinarySerializer::Serialize(current);
        current = BinarySerializer::Deserialize(serialized.AsSpan());

        ASSERT_NE(nullptr, current);
        auto intItem = std::dynamic_pointer_cast<IntegerStackItem>(current);
        ASSERT_NE(nullptr, intItem);
        EXPECT_EQ(999, intItem->GetValue());
    }
}

TEST_F(BinarySerializerTest, NullInput)
{
    // Test serializing null pointer
    EXPECT_THROW({ BinarySerializer::Serialize(nullptr); }, std::exception);
}

TEST_F(BinarySerializerTest, EmptyByteArray)
{
    ByteVector emptyBytes;

    // Test deserializing empty byte array
    EXPECT_THROW({ BinarySerializer::Deserialize(emptyBytes.AsSpan()); }, std::exception);
}

TEST_F(BinarySerializerTest, CorruptedData)
{
    // Create valid data first
    auto original = CreateIntegerItem(42);
    ByteVector validData = BinarySerializer::Serialize(original);

    // Corrupt the data
    ByteVector corruptedData = validData;
    if (!corruptedData.empty())
    {
        corruptedData[0] = 0xFF;  // Invalid type marker
    }

    // Should throw when deserializing corrupted data
    EXPECT_THROW({ BinarySerializer::Deserialize(corruptedData.AsSpan()); }, std::exception);
}
