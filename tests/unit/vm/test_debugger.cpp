#include <gtest/gtest.h>

#include <neo/vm/debugger.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script.h>
#include <neo/vm/script_builder.h>

namespace neo::vm::tests
{
class DebuggerTest : public ::testing::Test
{
  protected:
    void SetUp() override { engine_ = std::make_unique<ExecutionEngine>(); }

    std::unique_ptr<ExecutionEngine> engine_;
};

TEST_F(DebuggerTest, BreakpointsPauseExecution)
{
    ScriptBuilder builder;
    builder.Emit(OpCode::NOP);
    builder.Emit(OpCode::NOP);
    builder.Emit(OpCode::NOP);
    builder.Emit(OpCode::NOP);

    Script script(builder.ToArray());
    engine_->LoadScript(script);

    Debugger debugger(*engine_);

    const auto& contextScript = engine_->GetCurrentContext().GetScript();
    EXPECT_FALSE(debugger.RemoveBreakPoint(contextScript, 3));

    EXPECT_EQ(OpCode::NOP, engine_->GetCurrentContext().GetNextInstructionOpCode());

    debugger.AddBreakPoint(contextScript, 2);
    debugger.AddBreakPoint(contextScript, 3);

    EXPECT_EQ(VMState::Break, debugger.Execute());
    EXPECT_EQ(OpCode::NOP, engine_->GetCurrentContext().GetNextInstructionOpCode());
    EXPECT_EQ(2, engine_->GetCurrentContext().GetInstructionPointer());
    EXPECT_EQ(VMState::Break, engine_->State());

    EXPECT_TRUE(debugger.RemoveBreakPoint(contextScript, 2));
    EXPECT_FALSE(debugger.RemoveBreakPoint(contextScript, 2));
    EXPECT_TRUE(debugger.RemoveBreakPoint(contextScript, 3));
    EXPECT_FALSE(debugger.RemoveBreakPoint(contextScript, 3));

    EXPECT_EQ(VMState::Halt, debugger.Execute());
    EXPECT_EQ(VMState::Halt, engine_->State());
}

TEST_F(DebuggerTest, ExecuteWithoutBreakpointsRunsToCompletion)
{
    ScriptBuilder builder;
    builder.Emit(OpCode::NOP);
    builder.Emit(OpCode::NOP);
    builder.Emit(OpCode::NOP);
    builder.Emit(OpCode::NOP);

    Script script(builder.ToArray());
    engine_->LoadScript(script);

    Debugger debugger(*engine_);

    EXPECT_EQ(OpCode::NOP, engine_->GetCurrentContext().GetNextInstructionOpCode());
    EXPECT_EQ(VMState::Halt, debugger.Execute());
    EXPECT_TRUE(engine_->GetInvocationStack().empty());
    EXPECT_EQ(VMState::Halt, engine_->State());
}

TEST_F(DebuggerTest, ExecutionWithoutDebuggerHalts)
{
    ScriptBuilder builder;
    builder.Emit(OpCode::NOP);
    builder.Emit(OpCode::NOP);
    builder.Emit(OpCode::NOP);
    builder.Emit(OpCode::NOP);

    Script script(builder.ToArray());
    engine_->LoadScript(script);

    EXPECT_EQ(OpCode::NOP, engine_->GetCurrentContext().GetNextInstructionOpCode());
    EXPECT_EQ(VMState::Halt, engine_->Execute());
    EXPECT_TRUE(engine_->GetInvocationStack().empty());
    EXPECT_EQ(VMState::Halt, engine_->State());
}

TEST_F(DebuggerTest, StepOverSkipsMethodBodies)
{
    ScriptBuilder builder;
    builder.EmitCall(4);
    builder.Emit(OpCode::NOT);
    builder.Emit(OpCode::RET);
    builder.Emit(OpCode::PUSH0);
    builder.Emit(OpCode::RET);

    Script script(builder.ToArray());
    engine_->LoadScript(script);

    Debugger debugger(*engine_);

    auto nextInstruction = engine_->GetCurrentContext().GetNextInstructionObject();
    ASSERT_NE(nextInstruction, nullptr);
    EXPECT_EQ(OpCode::NOT, nextInstruction->opcode);

    EXPECT_EQ(VMState::Break, debugger.StepOver());
    EXPECT_EQ(2, engine_->GetCurrentContext().GetInstructionPointer());
    nextInstruction = engine_->GetCurrentContext().GetNextInstructionObject();
    ASSERT_NE(nextInstruction, nullptr);
    EXPECT_EQ(OpCode::RET, nextInstruction->opcode);
    EXPECT_EQ(VMState::Break, engine_->State());

    EXPECT_EQ(VMState::Halt, debugger.Execute());
    auto result = engine_->ResultStack().Pop();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->GetBoolean());
    EXPECT_EQ(VMState::Halt, engine_->State());

    EXPECT_EQ(VMState::Halt, debugger.StepOver());
    EXPECT_EQ(VMState::Halt, engine_->State());
}

TEST_F(DebuggerTest, StepIntoTraversesCalls)
{
    ScriptBuilder builder;
    builder.EmitCall(4);
    builder.Emit(OpCode::NOT);
    builder.Emit(OpCode::RET);
    builder.Emit(OpCode::PUSH0);
    builder.Emit(OpCode::RET);

    Script script(builder.ToArray());
    engine_->LoadScript(script);

    Debugger debugger(*engine_);

    auto entryContext = engine_->GetInvocationStack().back();
    ASSERT_TRUE(entryContext);
    EXPECT_EQ(entryContext.get(), &engine_->GetCurrentContext());
    EXPECT_EQ(entryContext.get(), engine_->GetEntryContext().get());

    EXPECT_EQ(VMState::Break, debugger.StepInto());
    EXPECT_NE(entryContext.get(), &engine_->GetCurrentContext());
    EXPECT_EQ(entryContext.get(), engine_->GetEntryContext().get());

    auto nextInstruction = engine_->GetCurrentContext().GetNextInstructionObject();
    ASSERT_NE(nextInstruction, nullptr);
    EXPECT_EQ(OpCode::RET, nextInstruction->opcode);

    EXPECT_EQ(VMState::Break, debugger.StepInto());
    EXPECT_EQ(VMState::Break, debugger.StepInto());

    EXPECT_EQ(entryContext.get(), &engine_->GetCurrentContext());
    EXPECT_EQ(entryContext.get(), engine_->GetEntryContext().get());

    nextInstruction = engine_->GetCurrentContext().GetNextInstructionObject();
    ASSERT_NE(nextInstruction, nullptr);
    EXPECT_EQ(OpCode::RET, nextInstruction->opcode);

    EXPECT_EQ(VMState::Break, debugger.StepInto());
    EXPECT_EQ(VMState::Halt, debugger.StepInto());

    auto result = engine_->ResultStack().Pop();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->GetBoolean());
    EXPECT_EQ(VMState::Halt, engine_->State());

    EXPECT_EQ(VMState::Halt, debugger.StepInto());
    EXPECT_EQ(VMState::Halt, engine_->State());
}

TEST_F(DebuggerTest, StepOutReturnsToCallerContext)
{
    ScriptBuilder builder;
    builder.EmitCall(4);
    builder.Emit(OpCode::NOT);
    builder.Emit(OpCode::RET);
    builder.Emit(OpCode::PUSH0);
    builder.Emit(OpCode::RET);

    Script script(builder.ToArray());
    engine_->LoadScript(script);

    Debugger debugger(*engine_);

    auto entryContext = engine_->GetInvocationStack().back();
    ASSERT_TRUE(entryContext);

    // Enter the called context.
    EXPECT_EQ(VMState::Break, debugger.StepInto());
    EXPECT_NE(entryContext.get(), &engine_->GetCurrentContext());

    // Step out should return to the caller and pause in Break state.
    EXPECT_EQ(VMState::Break, debugger.StepOut());
    EXPECT_EQ(entryContext.get(), &engine_->GetCurrentContext());
    EXPECT_EQ(VMState::Break, engine_->State());

    auto nextInstruction = engine_->GetCurrentContext().GetNextInstructionObject();
    ASSERT_NE(nextInstruction, nullptr);
    EXPECT_EQ(OpCode::RET, nextInstruction->opcode);

    // Finish execution to confirm the returned value propagates correctly.
    EXPECT_EQ(VMState::Halt, debugger.Execute());
    auto result = engine_->ResultStack().Pop();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->GetBoolean());
}
}  // namespace neo::vm::tests
