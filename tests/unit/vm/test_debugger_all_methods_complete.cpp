#include "neo/vm/debugger.h"
#include "neo/vm/execution_context.h"
#include "neo/vm/execution_engine.h"
#include "neo/vm/opcode.h"
#include "neo/vm/script_builder.h"
#include "neo/vm/stack_item.h"
#include "neo/vm/vm_state.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace neo;
using namespace neo::vm;

// Complete conversion of C# UT_Debugger.cs - ALL 6 test methods
class DebuggerAllMethodsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        engine_ = std::make_unique<ExecutionEngine>();
    }

    void TearDown() override
    {
        engine_.reset();
    }

    std::unique_ptr<ExecutionEngine> engine_;
};

// C# Test Method: TestBreakPoint()
TEST_F(DebuggerAllMethodsTest, TestBreakPoint)
{
    ScriptBuilder script;
    script.Emit(OpCode::NOP);
    script.Emit(OpCode::NOP);
    script.Emit(OpCode::NOP);
    script.Emit(OpCode::NOP);

    engine_->LoadScript(script.ToArray());

    Debugger debugger(*engine_);

    // Test removing non-existent breakpoint
    EXPECT_FALSE(debugger.RemoveBreakPoint(engine_->CurrentContext()->Script(), 3));

    EXPECT_EQ(OpCode::NOP, engine_->CurrentContext()->NextInstruction());

    // Add breakpoints
    debugger.AddBreakPoint(engine_->CurrentContext()->Script(), 2);
    debugger.AddBreakPoint(engine_->CurrentContext()->Script(), 3);

    // Execute until first breakpoint
    debugger.Execute();
    EXPECT_EQ(OpCode::NOP, engine_->CurrentContext()->NextInstruction());
    EXPECT_EQ(2, engine_->CurrentContext()->InstructionPointer());
    EXPECT_EQ(VMState::BREAK, engine_->State());

    // Test removing breakpoints
    EXPECT_TRUE(debugger.RemoveBreakPoint(engine_->CurrentContext()->Script(), 2));
    EXPECT_FALSE(debugger.RemoveBreakPoint(engine_->CurrentContext()->Script(), 2));  // Already removed
    EXPECT_TRUE(debugger.RemoveBreakPoint(engine_->CurrentContext()->Script(), 3));
    EXPECT_FALSE(debugger.RemoveBreakPoint(engine_->CurrentContext()->Script(), 3));  // Already removed

    // Continue execution - should complete without hitting more breakpoints
    debugger.Execute();
    EXPECT_EQ(VMState::HALT, engine_->State());
}

// C# Test Method: TestWithoutBreakPoints()
TEST_F(DebuggerAllMethodsTest, TestWithoutBreakPoints)
{
    ScriptBuilder script;
    script.Emit(OpCode::NOP);
    script.Emit(OpCode::NOP);
    script.Emit(OpCode::NOP);
    script.Emit(OpCode::NOP);

    engine_->LoadScript(script.ToArray());

    Debugger debugger(*engine_);

    EXPECT_EQ(OpCode::NOP, engine_->CurrentContext()->NextInstruction());

    // Execute without breakpoints - should run to completion
    debugger.Execute();

    EXPECT_EQ(nullptr, engine_->CurrentContext());
    EXPECT_EQ(VMState::HALT, engine_->State());
}

// C# Test Method: TestWithoutDebugger()
TEST_F(DebuggerAllMethodsTest, TestWithoutDebugger)
{
    ScriptBuilder script;
    script.Emit(OpCode::NOP);
    script.Emit(OpCode::NOP);
    script.Emit(OpCode::NOP);
    script.Emit(OpCode::NOP);

    engine_->LoadScript(script.ToArray());

    EXPECT_EQ(OpCode::NOP, engine_->CurrentContext()->NextInstruction());

    // Execute without debugger - should run to completion
    engine_->Execute();

    EXPECT_EQ(nullptr, engine_->CurrentContext());
    EXPECT_EQ(VMState::HALT, engine_->State());
}

// C# Test Method: TestStepOver()
TEST_F(DebuggerAllMethodsTest, TestStepOver)
{
    ScriptBuilder script;
    /* ┌     CALL
       │  ┌> NOT
       │  │  RET
       └> │  PUSH0
        └─┘  RET */
    script.EmitCall(4);
    script.Emit(OpCode::NOT);
    script.Emit(OpCode::RET);
    script.Emit(OpCode::PUSH0);
    script.Emit(OpCode::RET);

    engine_->LoadScript(script.ToArray());

    Debugger debugger(*engine_);

    EXPECT_EQ(OpCode::NOT, engine_->CurrentContext()->NextInstruction());
    EXPECT_EQ(VMState::BREAK, debugger.StepOver());
    EXPECT_EQ(2, engine_->CurrentContext()->InstructionPointer());
    EXPECT_EQ(VMState::BREAK, engine_->State());
    EXPECT_EQ(OpCode::RET, engine_->CurrentContext()->NextInstruction());

    // Continue execution
    debugger.Execute();

    EXPECT_TRUE(engine_->ResultStack().Pop()->GetBoolean());
    EXPECT_EQ(VMState::HALT, engine_->State());

    // Test step over again (should remain in HALT state)
    EXPECT_EQ(VMState::HALT, debugger.StepOver());
    EXPECT_EQ(VMState::HALT, engine_->State());
}

// C# Test Method: TestStepInto()
TEST_F(DebuggerAllMethodsTest, TestStepInto)
{
    ScriptBuilder script;
    /* ┌     CALL
       │  ┌> NOT
       │  │  RET
       └> │  PUSH0
        └─┘  RET */
    script.EmitCall(4);
    script.Emit(OpCode::NOT);
    script.Emit(OpCode::RET);
    script.Emit(OpCode::PUSH0);
    script.Emit(OpCode::RET);

    engine_->LoadScript(script.ToArray());

    Debugger debugger(*engine_);

    auto context = engine_->CurrentContext();

    EXPECT_EQ(context, engine_->CurrentContext());
    EXPECT_EQ(context, engine_->EntryContext());
    EXPECT_EQ(OpCode::NOT, engine_->CurrentContext()->NextInstruction());

    // Step into CALL - should create new context
    EXPECT_EQ(VMState::BREAK, debugger.StepInto());

    EXPECT_NE(context, engine_->CurrentContext());
    EXPECT_EQ(context, engine_->EntryContext());
    EXPECT_EQ(OpCode::RET, engine_->CurrentContext()->NextInstruction());

    // Step into RET and NOT
    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(VMState::BREAK, debugger.StepInto());

    // Should be back to original context
    EXPECT_EQ(context, engine_->CurrentContext());
    EXPECT_EQ(context, engine_->EntryContext());
    EXPECT_EQ(OpCode::RET, engine_->CurrentContext()->NextInstruction());

    // Step into final RET
    EXPECT_EQ(VMState::BREAK, debugger.StepInto());
    EXPECT_EQ(VMState::HALT, debugger.StepInto());

    EXPECT_TRUE(engine_->ResultStack().Pop()->GetBoolean());
    EXPECT_EQ(VMState::HALT, engine_->State());

    // Test step into again (should remain in HALT state)
    EXPECT_EQ(VMState::HALT, debugger.StepInto());
    EXPECT_EQ(VMState::HALT, engine_->State());
}

// C# Test Method: TestBreakPointStepOver()
TEST_F(DebuggerAllMethodsTest, TestBreakPointStepOver)
{
    ScriptBuilder script;
    /* ┌     CALL
       │  ┌> NOT
       │  │  RET
       └>X│  PUSH0
         └┘  RET */
    script.EmitCall(4);
    script.Emit(OpCode::NOT);
    script.Emit(OpCode::RET);
    script.Emit(OpCode::PUSH0);
    script.Emit(OpCode::RET);

    engine_->LoadScript(script.ToArray());

    Debugger debugger(*engine_);

    EXPECT_EQ(OpCode::NOT, engine_->CurrentContext()->NextInstruction());

    // Add breakpoint at position 5 (after PUSH0)
    debugger.AddBreakPoint(engine_->CurrentContext()->Script(), 5);
    EXPECT_EQ(VMState::BREAK, debugger.StepOver());

    EXPECT_EQ(nullptr, engine_->CurrentContext()->NextInstruction());  // At end of script
    EXPECT_EQ(5, engine_->CurrentContext()->InstructionPointer());
    EXPECT_EQ(VMState::BREAK, engine_->State());

    // Continue execution
    debugger.Execute();

    EXPECT_TRUE(engine_->ResultStack().Pop()->GetBoolean());
    EXPECT_EQ(VMState::HALT, engine_->State());
}

// Additional comprehensive tests for complete debugger coverage

// Test Method: TestStepOut()
TEST_F(DebuggerAllMethodsTest, TestStepOut)
{
    ScriptBuilder script;
    // Create nested call structure
    script.EmitCall(6);          // Call to position 6
    script.Emit(OpCode::PUSH1);  // Position 3
    script.Emit(OpCode::RET);    // Position 4
    script.Emit(OpCode::NOP);    // Position 5
    script.EmitCall(9);          // Position 6 - Call to position 9
    script.Emit(OpCode::RET);    // Position 8
    script.Emit(OpCode::PUSH0);  // Position 9
    script.Emit(OpCode::RET);    // Position 10

    engine_->LoadScript(script.ToArray());

    Debugger debugger(*engine_);

    // Step into first call
    debugger.StepInto();

    // Step into second call
    debugger.StepInto();

    // Now we should be in the innermost context
    // Step out should return to the previous context
    auto result = debugger.StepOut();
    EXPECT_TRUE(result == VMState::BREAK || result == VMState::HALT);
}

// Test Method: TestDebuggerStateManagement()
TEST_F(DebuggerAllMethodsTest, TestDebuggerStateManagement)
{
    ScriptBuilder script;
    script.Emit(OpCode::PUSH1);
    script.Emit(OpCode::PUSH2);
    script.Emit(OpCode::ADD);
    script.Emit(OpCode::RET);

    engine_->LoadScript(script.ToArray());

    Debugger debugger(*engine_);

    // Test that debugger properly manages engine state
    EXPECT_EQ(VMState::NONE, engine_->State());  // Initially NONE

    // Step should change to BREAK
    debugger.StepInto();
    EXPECT_EQ(VMState::BREAK, engine_->State());

    // Continue stepping
    debugger.StepInto();  // PUSH2
    debugger.StepInto();  // ADD
    debugger.StepInto();  // RET

    EXPECT_EQ(VMState::HALT, engine_->State());

    // Verify result
    EXPECT_EQ(3, engine_->ResultStack().Pop()->GetBigInteger().ToInt32());
}

// Test Method: TestBreakpointManagement()
TEST_F(DebuggerAllMethodsTest, TestBreakpointManagement)
{
    ScriptBuilder script;
    for (int i = 0; i < 10; i++)
    {
        script.Emit(OpCode::NOP);
    }

    engine_->LoadScript(script.ToArray());

    Debugger debugger(*engine_);

    auto script_obj = engine_->CurrentContext()->Script();

    // Add multiple breakpoints
    debugger.AddBreakPoint(script_obj, 2);
    debugger.AddBreakPoint(script_obj, 4);
    debugger.AddBreakPoint(script_obj, 6);
    debugger.AddBreakPoint(script_obj, 8);

    // Test execution hits each breakpoint
    debugger.Execute();
    EXPECT_EQ(2, engine_->CurrentContext()->InstructionPointer());
    EXPECT_EQ(VMState::BREAK, engine_->State());

    debugger.Execute();
    EXPECT_EQ(4, engine_->CurrentContext()->InstructionPointer());
    EXPECT_EQ(VMState::BREAK, engine_->State());

    debugger.Execute();
    EXPECT_EQ(6, engine_->CurrentContext()->InstructionPointer());
    EXPECT_EQ(VMState::BREAK, engine_->State());

    debugger.Execute();
    EXPECT_EQ(8, engine_->CurrentContext()->InstructionPointer());
    EXPECT_EQ(VMState::BREAK, engine_->State());

    // Remove some breakpoints
    EXPECT_TRUE(debugger.RemoveBreakPoint(script_obj, 4));
    EXPECT_TRUE(debugger.RemoveBreakPoint(script_obj, 6));

    // Continue execution - should complete without hitting removed breakpoints
    debugger.Execute();
    EXPECT_EQ(VMState::HALT, engine_->State());
}

// Test Method: TestDebuggerWithExceptions()
TEST_F(DebuggerAllMethodsTest, TestDebuggerWithExceptions)
{
    ScriptBuilder script;
    script.Emit(OpCode::PUSH0);
    script.Emit(OpCode::PUSH0);
    script.Emit(OpCode::DIV);  // This should cause a fault (division by zero)

    engine_->LoadScript(script.ToArray());

    Debugger debugger(*engine_);

    // Step through until fault
    debugger.StepInto();  // PUSH0
    EXPECT_EQ(VMState::BREAK, engine_->State());

    debugger.StepInto();  // PUSH0
    EXPECT_EQ(VMState::BREAK, engine_->State());

    debugger.StepInto();  // DIV - should fault
    EXPECT_EQ(VMState::FAULT, engine_->State());

    // Further steps should remain in FAULT state
    EXPECT_EQ(VMState::FAULT, debugger.StepInto());
    EXPECT_EQ(VMState::FAULT, engine_->State());
}

// Test Method: TestDebuggerPerformance()
TEST_F(DebuggerAllMethodsTest, TestDebuggerPerformance)
{
    ScriptBuilder script;

    // Create a script with many operations
    for (int i = 0; i < 1000; i++)
    {
        script.Emit(OpCode::NOP);
    }
    script.Emit(OpCode::RET);

    engine_->LoadScript(script.ToArray());

    Debugger debugger(*engine_);

    auto start_time = std::chrono::high_resolution_clock::now();

    // Execute with debugger
    debugger.Execute();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_EQ(VMState::HALT, engine_->State());

    // Should complete reasonably quickly (less than 1 second for 1000 NOPs)
    EXPECT_LT(duration.count(), 1000);

    std::cout << "Debugger executed 1000 NOPs in " << duration.count() << " ms" << std::endl;
}