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
    // Create a simple script: PUSH1 PUSH2
    ByteVector bytes;
    bytes.Push(static_cast<uint8_t>(OpCode::PUSH1));
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
        std::cout << "Result[" << i << "] = " << engine.GetResultStack()[i]->GetInteger() << std::endl;
    }
    
    return 0;
}