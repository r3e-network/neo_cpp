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
    
    std::cout << "Initial state: " << static_cast<int>(engine.GetState()) << std::endl;
    
    VMState state = engine.Execute();
    
    std::cout << "Final state: " << static_cast<int>(state) << std::endl;
    std::cout << "Result stack size: " << engine.GetResultStack().size() << std::endl;
    
    if (engine.GetResultStack().size() > 0)
    {
        auto result = engine.GetResultStack()[0];
        std::cout << "Result type: " << static_cast<int>(result->GetType()) << std::endl;
        std::cout << "Result boolean: " << result->GetBoolean() << std::endl;
    }
    
    return 0;
}