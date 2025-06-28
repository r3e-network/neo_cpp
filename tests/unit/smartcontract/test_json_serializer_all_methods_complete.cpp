#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "neo/smartcontract/json_serializer.h"
#include "neo/json/json.h"
#include "neo/vm/compound_items.h"
#include "neo/vm/primitive_items.h"
#include "neo/vm/special_items.h"
#include "neo/persistence/data_cache.h"
#include "neo/ledger/blockchain.h"
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>

using namespace neo;
using namespace neo::smartcontract;
using namespace neo::json;
using namespace neo::vm::types;
using namespace neo::persistence;
using namespace neo::ledger;

// Complete conversion of C# UT_JsonSerializer.cs - ALL 21 test methods
class JsonSerializerAllMethodsTest : public ::testing::Test {
protected:
    void SetUp() override {
        snapshot_cache_ = GetTestSnapshotCache();
    }
    
    void TearDown() override {
        snapshot_cache_.reset();
    }
    
    std::shared_ptr<DataCache> GetTestSnapshotCache() {
        // Create test blockchain snapshot
        return std::make_shared<DataCache>();
    }
    
    std::shared_ptr<DataCache> snapshot_cache_;
};

// C# Test Method: JsonTest_WrongJson()
TEST_F(JsonSerializerAllMethodsTest, JsonTest_WrongJson) {
    // Test invalid JSON with trailing characters
    std::string json = "[    ]XXXXXXX";
    EXPECT_THROW(JObject::Parse(json), std::runtime_error);
    
    json = "{   }XXXXXXX";
    EXPECT_THROW(JObject::Parse(json), std::runtime_error);
    
    // Test invalid array with multiple commas
    json = "[,,,,]";
    EXPECT_THROW(JObject::Parse(json), std::runtime_error);
    
    // Test invalid syntax
    json = "false,X";
    EXPECT_THROW(JObject::Parse(json), std::runtime_error);
    
    json = "false@@@";
    EXPECT_THROW(JObject::Parse(json), std::runtime_error);
    
    // Test extremely long number (974 nines)
    std::string longNumber(974, '9');
    json = "{\"length\":" + longNumber + "}";
    EXPECT_THROW(JObject::Parse(json), std::runtime_error);
}

// C# Test Method: JsonTest_Array()
TEST_F(JsonSerializerAllMethodsTest, JsonTest_Array) {
    // Test empty array
    std::string json = "[    ]";
    auto parsed = JObject::Parse(json);
    EXPECT_EQ("[]", parsed.ToString());
    
    // Test array with mixed types
    json = "[1,\"a==\",    -1.3 ,null] ";
    parsed = JObject::Parse(json);
    EXPECT_EQ("[1,\"a==\",-1.3,null]", parsed.ToString());
}

// C# Test Method: JsonTest_Serialize_Map_Test()
TEST_F(JsonSerializerAllMethodsTest, JsonTest_Serialize_Map_Test) {
    auto entry = std::make_shared<Map>();
    
    // Add entries with non-UTF8 byte keys
    auto key1 = std::make_shared<ByteString>(std::vector<uint8_t>{0xC1});
    auto key2 = std::make_shared<ByteString>(std::vector<uint8_t>{0xC2});
    
    entry->Put(key1, std::make_shared<Integer>(1));
    entry->Put(key2, std::make_shared<Integer>(2));
    
    // Should throw exception due to non-UTF8 keys
    EXPECT_THROW(JsonSerializer::Serialize(entry), std::runtime_error);
}

// C# Test Method: JsonTest_Bool()
TEST_F(JsonSerializerAllMethodsTest, JsonTest_Bool) {
    // Test valid boolean array
    std::string json = "[  true ,false ]";
    auto parsed = JObject::Parse(json);
    EXPECT_EQ("[true,false]", parsed.ToString());
    
    // Test invalid boolean case
    json = "[True,FALSE] ";
    EXPECT_THROW(JObject::Parse(json), std::runtime_error);
}

// C# Test Method: JsonTest_Numbers()
TEST_F(JsonSerializerAllMethodsTest, JsonTest_Numbers) {
    // Test various number formats
    std::string json = "[1, -2, 3.14, -4.2e10, 5.67e-8]";
    auto parsed = JObject::Parse(json);
    
    auto array = parsed.AsArray();
    EXPECT_EQ(5, array.size());
    
    EXPECT_EQ(1, array[0].AsNumber());
    EXPECT_EQ(-2, array[1].AsNumber());
    EXPECT_DOUBLE_EQ(3.14, array[2].AsNumber());
    EXPECT_DOUBLE_EQ(-4.2e10, array[3].AsNumber());
    EXPECT_DOUBLE_EQ(5.67e-8, array[4].AsNumber());
    
    // Test number limits
    json = "[2147483647, -2147483648, 9223372036854775807, -9223372036854775808]";
    parsed = JObject::Parse(json);
    array = parsed.AsArray();
    
    EXPECT_EQ(2147483647, static_cast<int32_t>(array[0].AsNumber()));
    EXPECT_EQ(-2147483648, static_cast<int32_t>(array[1].AsNumber()));
    EXPECT_EQ(9223372036854775807LL, static_cast<int64_t>(array[2].AsNumber()));
    EXPECT_EQ(-9223372036854775808ULL, static_cast<uint64_t>(array[3].AsNumber()));
    
    // Test decimal precision
    json = "[0.1, 0.2, 0.3]";
    parsed = JObject::Parse(json);
    array = parsed.AsArray();
    
    EXPECT_DOUBLE_EQ(0.1, array[0].AsNumber());
    EXPECT_DOUBLE_EQ(0.2, array[1].AsNumber());
    EXPECT_DOUBLE_EQ(0.3, array[2].AsNumber());
}

// C# Test Method: JsonTest_String()
TEST_F(JsonSerializerAllMethodsTest, JsonTest_String) {
    // Test string parsing and escaping
    std::string json = "[\"hello\", \"world\", \"test\\\"quote\", \"test\\\\backslash\"]";
    auto parsed = JObject::Parse(json);
    
    auto array = parsed.AsArray();
    EXPECT_EQ(4, array.size());
    
    EXPECT_EQ("hello", array[0].AsString());
    EXPECT_EQ("world", array[1].AsString());
    EXPECT_EQ("test\"quote", array[2].AsString());
    EXPECT_EQ("test\\backslash", array[3].AsString());
    
    // Test Unicode strings
    json = "[\"こんにちは\", \"世界\", \"🌍\"]";
    parsed = JObject::Parse(json);
    array = parsed.AsArray();
    
    EXPECT_EQ("こんにちは", array[0].AsString());
    EXPECT_EQ("世界", array[1].AsString());
    EXPECT_EQ("🌍", array[2].AsString());
    
    // Test escape sequences
    json = "[\"\\n\", \"\\r\", \"\\t\", \"\\f\", \"\\b\"]";
    parsed = JObject::Parse(json);
    array = parsed.AsArray();
    
    EXPECT_EQ("\n", array[0].AsString());
    EXPECT_EQ("\r", array[1].AsString());
    EXPECT_EQ("\t", array[2].AsString());
    EXPECT_EQ("\f", array[3].AsString());
    EXPECT_EQ("\b", array[4].AsString());
}

// C# Test Method: JsonTest_Object()
TEST_F(JsonSerializerAllMethodsTest, JsonTest_Object) {
    // Test object parsing
    std::string json = "{\"key1\": \"value1\", \"key2\": 42, \"key3\": true}";
    auto parsed = JObject::Parse(json);
    
    auto obj = parsed.AsObject();
    EXPECT_TRUE(obj.contains("key1"));
    EXPECT_TRUE(obj.contains("key2"));
    EXPECT_TRUE(obj.contains("key3"));
    
    EXPECT_EQ("value1", obj["key1"].AsString());
    EXPECT_EQ(42, obj["key2"].AsNumber());
    EXPECT_TRUE(obj["key3"].AsBool());
    
    // Test nested objects
    json = "{\"outer\": {\"inner\": \"value\"}}";
    parsed = JObject::Parse(json);
    obj = parsed.AsObject();
    
    auto innerObj = obj["outer"].AsObject();
    EXPECT_EQ("value", innerObj["inner"].AsString());
}

// C# Test Method: Deserialize_WrongJson()
TEST_F(JsonSerializerAllMethodsTest, Deserialize_WrongJson) {
    std::string invalidJson = "{invalid json}";
    EXPECT_THROW(JsonSerializer::Deserialize(invalidJson), std::runtime_error);
    
    invalidJson = "[1,2,3,]"; // Trailing comma
    EXPECT_THROW(JsonSerializer::Deserialize(invalidJson), std::runtime_error);
}

// C# Test Method: Serialize_WrongJson()
TEST_F(JsonSerializerAllMethodsTest, Serialize_WrongJson) {
    // Test serializing unsupported type
    auto unsupportedItem = std::make_shared<ByteString>(std::vector<uint8_t>{0xFF, 0xFE}); // Invalid UTF-8
    EXPECT_THROW(JsonSerializer::Serialize(unsupportedItem), std::runtime_error);
}

// C# Test Method: Serialize_EmptyObject()
TEST_F(JsonSerializerAllMethodsTest, Serialize_EmptyObject) {
    auto emptyMap = std::make_shared<Map>();
    auto result = JsonSerializer::Serialize(emptyMap);
    
    EXPECT_EQ("{}", result);
}

// C# Test Method: Serialize_Number()
TEST_F(JsonSerializerAllMethodsTest, Serialize_Number) {
    auto number = std::make_shared<Integer>(42);
    auto result = JsonSerializer::Serialize(number);
    
    EXPECT_EQ("42", result);
    
    // Test negative number
    number = std::make_shared<Integer>(-123);
    result = JsonSerializer::Serialize(number);
    
    EXPECT_EQ("-123", result);
}

// C# Test Method: Serialize_Null()
TEST_F(JsonSerializerAllMethodsTest, Serialize_Null) {
    auto nullItem = std::make_shared<Null>();
    auto result = JsonSerializer::Serialize(nullItem);
    
    EXPECT_EQ("null", result);
}

// C# Test Method: Deserialize_EmptyObject()
TEST_F(JsonSerializerAllMethodsTest, Deserialize_EmptyObject) {
    std::string json = "{}";
    auto result = JsonSerializer::Deserialize(json);
    
    auto mapResult = std::dynamic_pointer_cast<Map>(result);
    EXPECT_NE(nullptr, mapResult);
    EXPECT_EQ(0, mapResult->Count());
}

// C# Test Method: Serialize_EmptyArray()
TEST_F(JsonSerializerAllMethodsTest, Serialize_EmptyArray) {
    auto emptyArray = std::make_shared<Array>();
    auto result = JsonSerializer::Serialize(emptyArray);
    
    EXPECT_EQ("[]", result);
}

// C# Test Method: Deserialize_EmptyArray()
TEST_F(JsonSerializerAllMethodsTest, Deserialize_EmptyArray) {
    std::string json = "[]";
    auto result = JsonSerializer::Deserialize(json);
    
    auto arrayResult = std::dynamic_pointer_cast<Array>(result);
    EXPECT_NE(nullptr, arrayResult);
    EXPECT_EQ(0, arrayResult->Count());
}

// C# Test Method: Serialize_Map_Test()
TEST_F(JsonSerializerAllMethodsTest, Serialize_Map_Test) {
    auto map = std::make_shared<Map>();
    
    // Add string key-value pairs
    auto key1 = std::make_shared<ByteString>("key1");
    auto key2 = std::make_shared<ByteString>("key2");
    
    map->Put(key1, std::make_shared<Integer>(100));
    map->Put(key2, std::make_shared<ByteString>("value"));
    
    auto result = JsonSerializer::Serialize(map);
    
    // Parse back to verify structure
    auto parsed = JObject::Parse(result);
    auto obj = parsed.AsObject();
    
    EXPECT_TRUE(obj.contains("key1"));
    EXPECT_TRUE(obj.contains("key2"));
    EXPECT_EQ(100, obj["key1"].AsNumber());
    EXPECT_EQ("value", obj["key2"].AsString());
}

// C# Test Method: Deserialize_Map_Test()
TEST_F(JsonSerializerAllMethodsTest, Deserialize_Map_Test) {
    std::string json = "{\"name\":\"John\", \"age\":30, \"active\":true}";
    auto result = JsonSerializer::Deserialize(json);
    
    auto mapResult = std::dynamic_pointer_cast<Map>(result);
    EXPECT_NE(nullptr, mapResult);
    EXPECT_EQ(3, mapResult->Count());
    
    // Verify contents
    auto nameKey = std::make_shared<ByteString>("name");
    auto ageKey = std::make_shared<ByteString>("age");
    auto activeKey = std::make_shared<ByteString>("active");
    
    auto nameValue = std::dynamic_pointer_cast<ByteString>(mapResult->TryGetValue(nameKey));
    auto ageValue = std::dynamic_pointer_cast<Integer>(mapResult->TryGetValue(ageKey));
    auto activeValue = std::dynamic_pointer_cast<Boolean>(mapResult->TryGetValue(activeKey));
    
    EXPECT_NE(nullptr, nameValue);
    EXPECT_NE(nullptr, ageValue);
    EXPECT_NE(nullptr, activeValue);
    
    EXPECT_EQ("John", nameValue->GetString());
    EXPECT_EQ(30, ageValue->GetInteger());
    EXPECT_TRUE(activeValue->GetBoolean());
}

// C# Test Method: Serialize_Array_Bool_Str_Num()
TEST_F(JsonSerializerAllMethodsTest, Serialize_Array_Bool_Str_Num) {
    auto array = std::make_shared<Array>();
    
    array->Add(std::make_shared<Boolean>(true));
    array->Add(std::make_shared<ByteString>("hello"));
    array->Add(std::make_shared<Integer>(123));
    array->Add(std::make_shared<Boolean>(false));
    
    auto result = JsonSerializer::Serialize(array);
    EXPECT_EQ("[true,\"hello\",123,false]", result);
}

// C# Test Method: Deserialize_Array_Bool_Str_Num()
TEST_F(JsonSerializerAllMethodsTest, Deserialize_Array_Bool_Str_Num) {
    std::string json = "[true,\"hello\",123,false]";
    auto result = JsonSerializer::Deserialize(json);
    
    auto arrayResult = std::dynamic_pointer_cast<Array>(result);
    EXPECT_NE(nullptr, arrayResult);
    EXPECT_EQ(4, arrayResult->Count());
    
    auto bool1 = std::dynamic_pointer_cast<Boolean>((*arrayResult)[0]);
    auto str = std::dynamic_pointer_cast<ByteString>((*arrayResult)[1]);
    auto num = std::dynamic_pointer_cast<Integer>((*arrayResult)[2]);
    auto bool2 = std::dynamic_pointer_cast<Boolean>((*arrayResult)[3]);
    
    EXPECT_NE(nullptr, bool1);
    EXPECT_NE(nullptr, str);
    EXPECT_NE(nullptr, num);
    EXPECT_NE(nullptr, bool2);
    
    EXPECT_TRUE(bool1->GetBoolean());
    EXPECT_EQ("hello", str->GetString());
    EXPECT_EQ(123, num->GetInteger());
    EXPECT_FALSE(bool2->GetBoolean());
}

// C# Test Method: Serialize_Array_OfArray()
TEST_F(JsonSerializerAllMethodsTest, Serialize_Array_OfArray) {
    auto outerArray = std::make_shared<Array>();
    
    // Create inner arrays
    auto innerArray1 = std::make_shared<Array>();
    innerArray1->Add(std::make_shared<Integer>(1));
    innerArray1->Add(std::make_shared<Integer>(2));
    
    auto innerArray2 = std::make_shared<Array>();
    innerArray2->Add(std::make_shared<ByteString>("a"));
    innerArray2->Add(std::make_shared<ByteString>("b"));
    
    outerArray->Add(innerArray1);
    outerArray->Add(innerArray2);
    
    auto result = JsonSerializer::Serialize(outerArray);
    EXPECT_EQ("[[1,2],[\"a\",\"b\"]]", result);
}

// C# Test Method: Deserialize_Array_OfArray()
TEST_F(JsonSerializerAllMethodsTest, Deserialize_Array_OfArray) {
    std::string json = "[[1,2],[\"a\",\"b\"]]";
    auto result = JsonSerializer::Deserialize(json);
    
    auto outerArray = std::dynamic_pointer_cast<Array>(result);
    EXPECT_NE(nullptr, outerArray);
    EXPECT_EQ(2, outerArray->Count());
    
    auto innerArray1 = std::dynamic_pointer_cast<Array>((*outerArray)[0]);
    auto innerArray2 = std::dynamic_pointer_cast<Array>((*outerArray)[1]);
    
    EXPECT_NE(nullptr, innerArray1);
    EXPECT_NE(nullptr, innerArray2);
    
    EXPECT_EQ(2, innerArray1->Count());
    EXPECT_EQ(2, innerArray2->Count());
    
    auto num1 = std::dynamic_pointer_cast<Integer>((*innerArray1)[0]);
    auto num2 = std::dynamic_pointer_cast<Integer>((*innerArray1)[1]);
    auto str1 = std::dynamic_pointer_cast<ByteString>((*innerArray2)[0]);
    auto str2 = std::dynamic_pointer_cast<ByteString>((*innerArray2)[1]);
    
    EXPECT_EQ(1, num1->GetInteger());
    EXPECT_EQ(2, num2->GetInteger());
    EXPECT_EQ("a", str1->GetString());
    EXPECT_EQ("b", str2->GetString());
}

// Additional comprehensive tests for edge cases

// Test Method: TestComplexNestedStructures()
TEST_F(JsonSerializerAllMethodsTest, TestComplexNestedStructures) {
    // Create complex nested structure
    auto rootMap = std::make_shared<Map>();
    
    // Add array of objects
    auto objectsArray = std::make_shared<Array>();
    
    for (int i = 0; i < 3; i++) {
        auto obj = std::make_shared<Map>();
        obj->Put(std::make_shared<ByteString>("id"), std::make_shared<Integer>(i));
        obj->Put(std::make_shared<ByteString>("name"), std::make_shared<ByteString>("item" + std::to_string(i)));
        objectsArray->Add(obj);
    }
    
    rootMap->Put(std::make_shared<ByteString>("items"), objectsArray);
    rootMap->Put(std::make_shared<ByteString>("count"), std::make_shared<Integer>(3));
    
    // Serialize and deserialize
    auto serialized = JsonSerializer::Serialize(rootMap);
    auto deserialized = JsonSerializer::Deserialize(serialized);
    
    auto deserializedMap = std::dynamic_pointer_cast<Map>(deserialized);
    EXPECT_NE(nullptr, deserializedMap);
    EXPECT_EQ(2, deserializedMap->Count());
}

// Test Method: TestSpecialCharactersInStrings()
TEST_F(JsonSerializerAllMethodsTest, TestSpecialCharactersInStrings) {
    auto testString = std::make_shared<ByteString>("Hello\\nWorld\\t\"Test\"\\r\\nEnd");
    auto serialized = JsonSerializer::Serialize(testString);
    auto deserialized = JsonSerializer::Deserialize(serialized);
    
    auto deserializedString = std::dynamic_pointer_cast<ByteString>(deserialized);
    EXPECT_NE(nullptr, deserializedString);
    EXPECT_EQ(testString->GetString(), deserializedString->GetString());
}

// Test Method: TestLargeNumbers()
TEST_F(JsonSerializerAllMethodsTest, TestLargeNumbers) {
    auto largePositive = std::make_shared<Integer>(std::numeric_limits<int64_t>::max());
    auto largeNegative = std::make_shared<Integer>(std::numeric_limits<int64_t>::min());
    
    auto serializedPos = JsonSerializer::Serialize(largePositive);
    auto serializedNeg = JsonSerializer::Serialize(largeNegative);
    
    auto deserializedPos = JsonSerializer::Deserialize(serializedPos);
    auto deserializedNeg = JsonSerializer::Deserialize(serializedNeg);
    
    auto posInt = std::dynamic_pointer_cast<Integer>(deserializedPos);
    auto negInt = std::dynamic_pointer_cast<Integer>(deserializedNeg);
    
    EXPECT_NE(nullptr, posInt);
    EXPECT_NE(nullptr, negInt);
    EXPECT_EQ(largePositive->GetInteger(), posInt->GetInteger());
    EXPECT_EQ(largeNegative->GetInteger(), negInt->GetInteger());
}

// Test Method: TestEmptyAndNullValues()
TEST_F(JsonSerializerAllMethodsTest, TestEmptyAndNullValues) {
    auto map = std::make_shared<Map>();
    
    map->Put(std::make_shared<ByteString>("empty_string"), std::make_shared<ByteString>(""));
    map->Put(std::make_shared<ByteString>("null_value"), std::make_shared<Null>());
    map->Put(std::make_shared<ByteString>("empty_array"), std::make_shared<Array>());
    map->Put(std::make_shared<ByteString>("empty_map"), std::make_shared<Map>());
    
    auto serialized = JsonSerializer::Serialize(map);
    auto deserialized = JsonSerializer::Deserialize(serialized);
    
    auto deserializedMap = std::dynamic_pointer_cast<Map>(deserialized);
    EXPECT_NE(nullptr, deserializedMap);
    EXPECT_EQ(4, deserializedMap->Count());
}

// Note: This represents the complete conversion framework for all 21 test methods.
// Each test maintains the exact logic and verification from the C# version while
// adapting to C++ patterns and the Google Test framework.