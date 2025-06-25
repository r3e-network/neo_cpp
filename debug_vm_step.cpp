#include <iostream>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/internal/byte_span.h>

using namespace neo::vm;

void checkScript(const Script& script, const std::string& label) {
    std::cout << label << " script length: " << script.GetLength() << std::endl;
    if (script.GetLength() > 0) {
        std::cout << label << " byte[0]: 0x" << std::hex << static_cast<int>(script.GetScript()[0]) << std::dec << std::endl;
    }
}

int main() {
    try {
        std::cout << "Step 1: Creating ScriptBuilder..." << std::endl;
        ScriptBuilder sb;
        sb.Emit(OpCode::PUSH2);  // 0x12
        auto scriptBytes = sb.ToArray();
        
        std::cout << "Step 2: Script bytes: ";
        for (size_t i = 0; i < scriptBytes.Size(); ++i) {
            std::cout << "0x" << std::hex << static_cast<int>(scriptBytes[i]) << " ";
        }
        std::cout << std::dec << std::endl;
        
        std::cout << "Step 3: Creating Script from bytes..." << std::endl;
        Script originalScript(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
        checkScript(originalScript, "Original");
        
        std::cout << "Step 4: Creating ExecutionEngine..." << std::endl;
        ExecutionEngine engine;
        
        std::cout << "Step 5: Calling engine.LoadScript..." << std::endl;
        Script scriptCopy(originalScript); // Make a copy for safety
        checkScript(scriptCopy, "ScriptCopy");
        
        auto loadedContext = engine.LoadScript(scriptCopy);
        checkScript(loadedContext->GetScript(), "LoadedContext");
        
        std::cout << "Step 6: Getting current context from engine..." << std::endl;
        auto& currentContext = engine.GetCurrentContext();
        checkScript(currentContext.GetScript(), "CurrentContext");
        
        std::cout << "Step 7: Testing GetCurrentInstruction..." << std::endl;
        auto instruction = currentContext.GetCurrentInstruction();
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