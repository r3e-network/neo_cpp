#include "neo/vm/execution_engine_limits.h"
#include "neo/vm/reference_counter.h"
#include "neo/vm/types/byte_string.h"
#include "neo/vm/types/integer.h"
#include "neo/vm/types/struct.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string>

using namespace neo;
using namespace neo::vm;
using namespace neo::vm::types;

// Complete conversion of C# UT_Struct.cs - ALL 3 test methods
class StructAllMethodsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        reference_counter_ = std::make_shared<ReferenceCounter>();

        // Create deeply nested struct equivalent to C# constructor
        struct_ = std::make_shared<Struct>(reference_counter_.get());
        struct_->Add(std::make_shared<Integer>(1, reference_counter_.get()));

        for (int i = 0; i < 20000; i++)
        {
            auto new_struct = std::make_shared<Struct>(reference_counter_.get());
            new_struct->Add(struct_);
            struct_ = new_struct;
        }
    }

    void TearDown() override
    {
        struct_.reset();
        reference_counter_.reset();
    }

    // Helper to create struct items with reference counter
    std::shared_ptr<Struct> CreateStruct()
    {
        return std::make_shared<Struct>(reference_counter_.get());
    }

    std::shared_ptr<Integer> CreateInteger(int value)
    {
        return std::make_shared<Integer>(value, reference_counter_.get());
    }

    std::shared_ptr<ByteString> CreateByteString(const std::string& value)
    {
        return std::make_shared<ByteString>(value, reference_counter_.get());
    }

    std::shared_ptr<ReferenceCounter> reference_counter_;
    std::shared_ptr<Struct> struct_;  // Deeply nested struct for testing limits
};

// C# Test Method: TestClone()
TEST_F(StructAllMethodsTest, TestClone)
{
    // Create s1: Struct containing [1, Struct containing [2]]
    auto s1 = CreateStruct();
    s1->Add(CreateInteger(1));

    auto inner_struct = CreateStruct();
    inner_struct->Add(CreateInteger(2));
    s1->Add(inner_struct);

    // Clone s1 to s2
    auto limits = ExecutionEngineLimits::Default();
    auto s2 = std::dynamic_pointer_cast<Struct>(s1->Clone(limits));
    ASSERT_NE(nullptr, s2);

    // Modify s1[0] to 3
    (*s1)[0] = CreateInteger(3);

    // s2[0] should still be 1 (deep copy)
    auto s2_first = std::dynamic_pointer_cast<Integer>((*s2)[0]);
    ASSERT_NE(nullptr, s2_first);
    EXPECT_EQ(1, s2_first->GetBigInteger().ToInt32());

    // Modify s1[1][0] (inner struct) to 3
    auto s1_inner = std::dynamic_pointer_cast<Struct>((*s1)[1]);
    ASSERT_NE(nullptr, s1_inner);
    (*s1_inner)[0] = CreateInteger(3);

    // s2[1][0] should still be 2 (deep copy)
    auto s2_inner = std::dynamic_pointer_cast<Struct>((*s2)[1]);
    ASSERT_NE(nullptr, s2_inner);
    auto s2_inner_first = std::dynamic_pointer_cast<Integer>((*s2_inner)[0]);
    ASSERT_NE(nullptr, s2_inner_first);
    EXPECT_EQ(2, s2_inner_first->GetBigInteger().ToInt32());

    // Test that deeply nested struct throws InvalidOperationException on clone
    EXPECT_THROW(struct_->Clone(limits), std::invalid_argument);
}

// C# Test Method: TestEquals()
TEST_F(StructAllMethodsTest, TestEquals)
{
    auto limits = ExecutionEngineLimits::Default();

    // Create s1: Struct containing [1, Struct containing [2]]
    auto s1 = CreateStruct();
    s1->Add(CreateInteger(1));

    auto inner_struct1 = CreateStruct();
    inner_struct1->Add(CreateInteger(2));
    s1->Add(inner_struct1);

    // Create s2: Struct containing [1, Struct containing [2]] (same as s1)
    auto s2 = CreateStruct();
    s2->Add(CreateInteger(1));

    auto inner_struct2 = CreateStruct();
    inner_struct2->Add(CreateInteger(2));
    s2->Add(inner_struct2);

    // s1 should equal s2
    EXPECT_TRUE(s1->Equals(*s2, limits));

    // Create s3: Struct containing [1, Struct containing [3]] (different from s1)
    auto s3 = CreateStruct();
    s3->Add(CreateInteger(1));

    auto inner_struct3 = CreateStruct();
    inner_struct3->Add(CreateInteger(3));
    s3->Add(inner_struct3);

    // s1 should not equal s3
    EXPECT_FALSE(s1->Equals(*s3, limits));

    // Test that deeply nested struct throws InvalidOperationException on equals
    // Note: We can't clone the deeply nested struct, so we test with itself
    // In C#, this would throw because of the circular reference depth
    EXPECT_THROW(struct_->Equals(*struct_, limits), std::invalid_argument);
}

// C# Test Method: TestEqualsDos()
TEST_F(StructAllMethodsTest, TestEqualsDos)
{
    auto limits = ExecutionEngineLimits::Default();

    // Create a large string payload (65535 characters)
    std::string payload_str(65535, 'h');

    auto s1 = CreateStruct();
    auto s2 = CreateStruct();

    // Add 2 large strings to each struct
    for (int i = 0; i < 2; i++)
    {
        s1->Add(CreateByteString(payload_str));
        s2->Add(CreateByteString(payload_str));
    }

    // Should throw InvalidOperationException due to comparison limits
    EXPECT_THROW(s1->Equals(*s2, limits), std::invalid_argument);

    // Add 1000 more large strings to each struct
    for (int i = 0; i < 1000; i++)
    {
        s1->Add(CreateByteString(payload_str));
        s2->Add(CreateByteString(payload_str));
    }

    // Should still throw InvalidOperationException due to comparison limits
    EXPECT_THROW(s1->Equals(*s2, limits), std::invalid_argument);
}

// Additional comprehensive tests for complete coverage

// Test Method: TestStructBasicOperations()
TEST_F(StructAllMethodsTest, TestStructBasicOperations)
{
    auto struct_item = CreateStruct();

    // Test empty struct
    EXPECT_EQ(0, struct_item->Count());
    EXPECT_TRUE(struct_item->IsEmpty());

    // Add elements
    struct_item->Add(CreateInteger(1));
    struct_item->Add(CreateInteger(2));
    struct_item->Add(CreateInteger(3));

    EXPECT_EQ(3, struct_item->Count());
    EXPECT_FALSE(struct_item->IsEmpty());

    // Test element access
    auto first = std::dynamic_pointer_cast<Integer>((*struct_item)[0]);
    ASSERT_NE(nullptr, first);
    EXPECT_EQ(1, first->GetBigInteger().ToInt32());

    auto second = std::dynamic_pointer_cast<Integer>((*struct_item)[1]);
    ASSERT_NE(nullptr, second);
    EXPECT_EQ(2, second->GetBigInteger().ToInt32());

    auto third = std::dynamic_pointer_cast<Integer>((*struct_item)[2]);
    ASSERT_NE(nullptr, third);
    EXPECT_EQ(3, third->GetBigInteger().ToInt32());
}

// Test Method: TestStructModification()
TEST_F(StructAllMethodsTest, TestStructModification)
{
    auto struct_item = CreateStruct();

    // Add initial elements
    struct_item->Add(CreateInteger(10));
    struct_item->Add(CreateInteger(20));

    EXPECT_EQ(2, struct_item->Count());

    // Modify elements
    (*struct_item)[0] = CreateInteger(100);
    (*struct_item)[1] = CreateInteger(200);

    auto first = std::dynamic_pointer_cast<Integer>((*struct_item)[0]);
    auto second = std::dynamic_pointer_cast<Integer>((*struct_item)[1]);

    ASSERT_NE(nullptr, first);
    ASSERT_NE(nullptr, second);
    EXPECT_EQ(100, first->GetBigInteger().ToInt32());
    EXPECT_EQ(200, second->GetBigInteger().ToInt32());

    // Test removal
    struct_item->RemoveAt(0);
    EXPECT_EQ(1, struct_item->Count());

    auto remaining = std::dynamic_pointer_cast<Integer>((*struct_item)[0]);
    ASSERT_NE(nullptr, remaining);
    EXPECT_EQ(200, remaining->GetBigInteger().ToInt32());
}

// Test Method: TestStructNesting()
TEST_F(StructAllMethodsTest, TestStructNesting)
{
    auto outer_struct = CreateStruct();
    auto inner_struct1 = CreateStruct();
    auto inner_struct2 = CreateStruct();

    // Build nested structure
    inner_struct1->Add(CreateInteger(1));
    inner_struct1->Add(CreateInteger(2));

    inner_struct2->Add(CreateInteger(3));
    inner_struct2->Add(CreateInteger(4));

    outer_struct->Add(inner_struct1);
    outer_struct->Add(inner_struct2);
    outer_struct->Add(CreateInteger(5));

    EXPECT_EQ(3, outer_struct->Count());

    // Test nested access
    auto nested1 = std::dynamic_pointer_cast<Struct>((*outer_struct)[0]);
    ASSERT_NE(nullptr, nested1);
    EXPECT_EQ(2, nested1->Count());

    auto nested1_first = std::dynamic_pointer_cast<Integer>((*nested1)[0]);
    ASSERT_NE(nullptr, nested1_first);
    EXPECT_EQ(1, nested1_first->GetBigInteger().ToInt32());
}

// Test Method: TestStructCloneEdgeCases()
TEST_F(StructAllMethodsTest, TestStructCloneEdgeCases)
{
    auto limits = ExecutionEngineLimits::Default();

    // Test cloning empty struct
    auto empty_struct = CreateStruct();
    auto cloned_empty = std::dynamic_pointer_cast<Struct>(empty_struct->Clone(limits));
    ASSERT_NE(nullptr, cloned_empty);
    EXPECT_EQ(0, cloned_empty->Count());

    // Test cloning struct with various types
    auto mixed_struct = CreateStruct();
    mixed_struct->Add(CreateInteger(42));
    mixed_struct->Add(CreateByteString("hello"));
    mixed_struct->Add(StackItem::Null());

    auto cloned_mixed = std::dynamic_pointer_cast<Struct>(mixed_struct->Clone(limits));
    ASSERT_NE(nullptr, cloned_mixed);
    EXPECT_EQ(3, cloned_mixed->Count());

    // Verify elements are cloned properly
    auto cloned_int = std::dynamic_pointer_cast<Integer>((*cloned_mixed)[0]);
    ASSERT_NE(nullptr, cloned_int);
    EXPECT_EQ(42, cloned_int->GetBigInteger().ToInt32());
}

// Test Method: TestStructEqualsEdgeCases()
TEST_F(StructAllMethodsTest, TestStructEqualsEdgeCases)
{
    auto limits = ExecutionEngineLimits::Default();

    // Test empty structs
    auto empty1 = CreateStruct();
    auto empty2 = CreateStruct();
    EXPECT_TRUE(empty1->Equals(*empty2, limits));

    // Test different sizes
    auto small_struct = CreateStruct();
    small_struct->Add(CreateInteger(1));

    auto large_struct = CreateStruct();
    large_struct->Add(CreateInteger(1));
    large_struct->Add(CreateInteger(2));

    EXPECT_FALSE(small_struct->Equals(*large_struct, limits));
    EXPECT_FALSE(large_struct->Equals(*small_struct, limits));

    // Test null elements
    auto struct_with_null1 = CreateStruct();
    struct_with_null1->Add(StackItem::Null());
    struct_with_null1->Add(CreateInteger(1));

    auto struct_with_null2 = CreateStruct();
    struct_with_null2->Add(StackItem::Null());
    struct_with_null2->Add(CreateInteger(1));

    EXPECT_TRUE(struct_with_null1->Equals(*struct_with_null2, limits));
}

// Test Method: TestStructLimitsAndPerformance()
TEST_F(StructAllMethodsTest, TestStructLimitsAndPerformance)
{
    auto limits = ExecutionEngineLimits::Default();

    // Test struct with many elements (but within reasonable limits for test)
    auto large_struct = CreateStruct();

    for (int i = 0; i < 100; i++)
    {
        large_struct->Add(CreateInteger(i));
    }

    EXPECT_EQ(100, large_struct->Count());

    // Test cloning large struct
    auto cloned_large = std::dynamic_pointer_cast<Struct>(large_struct->Clone(limits));
    ASSERT_NE(nullptr, cloned_large);
    EXPECT_EQ(100, cloned_large->Count());

    // Verify elements are properly cloned
    for (int i = 0; i < 100; i++)
    {
        auto original_item = std::dynamic_pointer_cast<Integer>((*large_struct)[i]);
        auto cloned_item = std::dynamic_pointer_cast<Integer>((*cloned_large)[i]);

        ASSERT_NE(nullptr, original_item);
        ASSERT_NE(nullptr, cloned_item);
        EXPECT_EQ(original_item->GetBigInteger().ToInt32(), cloned_item->GetBigInteger().ToInt32());
    }
}