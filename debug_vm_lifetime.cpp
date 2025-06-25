#include <iostream>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/internal/byte_span.h>

using namespace neo::vm;

int main() {
    try {
        std::cout << "Creating Script..." << std::endl;
        ScriptBuilder sb;
        sb.Emit(OpCode::PUSH2);  // 0x12
        auto scriptBytes = sb.ToArray();
        
        Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
        std::cout << "Script byte[0]: 0x" << std::hex << static_cast<int>(script.GetScript()[0]) << std::dec << std::endl;
        
        ExecutionEngine engine;
        
        std::cout << "Loading script without storing return value..." << std::endl;
        engine.LoadScript(script);  // NOT storing the return value like the original
        
        std::cout << "Getting current context..." << std::endl;
        auto& context = engine.GetCurrentContext();
        
        std::cout << "Context script byte[0]: 0x" << std::hex << static_cast<int>(context.GetScript().GetScript()[0]) << std::dec << std::endl;
        
        auto instruction = context.GetCurrentInstruction();
        if (instruction) {
            std::cout << "Current instruction opcode: 0x" << std::hex << static_cast<int>(instruction->opcode) << std::dec << std::endl;
        } else {
            std::cout << "Current instruction is nullptr!" << std::endl;
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
}