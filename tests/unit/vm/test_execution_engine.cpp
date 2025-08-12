#include <gtest/gtest.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script.h>
#include <neo/vm/opcode.h>
#include <neo/vm/vm_state.h>
#include <neo/vm/evaluation_stack.h>
#include <neo/vm/primitive_items.h>

namespace neo {
namespace vm {
namespace tests {

class ExecutionEngineTest : public ::testing::Test {
protected:
    std::unique_ptr<ExecutionEngine> engine;
    
    void SetUp() override {
        engine = std::make_unique<ExecutionEngine>();
    }
    
    void TearDown() override {
        engine.reset();
    }
    
    // Helper to create a script from opcodes
    std::shared_ptr<Script> CreateScript(std::initializer_list<uint8_t> opcodes) {
        return std::make_shared<Script>(std::vector<uint8_t>(opcodes));
    }
};

// Test basic arithmetic operations
TEST_F(ExecutionEngineTest, PushAndAdd) {
    // Create script: PUSH1 PUSH2 ADD
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::PUSH1),
        static_cast<uint8_t>(Opcode::PUSH2),
        static_cast<uint8_t>(Opcode::ADD)
    });
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 1);
    
    auto result = engine->ResultStack().Pop();
    EXPECT_EQ(result->GetInteger(), 3);
}

TEST_F(ExecutionEngineTest, PushAndSubtract) {
    // Create script: PUSH5 PUSH3 SUB
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::PUSH5),
        static_cast<uint8_t>(Opcode::PUSH3),
        static_cast<uint8_t>(Opcode::SUB)
    });
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 1);
    
    auto result = engine->ResultStack().Pop();
    EXPECT_EQ(result->GetInteger(), 2);
}

TEST_F(ExecutionEngineTest, PushAndMultiply) {
    // Create script: PUSH3 PUSH4 MUL
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::PUSH3),
        static_cast<uint8_t>(Opcode::PUSH4),
        static_cast<uint8_t>(Opcode::MUL)
    });
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 1);
    
    auto result = engine->ResultStack().Pop();
    EXPECT_EQ(result->GetInteger(), 12);
}

// Test comparison operations
TEST_F(ExecutionEngineTest, ComparisonEqual) {
    // Create script: PUSH2 PUSH2 EQUAL
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::PUSH2),
        static_cast<uint8_t>(Opcode::PUSH2),
        static_cast<uint8_t>(Opcode::EQUAL)
    });
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 1);
    
    auto result = engine->ResultStack().Pop();
    EXPECT_TRUE(result->GetBoolean());
}

TEST_F(ExecutionEngineTest, ComparisonNotEqual) {
    // Create script: PUSH1 PUSH2 EQUAL
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::PUSH1),
        static_cast<uint8_t>(Opcode::PUSH2),
        static_cast<uint8_t>(Opcode::EQUAL)
    });
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 1);
    
    auto result = engine->ResultStack().Pop();
    EXPECT_FALSE(result->GetBoolean());
}

TEST_F(ExecutionEngineTest, ComparisonLessThan) {
    // Create script: PUSH1 PUSH2 LT
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::PUSH1),
        static_cast<uint8_t>(Opcode::PUSH2),
        static_cast<uint8_t>(Opcode::LT)
    });
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 1);
    
    auto result = engine->ResultStack().Pop();
    EXPECT_TRUE(result->GetBoolean());
}

// Test logical operations
TEST_F(ExecutionEngineTest, LogicalAnd) {
    // Create script: PUSH1 PUSH1 BOOLAND
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::PUSH1),
        static_cast<uint8_t>(Opcode::PUSH1),
        static_cast<uint8_t>(Opcode::BOOLAND)
    });
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 1);
    
    auto result = engine->ResultStack().Pop();
    EXPECT_TRUE(result->GetBoolean());
}

TEST_F(ExecutionEngineTest, LogicalOr) {
    // Create script: PUSH0 PUSH1 BOOLOR
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::PUSH0),
        static_cast<uint8_t>(Opcode::PUSH1),
        static_cast<uint8_t>(Opcode::BOOLOR)
    });
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 1);
    
    auto result = engine->ResultStack().Pop();
    EXPECT_TRUE(result->GetBoolean());
}

TEST_F(ExecutionEngineTest, LogicalNot) {
    // Create script: PUSH0 NOT
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::PUSH0),
        static_cast<uint8_t>(Opcode::NOT)
    });
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 1);
    
    auto result = engine->ResultStack().Pop();
    EXPECT_TRUE(result->GetBoolean());
}

// Test stack manipulation
TEST_F(ExecutionEngineTest, StackDuplicate) {
    // Create script: PUSH1 DUP
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::PUSH1),
        static_cast<uint8_t>(Opcode::DUP)
    });
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 2);
    
    auto first = engine->ResultStack().Pop();
    auto second = engine->ResultStack().Pop();
    EXPECT_EQ(first->GetInteger(), 1);
    EXPECT_EQ(second->GetInteger(), 1);
}

TEST_F(ExecutionEngineTest, StackSwap) {
    // Create script: PUSH1 PUSH2 SWAP
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::PUSH1),
        static_cast<uint8_t>(Opcode::PUSH2),
        static_cast<uint8_t>(Opcode::SWAP)
    });
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 2);
    
    auto top = engine->ResultStack().Pop();
    auto bottom = engine->ResultStack().Pop();
    EXPECT_EQ(top->GetInteger(), 1);
    EXPECT_EQ(bottom->GetInteger(), 2);
}

TEST_F(ExecutionEngineTest, StackDrop) {
    // Create script: PUSH1 PUSH2 DROP
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::PUSH1),
        static_cast<uint8_t>(Opcode::PUSH2),
        static_cast<uint8_t>(Opcode::DROP)
    });
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 1);
    
    auto result = engine->ResultStack().Pop();
    EXPECT_EQ(result->GetInteger(), 1);
}

// Test control flow
TEST_F(ExecutionEngineTest, ConditionalJumpTrue) {
    // Create script: PUSH1 JMPIF [skip] PUSH0 [target] PUSH5
    std::vector<uint8_t> script_bytes = {
        static_cast<uint8_t>(Opcode::PUSH1),
        static_cast<uint8_t>(Opcode::JMPIF),
        0x02,  // Jump offset (skip PUSH0)
        static_cast<uint8_t>(Opcode::PUSH0),
        static_cast<uint8_t>(Opcode::PUSH5)
    };
    
    auto script = std::make_shared<Script>(script_bytes);
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 1);
    
    auto result = engine->ResultStack().Pop();
    EXPECT_EQ(result->GetInteger(), 5);
}

TEST_F(ExecutionEngineTest, ConditionalJumpFalse) {
    // Create script: PUSH0 JMPIF [skip] PUSH3 [target] PUSH5
    std::vector<uint8_t> script_bytes = {
        static_cast<uint8_t>(Opcode::PUSH0),
        static_cast<uint8_t>(Opcode::JMPIF),
        0x02,  // Jump offset (would skip PUSH3 if condition was true)
        static_cast<uint8_t>(Opcode::PUSH3),
        static_cast<uint8_t>(Opcode::PUSH5)
    };
    
    auto script = std::make_shared<Script>(script_bytes);
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 2);
    
    auto top = engine->ResultStack().Pop();
    auto bottom = engine->ResultStack().Pop();
    EXPECT_EQ(top->GetInteger(), 5);
    EXPECT_EQ(bottom->GetInteger(), 3);
}

// Test NOP operation
TEST_F(ExecutionEngineTest, NopOperation) {
    // Create script: PUSH1 NOP PUSH2
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::PUSH1),
        static_cast<uint8_t>(Opcode::NOP),
        static_cast<uint8_t>(Opcode::PUSH2)
    });
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 2);
}

// Test error conditions
TEST_F(ExecutionEngineTest, StackUnderflow) {
    // Create script: ADD (without pushing any values)
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::ADD)
    });
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::FAULT);
}

TEST_F(ExecutionEngineTest, DivisionByZero) {
    // Create script: PUSH1 PUSH0 DIV
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::PUSH1),
        static_cast<uint8_t>(Opcode::PUSH0),
        static_cast<uint8_t>(Opcode::DIV)
    });
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::FAULT);
}

// Test nested calls
TEST_F(ExecutionEngineTest, CallAndReturn) {
    // Create script with CALL and RET
    std::vector<uint8_t> script_bytes = {
        static_cast<uint8_t>(Opcode::PUSH1),
        static_cast<uint8_t>(Opcode::CALL),
        0x04,  // Jump to subroutine
        static_cast<uint8_t>(Opcode::PUSH2),
        static_cast<uint8_t>(Opcode::RET),
        // Subroutine
        static_cast<uint8_t>(Opcode::PUSH3),
        static_cast<uint8_t>(Opcode::RET)
    };
    
    auto script = std::make_shared<Script>(script_bytes);
    
    engine->LoadScript(script);
    engine->Execute();
    
    EXPECT_EQ(engine->State(), VMState::HALT);
    // Should have PUSH1, PUSH3 (from subroutine), PUSH2
    EXPECT_EQ(engine->ResultStack().Count(), 3);
}

// Test limits
TEST_F(ExecutionEngineTest, MaxStackSize) {
    std::vector<uint8_t> script_bytes;
    
    // Push many items to exceed stack limit
    for (int i = 0; i < 2050; i++) {
        script_bytes.push_back(static_cast<uint8_t>(Opcode::PUSH1));
    }
    
    auto script = std::make_shared<Script>(script_bytes);
    
    engine->LoadScript(script);
    engine->Execute();
    
    // Should fault due to stack overflow
    EXPECT_EQ(engine->State(), VMState::FAULT);
}

// Test step execution
TEST_F(ExecutionEngineTest, StepExecution) {
    // Create script: PUSH1 PUSH2 ADD
    auto script = CreateScript({
        static_cast<uint8_t>(Opcode::PUSH1),
        static_cast<uint8_t>(Opcode::PUSH2),
        static_cast<uint8_t>(Opcode::ADD)
    });
    
    engine->LoadScript(script);
    
    // Step 1: PUSH1
    engine->StepInto();
    EXPECT_EQ(engine->State(), VMState::NONE);
    EXPECT_EQ(engine->ResultStack().Count(), 1);
    
    // Step 2: PUSH2
    engine->StepInto();
    EXPECT_EQ(engine->State(), VMState::NONE);
    EXPECT_EQ(engine->ResultStack().Count(), 2);
    
    // Step 3: ADD
    engine->StepInto();
    EXPECT_EQ(engine->State(), VMState::HALT);
    EXPECT_EQ(engine->ResultStack().Count(), 1);
    
    auto result = engine->ResultStack().Pop();
    EXPECT_EQ(result->GetInteger(), 3);
}

} // namespace tests
} // namespace vm
} // namespace neo