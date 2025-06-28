#include <gtest/gtest.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/opcode.h>
#include <neo/io/byte_vector.h>

using namespace neo::vm;
using namespace neo::io;

TEST(SimpleResultTest, SinglePush)
{
    // Just PUSH2
    ByteVector bytes;
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH2));
    
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    ExecutionEngine engine;
    engine.LoadScript(script);
    
    VMState state = engine.Execute();
    
    EXPECT_EQ(state, VMState::Halt);
    EXPECT_EQ(engine.GetResultStack().size(), 1);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 2);
}

TEST(SimpleResultTest, MultiplePush)
{
    // PUSH1 PUSH2
    ByteVector bytes;
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH1));
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH2));
    
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i)
    {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    ExecutionEngine engine;
    engine.LoadScript(script);
    
    VMState state = engine.Execute();
    
    EXPECT_EQ(state, VMState::Halt);
    // Should have 2 items: [2, 1] (reversed order)
    EXPECT_EQ(engine.GetResultStack().size(), 2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}