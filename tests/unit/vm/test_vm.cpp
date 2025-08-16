/**
 * @file test_vm.cpp
 * @brief Virtual Machine test suite
 */

#include <gtest/gtest.h>
#include <neo/vm/vm.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/opcode.h>

namespace neo::vm::tests {

class VMTest : public ::testing::Test {
protected:
    std::unique_ptr<ExecutionEngine> engine;
    
    void SetUp() override {
        engine = std::make_unique<ExecutionEngine>();
    }
    
    void TearDown() override {
        engine.reset();
    }
};

TEST_F(VMTest, PushData) {
    ScriptBuilder sb;
    sb.EmitPush(42);
    
    engine->LoadScript(sb.ToArray());
    engine->Execute();
    
    EXPECT_EQ(engine->State, VMState::HALT);
    EXPECT_EQ(engine->ResultStack.GetCount(), 1);
    
    auto result = engine->ResultStack.Pop();
    EXPECT_EQ(result->GetInteger(), 42);
}

TEST_F(VMTest, Addition) {
    ScriptBuilder sb;
    sb.EmitPush(2);
    sb.EmitPush(3);
    sb.Emit(OpCode::ADD);
    
    engine->LoadScript(sb.ToArray());
    engine->Execute();
    
    EXPECT_EQ(engine->State, VMState::HALT);
    EXPECT_EQ(engine->ResultStack.GetCount(), 1);
    
    auto result = engine->ResultStack.Pop();
    EXPECT_EQ(result->GetInteger(), 5);
}

TEST_F(VMTest, Subtraction) {
    ScriptBuilder sb;
    sb.EmitPush(10);
    sb.EmitPush(3);
    sb.Emit(OpCode::SUB);
    
    engine->LoadScript(sb.ToArray());
    engine->Execute();
    
    EXPECT_EQ(engine->State, VMState::HALT);
    auto result = engine->ResultStack.Pop();
    EXPECT_EQ(result->GetInteger(), 7);
}

TEST_F(VMTest, Multiplication) {
    ScriptBuilder sb;
    sb.EmitPush(4);
    sb.EmitPush(5);
    sb.Emit(OpCode::MUL);
    
    engine->LoadScript(sb.ToArray());
    engine->Execute();
    
    EXPECT_EQ(engine->State, VMState::HALT);
    auto result = engine->ResultStack.Pop();
    EXPECT_EQ(result->GetInteger(), 20);
}

TEST_F(VMTest, Division) {
    ScriptBuilder sb;
    sb.EmitPush(20);
    sb.EmitPush(4);
    sb.Emit(OpCode::DIV);
    
    engine->LoadScript(sb.ToArray());
    engine->Execute();
    
    EXPECT_EQ(engine->State, VMState::HALT);
    auto result = engine->ResultStack.Pop();
    EXPECT_EQ(result->GetInteger(), 5);
}

TEST_F(VMTest, BooleanOperations) {
    ScriptBuilder sb;
    sb.EmitPush(true);
    sb.EmitPush(false);
    sb.Emit(OpCode::BOOLAND);
    
    engine->LoadScript(sb.ToArray());
    engine->Execute();
    
    EXPECT_EQ(engine->State, VMState::HALT);
    auto result = engine->ResultStack.Pop();
    EXPECT_FALSE(result->GetBoolean());
}

TEST_F(VMTest, Comparison) {
    ScriptBuilder sb;
    sb.EmitPush(5);
    sb.EmitPush(3);
    sb.Emit(OpCode::GT);
    
    engine->LoadScript(sb.ToArray());
    engine->Execute();
    
    EXPECT_EQ(engine->State, VMState::HALT);
    auto result = engine->ResultStack.Pop();
    EXPECT_TRUE(result->GetBoolean());
}

TEST_F(VMTest, ConditionalJump) {
    ScriptBuilder sb;
    sb.EmitPush(true);
    sb.EmitJump(OpCode::JMPIF, 3);
    sb.EmitPush(1);
    sb.Emit(OpCode::JMP, 2);
    sb.EmitPush(2);
    
    engine->LoadScript(sb.ToArray());
    engine->Execute();
    
    EXPECT_EQ(engine->State, VMState::HALT);
    auto result = engine->ResultStack.Pop();
    EXPECT_EQ(result->GetInteger(), 2);
}

TEST_F(VMTest, ArrayOperations) {
    ScriptBuilder sb;
    sb.EmitPush(3); // Array size
    sb.Emit(OpCode::NEWARRAY);
    sb.Emit(OpCode::DUP);
    sb.EmitPush(0); // Index
    sb.EmitPush(42); // Value
    sb.Emit(OpCode::SETITEM);
    sb.EmitPush(0); // Index
    sb.Emit(OpCode::PICKITEM);
    
    engine->LoadScript(sb.ToArray());
    engine->Execute();
    
    EXPECT_EQ(engine->State, VMState::HALT);
    auto result = engine->ResultStack.Pop();
    EXPECT_EQ(result->GetInteger(), 42);
}

TEST_F(VMTest, ExceptionHandling) {
    ScriptBuilder sb;
    sb.EmitPush(0);
    sb.EmitPush(1);
    sb.Emit(OpCode::DIV); // Division by zero
    
    engine->LoadScript(sb.ToArray());
    engine->Execute();
    
    EXPECT_EQ(engine->State, VMState::FAULT);
}

TEST_F(VMTest, StackOperations) {
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(2);
    sb.Emit(OpCode::SWAP);
    
    engine->LoadScript(sb.ToArray());
    engine->Execute();
    
    EXPECT_EQ(engine->State, VMState::HALT);
    EXPECT_EQ(engine->ResultStack.GetCount(), 2);
    
    auto first = engine->ResultStack.Pop();
    auto second = engine->ResultStack.Pop();
    EXPECT_EQ(first->GetInteger(), 1);
    EXPECT_EQ(second->GetInteger(), 2);
}

} // namespace neo::vm::tests