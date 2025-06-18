#include <gtest/gtest.h>
#include <neo/vm/debugger.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script.h>
#include <neo/io/byte_vector.h>

using namespace neo::vm;
using namespace neo::io;

// Test fixture for Debugger tests
class DebuggerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create a simple script with PUSH1, PUSH2, ADD operations
        script_ = Script(ByteVector::Parse("111293"));
        engine_ = std::make_unique<ExecutionEngine>();
        debugger_ = std::make_unique<Debugger>(*engine_);
    }

    Script script_;
    std::unique_ptr<ExecutionEngine> engine_;
    std::unique_ptr<Debugger> debugger_;
};

// Test StepInto functionality
TEST_F(DebuggerTest, StepInto)
{
    // Load script into engine
    engine_->LoadScript(script_);
    
    // Initially in BREAK state
    EXPECT_EQ(engine_->GetState(), VMState::Break);
    
    // Step into first instruction (PUSH1)
    VMState state = debugger_->StepInto();
    
    // Should still be in BREAK state
    EXPECT_EQ(state, VMState::Break);
    
    // Stack should have one item with value 1
    EXPECT_EQ(engine_->GetCurrentContext().GetEvaluationStack().size(), 1);
    EXPECT_EQ(engine_->GetCurrentContext().Peek()->GetInteger(), 1);
    
    // Step into second instruction (PUSH2)
    state = debugger_->StepInto();
    
    // Should still be in BREAK state
    EXPECT_EQ(state, VMState::Break);
    
    // Stack should have two items with values 1 and 2
    EXPECT_EQ(engine_->GetCurrentContext().GetEvaluationStack().size(), 2);
    EXPECT_EQ(engine_->GetCurrentContext().Peek()->GetInteger(), 2);
    EXPECT_EQ(engine_->GetCurrentContext().Peek(1)->GetInteger(), 1);
    
    // Step into third instruction (ADD)
    state = debugger_->StepInto();
    
    // Should still be in BREAK state
    EXPECT_EQ(state, VMState::Break);
    
    // Stack should have one item with value 3 (1+2)
    EXPECT_EQ(engine_->GetCurrentContext().GetEvaluationStack().size(), 1);
    EXPECT_EQ(engine_->GetCurrentContext().Peek()->GetInteger(), 3);
    
    // Script is now complete, stepping again should halt
    state = debugger_->StepInto();
    
    // Should now be in HALT state
    EXPECT_EQ(state, VMState::Halt);
}

// Test StepOut functionality
TEST_F(DebuggerTest, StepOut)
{
    // Create a script with a CALL to a function that pushes two values and adds them
    // Main script: PUSH1, CALL (to func), RET
    // Function: PUSH2, PUSH3, ADD, RET
    auto mainScript = ByteVector::Parse("1134040040");
    auto funcStartPos = 2;  // Position of function start (PUSH2)
    
    Script script(mainScript);
    auto engine = std::make_unique<ExecutionEngine>();
    auto debugger = std::make_unique<Debugger>(*engine);
    
    // Load script into engine
    engine->LoadScript(script);
    
    // Step into first instruction (PUSH1)
    VMState state = debugger->StepInto();
    EXPECT_EQ(state, VMState::Break);
    EXPECT_EQ(engine->GetCurrentContext().GetEvaluationStack().size(), 1);
    
    // Step into CALL instruction, which should create a new frame for the function
    state = debugger->StepInto();
    EXPECT_EQ(state, VMState::Break);
    EXPECT_EQ(engine->GetInvocationStack().size(), 2);
    
    // Now we're at the beginning of the function, step out should execute until the function returns
    state = debugger->StepOut();
    EXPECT_EQ(state, VMState::Break);
    
    // Should be back to main script with one stack frame
    EXPECT_EQ(engine->GetInvocationStack().size(), 1);
    
    // Stack should have the result of the function (5)
    EXPECT_EQ(engine->GetCurrentContext().GetEvaluationStack().size(), 2);
    EXPECT_EQ(engine->GetCurrentContext().Peek()->GetInteger(), 5);
}

// Test StepOver functionality
TEST_F(DebuggerTest, StepOver)
{
    // Create a script with a CALL to a function
    // Main script: PUSH1, CALL (to func), PUSH4, ADD, RET
    // Function: PUSH2, PUSH3, ADD, RET
    auto mainScript = ByteVector::Parse("113404145E40");
    auto funcStartPos = 2;  // Position of function start (PUSH2)
    
    Script script(mainScript);
    auto engine = std::make_unique<ExecutionEngine>();
    auto debugger = std::make_unique<Debugger>(*engine);
    
    // Load script into engine
    engine->LoadScript(script);
    
    // Step into first instruction (PUSH1)
    VMState state = debugger->StepInto();
    EXPECT_EQ(state, VMState::Break);
    EXPECT_EQ(engine->GetCurrentContext().GetEvaluationStack().size(), 1);
    
    // Step over CALL instruction, which should execute the entire function
    state = debugger->StepOver();
    EXPECT_EQ(state, VMState::Break);
    
    // Should still be in main script with result of function call
    EXPECT_EQ(engine->GetInvocationStack().size(), 1);
    EXPECT_EQ(engine->GetCurrentContext().GetEvaluationStack().size(), 2);
    EXPECT_EQ(engine->GetCurrentContext().Peek()->GetInteger(), 5);
    
    // Step over PUSH4
    state = debugger->StepOver();
    EXPECT_EQ(state, VMState::Break);
    EXPECT_EQ(engine->GetCurrentContext().GetEvaluationStack().size(), 3);
    EXPECT_EQ(engine->GetCurrentContext().Peek()->GetInteger(), 4);
    
    // Step over ADD
    state = debugger->StepOver();
    EXPECT_EQ(state, VMState::Break);
    EXPECT_EQ(engine->GetCurrentContext().GetEvaluationStack().size(), 2);
    EXPECT_EQ(engine->GetCurrentContext().Peek()->GetInteger(), 9); // 5 + 4 = 9
}

// Test Continue functionality
TEST_F(DebuggerTest, Continue)
{
    // Load script into engine
    engine_->LoadScript(script_);
    
    // Initially in BREAK state
    EXPECT_EQ(engine_->GetState(), VMState::Break);
    
    // Continue execution until completion
    VMState state = debugger_->Continue();
    
    // Should now be in HALT state
    EXPECT_EQ(state, VMState::Halt);
    
    // Stack should have one item with value 3 (1+2)
    EXPECT_EQ(engine_->GetResultStack().size(), 1);
    EXPECT_EQ(engine_->GetResultStack()[0]->GetInteger(), 3);
} 