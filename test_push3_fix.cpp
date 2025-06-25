#include <iostream>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/internal/byte_span.h>

using namespace neo::vm;

int main() {
    try {
        std::cout << "=== TESTING PUSH3 FIX ===" << std::endl;
        
        // Test: PUSH2 PUSH3 ADD (the failing test)
        ScriptBuilder sb;
        sb.Emit(OpCode::PUSH2);
        sb.Emit(OpCode::PUSH3); 
        sb.Emit(OpCode::ADD);
        
        auto scriptBytes = sb.ToArray();
        std::cout << "Script bytes: ";
        for (size_t i = 0; i < scriptBytes.Size(); i++) {
            std::cout << "0x" << std::hex << static_cast<int>(scriptBytes[i]) << " ";
        }
        std::cout << std::dec << std::endl;
        
        Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
        ExecutionEngine engine;
        engine.LoadScript(script);
        
        std::cout << "Executing script..." << std::endl;
        VMState result = engine.Execute();
        
        std::cout << "VM State: " << static_cast<int>(result) << std::endl;
        std::cout << "Expected VMState::Halt (1), got: " << static_cast<int>(result) << std::endl;
        
        if (result == VMState::Halt) {
            std::cout << "SUCCESS! VM executed correctly!" << std::endl;
            auto item = engine.Pop();
            auto intItem = std::dynamic_pointer_cast<IntegerItem>(item);
            if (intItem) {
                std::cout << "Result value: " << intItem->GetInteger() << std::endl;
                std::cout << "Expected: 5" << std::endl;
                if (intItem->GetInteger() == 5) {
                    std::cout << "PERFECT! The fix works!" << std::endl;
                    return 0;
                } else {
                    std::cout << "ERROR: Wrong result value" << std::endl;
                    return 1;
                }
            } else {
                std::cout << "ERROR: Result is not an IntegerItem" << std::endl;
                return 1;
            }
        } else if (result == VMState::Fault) {
            std::cout << "FAILURE: VM execution still faulted" << std::endl;
            return 1;
        } else {
            std::cout << "UNKNOWN: VM execution returned unexpected state" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
}