#include <gtest/gtest.h>
#include <neo/io/byte_vector.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/reference_counter.h>
#include <neo/vm/stack_item.h>

using namespace neo::vm;
using namespace neo::io;

TEST(StructTest, Equals)
{
    // Create a struct
    std::vector<std::shared_ptr<StackItem>> items1;
    items1.push_back(StackItem::Create(static_cast<int64_t>(1)));
    items1.push_back(StackItem::Create(static_cast<int64_t>(2)));
    auto struct1 = std::make_shared<StructItem>(items1);

    // Create an identical struct
    std::vector<std::shared_ptr<StackItem>> items2;
    items2.push_back(StackItem::Create(static_cast<int64_t>(1)));
    items2.push_back(StackItem::Create(static_cast<int64_t>(2)));
    auto struct2 = std::make_shared<StructItem>(items2);

    // Create a different struct
    std::vector<std::shared_ptr<StackItem>> items3;
    items3.push_back(StackItem::Create(static_cast<int64_t>(1)));
    items3.push_back(StackItem::Create(static_cast<int64_t>(3)));
    auto struct3 = std::make_shared<StructItem>(items3);

    // Test equals
    EXPECT_TRUE(struct1->Equals(*struct2));
    EXPECT_TRUE(struct2->Equals(*struct1));
    EXPECT_FALSE(struct1->Equals(*struct3));
    EXPECT_FALSE(struct3->Equals(*struct1));

    // Test equals with arrays
    std::vector<std::shared_ptr<StackItem>> arrayItems;
    arrayItems.push_back(StackItem::Create(static_cast<int64_t>(1)));
    arrayItems.push_back(StackItem::Create(static_cast<int64_t>(2)));
    auto array1 = std::make_shared<ArrayItem>(arrayItems);

    EXPECT_FALSE(struct1->Equals(*array1));
    EXPECT_FALSE(array1->Equals(*struct1));
}

TEST(StructTest, DeepCopy)
{
    // Create a struct with nested elements
    std::vector<std::shared_ptr<StackItem>> nestedItems;
    nestedItems.push_back(StackItem::Create(static_cast<int64_t>(3)));
    auto nestedStruct = std::make_shared<StructItem>(nestedItems);

    std::vector<std::shared_ptr<StackItem>> items1;
    items1.push_back(StackItem::Create(static_cast<int64_t>(1)));
    items1.push_back(StackItem::Create(static_cast<int64_t>(2)));
    items1.push_back(nestedStruct);
    auto struct1 = std::make_shared<StructItem>(items1);

    // Create a deep copy
    auto structCopy = struct1->DeepCopy();

    // Test equals
    EXPECT_TRUE(struct1->Equals(*structCopy));

    // Test that the copy is a different object
    EXPECT_NE(struct1.get(), structCopy.get());

    // Test that modifying the original doesn't affect the copy
    struct1->Add(StackItem::Create(static_cast<int64_t>(4)));
    EXPECT_EQ(struct1->Count(), 4);
    EXPECT_EQ(std::dynamic_pointer_cast<StructItem>(structCopy)->Count(), 3);
    EXPECT_FALSE(struct1->Equals(*structCopy));

    // Test that modifying the nested struct in the original doesn't affect the copy
    auto originalNestedStruct = struct1->Get(2);
    auto originalNestedStructObj = std::dynamic_pointer_cast<StructItem>(originalNestedStruct);
    originalNestedStructObj->Add(StackItem::Create(static_cast<int64_t>(5)));

    auto copyNestedStruct = std::dynamic_pointer_cast<StructItem>(structCopy)->Get(2);
    auto copyNestedStructObj = std::dynamic_pointer_cast<StructItem>(copyNestedStruct);

    EXPECT_EQ(originalNestedStructObj->Count(), 2);
    EXPECT_EQ(copyNestedStructObj->Count(), 1);
    EXPECT_FALSE(originalNestedStructObj->Equals(*copyNestedStructObj));
}

TEST(StructTest, CircularReference)
{
    // Create a struct that references itself
    std::vector<std::shared_ptr<StackItem>> items1;
    items1.push_back(StackItem::Create(static_cast<int64_t>(1)));
    auto struct1 = std::make_shared<StructItem>(items1);
    struct1->Add(struct1);  // Add itself as an element

    // Create another struct for comparison
    std::vector<std::shared_ptr<StackItem>> items2;
    items2.push_back(StackItem::Create(static_cast<int64_t>(1)));
    auto struct2 = std::make_shared<StructItem>(items2);
    struct2->Add(struct2);  // Add itself as an element

    // Test equals
    EXPECT_TRUE(struct1->Equals(*struct2));
    EXPECT_TRUE(struct2->Equals(*struct1));
}

TEST(StructTest, Convert)
{
    // Create a struct
    std::vector<std::shared_ptr<StackItem>> items1;
    items1.push_back(StackItem::Create(static_cast<int64_t>(1)));
    items1.push_back(StackItem::Create(static_cast<int64_t>(2)));
    auto struct1 = std::make_shared<StructItem>(items1);

    // Test that struct acts like an array (since StructItem extends ArrayItem)
    EXPECT_EQ(struct1->Count(), 2);
    EXPECT_EQ(struct1->Get(0)->GetInteger(), 1);
    EXPECT_EQ(struct1->Get(1)->GetInteger(), 2);

    // Test conversion to Integer (should be zero)
    EXPECT_EQ(struct1->GetInteger(), 0);

    // Test conversion to Boolean (should be true)
    EXPECT_TRUE(struct1->GetBoolean());

    // Empty struct should convert to false
    std::vector<std::shared_ptr<StackItem>> emptyItems;
    auto emptyStruct = std::make_shared<StructItem>(emptyItems);
    EXPECT_FALSE(emptyStruct->GetBoolean());
}