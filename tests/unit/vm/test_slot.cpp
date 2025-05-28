#include <gtest/gtest.h>
#include <neo/vm/execution_context.h>
#include <neo/vm/script.h>
#include <neo/vm/stack_item.h>
#include <neo/io/byte_vector.h>

using namespace neo::vm;
using namespace neo::io;

// Test fixture for Slot tests
class SlotTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create a simple script
        script_ = std::make_unique<Script>(ByteVector::Parse("0102"));
        context_ = std::make_unique<ExecutionContext>(*script_);
    }

    std::unique_ptr<Script> script_;
    std::unique_ptr<ExecutionContext> context_;
};

// Test static field operations
TEST_F(SlotTest, StaticFields)
{
    // Initialize static fields
    context_->InitializeStaticFields(3);
    
    // Verify static fields array size
    EXPECT_EQ(context_->GetStaticFields().size(), 3);
    
    // Test storing and loading static fields
    auto item1 = StackItem::Create(123);
    auto item2 = StackItem::Create(456);
    
    context_->StoreStaticField(0, item1);
    context_->StoreStaticField(1, item2);
    
    EXPECT_EQ(context_->LoadStaticField(0), item1);
    EXPECT_EQ(context_->LoadStaticField(1), item2);
    EXPECT_EQ(context_->LoadStaticField(2), nullptr);
    
    // Test out-of-range access
    EXPECT_THROW(context_->LoadStaticField(3), std::out_of_range);
    EXPECT_THROW(context_->StoreStaticField(3, item1), std::out_of_range);
}

// Test local variables operations
TEST_F(SlotTest, LocalVariables)
{
    // Initialize local variables
    context_->InitializeLocalVariables(4);
    
    // Verify local variables array size
    EXPECT_EQ(context_->GetLocalVariables().size(), 4);
    
    // Test storing and loading local variables
    auto item1 = StackItem::Create(123);
    auto item2 = StackItem::Create(456);
    
    context_->StoreLocalVariable(0, item1);
    context_->StoreLocalVariable(2, item2);
    
    EXPECT_EQ(context_->LoadLocalVariable(0), item1);
    EXPECT_EQ(context_->LoadLocalVariable(1), nullptr);
    EXPECT_EQ(context_->LoadLocalVariable(2), item2);
    EXPECT_EQ(context_->LoadLocalVariable(3), nullptr);
    
    // Test out-of-range access
    EXPECT_THROW(context_->LoadLocalVariable(4), std::out_of_range);
    EXPECT_THROW(context_->StoreLocalVariable(4, item1), std::out_of_range);
}

// Test arguments operations
TEST_F(SlotTest, Arguments)
{
    // Initialize arguments and local variables together
    context_->InitializeLocalVariables(2, 3);
    
    // Verify array sizes
    EXPECT_EQ(context_->GetLocalVariables().size(), 2);
    EXPECT_EQ(context_->GetArguments().size(), 3);
    
    // Test storing and loading arguments
    auto item1 = StackItem::Create(123);
    auto item2 = StackItem::Create(456);
    
    context_->StoreArgument(0, item1);
    context_->StoreArgument(2, item2);
    
    EXPECT_EQ(context_->LoadArgument(0), item1);
    EXPECT_EQ(context_->LoadArgument(1), nullptr);
    EXPECT_EQ(context_->LoadArgument(2), item2);
    
    // Test out-of-range access
    EXPECT_THROW(context_->LoadArgument(3), std::out_of_range);
    EXPECT_THROW(context_->StoreArgument(3, item1), std::out_of_range);
}

// Test combined slot operations
TEST_F(SlotTest, CombinedSlots)
{
    // Initialize all slot types
    context_->InitializeStaticFields(2);
    context_->InitializeLocalVariables(2, 2);
    
    // Create items to store
    auto staticItem = StackItem::Create(1);
    auto localItem = StackItem::Create(2);
    auto argItem = StackItem::Create(3);
    
    // Store items in their respective slots
    context_->StoreStaticField(0, staticItem);
    context_->StoreLocalVariable(0, localItem);
    context_->StoreArgument(0, argItem);
    
    // Load and verify
    EXPECT_EQ(context_->LoadStaticField(0), staticItem);
    EXPECT_EQ(context_->LoadLocalVariable(0), localItem);
    EXPECT_EQ(context_->LoadArgument(0), argItem);
    
    // Verify that they're distinct slots
    EXPECT_NE(context_->LoadStaticField(0), context_->LoadLocalVariable(0));
    EXPECT_NE(context_->LoadLocalVariable(0), context_->LoadArgument(0));
    EXPECT_NE(context_->LoadArgument(0), context_->LoadStaticField(0));
}

// Test initialization with invalid parameters
TEST_F(SlotTest, InvalidInitialization)
{
    // Test initialization with negative values
    EXPECT_THROW(context_->InitializeStaticFields(-1), std::invalid_argument);
    EXPECT_THROW(context_->InitializeLocalVariables(-1), std::invalid_argument);
    EXPECT_THROW(context_->InitializeArguments(-1), std::invalid_argument);
}

// Test direct slot array manipulation
TEST_F(SlotTest, DirectSlotArrays)
{
    // Create arrays of stack items
    std::vector<std::shared_ptr<StackItem>> locals;
    locals.push_back(StackItem::Create(1));
    locals.push_back(StackItem::Create(2));
    
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(3));
    args.push_back(StackItem::Create(4));
    
    // Set the arrays directly
    context_->SetLocalVariables(locals);
    context_->SetArguments(args);
    
    // Verify sizes
    EXPECT_EQ(context_->GetLocalVariables().size(), 2);
    EXPECT_EQ(context_->GetArguments().size(), 2);
    
    // Verify values
    EXPECT_EQ(context_->LoadLocalVariable(0)->GetInteger(), 1);
    EXPECT_EQ(context_->LoadLocalVariable(1)->GetInteger(), 2);
    EXPECT_EQ(context_->LoadArgument(0)->GetInteger(), 3);
    EXPECT_EQ(context_->LoadArgument(1)->GetInteger(), 4);
} 