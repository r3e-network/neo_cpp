#include <iostream>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/opcode.h>

using namespace neo::vm;

int main() {
    try {
        // Exact copy of VMTest.BasicArithmetic
        ScriptBuilder sb;
        sb.Emit(OpCode::PUSH2);
        sb.Emit(OpCode::PUSH3); 
        sb.Emit(OpCode::ADD);
        
        auto scriptBytes = sb.ToArray();
        Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
        ExecutionEngine engine;
        engine.LoadScript(script);
        
        engine.Execute();
        
        std::cout << "Expected state: " << static_cast<int>(VMState::Halt) << std::endl;
        std::cout << "Actual state: " << static_cast<int>(engine.GetState()) << std::endl;
        
        auto result = engine.Pop();
        auto intResult = std::dynamic_pointer_cast<IntegerItem>(result);
        if (intResult) {
            std::cout << "Expected result: 5" << std::endl;
            std::cout << "Actual result: " << intResult->GetInteger() << std::endl;
        } else {
            std::cout << "Result is not an integer or is null" << std::endl;
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
}