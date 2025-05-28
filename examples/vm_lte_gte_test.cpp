#include <iostream>
#include <gtest/gtest.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/stack_item.h>

using namespace neo::vm;
using namespace neo::io;

// Test LTE Operation
TEST(VMLteGteTest, LteOperation) {
    // Test case 1: 0 <= 0 should be true
    {
        Script script;
        script.EmitPush(0);
        script.EmitPush(0);
        script.Emit(OpCode::LTE);
        script.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(script.GetScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_TRUE(engine.GetResultStack()[0]->GetBoolean());
    }

    // Test case 2: 1 <= 0 should be false
    {
        Script script;
        script.EmitPush(1);
        script.EmitPush(0);
        script.Emit(OpCode::LTE);
        script.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(script.GetScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_FALSE(engine.GetResultStack()[0]->GetBoolean());
    }

    // Test case 3: 0 <= 1 should be true
    {
        Script script;
        script.EmitPush(0);
        script.EmitPush(1);
        script.Emit(OpCode::LTE);
        script.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(script.GetScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_TRUE(engine.GetResultStack()[0]->GetBoolean());
    }

    // Test case 4: null <= 1 should be false
    {
        Script script;
        script.Emit(OpCode::PUSHNULL);
        script.EmitPush(1);
        script.Emit(OpCode::LTE);
        script.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(script.GetScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_FALSE(engine.GetResultStack()[0]->GetBoolean());
    }

    // Test case 5: 1 <= null should be false
    {
        Script script;
        script.EmitPush(1);
        script.Emit(OpCode::PUSHNULL);
        script.Emit(OpCode::LTE);
        script.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(script.GetScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_FALSE(engine.GetResultStack()[0]->GetBoolean());
    }
}

// Test GTE Operation
TEST(VMLteGteTest, GteOperation) {
    // Test case 1: 0 >= 0 should be true
    {
        Script script;
        script.EmitPush(0);
        script.EmitPush(0);
        script.Emit(OpCode::GTE);
        script.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(script.GetScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_TRUE(engine.GetResultStack()[0]->GetBoolean());
    }

    // Test case 2: 1 >= 0 should be true
    {
        Script script;
        script.EmitPush(1);
        script.EmitPush(0);
        script.Emit(OpCode::GTE);
        script.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(script.GetScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_TRUE(engine.GetResultStack()[0]->GetBoolean());
    }

    // Test case 3: 0 >= 1 should be false
    {
        Script script;
        script.EmitPush(0);
        script.EmitPush(1);
        script.Emit(OpCode::GTE);
        script.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(script.GetScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_FALSE(engine.GetResultStack()[0]->GetBoolean());
    }

    // Test case 4: null >= 1 should be false
    {
        Script script;
        script.Emit(OpCode::PUSHNULL);
        script.EmitPush(1);
        script.Emit(OpCode::GTE);
        script.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(script.GetScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_FALSE(engine.GetResultStack()[0]->GetBoolean());
    }

    // Test case 5: 1 >= null should be false
    {
        Script script;
        script.EmitPush(1);
        script.Emit(OpCode::PUSHNULL);
        script.Emit(OpCode::GTE);
        script.Emit(OpCode::RET);

        ExecutionEngine engine;
        engine.LoadScript(script.GetScript());
        EXPECT_EQ(engine.Execute(), VMState::Halt);
        EXPECT_EQ(engine.GetResultStack().size(), 1);
        EXPECT_FALSE(engine.GetResultStack()[0]->GetBoolean());
    }
}

int main(int argc, char** argv) {
    std::cout << "Running VM LTE/GTE test..." << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
