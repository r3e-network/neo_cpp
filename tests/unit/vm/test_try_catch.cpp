#include <gtest/gtest.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/opcode.h>
#include <neo/io/byte_vector.h>

using namespace neo::vm;
using namespace neo::io;

TEST(TryCatchTest, BasicTryCatch)
{
    // Create a script with a try-catch block
    // TRY
    //   PUSH0
    //   THROW
    // CATCH
    //   PUSH1
    // ENDTRY
    // PUSH2
    ByteVector bytes;
    bytes.Push(static_cast<uint8_t>(OpCode::TRY));
    bytes.Push(static_cast<uint8_t>(0x05)); // catch position (relative to next instruction)
    bytes.Push(static_cast<uint8_t>(0x00)); // finally position (no finally block)
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH0));
    bytes.Push(static_cast<uint8_t>(OpCode::THROW));
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH1));
    bytes.Push(static_cast<uint8_t>(OpCode::ENDTRY));
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH2));
    
    // Convert io::ByteVector to internal::ByteVector
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
    
    // The script should halt successfully
    EXPECT_EQ(state, VMState::Halt);
    
    // The result stack should contain [2, 1]
    EXPECT_EQ(engine.GetResultStack().size(), 2);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 2);
    EXPECT_EQ(engine.GetResultStack()[1]->GetInteger(), 1);
}

TEST(TryCatchTest, TryFinally)
{
    // Create a script with a try-finally block (no catch)
    // TRY
    //   PUSH0
    // FINALLY
    //   PUSH1
    // ENDFINALLY
    // PUSH2
    ByteVector bytes;
    bytes.Push(static_cast<uint8_t>(OpCode::TRY));
    bytes.Push(static_cast<uint8_t>(0x00)); // catch position (no catch block)
    bytes.Push(static_cast<uint8_t>(0x03)); // finally position (relative to next instruction)
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH0));
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH1));
    bytes.Push(static_cast<uint8_t>(OpCode::ENDFINALLY));
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH2));
    
    // Convert io::ByteVector to internal::ByteVector
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
    
    // The script should halt successfully
    EXPECT_EQ(state, VMState::Halt);
    
    // The result stack should contain [2, 1, 0]
    EXPECT_EQ(engine.GetResultStack().size(), 3);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 2);
    EXPECT_EQ(engine.GetResultStack()[1]->GetInteger(), 1);
    EXPECT_EQ(engine.GetResultStack()[2]->GetInteger(), 0);
}

TEST(TryCatchTest, TryCatchFinally)
{
    // Create a script with a try-catch-finally block
    // TRY
    //   PUSH0
    //   THROW
    // CATCH
    //   PUSH1
    // FINALLY
    //   PUSH2
    // ENDFINALLY
    // PUSH3
    ByteVector bytes;
    bytes.Push(static_cast<uint8_t>(OpCode::TRY));
    bytes.Push(static_cast<uint8_t>(0x05)); // catch position (relative to next instruction)
    bytes.Push(static_cast<uint8_t>(0x08)); // finally position (relative to next instruction)
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH0));
    bytes.Push(static_cast<uint8_t>(OpCode::THROW));
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH1));
    bytes.Push(static_cast<uint8_t>(OpCode::JMP));
    bytes.Push(static_cast<uint8_t>(0x03)); // jump to after finally block
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH2));
    bytes.Push(static_cast<uint8_t>(OpCode::ENDFINALLY));
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH3));
    
    // Convert io::ByteVector to internal::ByteVector
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
    
    // The script should halt successfully
    EXPECT_EQ(state, VMState::Halt);
    
    // The result stack should contain [3, 2, 1]
    EXPECT_EQ(engine.GetResultStack().size(), 3);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 3);
    EXPECT_EQ(engine.GetResultStack()[1]->GetInteger(), 2);
    EXPECT_EQ(engine.GetResultStack()[2]->GetInteger(), 1);
}

TEST(TryCatchTest, NestedTryCatch)
{
    // Create a script with nested try-catch blocks
    // TRY
    //   PUSH0
    //   TRY
    //     PUSH1
    //     THROW
    //   CATCH
    //     PUSH2
    //   ENDTRY
    // CATCH
    //   PUSH3
    // ENDTRY
    // PUSH4
    ByteVector bytes;
    bytes.Push(static_cast<uint8_t>(OpCode::TRY));
    bytes.Push(static_cast<uint8_t>(0x0C)); // catch position (relative to next instruction)
    bytes.Push(static_cast<uint8_t>(0x00)); // finally position (no finally block)
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH0));
    bytes.Push(static_cast<uint8_t>(OpCode::TRY));
    bytes.Push(static_cast<uint8_t>(0x05)); // catch position (relative to next instruction)
    bytes.Push(static_cast<uint8_t>(0x00)); // finally position (no finally block)
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH1));
    bytes.Push(static_cast<uint8_t>(OpCode::THROW));
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH2));
    bytes.Push(static_cast<uint8_t>(OpCode::ENDTRY));
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH3));
    bytes.Push(static_cast<uint8_t>(OpCode::ENDTRY));
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH4));
    
    // Convert io::ByteVector to internal::ByteVector
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
    
    // The script should halt successfully
    EXPECT_EQ(state, VMState::Halt);
    
    // The result stack should contain [4, 2, 1, 0]
    EXPECT_EQ(engine.GetResultStack().size(), 4);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 4);
    EXPECT_EQ(engine.GetResultStack()[1]->GetInteger(), 2);
    EXPECT_EQ(engine.GetResultStack()[2]->GetInteger(), 1);
    EXPECT_EQ(engine.GetResultStack()[3]->GetInteger(), 0);
}

TEST(TryCatchTest, IsNull)
{
    // Create a script that tests ISNULL
    // PUSHNULL
    // ISNULL
    ByteVector bytes;
    bytes.Push(static_cast<uint8_t>(OpCode::PUSHNULL));
    bytes.Push(static_cast<uint8_t>(OpCode::ISNULL));
    
    // Convert io::ByteVector to internal::ByteVector
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
    
    // The script should halt successfully
    EXPECT_EQ(state, VMState::Halt);
    
    // The result stack should contain [true]
    EXPECT_EQ(engine.GetResultStack().size(), 1);
    EXPECT_TRUE(engine.GetResultStack()[0]->GetBoolean());
}

TEST(TryCatchTest, ThrowIfNot)
{
    // Create a script that tests ASSERTMSG (THROWIFNOT)
    // PUSH1 (true)
    // PUSH "Error message"
    // ASSERTMSG (should not throw)
    // PUSH2
    ByteVector bytes;
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH1));
    
    // Push error message
    bytes.Push(static_cast<uint8_t>(OpCode::PUSHDATA1));
    bytes.Push(static_cast<uint8_t>(13)); // length
    for (char c : "Error message") {
        bytes.Push(static_cast<uint8_t>(c));
    }
    
    bytes.Push(static_cast<uint8_t>(OpCode::ASSERTMSG));
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH2));
    
    // Convert io::ByteVector to internal::ByteVector
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
    
    // The script should halt successfully
    EXPECT_EQ(state, VMState::Halt);
    
    // The result stack should contain [2]
    EXPECT_EQ(engine.GetResultStack().size(), 1);
    EXPECT_EQ(engine.GetResultStack()[0]->GetInteger(), 2);
}
