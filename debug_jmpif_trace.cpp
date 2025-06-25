#include <iostream>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/opcode.h>

using namespace neo::vm;

int main() {
    try {
        std::cout << "=== JMPIF Debug Trace ===" << std::endl;
        
        ScriptBuilder sb;
        sb.EmitPush(true);                         // Should push true onto stack
        sb.EmitJump(OpCode::JMPIF, 3);            // Jump 3 bytes forward if true
        sb.EmitPush(static_cast<int64_t>(1));     // This should be skipped
        sb.Emit(OpCode::RET);                     // Return (should not reach here)
        sb.EmitPush(static_cast<int64_t>(2));     // This should execute after jump
        
        auto scriptBytes = sb.ToArray();
        std::cout << "Script bytes (" << scriptBytes.Size() << "): ";
        for (size_t i = 0; i < scriptBytes.Size(); ++i) {
            std::cout << std::hex << "0x" << static_cast<int>(scriptBytes[i]) << " ";
        }
        std::cout << std::dec << std::endl;
        
        // Create and execute
        Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
        ExecutionEngine engine;
        engine.LoadScript(script);
        
        std::cout << "Initial stack size: " << engine.GetResultStack().size() << std::endl;
        
        // Execute step by step if possible or check final result
        auto state = engine.Execute();
        std::cout << "Final execution state: " << static_cast<int>(state) << std::endl;
        std::cout << "Final stack size: " << engine.GetResultStack().size() << std::endl;
        
        if (engine.GetResultStack().size() > 0) {
            auto result = engine.Pop();
            auto intResult = std::dynamic_pointer_cast<IntegerItem>(result);
            if (intResult) {
                std::cout << "Result: " << intResult->GetInteger() << std::endl;
            } else {
                std::cout << "Result is not an integer" << std::endl;
            }
        } else {
            std::cout << "Stack is empty - no result" << std::endl;
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
}