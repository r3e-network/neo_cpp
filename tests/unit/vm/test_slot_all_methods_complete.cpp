#include "neo/vm/reference_counter.h"
#include "neo/vm/slot.h"
#include "neo/vm/types/integer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace neo;
using namespace neo::vm;
using namespace neo::vm::types;

// Complete conversion of C# UT_Slot.cs - ALL 2 test methods
class SlotAllMethodsTest : public ::testing::Test
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

    // Helper method equivalent to C# CreateOrderedSlot
    std::shared_ptr<Slot> CreateOrderedSlot(int count)
    {
        std::vector<std::shared_ptr<StackItem>> items;

        for (int x = 1; x <= count; x++)
        {
            items.push_back(std::make_shared<Integer>(x, reference_counter_.get()));
        }

        auto slot = std::make_shared<Slot>(items, reference_counter_.get());

        EXPECT_EQ(count, slot->Count());

        // Verify the items are in the correct order
        auto slot_array = slot->ToArray();
        EXPECT_EQ(count, slot_array.size());

        for (int i = 0; i < count; i++)
        {
            auto integer_item = std::dynamic_pointer_cast<Integer>(slot_array[i]);
            EXPECT_NE(nullptr, integer_item);
            if (integer_item)
            {
                EXPECT_EQ(i + 1, integer_item->GetBigInteger().ToInt32());
            }
        }

        return slot;
    }

    // Helper method equivalent to C# GetEnumerable
    std::vector<std::shared_ptr<StackItem>> GetEnumerable(std::shared_ptr<Slot> slot)
    {
        std::vector<std::shared_ptr<StackItem>> result;

        // Simulate IEnumerator behavior
        auto enumerator = slot->GetEnumerator();
        while (enumerator->MoveNext())
        {
            result.push_back(enumerator->Current());
        }

        return result;
    }

    std::shared_ptr<ReferenceCounter> reference_counter_;
};

// C# Test Method: TestGet()
TEST_F(SlotAllMethodsTest, TestGet)
{
    auto slot = CreateOrderedSlot(3);

    // Test accessing valid indices
    {
        auto item0 = (*slot)[0];
        auto integer0 = std::dynamic_pointer_cast<Integer>(item0);
        ASSERT_NE(nullptr, integer0);
        EXPECT_EQ(1, integer0->GetBigInteger().ToInt32());
    }

    {
        auto item1 = (*slot)[1];
        auto integer1 = std::dynamic_pointer_cast<Integer>(item1);
        ASSERT_NE(nullptr, integer1);
        EXPECT_EQ(2, integer1->GetBigInteger().ToInt32());
    }

    {
        auto item2 = (*slot)[2];
        auto integer2 = std::dynamic_pointer_cast<Integer>(item2);
        ASSERT_NE(nullptr, integer2);
        EXPECT_EQ(3, integer2->GetBigInteger().ToInt32());
    }

    // Test accessing invalid index should throw
    EXPECT_THROW(auto item3 = (*slot)[3], std::out_of_range);
}

// C# Test Method: TestEnumerable()
TEST_F(SlotAllMethodsTest, TestEnumerable)
{
    auto slot = CreateOrderedSlot(3);

    // Test foreach equivalent using range-based for loop
    {
        int expected_value = 1;
        for (const auto& item : *slot)
        {
            auto integer_item = std::dynamic_pointer_cast<Integer>(item);
            ASSERT_NE(nullptr, integer_item);
            EXPECT_EQ(expected_value, integer_item->GetBigInteger().ToInt32());
            expected_value++;
        }
    }

    // Test ToArray method
    {
        auto slot_array = slot->ToArray();
        EXPECT_EQ(3, slot_array.size());

        for (int i = 0; i < 3; i++)
        {
            auto integer_item = std::dynamic_pointer_cast<Integer>(slot_array[i]);
            ASSERT_NE(nullptr, integer_item);
            EXPECT_EQ(i + 1, integer_item->GetBigInteger().ToInt32());
        }
    }

    // Test IEnumerable equivalent using GetEnumerable helper
    {
        auto enumerable_result = GetEnumerable(slot);
        EXPECT_EQ(3, enumerable_result.size());

        for (int i = 0; i < 3; i++)
        {
            auto integer_item = std::dynamic_pointer_cast<Integer>(enumerable_result[i]);
            ASSERT_NE(nullptr, integer_item);
            EXPECT_EQ(i + 1, integer_item->GetBigInteger().ToInt32());
        }
    }

    EXPECT_EQ(3, slot->Count());

    // Verify ToArray again
    {
        auto slot_array = slot->ToArray();
        EXPECT_EQ(3, slot_array.size());

        for (int i = 0; i < 3; i++)
        {
            auto integer_item = std::dynamic_pointer_cast<Integer>(slot_array[i]);
            ASSERT_NE(nullptr, integer_item);
            EXPECT_EQ(i + 1, integer_item->GetBigInteger().ToInt32());
        }
    }

    // Test empty slot
    auto empty_slot = CreateOrderedSlot(0);

    {
        auto empty_array = empty_slot->ToArray();
        EXPECT_EQ(0, empty_array.size());
        EXPECT_TRUE(empty_array.empty());
    }

    // Test IEnumerable with empty slot
    {
        auto empty_enumerable = GetEnumerable(empty_slot);
        EXPECT_EQ(0, empty_enumerable.size());
        EXPECT_TRUE(empty_enumerable.empty());
    }

    EXPECT_EQ(0, empty_slot->Count());

    // Verify empty ToArray again
    {
        auto empty_array = empty_slot->ToArray();
        EXPECT_EQ(0, empty_array.size());
        EXPECT_TRUE(empty_array.empty());
    }
}

// Additional comprehensive tests for complete coverage

// Test Method: TestSlotConstruction()
TEST_F(SlotAllMethodsTest, TestSlotConstruction)
{
    // Test constructing slot with various sizes
    for (int size : {0, 1, 5, 10, 100})
    {
        auto slot = CreateOrderedSlot(size);
        EXPECT_EQ(size, slot->Count());

        if (size > 0)
        {
            auto first_item = std::dynamic_pointer_cast<Integer>((*slot)[0]);
            ASSERT_NE(nullptr, first_item);
            EXPECT_EQ(1, first_item->GetBigInteger().ToInt32());

            auto last_item = std::dynamic_pointer_cast<Integer>((*slot)[size - 1]);
            ASSERT_NE(nullptr, last_item);
            EXPECT_EQ(size, last_item->GetBigInteger().ToInt32());
        }
    }
}

// Test Method: TestSlotModification()
TEST_F(SlotAllMethodsTest, TestSlotModification)
{
    auto slot = CreateOrderedSlot(3);

    // Test setting values
    (*slot)[0] = std::make_shared<Integer>(100, reference_counter_.get());
    (*slot)[1] = std::make_shared<Integer>(200, reference_counter_.get());
    (*slot)[2] = std::make_shared<Integer>(300, reference_counter_.get());

    // Verify the changes
    {
        auto item0 = std::dynamic_pointer_cast<Integer>((*slot)[0]);
        ASSERT_NE(nullptr, item0);
        EXPECT_EQ(100, item0->GetBigInteger().ToInt32());
    }

    {
        auto item1 = std::dynamic_pointer_cast<Integer>((*slot)[1]);
        ASSERT_NE(nullptr, item1);
        EXPECT_EQ(200, item1->GetBigInteger().ToInt32());
    }

    {
        auto item2 = std::dynamic_pointer_cast<Integer>((*slot)[2]);
        ASSERT_NE(nullptr, item2);
        EXPECT_EQ(300, item2->GetBigInteger().ToInt32());
    }
}

// Test Method: TestSlotBoundaryConditions()
TEST_F(SlotAllMethodsTest, TestSlotBoundaryConditions)
{
    auto slot = CreateOrderedSlot(5);

    // Test valid boundary access
    EXPECT_NO_THROW(auto item = (*slot)[0]);  // First element
    EXPECT_NO_THROW(auto item = (*slot)[4]);  // Last element

    // Test invalid boundary access
    EXPECT_THROW(auto item = (*slot)[-1], std::out_of_range);   // Negative index
    EXPECT_THROW(auto item = (*slot)[5], std::out_of_range);    // Beyond end
    EXPECT_THROW(auto item = (*slot)[100], std::out_of_range);  // Way beyond end
}

// Test Method: TestSlotIteratorStability()
TEST_F(SlotAllMethodsTest, TestSlotIteratorStability)
{
    auto slot = CreateOrderedSlot(5);

    // Test multiple iterations yield same results
    std::vector<int> iteration1_values;
    std::vector<int> iteration2_values;

    // First iteration
    for (const auto& item : *slot)
    {
        auto integer_item = std::dynamic_pointer_cast<Integer>(item);
        ASSERT_NE(nullptr, integer_item);
        iteration1_values.push_back(integer_item->GetBigInteger().ToInt32());
    }

    // Second iteration
    for (const auto& item : *slot)
    {
        auto integer_item = std::dynamic_pointer_cast<Integer>(item);
        ASSERT_NE(nullptr, integer_item);
        iteration2_values.push_back(integer_item->GetBigInteger().ToInt32());
    }

    EXPECT_EQ(iteration1_values, iteration2_values);
    EXPECT_EQ(std::vector<int>({1, 2, 3, 4, 5}), iteration1_values);
}

// Test Method: TestSlotWithMixedTypes()
TEST_F(SlotAllMethodsTest, TestSlotWithMixedTypes)
{
    std::vector<std::shared_ptr<StackItem>> mixed_items;
    mixed_items.push_back(std::make_shared<Integer>(42, reference_counter_.get()));
    mixed_items.push_back(StackItem::Null());
    mixed_items.push_back(std::make_shared<Integer>(84, reference_counter_.get()));

    auto slot = std::make_shared<Slot>(mixed_items, reference_counter_.get());

    EXPECT_EQ(3, slot->Count());

    // Test accessing mixed types
    {
        auto item0 = std::dynamic_pointer_cast<Integer>((*slot)[0]);
        ASSERT_NE(nullptr, item0);
        EXPECT_EQ(42, item0->GetBigInteger().ToInt32());
    }

    {
        auto item1 = (*slot)[1];
        EXPECT_TRUE(item1->IsNull());
    }

    {
        auto item2 = std::dynamic_pointer_cast<Integer>((*slot)[2]);
        ASSERT_NE(nullptr, item2);
        EXPECT_EQ(84, item2->GetBigInteger().ToInt32());
    }
}

// Test Method: TestSlotEnumeratorReset()
TEST_F(SlotAllMethodsTest, TestSlotEnumeratorReset)
{
    auto slot = CreateOrderedSlot(3);

    auto enumerator = slot->GetEnumerator();

    // First enumeration
    std::vector<int> first_pass;
    while (enumerator->MoveNext())
    {
        auto integer_item = std::dynamic_pointer_cast<Integer>(enumerator->Current());
        ASSERT_NE(nullptr, integer_item);
        first_pass.push_back(integer_item->GetBigInteger().ToInt32());
    }

    // Reset and enumerate again
    enumerator->Reset();
    std::vector<int> second_pass;
    while (enumerator->MoveNext())
    {
        auto integer_item = std::dynamic_pointer_cast<Integer>(enumerator->Current());
        ASSERT_NE(nullptr, integer_item);
        second_pass.push_back(integer_item->GetBigInteger().ToInt32());
    }

    EXPECT_EQ(first_pass, second_pass);
    EXPECT_EQ(std::vector<int>({1, 2, 3}), first_pass);
}

// Test Method: TestSlotMemoryManagement()
TEST_F(SlotAllMethodsTest, TestSlotMemoryManagement)
{
    // Test that slot properly manages reference counting
    auto initial_count = reference_counter_->Count();

    {
        auto slot = CreateOrderedSlot(10);
        EXPECT_GT(reference_counter_->Count(), initial_count);
    }  // slot goes out of scope

    // Reference counter should handle cleanup properly
    EXPECT_TRUE(reference_counter_->CheckZeroReferred());
}