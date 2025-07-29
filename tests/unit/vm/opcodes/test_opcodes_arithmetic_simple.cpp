#include <gtest/gtest.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/stack_item.h>

using namespace neo::vm;

class UT_OpCodes_Arithmetic_Simple : public testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(UT_OpCodes_Arithmetic_Simple, ADD)
{
    // Test simple addition: 2 + 3 = 5
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(2));
    sb.EmitPush(static_cast<int64_t>(3));
    sb.Emit(OpCode::ADD);

    ExecutionEngine engine;
    auto bytes = sb.ToArray();
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    auto resultStack = engine.GetResultStack();
    EXPECT_EQ(resultStack.size(), 1);

    auto result = resultStack[0];
    EXPECT_EQ(result->GetInteger(), 5);
}

TEST_F(UT_OpCodes_Arithmetic_Simple, SUB)
{
    // Test subtraction: 5 - 3 = 2
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(5));
    sb.EmitPush(static_cast<int64_t>(3));
    sb.Emit(OpCode::SUB);

    ExecutionEngine engine;
    auto bytes = sb.ToArray();
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    auto resultStack = engine.GetResultStack();
    EXPECT_EQ(resultStack.size(), 1);

    auto result = resultStack[0];
    EXPECT_EQ(result->GetInteger(), 2);
}

TEST_F(UT_OpCodes_Arithmetic_Simple, MUL)
{
    // Test multiplication: 4 * 3 = 12
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(4));
    sb.EmitPush(static_cast<int64_t>(3));
    sb.Emit(OpCode::MUL);

    ExecutionEngine engine;
    auto bytes = sb.ToArray();
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    auto resultStack = engine.GetResultStack();
    EXPECT_EQ(resultStack.size(), 1);

    auto result = resultStack[0];
    EXPECT_EQ(result->GetInteger(), 12);
}

TEST_F(UT_OpCodes_Arithmetic_Simple, DIV)
{
    // Test division: 12 / 3 = 4
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(12));
    sb.EmitPush(static_cast<int64_t>(3));
    sb.Emit(OpCode::DIV);

    ExecutionEngine engine;
    auto bytes = sb.ToArray();
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    auto resultStack = engine.GetResultStack();
    EXPECT_EQ(resultStack.size(), 1);

    auto result = resultStack[0];
    EXPECT_EQ(result->GetInteger(), 4);
}

TEST_F(UT_OpCodes_Arithmetic_Simple, MOD)
{
    // Test modulo: 10 % 3 = 1
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(10));
    sb.EmitPush(static_cast<int64_t>(3));
    sb.Emit(OpCode::MOD);

    ExecutionEngine engine;
    auto bytes = sb.ToArray();
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    auto resultStack = engine.GetResultStack();
    EXPECT_EQ(resultStack.size(), 1);

    auto result = resultStack[0];
    EXPECT_EQ(result->GetInteger(), 1);
}

TEST_F(UT_OpCodes_Arithmetic_Simple, POW)
{
    // Test power: 2^3 = 8
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(2));
    sb.EmitPush(static_cast<int64_t>(3));
    sb.Emit(OpCode::POW);

    ExecutionEngine engine;
    auto bytes = sb.ToArray();
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    auto resultStack = engine.GetResultStack();
    EXPECT_EQ(resultStack.size(), 1);

    auto result = resultStack[0];
    EXPECT_EQ(result->GetInteger(), 8);
}

TEST_F(UT_OpCodes_Arithmetic_Simple, NEGATE)
{
    // Test negation: -5
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(5));
    sb.Emit(OpCode::NEGATE);

    ExecutionEngine engine;
    auto bytes = sb.ToArray();
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    auto resultStack = engine.GetResultStack();
    EXPECT_EQ(resultStack.size(), 1);

    auto result = resultStack[0];
    EXPECT_EQ(result->GetInteger(), -5);
}

TEST_F(UT_OpCodes_Arithmetic_Simple, ABS)
{
    // Test absolute value: abs(-5) = 5
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(-5));
    sb.Emit(OpCode::ABS);

    ExecutionEngine engine;
    auto bytes = sb.ToArray();
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    auto resultStack = engine.GetResultStack();
    EXPECT_EQ(resultStack.size(), 1);

    auto result = resultStack[0];
    EXPECT_EQ(result->GetInteger(), 5);
}

TEST_F(UT_OpCodes_Arithmetic_Simple, INC)
{
    // Test increment: 5++ = 6
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(5));
    sb.Emit(OpCode::INC);

    ExecutionEngine engine;
    auto bytes = sb.ToArray();
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    auto resultStack = engine.GetResultStack();
    EXPECT_EQ(resultStack.size(), 1);

    auto result = resultStack[0];
    EXPECT_EQ(result->GetInteger(), 6);
}

TEST_F(UT_OpCodes_Arithmetic_Simple, DEC)
{
    // Test decrement: 5-- = 4
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(5));
    sb.Emit(OpCode::DEC);

    ExecutionEngine engine;
    auto bytes = sb.ToArray();
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    auto resultStack = engine.GetResultStack();
    EXPECT_EQ(resultStack.size(), 1);

    auto result = resultStack[0];
    EXPECT_EQ(result->GetInteger(), 4);
}

TEST_F(UT_OpCodes_Arithmetic_Simple, PUSH_AND_DROP)
{
    // Test basic push and drop operations
    ScriptBuilder sb;
    sb.EmitPush(static_cast<int64_t>(42));
    sb.EmitPush(static_cast<int64_t>(100));
    sb.Emit(OpCode::DROP);  // Drop the 100, leaving 42

    ExecutionEngine engine;
    auto bytes = sb.ToArray();
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    engine.LoadScript(script);

    EXPECT_EQ(engine.Execute(), VMState::Halt);
    auto resultStack = engine.GetResultStack();
    EXPECT_EQ(resultStack.size(), 1);

    auto result = resultStack[0];
    EXPECT_EQ(result->GetInteger(), 42);
}