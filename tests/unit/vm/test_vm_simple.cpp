#include <gtest/gtest.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/stack_item_types.h>

namespace neo {
namespace vm {
namespace tests {

class VMSimpleTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test basic boolean operations
TEST_F(VMSimpleTest, BooleanItemTrue) {
    auto item = std::make_shared<BooleanItem>(true);
    EXPECT_EQ(item->GetType(), StackItemType::Boolean);
    EXPECT_TRUE(item->GetBoolean());
    EXPECT_EQ(item->GetInteger(), 1);
}

TEST_F(VMSimpleTest, BooleanItemFalse) {
    auto item = std::make_shared<BooleanItem>(false);
    EXPECT_EQ(item->GetType(), StackItemType::Boolean);
    EXPECT_FALSE(item->GetBoolean());
    EXPECT_EQ(item->GetInteger(), 0);
}

// Test basic integer operations
TEST_F(VMSimpleTest, IntegerItemPositive) {
    auto item = std::make_shared<IntegerItem>(42);
    EXPECT_EQ(item->GetType(), StackItemType::Integer);
    EXPECT_EQ(item->GetInteger(), 42);
    EXPECT_TRUE(item->GetBoolean()); // Non-zero is true
}

TEST_F(VMSimpleTest, IntegerItemNegative) {
    auto item = std::make_shared<IntegerItem>(-100);
    EXPECT_EQ(item->GetType(), StackItemType::Integer);
    EXPECT_EQ(item->GetInteger(), -100);
    EXPECT_TRUE(item->GetBoolean()); // Non-zero is true
}

TEST_F(VMSimpleTest, IntegerItemZero) {
    auto item = std::make_shared<IntegerItem>(0);
    EXPECT_EQ(item->GetType(), StackItemType::Integer);
    EXPECT_EQ(item->GetInteger(), 0);
    EXPECT_FALSE(item->GetBoolean()); // Zero is false
}

// Test equality
TEST_F(VMSimpleTest, IntegerEquality) {
    auto item1 = std::make_shared<IntegerItem>(42);
    auto item2 = std::make_shared<IntegerItem>(42);
    auto item3 = std::make_shared<IntegerItem>(43);
    
    EXPECT_TRUE(item1->Equals(*item2));
    EXPECT_FALSE(item1->Equals(*item3));
}

TEST_F(VMSimpleTest, BooleanEquality) {
    auto item1 = std::make_shared<BooleanItem>(true);
    auto item2 = std::make_shared<BooleanItem>(true);
    auto item3 = std::make_shared<BooleanItem>(false);
    
    EXPECT_TRUE(item1->Equals(*item2));
    EXPECT_FALSE(item1->Equals(*item3));
}

// Test type conversions
TEST_F(VMSimpleTest, BooleanToInteger) {
    auto true_item = std::make_shared<BooleanItem>(true);
    auto false_item = std::make_shared<BooleanItem>(false);
    
    EXPECT_EQ(true_item->GetInteger(), 1);
    EXPECT_EQ(false_item->GetInteger(), 0);
}

TEST_F(VMSimpleTest, IntegerToBoolean) {
    auto zero = std::make_shared<IntegerItem>(0);
    auto positive = std::make_shared<IntegerItem>(10);
    auto negative = std::make_shared<IntegerItem>(-5);
    
    EXPECT_FALSE(zero->GetBoolean());
    EXPECT_TRUE(positive->GetBoolean());
    EXPECT_TRUE(negative->GetBoolean());
}

// Test shared pointer behavior
TEST_F(VMSimpleTest, SharedPointerReference) {
    auto item = std::make_shared<IntegerItem>(100);
    EXPECT_EQ(item.use_count(), 1);
    
    {
        auto copy = item;
        EXPECT_EQ(item.use_count(), 2);
        EXPECT_EQ(copy->GetInteger(), 100);
    }
    
    EXPECT_EQ(item.use_count(), 1);
    EXPECT_EQ(item->GetInteger(), 100);
}

} // namespace tests
} // namespace vm
} // namespace neo