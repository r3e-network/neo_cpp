#include <neo/vm/execution_engine.h>
#include <neo/vm/script.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/opcode.h>
#include <neo/io/byte_vector.h>
#include <iostream>

using namespace neo::vm;
using namespace neo::io;

int main()
{
    // Create the same script as ThrowIfNot test
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
    
    std::cout << "State: " << static_cast<int>(state) << std::endl;
    std::cout << "Result stack size: " << engine.GetResultStack().size() << std::endl;
    
    for (size_t i = 0; i < engine.GetResultStack().size(); ++i)
    {
        auto item = engine.GetResultStack()[i];
        std::cout << "Result[" << i << "] type: " << static_cast<int>(item->GetType()) << std::endl;
        if (item->GetType() == StackItemType::Integer)
        {
            std::cout << "  value: " << item->GetInteger() << std::endl;
        }
    }
    
    return 0;
}