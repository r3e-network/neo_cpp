#include <iostream>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script.h>
#include <neo/vm/opcode.h>
#include <neo/io/byte_vector.h>

using namespace neo::vm;
using namespace neo::io;

int main()
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
    
    std::cout << "Before LoadScript - evaluation stack size: " << engine.GetEvaluationStack().size() << std::endl;
    engine.LoadScript(script);
    std::cout << "After LoadScript - evaluation stack size: " << engine.GetEvaluationStack().size() << std::endl;
    
    VMState state = engine.Execute();
    
    std::cout << "Final state: " << static_cast<int>(state) << std::endl;
    std::cout << "Evaluation stack size: " << engine.GetEvaluationStack().size() << std::endl;
    std::cout << "Result stack size: " << engine.GetResultStack().size() << std::endl;
    
    if (!engine.GetResultStack().empty()) {
        for (size_t i = 0; i < engine.GetResultStack().size(); i++) {
            std::cout << "Result[" << i << "] = " << engine.GetResultStack()[i]->GetInteger() << std::endl;
        }
    }
    
    return 0;
}