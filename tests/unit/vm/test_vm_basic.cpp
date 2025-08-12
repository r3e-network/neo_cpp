#include <gtest/gtest.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/stack_item_types.h>

namespace neo {
namespace vm {
namespace tests {

class VMBasicTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup if needed
    }
    
    void TearDown() override {
        // Test cleanup if needed
    }
};

// Test primitive stack items
TEST_F(VMBasicTest, CreateBooleanItem) {
    auto true_item = std::make_shared<BooleanItem>(true);
    EXPECT_EQ(true_item->GetType(), StackItemType::Boolean);
    EXPECT_TRUE(true_item->GetBoolean());
    
    auto false_item = std::make_shared<BooleanItem>(false);
    EXPECT_EQ(false_item->GetType(), StackItemType::Boolean);
    EXPECT_FALSE(false_item->GetBoolean());
}

TEST_F(VMBasicTest, CreateIntegerItem) {
    auto int_item = std::make_shared<IntegerItem>(42);
    EXPECT_EQ(int_item->GetType(), StackItemType::Integer);
    EXPECT_EQ(int_item->GetInteger(), 42);
    
    auto negative_item = std::make_shared<IntegerItem>(-100);
    EXPECT_EQ(negative_item->GetType(), StackItemType::Integer);
    EXPECT_EQ(negative_item->GetInteger(), -100);
}

TEST_F(VMBasicTest, CreateByteStringItem) {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    auto byte_item = std::make_shared<ByteStringItem>(data);
    EXPECT_EQ(byte_item->GetType(), StackItemType::ByteString);
    EXPECT_EQ(byte_item->GetByteArray().size(), 4);
}

// Test compound stack items
TEST_F(VMBasicTest, CreateArrayItem) {
    auto array = std::make_shared<ArrayItem>();
    EXPECT_EQ(array->GetType(), StackItemType::Array);
    EXPECT_EQ(array->Count(), 0);
    
    // Add items to array
    array->Add(std::make_shared<IntegerItem>(1));
    array->Add(std::make_shared<IntegerItem>(2));
    array->Add(std::make_shared<IntegerItem>(3));
    
    EXPECT_EQ(array->Count(), 3);
}

TEST_F(VMBasicTest, CreateStructItem) {
    auto struct_item = std::make_shared<StructItem>();
    EXPECT_EQ(struct_item->GetType(), StackItemType::Struct);
    EXPECT_EQ(struct_item->Count(), 0);
    
    // Struct can hold different types
    struct_item->Add(std::make_shared<IntegerItem>(100));
    struct_item->Add(std::make_shared<BooleanItem>(true));
    
    EXPECT_EQ(struct_item->Count(), 2);
}

TEST_F(VMBasicTest, CreateMapItem) {
    auto map = std::make_shared<MapItem>();
    EXPECT_EQ(map->GetType(), StackItemType::Map);
    EXPECT_EQ(map->Count(), 0);
    
    // Add key-value pairs
    auto key1 = std::make_shared<ByteStringItem>(std::vector<uint8_t>{0x01});
    auto value1 = std::make_shared<IntegerItem>(100);
    map->Set(key1, value1);
    
    EXPECT_EQ(map->Count(), 1);
    EXPECT_TRUE(map->ContainsKey(key1));
}

// Test stack item conversions
TEST_F(VMBasicTest, ConvertToBoolean) {
    auto zero = std::make_shared<IntegerItem>(0);
    EXPECT_FALSE(zero->GetBoolean());
    
    auto non_zero = std::make_shared<IntegerItem>(1);
    EXPECT_TRUE(non_zero->GetBoolean());
    
    auto empty_bytes = std::make_shared<ByteStringItem>(std::vector<uint8_t>{});
    EXPECT_FALSE(empty_bytes->GetBoolean());
    
    auto non_empty_bytes = std::make_shared<ByteStringItem>(std::vector<uint8_t>{0x01});
    EXPECT_TRUE(non_empty_bytes->GetBoolean());
}

TEST_F(VMBasicTest, ConvertToInteger) {
    auto bool_true = std::make_shared<BooleanItem>(true);
    EXPECT_EQ(bool_true->GetInteger(), 1);
    
    auto bool_false = std::make_shared<BooleanItem>(false);
    EXPECT_EQ(bool_false->GetInteger(), 0);
    
    std::vector<uint8_t> bytes = {0x0A}; // 10 in little-endian
    auto byte_string = std::make_shared<ByteStringItem>(bytes);
    EXPECT_EQ(byte_string->GetInteger(), 10);
}

// Test stack item equality
TEST_F(VMBasicTest, StackItemEquality) {
    auto int1 = std::make_shared<IntegerItem>(42);
    auto int2 = std::make_shared<IntegerItem>(42);
    auto int3 = std::make_shared<IntegerItem>(43);
    
    EXPECT_TRUE(int1->Equals(*int2));
    EXPECT_FALSE(int1->Equals(*int3));
    
    auto bool1 = std::make_shared<BooleanItem>(true);
    auto bool2 = std::make_shared<BooleanItem>(true);
    auto bool3 = std::make_shared<BooleanItem>(false);
    
    EXPECT_TRUE(bool1->Equals(*bool2));
    EXPECT_FALSE(bool1->Equals(*bool3));
}

// Test reference counting
TEST_F(VMBasicTest, ReferenceCountingBasic) {
    auto item = std::make_shared<IntegerItem>(42);
    EXPECT_EQ(item.use_count(), 1);
    
    auto ref = item;
    EXPECT_EQ(item.use_count(), 2);
    
    ref.reset();
    EXPECT_EQ(item.use_count(), 1);
}

} // namespace tests
} // namespace vm
} // namespace neo