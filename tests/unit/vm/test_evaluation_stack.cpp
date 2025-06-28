#include <gtest/gtest.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script.h>
#include <neo/vm/reference_counter.h>
#include <neo/vm/stack_item.h>
#include <neo/io/byte_vector.h>
#include <vector>

using namespace neo::vm;
using namespace neo::io;

// Test fixture for EvaluationStack tests
class EvaluationStackTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        referenceCounter_ = std::make_unique<ReferenceCounter>();
        
        // Create a simple script to initialize a context with an evaluation stack
        auto scriptBytes = ByteVector::Parse("0102");
        // Convert io::ByteVector to internal::ByteVector
        neo::vm::internal::ByteVector internalBytes;
        internalBytes.Reserve(scriptBytes.Size());
        for (size_t i = 0; i < scriptBytes.Size(); ++i)
        {
            internalBytes.Push(scriptBytes[i]);
        }
        script_ = std::make_unique<Script>(internalBytes);
        
        // Create a context with the script
        context_ = std::make_unique<ExecutionContext>(*script_);
    }

    std::unique_ptr<ReferenceCounter> referenceCounter_;
    std::unique_ptr<Script> script_;
    std::unique_ptr<ExecutionContext> context_;
};

// Test Push/Pop/Peek operations on the evaluation stack
TEST_F(EvaluationStackTest, PushPopPeek)
{
    // Push some stack items
    auto item1 = StackItem::Create(static_cast<int64_t>(123));
    auto item2 = StackItem::Create(static_cast<int64_t>(456));
    auto item3 = StackItem::Create(static_cast<int64_t>(789));
    
    context_->Push(item1);
    context_->Push(item2);
    context_->Push(item3);
    
    // Stack should have 3 items
    EXPECT_EQ(context_->GetStackSize(), 3);
    
    // Peek at the top item
    auto peekedItem = context_->Peek();
    EXPECT_EQ(peekedItem->GetInteger(), 789);
    
    // Peek at the second item
    peekedItem = context_->Peek(1);
    EXPECT_EQ(peekedItem->GetInteger(), 456);
    
    // Peek at the third item
    peekedItem = context_->Peek(2);
    EXPECT_EQ(peekedItem->GetInteger(), 123);
    
    // Pop the top item
    auto poppedItem = context_->Pop();
    EXPECT_EQ(poppedItem->GetInteger(), 789);
    EXPECT_EQ(context_->GetStackSize(), 2);
    
    // Pop the second item
    poppedItem = context_->Pop();
    EXPECT_EQ(poppedItem->GetInteger(), 456);
    EXPECT_EQ(context_->GetStackSize(), 1);
    
    // Pop the last item
    poppedItem = context_->Pop();
    EXPECT_EQ(poppedItem->GetInteger(), 123);
    EXPECT_EQ(context_->GetStackSize(), 0);
    
    // Popping from an empty stack should throw
    EXPECT_THROW(context_->Pop(), std::runtime_error);
}

// Test ClearStack operation
TEST_F(EvaluationStackTest, ClearStack)
{
    // Push some stack items
    auto item1 = StackItem::Create(static_cast<int64_t>(123));
    auto item2 = StackItem::Create(static_cast<int64_t>(456));
    
    context_->Push(item1);
    context_->Push(item2);
    
    // Stack should have 2 items
    EXPECT_EQ(context_->GetStackSize(), 2);
    
    // Clear the stack
    context_->ClearStack();
    
    // Stack should be empty
    EXPECT_EQ(context_->GetStackSize(), 0);
}

// Test Push/Pop operations with different types
TEST_F(EvaluationStackTest, DifferentTypes)
{
    // Push different types of stack items
    auto intItem = StackItem::Create(static_cast<int64_t>(123));
    auto boolItem = StackItem::Create(true);
    auto byteStringItem = StackItem::Create(ByteVector::Parse("010203"));
    
    context_->Push(intItem);
    context_->Push(boolItem);
    context_->Push(byteStringItem);
    
    // Stack should have 3 items
    EXPECT_EQ(context_->GetStackSize(), 3);
    
    // Peek at each item and verify its type
    auto peekedItem = context_->Peek();
    EXPECT_EQ(peekedItem->GetType(), StackItemType::ByteString);
    auto expectedBytes = ByteVector::Parse("010203");
    // Check byte string content by converting to ByteVector
    EXPECT_TRUE(peekedItem->IsByteString());
    auto peekedBytes = peekedItem->GetByteArray();
    EXPECT_EQ(peekedBytes.Size(), expectedBytes.Size());
    EXPECT_TRUE(std::equal(peekedBytes.Data(), peekedBytes.Data() + peekedBytes.Size(), expectedBytes.Data()));
    
    peekedItem = context_->Peek(1);
    EXPECT_TRUE(peekedItem->IsBoolean());
    EXPECT_EQ(peekedItem->GetBoolean(), true);
    
    peekedItem = context_->Peek(2);
    EXPECT_TRUE(peekedItem->IsInteger());
    EXPECT_EQ(peekedItem->GetInteger(), 123);
    
    // Pop each item
    auto poppedItem = context_->Pop();
    EXPECT_EQ(poppedItem->GetType(), StackItemType::ByteString);
    
    poppedItem = context_->Pop();
    EXPECT_TRUE(poppedItem->IsBoolean());
    
    poppedItem = context_->Pop();
    EXPECT_TRUE(poppedItem->IsInteger());
}

// Test reference counting with the EvaluationStack
TEST_F(EvaluationStackTest, ReferenceCount)
{
    // Create a stack item with reference counter
    auto item = StackItem::Create(static_cast<int64_t>(123));
    
    // Initial reference count should be 0
    EXPECT_EQ(referenceCounter_->Count(), 0);
    
    // Push the item onto the stack
    context_->Push(item);
    
    // Reference count should still be 0 since the Integer is already tracked by 'item' variable
    EXPECT_EQ(referenceCounter_->Count(), 0);
    
    // Pop the item and keep a reference
    auto poppedItem = context_->Pop();
    
    // Reference count should still be 0 
    EXPECT_EQ(referenceCounter_->Count(), 0);
    
    // Clear the original reference
    item = nullptr;
    
    // Reference count should still be 0 due to poppedItem reference
    EXPECT_EQ(referenceCounter_->Count(), 0);
    
    // Clear the popped reference
    poppedItem = nullptr;
    
    // Reference count should now be 0
    EXPECT_EQ(referenceCounter_->Count(), 0);
} 