#include <gtest/gtest.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/opcode.h>
#include <neo/io/byte_vector.h>

using namespace neo::vm;
using namespace neo::io;

TEST(DebugTest, ThrowIfNotDebug)
{
    // Same as ThrowIfNot test but with debugging
    ByteVector bytes;
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH1));
    
    // Push error message
    bytes.Push(static_cast<uint8_t>(OpCode::PUSHDATA1));
    bytes.Push(static_cast<uint8_t>(13)); // length
    std::string errorMsg = "Error message";
    for (char c : errorMsg) {
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
    
    // Debug: LoadScript with custom handler to see evaluation stack
    auto context = engine.LoadScript(script);
    
    // Step through execution
    while (engine.GetState() == VMState::None)
    {
        auto& ctx = engine.GetCurrentContext();
        auto evalStack = ctx.GetEvaluationStack();
        
        std::cout << "Before instruction at IP " << ctx.GetInstructionPointer() << ":" << std::endl;
        std::cout << "  Eval stack size: " << evalStack.size() << std::endl;
        for (size_t i = 0; i < evalStack.size(); ++i) {
            std::cout << "    [" << i << "] type=" << static_cast<int>(evalStack[i]->GetType()) << std::endl;
        }
        
        engine.ExecuteNext();
        
        if (engine.GetState() != VMState::None) break;
    }
    
    std::cout << "\nFinal state: " << static_cast<int>(engine.GetState()) << std::endl;
    std::cout << "Result stack size: " << engine.GetResultStack().size() << std::endl;
    
    for (size_t i = 0; i < engine.GetResultStack().size(); ++i)
    {
        auto item = engine.GetResultStack()[i];
        std::cout << "Result[" << i << "] type=" << static_cast<int>(item->GetType());
        if (item->GetType() == StackItemType::Integer) {
            std::cout << " value=" << item->GetInteger();
        }
        std::cout << std::endl;
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}