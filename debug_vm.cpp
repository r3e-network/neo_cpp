#include <iostream>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/internal/byte_span.h>

using namespace neo::vm;

int main() {
    try {
        std::cout << "Creating script builder..." << std::endl;
        ScriptBuilder sb;
        
        std::cout << "Adding PUSH2 instruction..." << std::endl;
        sb.Emit(OpCode::PUSH2);
        
        std::cout << "Adding PUSH3 instruction..." << std::endl;
        sb.Emit(OpCode::PUSH3);
        
        std::cout << "Adding ADD instruction..." << std::endl;
        sb.Emit(OpCode::ADD);
        
        std::cout << "Converting to script..." << std::endl;
        auto scriptBytes = sb.ToArray();
        std::cout << "Script size: " << scriptBytes.Size() << " bytes" << std::endl;
        std::cout << "Script bytes: ";
        for (size_t i = 0; i < scriptBytes.Size(); i++) {
            std::cout << "0x" << std::hex << static_cast<int>(scriptBytes[i]) << " ";
        }
        std::cout << std::dec << std::endl;
        
        Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
        std::cout << "Script created successfully" << std::endl;
        
        std::cout << "Creating execution engine..." << std::endl;
        ExecutionEngine engine;
        
        std::cout << "Loading script..." << std::endl;
        engine.LoadScript(script);
        
        std::cout << "Initial state: " << static_cast<int>(engine.GetState()) << std::endl;
        
        std::cout << "Executing..." << std::endl;
        VMState result = engine.Execute();
        
        std::cout << "Final state: " << static_cast<int>(result) << std::endl;
        std::cout << "Expected Halt: " << static_cast<int>(VMState::Halt) << std::endl;
        std::cout << "Expected Fault: " << static_cast<int>(VMState::Fault) << std::endl;
        
        if (result == VMState::Halt) {
            std::cout << "Execution successful!" << std::endl;
            try {
                auto resultItem = engine.Pop();
                auto intResult = std::dynamic_pointer_cast<IntegerItem>(resultItem);
                if (intResult) {
                    std::cout << "Result value: " << intResult->GetInteger() << std::endl;
                } else {
                    std::cout << "Result is not an integer" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "Error popping result: " << e.what() << std::endl;
            }
        } else {
            std::cout << "Execution failed with fault state" << std::endl;
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
}