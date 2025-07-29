#include "neo/vm/execution_context.h"
#include "neo/vm/execution_engine.h"
#include "neo/vm/opcode.h"
#include "neo/vm/script_builder.h"
#include "neo/vm/stack_item.h"
#include <gtest/gtest.h>

using namespace neo;
using namespace neo::vm;

class ControlOpcodeTest : public ::testing::Test
{
  protected:
    std::unique_ptr<ExecutionEngine> engine;

    void SetUp() override
    {
        engine = std::make_unique<ExecutionEngine>();
    }

    void ExecuteScript(const ByteVector& script)
    {
        engine->LoadScript(script);
        engine->Execute();
    }

    void CheckState(VMState expected)
    {
        EXPECT_EQ(engine->State(), expected);
    }

    void CheckStackSize(size_t expected)
    {
        ASSERT_EQ(engine->EvaluationStack().size(), expected);
    }

    void CheckResult(const StackItem& expected)
    {
        ASSERT_EQ(engine->State(), VMState::HALT);
        ASSERT_EQ(engine->EvaluationStack().size(), 1);
        EXPECT_EQ(engine->EvaluationStack().Pop(), expected);
    }
};

// NOP Tests
TEST_F(ControlOpcodeTest, NOP_BasicOperation)
{
    ScriptBuilder sb;
    sb.EmitOpCode(OpCode::NOP);
    sb.EmitPush(42);

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 42);
}

// JMP Tests
TEST_F(ControlOpcodeTest, JMP_ForwardJump)
{
    ScriptBuilder sb;
    sb.EmitJump(OpCode::JMP, 5);  // Jump over PUSH1
    sb.EmitPush(1);               // This should be skipped
    sb.EmitPush(2);               // This should execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
}

TEST_F(ControlOpcodeTest, JMP_BackwardJump)
{
    ScriptBuilder sb;
    sb.EmitPush(0);  // Counter
    // Loop start (offset 2)
    auto loop_start = sb.Length();
    sb.EmitOpCode(OpCode::DUP);                                 // Duplicate counter
    sb.EmitPush(3);                                             // Target count
    sb.EmitOpCode(OpCode::LT);                                  // Check if counter < 3
    sb.EmitJump(OpCode::JMPIFNOT, 8);                           // Exit if counter >= 3
    sb.EmitOpCode(OpCode::INC);                                 // Increment counter
    sb.EmitJump(OpCode::JMP, -(sb.Length() - loop_start + 3));  // Jump back

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);
}

// JMPIF Tests
TEST_F(ControlOpcodeTest, JMPIF_TrueCondition)
{
    ScriptBuilder sb;
    sb.EmitPush(true);
    sb.EmitJump(OpCode::JMPIF, 3);  // Jump over PUSH1
    sb.EmitPush(1);                 // Should be skipped
    sb.EmitPush(2);                 // Should execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
}

TEST_F(ControlOpcodeTest, JMPIF_FalseCondition)
{
    ScriptBuilder sb;
    sb.EmitPush(false);
    sb.EmitJump(OpCode::JMPIF, 3);  // Should not jump
    sb.EmitPush(1);                 // Should execute
    sb.EmitPush(2);                 // Should also execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);
}

TEST_F(ControlOpcodeTest, JMPIF_NonBooleanCondition)
{
    ScriptBuilder sb;
    sb.EmitPush(1);                 // Non-zero integer (truthy)
    sb.EmitJump(OpCode::JMPIF, 3);  // Should jump
    sb.EmitPush(1);                 // Should be skipped
    sb.EmitPush(2);                 // Should execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
}

// JMPIFNOT Tests
TEST_F(ControlOpcodeTest, JMPIFNOT_FalseCondition)
{
    ScriptBuilder sb;
    sb.EmitPush(false);
    sb.EmitJump(OpCode::JMPIFNOT, 3);  // Should jump
    sb.EmitPush(1);                    // Should be skipped
    sb.EmitPush(2);                    // Should execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
}

TEST_F(ControlOpcodeTest, JMPIFNOT_TrueCondition)
{
    ScriptBuilder sb;
    sb.EmitPush(true);
    sb.EmitJump(OpCode::JMPIFNOT, 3);  // Should not jump
    sb.EmitPush(1);                    // Should execute
    sb.EmitPush(2);                    // Should also execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);
}

// JMPEQ Tests
TEST_F(ControlOpcodeTest, JMPEQ_EqualValues)
{
    ScriptBuilder sb;
    sb.EmitPush(5);
    sb.EmitPush(5);
    sb.EmitJump(OpCode::JMPEQ, 3);  // Should jump
    sb.EmitPush(1);                 // Should be skipped
    sb.EmitPush(2);                 // Should execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
}

TEST_F(ControlOpcodeTest, JMPEQ_UnequalValues)
{
    ScriptBuilder sb;
    sb.EmitPush(5);
    sb.EmitPush(3);
    sb.EmitJump(OpCode::JMPEQ, 3);  // Should not jump
    sb.EmitPush(1);                 // Should execute
    sb.EmitPush(2);                 // Should also execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);
}

// JMPNE Tests
TEST_F(ControlOpcodeTest, JMPNE_UnequalValues)
{
    ScriptBuilder sb;
    sb.EmitPush(5);
    sb.EmitPush(3);
    sb.EmitJump(OpCode::JMPNE, 3);  // Should jump
    sb.EmitPush(1);                 // Should be skipped
    sb.EmitPush(2);                 // Should execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
}

TEST_F(ControlOpcodeTest, JMPNE_EqualValues)
{
    ScriptBuilder sb;
    sb.EmitPush(5);
    sb.EmitPush(5);
    sb.EmitJump(OpCode::JMPNE, 3);  // Should not jump
    sb.EmitPush(1);                 // Should execute
    sb.EmitPush(2);                 // Should also execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);
}

// JMPGT Tests
TEST_F(ControlOpcodeTest, JMPGT_GreaterValue)
{
    ScriptBuilder sb;
    sb.EmitPush(3);
    sb.EmitPush(5);
    sb.EmitJump(OpCode::JMPGT, 3);  // 5 > 3, should jump
    sb.EmitPush(1);                 // Should be skipped
    sb.EmitPush(2);                 // Should execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
}

TEST_F(ControlOpcodeTest, JMPGT_LessOrEqualValue)
{
    ScriptBuilder sb;
    sb.EmitPush(5);
    sb.EmitPush(3);
    sb.EmitJump(OpCode::JMPGT, 3);  // 3 <= 5, should not jump
    sb.EmitPush(1);                 // Should execute
    sb.EmitPush(2);                 // Should also execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);
}

// JMPGE Tests
TEST_F(ControlOpcodeTest, JMPGE_GreaterOrEqualValue)
{
    ScriptBuilder sb;
    sb.EmitPush(5);
    sb.EmitPush(5);
    sb.EmitJump(OpCode::JMPGE, 3);  // 5 >= 5, should jump
    sb.EmitPush(1);                 // Should be skipped
    sb.EmitPush(2);                 // Should execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
}

// JMPLT Tests
TEST_F(ControlOpcodeTest, JMPLT_LessValue)
{
    ScriptBuilder sb;
    sb.EmitPush(5);
    sb.EmitPush(3);
    sb.EmitJump(OpCode::JMPLT, 3);  // 3 < 5, should jump
    sb.EmitPush(1);                 // Should be skipped
    sb.EmitPush(2);                 // Should execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
}

// JMPLE Tests
TEST_F(ControlOpcodeTest, JMPLE_LessOrEqualValue)
{
    ScriptBuilder sb;
    sb.EmitPush(5);
    sb.EmitPush(5);
    sb.EmitJump(OpCode::JMPLE, 3);  // 5 <= 5, should jump
    sb.EmitPush(1);                 // Should be skipped
    sb.EmitPush(2);                 // Should execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);
}

// CALL Tests
TEST_F(ControlOpcodeTest, CALL_SimpleCall)
{
    ScriptBuilder sb;
    sb.EmitCall(8);              // Call function at offset 8
    sb.EmitPush(1);              // Main body
    sb.EmitOpCode(OpCode::RET);  // Return from main
    // Function starts here (offset 8)
    sb.EmitPush(42);             // Function body
    sb.EmitOpCode(OpCode::RET);  // Return from function

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);   // Main result
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 42);  // Function result
}

TEST_F(ControlOpcodeTest, CALL_NestedCalls)
{
    ScriptBuilder sb;
    sb.EmitCall(10);             // Call function1
    sb.EmitPush(1);              // Main body
    sb.EmitOpCode(OpCode::RET);  // Return from main

    // Function1 starts here
    sb.EmitCall(6);              // Call function2 (relative)
    sb.EmitPush(2);              // Function1 body
    sb.EmitOpCode(OpCode::RET);  // Return from function1

    // Function2 starts here
    sb.EmitPush(3);              // Function2 body
    sb.EmitOpCode(OpCode::RET);  // Return from function2

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(3);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);  // Main
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);  // Function1
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 3);  // Function2
}

// CALLT Tests (Call Token)
TEST_F(ControlOpcodeTest, CALLT_ValidToken)
{
    // CALLT requires a valid token in the token table
    // This test assumes token 0 is defined
    ScriptBuilder sb;
    sb.EmitOpCode(OpCode::CALLT, {0x00, 0x00});  // Call token 0
    sb.EmitPush(1);

    ExecuteScript(sb.ToByteArray());
    // Result depends on what token 0 does
    // Should not fault if token is valid
    EXPECT_NE(engine->State(), VMState::FAULT);
}

// RET Tests
TEST_F(ControlOpcodeTest, RET_SimpleReturn)
{
    ScriptBuilder sb;
    sb.EmitPush(42);
    sb.EmitOpCode(OpCode::RET);
    sb.EmitPush(1);  // Should not execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 42);
}

// SYSCALL Tests
TEST_F(ControlOpcodeTest, SYSCALL_ValidCall)
{
    ScriptBuilder sb;
    sb.EmitSysCall("System.Runtime.Platform");  // Example syscall

    ExecuteScript(sb.ToByteArray());
    // Should execute without faulting
    EXPECT_NE(engine->State(), VMState::FAULT);
}

TEST_F(ControlOpcodeTest, SYSCALL_InvalidCall)
{
    ScriptBuilder sb;
    sb.EmitSysCall("Invalid.Syscall.Name");

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// TRY/CATCH/FINALLY Tests
TEST_F(ControlOpcodeTest, TRY_CATCH_WithException)
{
    ScriptBuilder sb;
    sb.EmitOpCode(OpCode::TRY, {0x08, 0x0C});  // try_offset=8, catch_offset=12
    sb.EmitOpCode(OpCode::THROW);              // Throw exception
    sb.EmitOpCode(OpCode::ENDTRY, {0x04});     // endtry_offset=4
    sb.EmitPush(1);                            // Normal path (skipped)
    sb.EmitOpCode(OpCode::RET);
    // Catch block starts here (offset 12)
    sb.EmitPush(2);                         // Exception handler
    sb.EmitOpCode(OpCode::ENDTRY, {0x00});  // End try

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);  // Catch block executed
}

TEST_F(ControlOpcodeTest, TRY_CATCH_NoException)
{
    ScriptBuilder sb;
    sb.EmitOpCode(OpCode::TRY, {0x08, 0x0C});  // try_offset=8, catch_offset=12
    sb.EmitPush(1);                            // Normal execution
    sb.EmitOpCode(OpCode::ENDTRY, {0x08});     // Skip catch block
    sb.EmitOpCode(OpCode::RET);
    // Catch block starts here (would be skipped)
    sb.EmitPush(2);
    sb.EmitOpCode(OpCode::ENDTRY, {0x00});

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 1);  // Normal path
}

// THROW Tests
TEST_F(ControlOpcodeTest, THROW_CausesException)
{
    ScriptBuilder sb;
    sb.EmitPush("Error message");
    sb.EmitOpCode(OpCode::THROW);
    sb.EmitPush(1);  // Should not execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// ABORT Tests
TEST_F(ControlOpcodeTest, ABORT_TerminatesExecution)
{
    ScriptBuilder sb;
    sb.EmitPush(42);
    sb.EmitOpCode(OpCode::ABORT);
    sb.EmitPush(1);  // Should not execute

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::FAULT);
}

// Complex Control Flow Tests
TEST_F(ControlOpcodeTest, ComplexControlFlow_FactorialCalculation)
{
    // Calculate 5! = 120 using loops and calls
    ScriptBuilder sb;
    sb.EmitPush(5);              // n
    sb.EmitCall(8);              // Call factorial function
    sb.EmitOpCode(OpCode::RET);  // Return result

    // Factorial function
    sb.EmitOpCode(OpCode::DUP);        // Duplicate n
    sb.EmitPush(1);                    // 1
    sb.EmitOpCode(OpCode::LE);         // n <= 1?
    sb.EmitJump(OpCode::JMPIFNOT, 3);  // If not, continue
    sb.EmitOpCode(OpCode::RET);        // Return n (base case)

    // Recursive case: n * factorial(n-1)
    sb.EmitOpCode(OpCode::DUP);  // Duplicate n
    sb.EmitOpCode(OpCode::DUP);  // Duplicate n again
    sb.EmitOpCode(OpCode::DEC);  // n-1
    sb.EmitCall(-16);            // Recursive call (go back to function start)
    sb.EmitOpCode(OpCode::MUL);  // n * factorial(n-1)
    sb.EmitOpCode(OpCode::RET);  // Return result

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(1);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 120);
}

TEST_F(ControlOpcodeTest, ExceptionHandling_NestedTryCatch)
{
    ScriptBuilder sb;

    // Outer try
    sb.EmitOpCode(OpCode::TRY, {0x10, 0x18});  // try_offset=16, catch_offset=24

    // Inner try
    sb.EmitOpCode(OpCode::TRY, {0x08, 0x0C});  // try_offset=8, catch_offset=12
    sb.EmitOpCode(OpCode::THROW);              // Throw in inner try
    sb.EmitOpCode(OpCode::ENDTRY, {0x04});     // End inner try
    sb.EmitPush(1);                            // Should be skipped

    // Inner catch
    sb.EmitPush(2);                         // Inner catch block
    sb.EmitOpCode(OpCode::THROW);           // Re-throw to outer
    sb.EmitOpCode(OpCode::ENDTRY, {0x04});  // End inner catch

    sb.EmitOpCode(OpCode::ENDTRY, {0x04});  // End outer try
    sb.EmitPush(3);                         // Should be skipped

    // Outer catch
    sb.EmitPush(4);                         // Outer catch block
    sb.EmitOpCode(OpCode::ENDTRY, {0x00});  // End outer catch

    ExecuteScript(sb.ToByteArray());
    CheckState(VMState::HALT);
    CheckStackSize(2);
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 4);  // Outer catch
    EXPECT_EQ(engine->EvaluationStack().Pop()->GetInteger(), 2);  // Inner catch
}