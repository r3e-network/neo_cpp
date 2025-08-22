#include <gtest/gtest.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script.h>
#include <neo/vm/opcode.h>

using namespace neo::vm;

TEST(VMOpcodesTests, TestPushOperations)
{
    // Test basic PUSH operations
    ExecutionEngine engine;
    
    // Test PUSH1 (OpCode::PUSH1 = 0x51)
    Script script;
    script.EmitOpCode(OpCode::PUSH1);
    script.EmitOpCode(OpCode::RET);
    
    engine.LoadScript(script);
    auto result = engine.Execute();
    
    EXPECT_EQ(result, VMState::HALT);
    EXPECT_EQ(engine.GetResultStack().size(), 1);
    
    auto top_item = engine.GetResultStack()[0];
    EXPECT_TRUE(top_item->IsInteger());
    EXPECT_EQ(top_item->GetBigInteger(), 1);
}

TEST(VMOpcodesTests, TestArithmeticOperations)
{
    // Test basic arithmetic: PUSH2, PUSH3, ADD
    ExecutionEngine engine;
    
    Script script;
    script.EmitOpCode(OpCode::PUSH2);  // Push 2
    script.EmitOpCode(OpCode::PUSH3);  // Push 3
    script.EmitOpCode(OpCode::ADD);    // Add them
    script.EmitOpCode(OpCode::RET);
    
    engine.LoadScript(script);
    auto result = engine.Execute();
    
    EXPECT_EQ(result, VMState::HALT);
    EXPECT_EQ(engine.GetResultStack().size(), 1);
    
    auto top_item = engine.GetResultStack()[0];
    EXPECT_TRUE(top_item->IsInteger());
    EXPECT_EQ(top_item->GetBigInteger(), 5);
}

TEST(VMOpcodesTests, TestStackOperations)
{
    // Test stack manipulation: PUSH1, DUP, SWAP
    ExecutionEngine engine;
    
    Script script;
    script.EmitOpCode(OpCode::PUSH1);  // Stack: [1]
    script.EmitOpCode(OpCode::PUSH2);  // Stack: [1, 2]
    script.EmitOpCode(OpCode::DUP);    // Stack: [1, 2, 2]
    script.EmitOpCode(OpCode::RET);
    
    engine.LoadScript(script);
    auto result = engine.Execute();
    
    EXPECT_EQ(result, VMState::HALT);
    EXPECT_EQ(engine.GetResultStack().size(), 3);
    
    // Check stack contents
    EXPECT_EQ(engine.GetResultStack()[0]->GetBigInteger(), 1);
    EXPECT_EQ(engine.GetResultStack()[1]->GetBigInteger(), 2);
    EXPECT_EQ(engine.GetResultStack()[2]->GetBigInteger(), 2);
}

TEST(VMOpcodesTests, TestBooleanOperations)
{
    // Test boolean operations: PUSH1, PUSH0, BOOLAND
    ExecutionEngine engine;
    
    Script script;
    script.EmitOpCode(OpCode::PUSH1);     // Push true
    script.EmitOpCode(OpCode::PUSH0);     // Push false
    script.EmitOpCode(OpCode::BOOLAND);   // AND operation
    script.EmitOpCode(OpCode::RET);
    
    engine.LoadScript(script);
    auto result = engine.Execute();
    
    EXPECT_EQ(result, VMState::HALT);
    EXPECT_EQ(engine.GetResultStack().size(), 1);
    
    auto result_item = engine.GetResultStack()[0];
    EXPECT_TRUE(result_item->IsBoolean());
    EXPECT_FALSE(result_item->GetBoolean());
}