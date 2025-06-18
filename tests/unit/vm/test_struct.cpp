#include <gtest/gtest.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/reference_counter.h>
#include <neo/io/byte_vector.h>

using namespace neo::vm;
using namespace neo::io;

TEST(StructTest, Equals)
{
    ReferenceCounter refCounter;
    
    // Create a struct
    auto struct1 = std::make_shared<Struct>(&refCounter);
    auto int1 = std::make_shared<Integer>(1, &refCounter);
    auto int2 = std::make_shared<Integer>(2, &refCounter);
    
    struct1->Add(int1);
    struct1->Add(int2);
    
    // Create an identical struct
    auto struct2 = std::make_shared<Struct>(&refCounter);
    auto int3 = std::make_shared<Integer>(1, &refCounter);
    auto int4 = std::make_shared<Integer>(2, &refCounter);
    
    struct2->Add(int3);
    struct2->Add(int4);
    
    // Create a different struct
    auto struct3 = std::make_shared<Struct>(&refCounter);
    auto int5 = std::make_shared<Integer>(1, &refCounter);
    auto int6 = std::make_shared<Integer>(3, &refCounter);
    
    struct3->Add(int5);
    struct3->Add(int6);
    
    // Test equals
    EXPECT_TRUE(struct1->Equals(*struct2));
    EXPECT_TRUE(struct2->Equals(*struct1));
    EXPECT_FALSE(struct1->Equals(*struct3));
    EXPECT_FALSE(struct3->Equals(*struct1));
    
    // Test equals with arrays
    auto array1 = std::make_shared<Array>(&refCounter);
    array1->Add(int1);
    array1->Add(int2);
    
    EXPECT_FALSE(struct1->Equals(*array1));
    EXPECT_FALSE(array1->Equals(*struct1));
}

TEST(StructTest, DeepCopy)
{
    ReferenceCounter refCounter;
    
    // Create a struct with nested elements
    auto struct1 = std::make_shared<Struct>(&refCounter);
    auto int1 = std::make_shared<Integer>(1, &refCounter);
    auto int2 = std::make_shared<Integer>(2, &refCounter);
    auto nestedStruct = std::make_shared<Struct>(&refCounter);
    auto int3 = std::make_shared<Integer>(3, &refCounter);
    
    nestedStruct->Add(int3);
    struct1->Add(int1);
    struct1->Add(int2);
    struct1->Add(nestedStruct);
    
    // Create a deep copy
    auto structCopy = struct1->DeepCopy();
    
    // Test equals
    EXPECT_TRUE(struct1->Equals(*structCopy));
    
    // Test that the copy is a different object
    EXPECT_NE(struct1.get(), structCopy.get());
    
    // Test that modifying the original doesn't affect the copy
    struct1->Add(std::make_shared<Integer>(4, &refCounter));
    EXPECT_EQ(struct1->Count(), 4);
    EXPECT_EQ(structCopy->Count(), 3);
    EXPECT_FALSE(struct1->Equals(*structCopy));
    
    // Test that modifying the nested struct in the original doesn't affect the copy
    auto originalNestedStruct = struct1->Get(2);
    auto originalNestedStructObj = std::dynamic_pointer_cast<Struct>(originalNestedStruct);
    originalNestedStructObj->Add(std::make_shared<Integer>(5, &refCounter));
    
    auto copyNestedStruct = structCopy->Get(2);
    auto copyNestedStructObj = std::dynamic_pointer_cast<Struct>(copyNestedStruct);
    
    EXPECT_EQ(originalNestedStructObj->Count(), 2);
    EXPECT_EQ(copyNestedStructObj->Count(), 1);
    EXPECT_FALSE(originalNestedStructObj->Equals(*copyNestedStructObj));
}

TEST(StructTest, CircularReference)
{
    ReferenceCounter refCounter;
    
    // Create a struct that references itself
    auto struct1 = std::make_shared<Struct>(&refCounter);
    auto int1 = std::make_shared<Integer>(1, &refCounter);
    
    struct1->Add(int1);
    struct1->Add(struct1);  // Add itself as an element
    
    // Create another struct for comparison
    auto struct2 = std::make_shared<Struct>(&refCounter);
    auto int2 = std::make_shared<Integer>(1, &refCounter);
    
    struct2->Add(int2);
    struct2->Add(struct2);  // Add itself as an element
    
    // Test equals
    EXPECT_TRUE(struct1->Equals(*struct2));
    EXPECT_TRUE(struct2->Equals(*struct1));
}

TEST(StructTest, Convert)
{
    ReferenceCounter refCounter;
    
    // Create a struct
    auto struct1 = std::make_shared<Struct>(&refCounter);
    auto int1 = std::make_shared<Integer>(1, &refCounter);
    auto int2 = std::make_shared<Integer>(2, &refCounter);
    
    struct1->Add(int1);
    struct1->Add(int2);
    
    // Test conversion to Array
    auto array = struct1->ToArray();
    EXPECT_EQ(array->Count(), 2);
    EXPECT_EQ(array->Get(0)->GetInteger(), 1);
    EXPECT_EQ(array->Get(1)->GetInteger(), 2);
    
    // Test conversion to Integer (should be zero)
    EXPECT_EQ(struct1->GetInteger(), 0);
    
    // Test conversion to Boolean (should be true)
    EXPECT_TRUE(struct1->GetBoolean());
    
    // Empty struct should convert to false
    auto emptyStruct = std::make_shared<Struct>(&refCounter);
    EXPECT_FALSE(emptyStruct->GetBoolean());
} 