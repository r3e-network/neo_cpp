#include <iostream>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/internal/byte_span.h>

using namespace neo::vm;

int main() {
    try {
        std::cout << "Creating PUSH2 + PUSH3 script..." << std::endl;
        ScriptBuilder sb;
        sb.Emit(OpCode::PUSH2);  // Push 2
        sb.Emit(OpCode::PUSH3);  // Push 3  
        sb.Emit(OpCode::ADD);    // Add them
        auto scriptBytes = sb.ToArray();
        
        std::cout << "Script bytes: ";
        for (size_t i = 0; i < scriptBytes.Size(); ++i) {
            std::cout << "0x" << std::hex << static_cast<int>(scriptBytes[i]) << " ";
        }
        std::cout << std::dec << std::endl;
        
        Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
        ExecutionEngine engine;
        engine.LoadScript(script);
        
        std::cout << "Executing..." << std::endl;
        auto state = engine.Execute();
        
        std::cout << "Final state: " << static_cast<int>(state) << std::endl;
        auto result = engine.Pop();
        if (result) {
            std::cout << "Result type: " << typeid(*result).name() << std::endl;
            auto intResult = std::dynamic_pointer_cast<IntegerItem>(result);
            if (intResult) {
                std::cout << "Result value: " << intResult->GetInteger() << std::endl;
            } else {
                std::cout << "Result is not an integer" << std::endl;
            }
        } else {
            std::cout << "No result on stack" << std::endl;
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
}