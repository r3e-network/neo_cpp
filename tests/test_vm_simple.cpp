#include <gtest/gtest.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/opcode.h>

using namespace neo::vm;

// Test basic arithmetic operations
TEST(VMTest, BasicArithmetic) {
    // Create script: PUSH2 PUSH3 ADD
    ScriptBuilder sb;
    sb.Emit(OpCode::PUSH2);
    sb.Emit(OpCode::PUSH3); 
    sb.Emit(OpCode::ADD);
    
    auto scriptBytes = sb.ToArray();
    Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
    ExecutionEngine engine;
    engine.LoadScript(script);
    
    engine.Execute();
    
    EXPECT_EQ(engine.GetState(), VMState::Halt);
    
    auto result = engine.Pop();
    auto intResult = std::dynamic_pointer_cast<IntegerItem>(result);
    EXPECT_NE(intResult, nullptr);
    EXPECT_EQ(intResult->GetInteger(), 5);
}

// Test push operations
TEST(VMTest, PushOperations) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(42));
    sb.EmitPush("Hello");
    sb.EmitPush(true);
    
    auto scriptBytes = sb.ToArray();
    Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
    ExecutionEngine engine;
    engine.LoadScript(script);
    
    engine.Execute();
    
    EXPECT_EQ(engine.GetState(), VMState::Halt);
    
    // Pop in reverse order (stack is LIFO)
    auto boolItem = engine.Pop();
    auto booleanItem = std::dynamic_pointer_cast<BooleanItem>(boolItem);
    EXPECT_NE(booleanItem, nullptr);
    EXPECT_TRUE(booleanItem->GetBoolean());
    
    auto strItem = engine.Pop();
    auto byteStringItem = std::dynamic_pointer_cast<ByteStringItem>(strItem);
    EXPECT_NE(byteStringItem, nullptr);
    auto bytes = byteStringItem->GetByteArray();
    EXPECT_EQ(std::string((char*)bytes.Data(), bytes.Size()), "Hello");
    
    auto intItem = engine.Pop();
    auto integerItem = std::dynamic_pointer_cast<IntegerItem>(intItem);
    EXPECT_NE(integerItem, nullptr);
    EXPECT_EQ(integerItem->GetInteger(), 42);
}

// Test comparison operations
TEST(VMTest, ComparisonOperations) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(5));
    sb.EmitPush(static_cast<int64_t>(3));
    sb.Emit(OpCode::GT);  // 5 > 3 = true
    
    auto scriptBytes = sb.ToArray();
    Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
    ExecutionEngine engine;
    engine.LoadScript(script);
    
    engine.Execute();
    
    EXPECT_EQ(engine.GetState(), VMState::Halt);
    
    auto result = engine.Pop();
    auto boolResult = std::dynamic_pointer_cast<BooleanItem>(result);
    EXPECT_NE(boolResult, nullptr);
    EXPECT_TRUE(boolResult->GetBoolean());
}

// Test stack operations
TEST(VMTest, StackOperations) {
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(1));
    sb.EmitPush(static_cast<int64_t>(2));
    sb.Emit(OpCode::DUP);   // Duplicate top item
    sb.Emit(OpCode::SWAP);  // Swap top two items
    
    auto scriptBytes = sb.ToArray();
    Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
    ExecutionEngine engine;
    engine.LoadScript(script);
    
    engine.Execute();
    
    EXPECT_EQ(engine.GetState(), VMState::Halt);
    
    // Stack should be: [1, 2, 2] -> after DUP
    // Then after SWAP: [1, 2, 2]
    auto top = engine.Pop();
    auto topInt = std::dynamic_pointer_cast<IntegerItem>(top);
    EXPECT_NE(topInt, nullptr);
    EXPECT_EQ(topInt->GetInteger(), 2);
    
    auto second = engine.Pop();
    auto secondInt = std::dynamic_pointer_cast<IntegerItem>(second);
    EXPECT_NE(secondInt, nullptr);
    EXPECT_EQ(secondInt->GetInteger(), 2);
    
    auto third = engine.Pop();
    auto thirdInt = std::dynamic_pointer_cast<IntegerItem>(third);
    EXPECT_NE(thirdInt, nullptr);
    EXPECT_EQ(thirdInt->GetInteger(), 1);
}

// Test control flow
TEST(VMTest, ControlFlow) {
    ScriptBuilder sb;
    sb.EmitPush(true);
    sb.EmitJump(OpCode::JMPIF, 1);  // Jump 1 byte forward to skip PUSH1
    sb.EmitPush(static_cast<int64_t>(1));      // This should be skipped
    sb.Emit(OpCode::RET);
    sb.EmitPush(static_cast<int64_t>(2));      // This should execute
    
    auto scriptBytes = sb.ToArray();
    Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
    ExecutionEngine engine;
    engine.LoadScript(script);
    
    engine.Execute();
    
    EXPECT_EQ(engine.GetState(), VMState::Halt);
    
    auto result = engine.Pop();
    auto intResult = std::dynamic_pointer_cast<IntegerItem>(result);
    EXPECT_NE(intResult, nullptr);
    EXPECT_EQ(intResult->GetInteger(), 2);
}

// Test with limits
TEST(VMTest, ExecutionLimits) {
    ScriptBuilder sb;
    // Create a script that would exceed the default instruction limit
    for (int i = 0; i < 100; i++) {
        sb.Emit(OpCode::NOP);
    }
    
    auto scriptBytes = sb.ToArray();
    Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
    ExecutionEngine engine;
    engine.LoadScript(script);
    
    // Set a low instruction limit
    // Note: ExecutionEngineLimits doesn't have SetMaxInstructions
    // This test needs to be updated when the API is available
    
    engine.Execute();
    
    // Should fault due to exceeding instruction limit
    EXPECT_EQ(engine.GetState(), VMState::Fault);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}