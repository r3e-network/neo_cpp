#include <gtest/gtest.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/types/null.h>
#include <neo/vm/exceptions.h>

using namespace neo::vm;

TEST(StackItemTests, TestNullEquals)
{
    // Test that Null equals Null
    auto null1 = StackItem::Null();
    auto null2 = StackItem::Null();
    EXPECT_TRUE(null1->Equals(*null2));
    
    // Test that Null is the same instance
    EXPECT_EQ(null1, null2);
}

TEST(StackItemTests, TestBooleanCaching)
{
    // Test that True and False are cached
    auto true1 = StackItem::True();
    auto true2 = StackItem::True();
    EXPECT_EQ(true1, true2);
    
    auto false1 = StackItem::False();
    auto false2 = StackItem::False();
    EXPECT_EQ(false1, false2);
    
    // Test that Create(bool) uses the cached instances
    auto trueCreated = StackItem::Create(true);
    auto falseCreated = StackItem::Create(false);
    EXPECT_EQ(true1, trueCreated);
    EXPECT_EQ(false1, falseCreated);
}

TEST(StackItemTests, TestConvertTo)
{
    // Test converting Boolean to Integer
    auto boolItem = StackItem::Create(true);
    auto intItem = boolItem->ConvertTo(StackItemType::Integer);
    EXPECT_EQ(intItem->GetType(), StackItemType::Integer);
    EXPECT_EQ(intItem->GetInteger(), 1);
    
    // Test converting Integer to Boolean
    auto intItem2 = StackItem::Create(42);
    auto boolItem2 = intItem2->ConvertTo(StackItemType::Boolean);
    EXPECT_EQ(boolItem2->GetType(), StackItemType::Boolean);
    EXPECT_TRUE(boolItem2->GetBoolean());
    
    // Test converting Integer to ByteString
    auto byteStringItem = intItem2->ConvertTo(StackItemType::ByteString);
    EXPECT_EQ(byteStringItem->GetType(), StackItemType::ByteString);
    EXPECT_EQ(byteStringItem->GetInteger(), 42);
    
    // Test converting Null to other types (should throw)
    auto nullItem = StackItem::Null();
    EXPECT_THROW(nullItem->ConvertTo(StackItemType::Integer), InvalidCastException);
    EXPECT_THROW(nullItem->ConvertTo(StackItemType::Boolean), InvalidCastException);
    EXPECT_THROW(nullItem->ConvertTo(StackItemType::ByteString), InvalidCastException);
}

TEST(StackItemTests, TestExecuteNextWithNullInstruction)
{
    // Create a script with no instructions
    Script script;
    
    // Create an execution engine
    ExecutionEngine engine;
    
    // Load the script
    engine.LoadScript(script);
    
    // Execute the script
    engine.Execute();
    
    // Verify that the engine halted
    EXPECT_EQ(engine.GetState(), VMState::Halt);
}

TEST(StackItemTests, TestExecuteNextWithException)
{
    // Create a script with an invalid opcode
    io::ByteVector scriptBytes = {0xFF}; // Invalid opcode
    Script script(scriptBytes);
    
    // Create an execution engine
    ExecutionEngine engine;
    
    // Load the script
    engine.LoadScript(script);
    
    // Execute the script
    engine.Execute();
    
    // Verify that the engine faulted
    EXPECT_EQ(engine.GetState(), VMState::Fault);
    
    // Verify that there is an uncaught exception
    EXPECT_TRUE(engine.HasUncaughtException());
}
