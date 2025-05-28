#include <gtest/gtest.h>
#include <neo/vm/reference_counter.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/types/array.h>
#include <neo/vm/types/buffer.h>
#include <neo/vm/types/byte_string.h>
#include <neo/vm/types/integer.h>
#include <neo/vm/types/map.h>
#include <neo/vm/types/struct.h>

using namespace neo::vm;

TEST(ReferenceCounterTests, TestCircularReferences)
{
    // Create a reference counter
    ReferenceCounter refCounter;

    // Create an array
    auto array1 = std::make_shared<types::Array>(refCounter);
    
    // Add a stack reference to the array
    refCounter.AddStackReference(array1);
    
    // Create another array and add it to the first array
    auto array2 = std::make_shared<types::Array>(refCounter);
    array1->Add(array2);
    
    // Create a circular reference by adding array1 to array2
    array2->Add(array1);
    
    // Check that both arrays have circular references
    EXPECT_TRUE(refCounter.HasCircularReference(array1));
    EXPECT_TRUE(refCounter.HasCircularReference(array2));
    
    // Remove the stack reference to array1
    refCounter.RemoveStackReference(array1);
    
    // Check zero referred to clean up circular references
    refCounter.CheckZeroReferred();
    
    // Verify that the reference count is now 0
    EXPECT_EQ(refCounter.Count(), 0);
}

TEST(ReferenceCounterTests, TestNestedArrays)
{
    // Create a reference counter
    ReferenceCounter refCounter;

    // Create a nested array structure
    auto array1 = std::make_shared<types::Array>(refCounter);
    auto array2 = std::make_shared<types::Array>(refCounter);
    auto array3 = std::make_shared<types::Array>(refCounter);
    
    // Add stack references
    refCounter.AddStackReference(array1);
    
    // Create the nested structure: array1 -> array2 -> array3
    array1->Add(array2);
    array2->Add(array3);
    
    // Add some primitive items to array3
    array3->Add(StackItem::Create(123));
    array3->Add(StackItem::Create(true));
    array3->Add(StackItem::Create("test"));
    
    // Verify reference counts
    EXPECT_EQ(refCounter.Count(), 7); // 1 stack ref + 6 object refs
    
    // Remove the stack reference to array1
    refCounter.RemoveStackReference(array1);
    
    // Check zero referred to clean up
    refCounter.CheckZeroReferred();
    
    // Verify that the reference count is now 0
    EXPECT_EQ(refCounter.Count(), 0);
}

TEST(ReferenceCounterTests, TestMapWithCircularReferences)
{
    // Create a reference counter
    ReferenceCounter refCounter;

    // Create a map
    auto map = std::make_shared<types::Map>(refCounter);
    
    // Add a stack reference to the map
    refCounter.AddStackReference(map);
    
    // Create an array and add a circular reference
    auto array = std::make_shared<types::Array>(refCounter);
    map->Set(StackItem::Create("array"), array);
    array->Add(map);
    
    // Check that both have circular references
    EXPECT_TRUE(refCounter.HasCircularReference(map));
    EXPECT_TRUE(refCounter.HasCircularReference(array));
    
    // Remove the stack reference to the map
    refCounter.RemoveStackReference(map);
    
    // Check zero referred to clean up circular references
    refCounter.CheckZeroReferred();
    
    // Verify that the reference count is now 0
    EXPECT_EQ(refCounter.Count(), 0);
}

TEST(ReferenceCounterTests, TestComplexCircularReferences)
{
    // Create a reference counter
    ReferenceCounter refCounter;

    // Create a complex structure with multiple circular references
    auto array1 = std::make_shared<types::Array>(refCounter);
    auto array2 = std::make_shared<types::Array>(refCounter);
    auto array3 = std::make_shared<types::Array>(refCounter);
    auto map1 = std::make_shared<types::Map>(refCounter);
    auto map2 = std::make_shared<types::Map>(refCounter);
    
    // Add stack references
    refCounter.AddStackReference(array1);
    
    // Create circular references
    array1->Add(array2);
    array2->Add(array3);
    array3->Add(array1); // Circular: array1 -> array2 -> array3 -> array1
    
    map1->Set(StackItem::Create("key1"), array1);
    map2->Set(StackItem::Create("key2"), map1);
    array3->Add(map2); // Circular: array3 -> map2 -> map1 -> array1 -> array2 -> array3
    
    // Verify reference counts
    EXPECT_TRUE(refCounter.HasCircularReference(array1));
    EXPECT_TRUE(refCounter.HasCircularReference(array2));
    EXPECT_TRUE(refCounter.HasCircularReference(array3));
    EXPECT_TRUE(refCounter.HasCircularReference(map1));
    EXPECT_TRUE(refCounter.HasCircularReference(map2));
    
    // Remove the stack reference to array1
    refCounter.RemoveStackReference(array1);
    
    // Check zero referred to clean up circular references
    refCounter.CheckZeroReferred();
    
    // Verify that the reference count is now 0
    EXPECT_EQ(refCounter.Count(), 0);
}
