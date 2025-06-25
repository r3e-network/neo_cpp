#include <iostream>
#include <neo/vm/script.h>
#include <neo/vm/execution_context.h>
#include <neo/vm/script_builder.h>

using namespace neo::vm;

int main() {
    try {
        std::cout << "Creating original script..." << std::endl;
        ScriptBuilder sb;
        sb.Emit(OpCode::PUSH2);  // 0x12
        auto scriptBytes = sb.ToArray();
        
        std::cout << "Original bytes: ";
        for (size_t i = 0; i < scriptBytes.Size(); ++i) {
            std::cout << "0x" << std::hex << static_cast<int>(scriptBytes[i]) << " ";
        }
        std::cout << std::dec << std::endl;
        
        Script originalScript(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
        std::cout << "Original script length: " << originalScript.GetLength() << std::endl;
        std::cout << "Original script byte[0]: 0x" << std::hex << static_cast<int>(originalScript.GetScript()[0]) << std::dec << std::endl;
        
        // Create execution context
        ExecutionContext context(originalScript);
        
        const Script& contextScript = context.GetScript();
        std::cout << "Context script length: " << contextScript.GetLength() << std::endl;
        std::cout << "Context script byte[0]: 0x" << std::hex << static_cast<int>(contextScript.GetScript()[0]) << std::dec << std::endl;
        
        // Check if they're the same
        if (originalScript.GetScript()[0] != contextScript.GetScript()[0]) {
            std::cout << "ERROR: Script was corrupted during copy!" << std::endl;
            std::cout << "Expected: 0x" << std::hex << static_cast<int>(originalScript.GetScript()[0]) << std::endl;
            std::cout << "Got: 0x" << std::hex << static_cast<int>(contextScript.GetScript()[0]) << std::dec << std::endl;
        } else {
            std::cout << "Script copied correctly!" << std::endl;
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
}