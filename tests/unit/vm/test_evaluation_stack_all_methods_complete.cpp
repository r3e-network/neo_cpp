#include "neo/extensions/utility.h"
#include "neo/vm/evaluation_stack.h"
#include "neo/vm/reference_counter.h"
#include "neo/vm/stack_item.h"
#include "neo/vm/types/boolean.h"
#include "neo/vm/types/byte_string.h"
#include "neo/vm/types/integer.h"
#include <algorithm>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

using namespace neo;
using namespace neo::vm;
using namespace neo::vm::types;

// Complete conversion of C# UT_EvaluationStack.cs - ALL 9 test methods
class EvaluationStackAllMethodsTest : public ::testing::Test
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

    // C++ equivalent of C# CreateOrderedStack helper method
    std::unique_ptr<EvaluationStack> CreateOrderedStack(int count)
    {
        std::vector<std::shared_ptr<Integer>> check(count);
        auto stack = std::make_unique<EvaluationStack>(reference_counter_);

        for (int x = 1; x <= count; x++)
        {
            auto integer_item = std::make_shared<Integer>(x, reference_counter_.get());
            stack->Push(integer_item);
            check[x - 1] = integer_item;
        }

        EXPECT_EQ(count, stack->Count());

        // Verify stack order matches expected
        auto stack_array = stack->ToArray();
        EXPECT_EQ(check.size(), stack_array.size());
        for (size_t i = 0; i < check.size(); i++)
        {
            auto expected_value = check[i]->GetBigInteger();
            auto actual_value = std::dynamic_pointer_cast<Integer>(stack_array[i])->GetBigInteger();
            EXPECT_EQ(expected_value, actual_value);
        }

        return stack;
    }

    // Helper to convert array to vector for comparison
    template <typename T>
    std::vector<T> ToVector(const std::vector<std::shared_ptr<StackItem>>& items)
    {
        std::vector<T> result;
        for (const auto& item : items)
        {
            auto typed_item = std::dynamic_pointer_cast<typename T::element_type>(item);
            if (typed_item)
            {
                result.push_back(typed_item);
            }
        }
        return result;
    }

    // Helper to compare integer arrays
    void AssertIntegerArraysEqual(const std::vector<int>& expected,
                                  const std::vector<std::shared_ptr<StackItem>>& actual)
    {
        EXPECT_EQ(expected.size(), actual.size());
        for (size_t i = 0; i < expected.size(); i++)
        {
            auto integer_item = std::dynamic_pointer_cast<Integer>(actual[i]);
            ASSERT_NE(nullptr, integer_item);
            EXPECT_EQ(expected[i], integer_item->GetBigInteger().ToInt32());
        }
    }

    std::shared_ptr<ReferenceCounter> reference_counter_;
};

// C# Test Method: TestClear()
TEST_F(EvaluationStackAllMethodsTest, TestClear)
{
    auto stack = CreateOrderedStack(3);
    stack->Clear();
    EXPECT_EQ(0, stack->Count());
}

// C# Test Method: TestCopyTo()
TEST_F(EvaluationStackAllMethodsTest, TestCopyTo)
{
    auto stack = CreateOrderedStack(3);
    auto copy = std::make_unique<EvaluationStack>(reference_counter_);

    // Test invalid arguments
    EXPECT_THROW(stack->CopyTo(*copy, -2), std::out_of_range);
    EXPECT_THROW(stack->CopyTo(*copy, 4), std::out_of_range);

    // Copy 0 items
    stack->CopyTo(*copy, 0);

    EXPECT_EQ(3, stack->Count());
    EXPECT_EQ(0, copy->Count());
    AssertIntegerArraysEqual({1, 2, 3}, stack->ToArray());

    // Copy all items (-1 means all)
    stack->CopyTo(*copy, -1);

    EXPECT_EQ(3, stack->Count());
    EXPECT_EQ(3, copy->Count());
    AssertIntegerArraysEqual({1, 2, 3}, stack->ToArray());
    AssertIntegerArraysEqual({1, 2, 3}, copy->ToArray());

    // Test enumerable interface
    auto enumerable_items = copy->ToArray();
    AssertIntegerArraysEqual({1, 2, 3}, enumerable_items);

    // Copy 2 items from copy to stack
    copy->CopyTo(*stack, 2);

    EXPECT_EQ(5, stack->Count());
    EXPECT_EQ(3, copy->Count());

    AssertIntegerArraysEqual({1, 2, 3, 2, 3}, stack->ToArray());
    AssertIntegerArraysEqual({1, 2, 3}, copy->ToArray());
}

// C# Test Method: TestMoveTo()
TEST_F(EvaluationStackAllMethodsTest, TestMoveTo)
{
    auto stack = CreateOrderedStack(3);
    auto other = std::make_unique<EvaluationStack>(reference_counter_);

    // Move 0 items
    stack->MoveTo(*other, 0);

    EXPECT_EQ(3, stack->Count());
    EXPECT_EQ(0, other->Count());
    AssertIntegerArraysEqual({1, 2, 3}, stack->ToArray());

    // Move all items (-1 means all)
    stack->MoveTo(*other, -1);

    EXPECT_EQ(0, stack->Count());
    EXPECT_EQ(3, other->Count());
    AssertIntegerArraysEqual({1, 2, 3}, other->ToArray());

    // Test enumerable interface
    auto enumerable_items = other->ToArray();
    AssertIntegerArraysEqual({1, 2, 3}, enumerable_items);

    // Move 2 items from other to stack
    other->MoveTo(*stack, 2);

    EXPECT_EQ(2, stack->Count());
    EXPECT_EQ(1, other->Count());

    AssertIntegerArraysEqual({2, 3}, stack->ToArray());
    AssertIntegerArraysEqual({1}, other->ToArray());
}

// C# Test Method: TestInsertPeek()
TEST_F(EvaluationStackAllMethodsTest, TestInsertPeek)
{
    auto stack = std::make_unique<EvaluationStack>(reference_counter_);

    // Insert items at specific positions
    auto item3 = std::make_shared<Integer>(3, reference_counter_.get());
    auto item1 = std::make_shared<Integer>(1, reference_counter_.get());
    auto item2 = std::make_shared<Integer>(2, reference_counter_.get());

    stack->Insert(0, item3);
    stack->Insert(1, item1);
    stack->Insert(1, item2);

    // Test invalid insert position
    auto item_invalid = std::make_shared<Integer>(2, reference_counter_.get());
    EXPECT_THROW(stack->Insert(4, item_invalid), std::invalid_argument);

    EXPECT_EQ(3, stack->Count());
    AssertIntegerArraysEqual({1, 2, 3}, stack->ToArray());

    // Test peek operations
    auto peek0 = std::dynamic_pointer_cast<Integer>(stack->Peek(0));
    EXPECT_EQ(3, peek0->GetBigInteger().ToInt32());

    auto peek1 = std::dynamic_pointer_cast<Integer>(stack->Peek(1));
    EXPECT_EQ(2, peek1->GetBigInteger().ToInt32());

    auto peek_neg1 = std::dynamic_pointer_cast<Integer>(stack->Peek(-1));
    EXPECT_EQ(1, peek_neg1->GetBigInteger().ToInt32());

    // Test invalid peek position
    EXPECT_THROW(stack->Peek(-4), std::invalid_argument);
}

// C# Test Method: TestPopPush()
TEST_F(EvaluationStackAllMethodsTest, TestPopPush)
{
    auto stack = CreateOrderedStack(3);

    // Test basic pop operations
    auto pop1 = std::dynamic_pointer_cast<Integer>(stack->Pop());
    EXPECT_EQ(3, pop1->GetBigInteger().ToInt32());

    auto pop2 = std::dynamic_pointer_cast<Integer>(stack->Pop());
    EXPECT_EQ(2, pop2->GetBigInteger().ToInt32());

    auto pop3 = std::dynamic_pointer_cast<Integer>(stack->Pop());
    EXPECT_EQ(1, pop3->GetBigInteger().ToInt32());

    // Test pop from empty stack
    EXPECT_THROW(stack->Pop(), std::out_of_range);

    // Test typed pop operations
    stack = CreateOrderedStack(3);

    auto typed_pop1 = stack->Pop<Integer>();
    EXPECT_EQ(3, typed_pop1->GetBigInteger().ToInt32());

    auto typed_pop2 = stack->Pop<Integer>();
    EXPECT_EQ(2, typed_pop2->GetBigInteger().ToInt32());

    auto typed_pop3 = stack->Pop<Integer>();
    EXPECT_EQ(1, typed_pop3->GetBigInteger().ToInt32());

    // Test typed pop from empty stack
    EXPECT_THROW(stack->Pop<Integer>(), std::out_of_range);
}

// C# Test Method: TestRemove()
TEST_F(EvaluationStackAllMethodsTest, TestRemove)
{
    auto stack = CreateOrderedStack(3);

    // Test remove operations
    auto remove1 = stack->Remove<Integer>(0);
    EXPECT_EQ(3, remove1->GetBigInteger().ToInt32());

    auto remove2 = stack->Remove<Integer>(0);
    EXPECT_EQ(2, remove2->GetBigInteger().ToInt32());

    auto remove3 = stack->Remove<Integer>(-1);
    EXPECT_EQ(1, remove3->GetBigInteger().ToInt32());

    // Test remove from empty stack
    EXPECT_THROW(stack->Remove<Integer>(0), std::out_of_range);
    EXPECT_THROW(stack->Remove<Integer>(-1), std::out_of_range);
}

// C# Test Method: TestReverse()
TEST_F(EvaluationStackAllMethodsTest, TestReverse)
{
    auto stack = CreateOrderedStack(3);

    // Reverse entire stack
    stack->Reverse(3);

    auto rev_pop1 = stack->Pop<Integer>();
    EXPECT_EQ(1, rev_pop1->GetBigInteger().ToInt32());

    auto rev_pop2 = stack->Pop<Integer>();
    EXPECT_EQ(2, rev_pop2->GetBigInteger().ToInt32());

    auto rev_pop3 = stack->Pop<Integer>();
    EXPECT_EQ(3, rev_pop3->GetBigInteger().ToInt32());

    EXPECT_THROW(stack->Pop<Integer>(), std::out_of_range);

    // Test reverse with invalid arguments
    stack = CreateOrderedStack(3);

    EXPECT_THROW(stack->Reverse(-1), std::out_of_range);
    EXPECT_THROW(stack->Reverse(4), std::out_of_range);

    // Reverse only 1 item (should not change order)
    stack->Reverse(1);

    auto norm_pop1 = stack->Pop<Integer>();
    EXPECT_EQ(3, norm_pop1->GetBigInteger().ToInt32());

    auto norm_pop2 = stack->Pop<Integer>();
    EXPECT_EQ(2, norm_pop2->GetBigInteger().ToInt32());

    auto norm_pop3 = stack->Pop<Integer>();
    EXPECT_EQ(1, norm_pop3->GetBigInteger().ToInt32());

    EXPECT_THROW(stack->Pop<Integer>(), std::out_of_range);
}

// C# Test Method: TestEvaluationStackPrint()
TEST_F(EvaluationStackAllMethodsTest, TestEvaluationStackPrint)
{
    auto stack = std::make_unique<EvaluationStack>(reference_counter_);

    auto item3 = std::make_shared<Integer>(3, reference_counter_.get());
    auto item1 = std::make_shared<Integer>(1, reference_counter_.get());
    auto item_test = std::make_shared<ByteString>("test", reference_counter_.get());
    auto item_true = std::make_shared<Boolean>(true, reference_counter_.get());

    stack->Insert(0, item3);
    stack->Insert(1, item1);
    stack->Insert(2, item_test);
    stack->Insert(3, item_true);

    std::string expected = "[Boolean(True), ByteString(\"test\"), Integer(1), Integer(3)]";
    EXPECT_EQ(expected, stack->ToString());
}

// C# Test Method: TestPrintInvalidUTF8()
TEST_F(EvaluationStackAllMethodsTest, TestPrintInvalidUTF8)
{
    auto stack = std::make_unique<EvaluationStack>(reference_counter_);

    // Create ByteString with invalid UTF-8 bytes from hex
    auto hex_data = Utility::FromHexString("4CC95219999D421243C8161E3FC0F4290C067845");
    auto invalid_utf8_item = std::make_shared<ByteString>(hex_data, reference_counter_.get());

    stack->Insert(0, invalid_utf8_item);

    std::string expected = "[ByteString(\"Base64: TMlSGZmdQhJDyBYeP8D0KQwGeEU=\")]";
    EXPECT_EQ(expected, stack->ToString());
}

// Additional comprehensive tests for complete coverage

// Test Method: TestStackCapacity()
TEST_F(EvaluationStackAllMethodsTest, TestStackCapacity)
{
    auto stack = std::make_unique<EvaluationStack>(reference_counter_);

    // Test stack growth
    const int large_count = 1000;
    for (int i = 0; i < large_count; i++)
    {
        auto item = std::make_shared<Integer>(i, reference_counter_.get());
        stack->Push(item);
    }

    EXPECT_EQ(large_count, stack->Count());

    // Verify items are in correct order
    for (int i = large_count - 1; i >= 0; i--)
    {
        auto popped = stack->Pop<Integer>();
        EXPECT_EQ(i, popped->GetBigInteger().ToInt32());
    }

    EXPECT_EQ(0, stack->Count());
}

// Test Method: TestStackWithDifferentTypes()
TEST_F(EvaluationStackAllMethodsTest, TestStackWithDifferentTypes)
{
    auto stack = std::make_unique<EvaluationStack>(reference_counter_);

    // Push different types
    auto int_item = std::make_shared<Integer>(42, reference_counter_.get());
    auto bool_item = std::make_shared<Boolean>(true, reference_counter_.get());
    auto string_item = std::make_shared<ByteString>("hello", reference_counter_.get());

    stack->Push(int_item);
    stack->Push(bool_item);
    stack->Push(string_item);

    EXPECT_EQ(3, stack->Count());

    // Pop and verify types
    auto popped_string = std::dynamic_pointer_cast<ByteString>(stack->Pop());
    EXPECT_NE(nullptr, popped_string);
    EXPECT_EQ("hello", popped_string->GetString());

    auto popped_bool = std::dynamic_pointer_cast<Boolean>(stack->Pop());
    EXPECT_NE(nullptr, popped_bool);
    EXPECT_TRUE(popped_bool->GetBoolean());

    auto popped_int = std::dynamic_pointer_cast<Integer>(stack->Pop());
    EXPECT_NE(nullptr, popped_int);
    EXPECT_EQ(42, popped_int->GetBigInteger().ToInt32());
}

// Test Method: TestStackIterator()
TEST_F(EvaluationStackAllMethodsTest, TestStackIterator)
{
    auto stack = CreateOrderedStack(5);

    // Test forward iteration
    auto items = stack->ToArray();
    EXPECT_EQ(5, items.size());

    for (size_t i = 0; i < items.size(); i++)
    {
        auto integer_item = std::dynamic_pointer_cast<Integer>(items[i]);
        EXPECT_NE(nullptr, integer_item);
        EXPECT_EQ(static_cast<int>(i + 1), integer_item->GetBigInteger().ToInt32());
    }
}

// Test Method: TestStackReferenceManagement()
TEST_F(EvaluationStackAllMethodsTest, TestStackReferenceManagement)
{
    auto stack = std::make_unique<EvaluationStack>(reference_counter_);

    // Test that items are properly managed
    {
        auto item = std::make_shared<Integer>(123, reference_counter_.get());
        stack->Push(item);
        EXPECT_EQ(1, stack->Count());
    }  // item goes out of scope

    // Item should still be alive in stack
    EXPECT_EQ(1, stack->Count());
    auto retrieved = stack->Pop<Integer>();
    EXPECT_EQ(123, retrieved->GetBigInteger().ToInt32());

    EXPECT_EQ(0, stack->Count());
}

// Test Method: TestStackOperationsEdgeCases()
TEST_F(EvaluationStackAllMethodsTest, TestStackOperationsEdgeCases)
{
    auto stack = std::make_unique<EvaluationStack>(reference_counter_);

    // Test peek on empty stack
    EXPECT_THROW(stack->Peek(0), std::out_of_range);

    // Test insert at position 0 on empty stack
    auto item = std::make_shared<Integer>(1, reference_counter_.get());
    EXPECT_NO_THROW(stack->Insert(0, item));
    EXPECT_EQ(1, stack->Count());

    // Test clear on single item
    stack->Clear();
    EXPECT_EQ(0, stack->Count());

    // Test reverse on empty stack
    EXPECT_THROW(stack->Reverse(1), std::out_of_range);

    // Test reverse with 0 count
    stack->Push(item);
    EXPECT_THROW(stack->Reverse(0), std::out_of_range);
}