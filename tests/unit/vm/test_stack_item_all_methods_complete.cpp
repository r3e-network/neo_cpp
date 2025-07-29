#include "neo/extensions/utility.h"
#include "neo/vm/execution_engine_limits.h"
#include "neo/vm/reference_counter.h"
#include "neo/vm/script.h"
#include "neo/vm/stack_item.h"
#include "neo/vm/types/array.h"
#include "neo/vm/types/boolean.h"
#include "neo/vm/types/buffer.h"
#include "neo/vm/types/byte_string.h"
#include "neo/vm/types/integer.h"
#include "neo/vm/types/interop_interface.h"
#include "neo/vm/types/map.h"
#include "neo/vm/types/null.h"
#include "neo/vm/types/pointer.h"
#include "neo/vm/types/struct.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <string>
#include <vector>

using namespace neo;
using namespace neo::vm;
using namespace neo::vm::types;

// Complete conversion of C# UT_StackItem.cs - ALL 6 test methods
class StackItemAllMethodsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        reference_counter_ = std::make_shared<ReferenceCounter>();
    }

    void TearDown() override
    {
        reference_counter_.reset();
    }

    // Helper to create stack items with reference counter
    std::shared_ptr<Integer> CreateInteger(const BigInteger& value)
    {
        return std::make_shared<Integer>(value, reference_counter_.get());
    }

    std::shared_ptr<Boolean> CreateBoolean(bool value)
    {
        return std::make_shared<Boolean>(value, reference_counter_.get());
    }

    std::shared_ptr<ByteString> CreateByteString(const std::string& value)
    {
        return std::make_shared<ByteString>(value, reference_counter_.get());
    }

    std::shared_ptr<ByteString> CreateByteString(const std::vector<uint8_t>& value)
    {
        return std::make_shared<ByteString>(value, reference_counter_.get());
    }

    std::shared_ptr<Buffer> CreateBuffer(size_t size)
    {
        return std::make_shared<Buffer>(size, reference_counter_.get());
    }

    std::shared_ptr<Buffer> CreateBuffer(const std::vector<uint8_t>& data)
    {
        return std::make_shared<Buffer>(data, reference_counter_.get());
    }

    std::shared_ptr<Null> CreateNull()
    {
        return std::make_shared<Null>(reference_counter_.get());
    }

    std::shared_ptr<Array> CreateArray()
    {
        return std::make_shared<Array>(reference_counter_.get());
    }

    std::shared_ptr<Struct> CreateStruct()
    {
        return std::make_shared<Struct>(reference_counter_.get());
    }

    std::shared_ptr<Map> CreateMap()
    {
        return std::make_shared<Map>(reference_counter_.get());
    }

    std::shared_ptr<InteropInterface> CreateInteropInterface(int value)
    {
        return std::make_shared<InteropInterface>(value, reference_counter_.get());
    }

    std::shared_ptr<Pointer> CreatePointer(std::shared_ptr<Script> script, int position)
    {
        return std::make_shared<Pointer>(script, position, reference_counter_.get());
    }

    std::shared_ptr<ReferenceCounter> reference_counter_;
};

// C# Test Method: TestCircularReference()
TEST_F(StackItemAllMethodsTest, TestCircularReference)
{
    auto item_a = CreateStruct();
    auto item_b = CreateStruct();
    auto item_c = CreateStruct();

    // Add initial elements
    item_a->Add(CreateBoolean(true));
    item_a->Add(CreateBoolean(false));
    item_b->Add(CreateBoolean(true));
    item_b->Add(CreateBoolean(false));
    item_c->Add(CreateBoolean(false));
    item_c->Add(CreateBoolean(false));

    // Create circular references
    (*item_a)[1] = item_a;  // itemA[1] = itemA
    (*item_b)[1] = item_b;  // itemB[1] = itemB
    (*item_c)[1] = item_c;  // itemC[1] = itemC

    // Items A and B should have same hash (both have [true, self])
    EXPECT_EQ(item_a->GetHashCode(), item_b->GetHashCode());

    // Item C should have different hash (has [false, self])
    EXPECT_NE(item_a->GetHashCode(), item_c->GetHashCode());
}

// C# Test Method: TestHashCode()
TEST_F(StackItemAllMethodsTest, TestHashCode)
{
    // Test ByteString hash codes
    {
        auto item_a = CreateByteString("NEO");
        auto item_b = CreateByteString("NEO");
        auto item_c = CreateByteString("SmartEconomy");

        EXPECT_EQ(item_b->GetHashCode(), item_a->GetHashCode());
        EXPECT_NE(item_c->GetHashCode(), item_a->GetHashCode());
    }

    // Test Buffer hash codes
    {
        auto item_a = CreateBuffer(1);
        auto item_b = CreateBuffer(1);
        auto item_c = CreateBuffer(2);

        EXPECT_EQ(item_b->GetHashCode(), item_a->GetHashCode());
        EXPECT_NE(item_c->GetHashCode(), item_a->GetHashCode());
    }

    // Test ByteString from byte arrays
    {
        auto item_a = CreateByteString({1, 2, 3});
        auto item_b = CreateByteString({1, 2, 3});
        auto item_c = CreateByteString({5, 6});

        EXPECT_EQ(item_b->GetHashCode(), item_a->GetHashCode());
        EXPECT_NE(item_c->GetHashCode(), item_a->GetHashCode());
    }

    // Test Boolean hash codes
    {
        auto item_a = CreateBoolean(true);
        auto item_b = CreateBoolean(true);
        auto item_c = CreateBoolean(false);

        EXPECT_EQ(item_b->GetHashCode(), item_a->GetHashCode());
        EXPECT_NE(item_c->GetHashCode(), item_a->GetHashCode());
    }

    // Test Integer hash codes
    {
        auto item_a = CreateInteger(1);
        auto item_b = CreateInteger(1);
        auto item_c = CreateInteger(123);

        EXPECT_EQ(item_b->GetHashCode(), item_a->GetHashCode());
        EXPECT_NE(item_c->GetHashCode(), item_a->GetHashCode());
    }

    // Test Null hash codes
    {
        auto item_a = CreateNull();
        auto item_b = CreateNull();

        EXPECT_EQ(item_b->GetHashCode(), item_a->GetHashCode());
    }

    // Test Array hash codes
    {
        auto item_a = CreateArray();
        item_a->Add(CreateBoolean(true));
        item_a->Add(CreateBoolean(false));
        item_a->Add(CreateInteger(0));

        auto item_b = CreateArray();
        item_b->Add(CreateBoolean(true));
        item_b->Add(CreateBoolean(false));
        item_b->Add(CreateInteger(0));

        auto item_c = CreateArray();
        item_c->Add(CreateBoolean(true));
        item_c->Add(CreateBoolean(false));
        item_c->Add(CreateInteger(1));

        EXPECT_EQ(item_b->GetHashCode(), item_a->GetHashCode());
        EXPECT_NE(item_c->GetHashCode(), item_a->GetHashCode());
    }

    // Test Struct hash codes
    {
        auto item_a = CreateStruct();
        item_a->Add(CreateBoolean(true));
        item_a->Add(CreateBoolean(false));
        item_a->Add(CreateInteger(0));

        auto item_b = CreateStruct();
        item_b->Add(CreateBoolean(true));
        item_b->Add(CreateBoolean(false));
        item_b->Add(CreateInteger(0));

        auto item_c = CreateStruct();
        item_c->Add(CreateBoolean(true));
        item_c->Add(CreateBoolean(false));
        item_c->Add(CreateInteger(1));

        EXPECT_EQ(item_b->GetHashCode(), item_a->GetHashCode());
        EXPECT_NE(item_c->GetHashCode(), item_a->GetHashCode());
    }

    // Test Map hash codes
    {
        auto item_a = CreateMap();
        item_a->Put(CreateBoolean(true), CreateBoolean(false));
        item_a->Put(CreateInteger(0), CreateInteger(1));

        auto item_b = CreateMap();
        item_b->Put(CreateBoolean(true), CreateBoolean(false));
        item_b->Put(CreateInteger(0), CreateInteger(1));

        auto item_c = CreateMap();
        item_c->Put(CreateBoolean(true), CreateBoolean(false));
        item_c->Put(CreateInteger(0), CreateInteger(2));

        EXPECT_EQ(item_b->GetHashCode(), item_a->GetHashCode());
        EXPECT_NE(item_c->GetHashCode(), item_a->GetHashCode());
    }

    // Test CompoundType GetHashCode for subitems
    {
        auto junk = CreateArray();
        junk->Add(CreateBoolean(true));
        junk->Add(CreateBoolean(false));
        junk->Add(CreateInteger(0));

        auto item_a = CreateMap();
        item_a->Put(CreateBoolean(true), junk);
        item_a->Put(CreateInteger(0), junk);

        auto item_b = CreateMap();
        item_b->Put(CreateBoolean(true), junk);
        item_b->Put(CreateInteger(0), junk);

        auto item_c = CreateMap();
        item_c->Put(CreateBoolean(true), junk);
        item_c->Put(CreateInteger(0), CreateInteger(2));

        EXPECT_EQ(item_b->GetHashCode(), item_a->GetHashCode());
        EXPECT_NE(item_c->GetHashCode(), item_a->GetHashCode());
    }

    // Test InteropInterface hash codes
    {
        auto item_a = CreateInteropInterface(123);
        auto item_b = CreateInteropInterface(123);
        auto item_c = CreateInteropInterface(124);

        EXPECT_EQ(item_b->GetHashCode(), item_a->GetHashCode());
        EXPECT_NE(item_c->GetHashCode(), item_a->GetHashCode());
    }

    // Test Pointer hash codes
    {
        auto script = std::make_shared<Script>(std::vector<uint8_t>());
        auto item_a = CreatePointer(script, 123);
        auto item_b = CreatePointer(script, 123);
        auto item_c = CreatePointer(script, 1234);

        EXPECT_EQ(item_b->GetHashCode(), item_a->GetHashCode());
        EXPECT_NE(item_c->GetHashCode(), item_a->GetHashCode());
    }
}

// C# Test Method: TestNull()
TEST_F(StackItemAllMethodsTest, TestNull)
{
    // Empty byte array should not equal null
    auto null_item = CreateByteString(std::vector<uint8_t>());
    EXPECT_NE(StackItem::Null(), null_item);

    // Actual Null item should equal static Null
    auto actual_null = CreateNull();
    EXPECT_EQ(StackItem::Null()->GetHashCode(), actual_null->GetHashCode());
    EXPECT_TRUE(actual_null->Equals(*StackItem::Null()));
}

// C# Test Method: TestEqual()
TEST_F(StackItemAllMethodsTest, TestEqual)
{
    auto item_a = CreateByteString("NEO");
    auto item_b = CreateByteString("NEO");
    auto item_c = CreateByteString("SmartEconomy");
    auto item_d = CreateByteString("Smarteconomy");
    auto item_e = CreateByteString("smarteconomy");

    EXPECT_TRUE(item_a->Equals(*item_b));
    EXPECT_FALSE(item_a->Equals(*item_c));
    EXPECT_FALSE(item_c->Equals(*item_d));
    EXPECT_FALSE(item_d->Equals(*item_e));

    // Test equality with different object type (should return false)
    EXPECT_FALSE(item_a->Equals(*CreateInteger(42)));
}

// C# Test Method: TestCast()
TEST_F(StackItemAllMethodsTest, TestCast)
{
    // Test signed byte
    {
        auto item = CreateInteger(std::numeric_limits<int8_t>::max());
        EXPECT_TRUE(item->IsInteger());
        EXPECT_EQ(BigInteger(std::numeric_limits<int8_t>::max()),
                  std::dynamic_pointer_cast<Integer>(item)->GetBigInteger());
    }

    // Test unsigned byte
    {
        auto item = CreateInteger(std::numeric_limits<uint8_t>::max());
        EXPECT_TRUE(item->IsInteger());
        EXPECT_EQ(BigInteger(std::numeric_limits<uint8_t>::max()),
                  std::dynamic_pointer_cast<Integer>(item)->GetBigInteger());
    }

    // Test signed short
    {
        auto item = CreateInteger(std::numeric_limits<int16_t>::max());
        EXPECT_TRUE(item->IsInteger());
        EXPECT_EQ(BigInteger(std::numeric_limits<int16_t>::max()),
                  std::dynamic_pointer_cast<Integer>(item)->GetBigInteger());
    }

    // Test unsigned short
    {
        auto item = CreateInteger(std::numeric_limits<uint16_t>::max());
        EXPECT_TRUE(item->IsInteger());
        EXPECT_EQ(BigInteger(std::numeric_limits<uint16_t>::max()),
                  std::dynamic_pointer_cast<Integer>(item)->GetBigInteger());
    }

    // Test signed integer
    {
        auto item = CreateInteger(std::numeric_limits<int32_t>::max());
        EXPECT_TRUE(item->IsInteger());
        EXPECT_EQ(BigInteger(std::numeric_limits<int32_t>::max()),
                  std::dynamic_pointer_cast<Integer>(item)->GetBigInteger());
    }

    // Test unsigned integer
    {
        auto item = CreateInteger(std::numeric_limits<uint32_t>::max());
        EXPECT_TRUE(item->IsInteger());
        EXPECT_EQ(BigInteger(std::numeric_limits<uint32_t>::max()),
                  std::dynamic_pointer_cast<Integer>(item)->GetBigInteger());
    }

    // Test signed long
    {
        auto item = CreateInteger(std::numeric_limits<int64_t>::max());
        EXPECT_TRUE(item->IsInteger());
        EXPECT_EQ(BigInteger(std::numeric_limits<int64_t>::max()),
                  std::dynamic_pointer_cast<Integer>(item)->GetBigInteger());
    }

    // Test unsigned long
    {
        auto item = CreateInteger(BigInteger::Parse(std::to_string(std::numeric_limits<uint64_t>::max())));
        EXPECT_TRUE(item->IsInteger());
        EXPECT_EQ(BigInteger::Parse(std::to_string(std::numeric_limits<uint64_t>::max())),
                  std::dynamic_pointer_cast<Integer>(item)->GetBigInteger());
    }

    // Test BigInteger
    {
        auto item = CreateInteger(BigInteger(-1));
        EXPECT_TRUE(item->IsInteger());
        EXPECT_EQ(BigInteger(-1), std::dynamic_pointer_cast<Integer>(item)->GetBigInteger());
    }

    // Test Boolean
    {
        auto item = CreateBoolean(true);
        EXPECT_TRUE(item->IsBoolean());
        EXPECT_TRUE(item->GetBoolean());
    }

    // Test ByteString
    {
        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
        auto item = CreateByteString(data);
        EXPECT_TRUE(item->IsByteString());

        auto span = item->GetSpan();
        std::vector<uint8_t> result(span.begin(), span.end());
        EXPECT_EQ(data, result);
    }
}

// C# Test Method: TestDeepCopy()
TEST_F(StackItemAllMethodsTest, TestDeepCopy)
{
    auto a = CreateArray();

    // Add various types of items
    a->Add(CreateBoolean(true));
    a->Add(CreateInteger(1));
    a->Add(CreateByteString({1}));
    a->Add(StackItem::Null());
    a->Add(CreateBuffer({1}));

    // Add a map
    auto map = CreateMap();
    map->Put(CreateInteger(0), CreateInteger(1));
    map->Put(CreateInteger(2), CreateInteger(3));
    a->Add(map);

    // Add a struct
    auto struct_item = CreateStruct();
    struct_item->Add(CreateInteger(1));
    struct_item->Add(CreateInteger(2));
    struct_item->Add(CreateInteger(3));
    a->Add(struct_item);

    // Add self-reference (circular reference)
    a->Add(a);

    // Create deep copy
    auto aa = std::dynamic_pointer_cast<Array>(a->DeepCopy());
    ASSERT_NE(nullptr, aa);

    // Verify it's a different object
    EXPECT_NE(a.get(), aa.get());
    EXPECT_FALSE(a->Equals(*aa));  // Different objects

    // Verify self-reference is preserved in copy
    auto last_item = (*aa)[aa->Count() - 1];
    EXPECT_EQ(aa.get(), last_item.get());  // Should point to aa itself

    // Verify map was deep copied (different object but equal content)
    auto original_map = (*a)[5];
    auto copied_map = (*aa)[5];
    EXPECT_NE(original_map.get(), copied_map.get());  // Different objects

    ExecutionEngineLimits limits;
    EXPECT_TRUE(original_map->Equals(*copied_map, limits));  // But equal content
}

// Additional comprehensive tests for complete coverage

// Test Method: TestStackItemTypes()
TEST_F(StackItemAllMethodsTest, TestStackItemTypes)
{
    // Test all stack item types
    auto integer = CreateInteger(42);
    auto boolean = CreateBoolean(true);
    auto byte_string = CreateByteString("test");
    auto buffer = CreateBuffer(10);
    auto null_item = CreateNull();
    auto array = CreateArray();
    auto struct_item = CreateStruct();
    auto map = CreateMap();

    EXPECT_TRUE(integer->IsInteger());
    EXPECT_FALSE(integer->IsBoolean());
    EXPECT_FALSE(integer->IsByteString());

    EXPECT_TRUE(boolean->IsBoolean());
    EXPECT_FALSE(boolean->IsInteger());

    EXPECT_TRUE(byte_string->IsByteString());
    EXPECT_FALSE(byte_string->IsBuffer());

    EXPECT_TRUE(buffer->IsBuffer());
    EXPECT_FALSE(buffer->IsByteString());

    EXPECT_TRUE(null_item->IsNull());
    EXPECT_FALSE(null_item->IsInteger());

    EXPECT_TRUE(array->IsArray());
    EXPECT_FALSE(array->IsStruct());

    EXPECT_TRUE(struct_item->IsStruct());
    EXPECT_FALSE(struct_item->IsArray());

    EXPECT_TRUE(map->IsMap());
    EXPECT_FALSE(map->IsArray());
}

// Test Method: TestStackItemConversions()
TEST_F(StackItemAllMethodsTest, TestStackItemConversions)
{
    // Test integer conversions
    auto integer = CreateInteger(123);
    EXPECT_EQ(123, integer->GetBigInteger().ToInt32());
    EXPECT_TRUE(integer->GetBoolean());  // Non-zero is true

    // Test boolean conversions
    auto bool_true = CreateBoolean(true);
    auto bool_false = CreateBoolean(false);
    EXPECT_EQ(1, bool_true->GetBigInteger().ToInt32());
    EXPECT_EQ(0, bool_false->GetBigInteger().ToInt32());
    EXPECT_TRUE(bool_true->GetBoolean());
    EXPECT_FALSE(bool_false->GetBoolean());

    // Test byte string conversions
    auto byte_string = CreateByteString("test");
    EXPECT_TRUE(byte_string->GetBoolean());  // Non-empty is true

    auto empty_string = CreateByteString("");
    EXPECT_FALSE(empty_string->GetBoolean());  // Empty is false
}

// Test Method: TestStackItemSerialization()
TEST_F(StackItemAllMethodsTest, TestStackItemSerialization)
{
    // Test serialization of various types
    auto integer = CreateInteger(42);
    auto serialized_int = integer->ToString();
    EXPECT_NE("", serialized_int);

    auto boolean = CreateBoolean(true);
    auto serialized_bool = boolean->ToString();
    EXPECT_NE("", serialized_bool);

    auto byte_string = CreateByteString("hello");
    auto serialized_string = byte_string->ToString();
    EXPECT_NE("", serialized_string);

    // Test complex types
    auto array = CreateArray();
    array->Add(CreateInteger(1));
    array->Add(CreateBoolean(true));
    auto serialized_array = array->ToString();
    EXPECT_NE("", serialized_array);
    EXPECT_NE(std::string::npos, serialized_array.find("Array"));
}

// Test Method: TestStackItemLimits()
TEST_F(StackItemAllMethodsTest, TestStackItemLimits)
{
    ExecutionEngineLimits limits;

    // Test large array
    auto large_array = CreateArray();
    for (int i = 0; i < 1000; i++)
    {
        large_array->Add(CreateInteger(i));
    }

    EXPECT_EQ(1000, large_array->Count());

    // Test deep nesting
    auto nested_array = CreateArray();
    auto current = nested_array;
    for (int i = 0; i < 10; i++)
    {
        auto inner = CreateArray();
        current->Add(inner);
        current = inner;
    }

    // Should be able to create deep structures
    EXPECT_GT(nested_array->Count(), 0);
}