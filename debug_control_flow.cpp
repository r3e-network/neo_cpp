#include <iostream>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/opcode.h>

using namespace neo::vm;

int main() {
    try {
        std::cout << "Testing control flow..." << std::endl;
        
        ScriptBuilder sb;
        sb.EmitPush(true);
        sb.EmitJump(OpCode::JMPIF, 3);  // Jump 3 bytes forward if true
        sb.EmitPush(static_cast<int64_t>(1));      // This should be skipped
        sb.Emit(OpCode::RET);
        sb.EmitPush(static_cast<int64_t>(2));      // This should execute
        
        auto scriptBytes = sb.ToArray();
        std::cout << "Script bytes: ";
        for (size_t i = 0; i < scriptBytes.Size(); ++i) {
            std::cout << "0x" << std::hex << static_cast<int>(scriptBytes[i]) << " ";
        }
        std::cout << std::dec << std::endl;
        
        Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
        ExecutionEngine engine;
        engine.LoadScript(script);
        
        auto state = engine.Execute();
        std::cout << "Execution state: " << static_cast<int>(state) << std::endl;
        
        auto result = engine.Pop();
        if (result) {
            auto intResult = std::dynamic_pointer_cast<IntegerItem>(result);
            if (intResult) {
                std::cout << "Result: " << intResult->GetInteger() << std::endl;
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