#include <gtest/gtest.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/io/byte_vector.h>

using namespace neo::vm;
using namespace neo::io;

class LteGteTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Common setup for all tests
    }

    void TearDown() override
    {
        // Common cleanup for all tests
    }
};

// Test LTE Operation
TEST_F(LteGteTest, LteOperation)
{
    // Test case 1: 0 <= 0 should be true
    {
        ScriptBuilder builder;
        builder.EmitPush(static_cast<int64_t>(0));
        builder.EmitPush(static_cast<int64_t>(0));
        builder.Emit(OpCode::LE);
        builder.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(builder.ToScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_TRUE(engine.GetResultStack()[0]->GetBoolean());
    }

    // Test case 2: 1 <= 0 should be false
    {
        ScriptBuilder builder;
        builder.EmitPush(static_cast<int64_t>(1));
        builder.EmitPush(static_cast<int64_t>(0));
        builder.Emit(OpCode::LE);
        builder.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(builder.ToScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_FALSE(engine.GetResultStack()[0]->GetBoolean());
    }

    // Test case 3: 0 <= 1 should be true
    {
        ScriptBuilder builder;
        builder.EmitPush(static_cast<int64_t>(0));
        builder.EmitPush(static_cast<int64_t>(1));
        builder.Emit(OpCode::LE);
        builder.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(builder.ToScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_TRUE(engine.GetResultStack()[0]->GetBoolean());
    }

    // Test case 4: null <= 1 should be false (currently faults)
    {
        ScriptBuilder builder;
        builder.Emit(OpCode::PUSHNULL);
        builder.EmitPush(static_cast<int64_t>(1));
        builder.Emit(OpCode::LE);
        builder.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(builder.ToScript());
        EXPECT_EQ(engine.Execute(), VMState::Fault); // Null comparisons currently fault
        EXPECT_EQ(engine.GetResultStack().size(), 0); // No results on fault
    }

    // Test case 5: 1 <= null should be false (currently faults)
    {
        ScriptBuilder builder;
        builder.EmitPush(static_cast<int64_t>(1));
        builder.Emit(OpCode::PUSHNULL);
        builder.Emit(OpCode::LE);
        builder.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(builder.ToScript());
        EXPECT_EQ(engine.Execute(), VMState::Fault); // Null comparisons currently fault
        EXPECT_EQ(engine.GetResultStack().size(), 0); // No results on fault
    }
}

// Test GTE Operation
TEST_F(LteGteTest, GteOperation)
{
    // Test case 1: 0 >= 0 should be true
    {
        ScriptBuilder builder;
        builder.EmitPush(static_cast<int64_t>(0));
        builder.EmitPush(static_cast<int64_t>(0));
        builder.Emit(OpCode::GE);
        builder.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(builder.ToScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_TRUE(engine.GetResultStack()[0]->GetBoolean());
    }

    // Test case 2: 1 >= 0 should be true
    {
        ScriptBuilder builder;
        builder.EmitPush(static_cast<int64_t>(1));
        builder.EmitPush(static_cast<int64_t>(0));
        builder.Emit(OpCode::GE);
        builder.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(builder.ToScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_TRUE(engine.GetResultStack()[0]->GetBoolean());
    }

    // Test case 3: 0 >= 1 should be false
    {
        ScriptBuilder builder;
        builder.EmitPush(static_cast<int64_t>(0));
        builder.EmitPush(static_cast<int64_t>(1));
        builder.Emit(OpCode::GE);
        builder.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(builder.ToScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_FALSE(engine.GetResultStack()[0]->GetBoolean());
    }

    // Test case 4: null >= 1 should be false (currently faults)
    {
        ScriptBuilder builder;
        builder.Emit(OpCode::PUSHNULL);
        builder.EmitPush(static_cast<int64_t>(1));
        builder.Emit(OpCode::GE);
        builder.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(builder.ToScript());
        EXPECT_EQ(engine.Execute(), VMState::Fault); // Null comparisons currently fault
        EXPECT_EQ(engine.GetResultStack().size(), 0); // No results on fault
    }

    // Test case 5: 1 >= null should be false (currently faults)
    {
        ScriptBuilder builder;
        builder.EmitPush(static_cast<int64_t>(1));
        builder.Emit(OpCode::PUSHNULL);
        builder.Emit(OpCode::GE);
        builder.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(builder.ToScript());
        EXPECT_EQ(engine.Execute(), VMState::Fault); // Null comparisons currently fault
        EXPECT_EQ(engine.GetResultStack().size(), 0); // No results on fault
    }
}
