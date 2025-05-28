#include <gtest/gtest.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script.h>
#include <neo/io/byte_vector.h>

using namespace neo::vm;
using namespace neo::io;

TEST(LeaveTest, LeaveFromTry)
{
    // Create a script with a TRY block and a LEAVE instruction
    // TRY (0C 08 0F)
    //   PUSH1 (11)
    //   LEAVE 2 (42 02) - leave and jump 2 bytes ahead
    //   PUSH2 (12) - this should be skipped
    // ENDTRY (3D 00)
    // PUSH3 (13) - execution should continue here
    // RET (40)
    
    auto scriptBytes = ByteVector::Parse("0C080F114202123D001340");
    
    // Create an execution engine
    auto engine = std::make_unique<ExecutionEngine>();
    
    // Load and execute the script
    engine->LoadScript(Script(scriptBytes));
    VMState state = engine->Execute();
    
    // Verify the execution was successful
    EXPECT_EQ(state, VMState::Halt);
    
    // Check the result stack - should have [3, 1] (PUSH3, PUSH1)
    auto resultStack = engine->GetResultStack();
    EXPECT_EQ(resultStack.size(), 2);
    EXPECT_EQ(resultStack[0]->GetInteger(), 3);
    EXPECT_EQ(resultStack[1]->GetInteger(), 1);
}

TEST(LeaveTest, LeaveFromCatch)
{
    // Create a script with a TRY-CATCH block where an exception is thrown and then LEAVE is used in the catch block
    // TRY (0C 08 0F)
    //   PUSH1 (11)
    //   THROW (3A)
    //   PUSH2 (12) - this should be skipped
    // CATCH
    //   PUSH3 (13)
    //   LEAVE 2 (42 02) - leave and jump 2 bytes ahead
    //   PUSH4 (14) - this should be skipped
    // ENDTRY (3D 00)
    // PUSH5 (15) - execution should continue here
    // RET (40)
    
    auto scriptBytes = ByteVector::Parse("0C080F113A12131342023D001540");
    
    // Create an execution engine
    auto engine = std::make_unique<ExecutionEngine>();
    
    // Load and execute the script
    engine->LoadScript(Script(scriptBytes));
    VMState state = engine->Execute();
    
    // Verify the execution was successful
    EXPECT_EQ(state, VMState::Halt);
    
    // Check the result stack - should have [5, 3, 1] (PUSH5, PUSH3, PUSH1)
    auto resultStack = engine->GetResultStack();
    EXPECT_EQ(resultStack.size(), 3);
    EXPECT_EQ(resultStack[0]->GetInteger(), 5);
    EXPECT_EQ(resultStack[1]->GetInteger(), 3);
    EXPECT_EQ(resultStack[2]->GetInteger(), 1);
}

TEST(LeaveTest, LeaveFromFinally)
{
    // Create a script with a TRY-FINALLY block where LEAVE is used in the finally block
    // TRY (0C 00 08)
    //   PUSH1 (11)
    // FINALLY
    //   PUSH2 (12)
    //   LEAVE 2 (42 02) - this should jump to the end but still execute the finally block
    //   PUSH3 (13) - this should be skipped
    // ENDFINALLY (3F)
    // PUSH4 (14) - execution should continue here
    // RET (40)
    
    auto scriptBytes = ByteVector::Parse("0C0008111242021340");
    
    // Create an execution engine
    auto engine = std::make_unique<ExecutionEngine>();
    
    // Load and execute the script
    engine->LoadScript(Script(scriptBytes));
    VMState state = engine->Execute();
    
    // Verify the execution was successful
    EXPECT_EQ(state, VMState::Halt);
    
    // Check the result stack - should have [4, 2, 1] (PUSH4, PUSH2, PUSH1)
    auto resultStack = engine->GetResultStack();
    EXPECT_EQ(resultStack.size(), 3);
    EXPECT_EQ(resultStack[0]->GetInteger(), 4);
    EXPECT_EQ(resultStack[1]->GetInteger(), 2);
    EXPECT_EQ(resultStack[2]->GetInteger(), 1);
}

TEST(LeaveTest, LeaveLongDistance)
{
    // Create a script with a TRY block and a LEAVE_L instruction for long jumps
    // TRY (0C 0A 0F)
    //   PUSH1 (11)
    //   LEAVE_L (44 04 00 00 00) - leave and jump 4 bytes ahead (32-bit distance)
    //   PUSH2 (12) - this should be skipped
    // ENDTRY (3D 00)
    // PUSH3 (13) - execution should continue here
    // RET (40)
    
    auto scriptBytes = ByteVector::Parse("0C0A0F1144040000003D001340");
    
    // Create an execution engine
    auto engine = std::make_unique<ExecutionEngine>();
    
    // Load and execute the script
    engine->LoadScript(Script(scriptBytes));
    VMState state = engine->Execute();
    
    // Verify the execution was successful
    EXPECT_EQ(state, VMState::Halt);
    
    // Check the result stack - should have [3, 1] (PUSH3, PUSH1)
    auto resultStack = engine->GetResultStack();
    EXPECT_EQ(resultStack.size(), 2);
    EXPECT_EQ(resultStack[0]->GetInteger(), 3);
    EXPECT_EQ(resultStack[1]->GetInteger(), 1);
}

TEST(LeaveTest, NestedTryLeave)
{
    // Create a script with nested TRY blocks and a LEAVE instruction that jumps out of both
    // TRY (0C 14 00)
    //   PUSH1 (11)
    //   TRY (0C 0A 00)
    //     PUSH2 (12)
    //     LEAVE 4 (42 04) - leave both try blocks and jump 4 bytes ahead
    //     PUSH3 (13) - this should be skipped
    //   ENDTRY (3D 00)
    //   PUSH4 (14) - this should be skipped
    // ENDTRY (3D 00)
    // PUSH5 (15) - execution should continue here
    // RET (40)
    
    auto scriptBytes = ByteVector::Parse("0C14001130132143D001440");
    
    // Create an execution engine
    auto engine = std::make_unique<ExecutionEngine>();
    
    // Load and execute the script
    engine->LoadScript(Script(scriptBytes));
    VMState state = engine->Execute();
    
    // Verify the execution was successful
    EXPECT_EQ(state, VMState::Halt);
    
    // Check the result stack - should have [5, 2, 1] (PUSH5, PUSH2, PUSH1)
    auto resultStack = engine->GetResultStack();
    EXPECT_EQ(resultStack.size(), 3);
    EXPECT_EQ(resultStack[0]->GetInteger(), 5);
    EXPECT_EQ(resultStack[1]->GetInteger(), 2);
    EXPECT_EQ(resultStack[2]->GetInteger(), 1);
} 