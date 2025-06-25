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
        
        std::cout << "Script bytes: ";
        for (size_t i = 0; i < scriptBytes.Size(); ++i) {
            std::cout << "0x" << std::hex << static_cast<int>(scriptBytes[i]) << " ";
        }
        std::cout << std::dec << std::endl;
        
        Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
        std::cout << "Script created. Length: " << script.GetLength() << std::endl;
        std::cout << "Script byte[0]: 0x" << std::hex << static_cast<int>(script.GetScript()[0]) << std::dec << std::endl;
        
        ExecutionEngine engine;
        std::cout << "\nLoading script into engine..." << std::endl;
        
        // LoadScript creates a new context and returns it
        auto contextPtr = engine.LoadScript(script);
        std::cout << "Script loaded." << std::endl;
        
        // Check the context
        const Script& contextScript = contextPtr->GetScript();
        std::cout << "Context script length: " << contextScript.GetLength() << std::endl;
        std::cout << "Context script byte[0]: 0x" << std::hex << static_cast<int>(contextScript.GetScript()[0]) << std::dec << std::endl;
        
        // Get current context from engine
        auto& currentContext = engine.GetCurrentContext();
        const Script& currentContextScript = currentContext.GetScript();
        std::cout << "\nCurrent context script length: " << currentContextScript.GetLength() << std::endl;
        std::cout << "Current context script byte[0]: 0x" << std::hex << static_cast<int>(currentContextScript.GetScript()[0]) << std::dec << std::endl;
        
        // Check if they're the same object
        std::cout << "\nAre context pointers the same? " << (contextPtr.get() == &currentContext) << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/internal/byte_span.h>

using namespace neo::vm;

class DebugExecutionEngine : public ExecutionEngine {
public:
    void PreExecuteInstruction(const Instruction& instruction) override {
        std::cout << "PreExecute: instruction.opcode = 0x" << std::hex << static_cast<int>(instruction.opcode) << std::dec << std::endl;
        
        // Also check what GetCurrentInstruction returns
        auto& context = GetCurrentContext();
        auto currentInstr = context.GetCurrentInstruction();
        if (currentInstr) {
            std::cout << "PreExecute: GetCurrentInstruction opcode = 0x" << std::hex << static_cast<int>(currentInstr->opcode) << std::dec << std::endl;
        } else {
            std::cout << "PreExecute: GetCurrentInstruction returned nullptr" << std::endl;
        }
    }
    
    void PostExecuteInstruction(const Instruction& instruction) override {
        std::cout << "PostExecute: instruction.opcode = 0x" << std::hex << static_cast<int>(instruction.opcode) << std::dec << std::endl;
        ExecutionEngine::PostExecuteInstruction(instruction);
    }
};

int main() {
    try {
        std::cout << "Creating script: PUSH2 only..." << std::endl;
        ScriptBuilder sb;
        sb.Emit(OpCode::PUSH2);
        
        auto scriptBytes = sb.ToArray();
        Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
        
        std::cout << "Script length: " << script.GetLength() << std::endl;
        
        // Try to manually get the instruction at position 0
        auto instruction0 = script.GetInstruction(0);
        if (instruction0) {
            std::cout << "Instruction at position 0: 0x" << std::hex << static_cast<int>(instruction0->opcode) << std::dec << std::endl;
        } else {
            std::cout << "No instruction at position 0!" << std::endl;
        }
        
        DebugExecutionEngine engine;
        engine.LoadScript(script);
        
        std::cout << "Starting execution..." << std::endl;
        VMState result = engine.Execute();
        
        std::cout << "Final result: " << static_cast<int>(result) << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
}