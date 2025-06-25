#include <iostream>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/internal/byte_span.h>

using namespace neo::vm;

int main() {
    try {
        std::cout << "Step 1: Creating ScriptBuilder..." << std::endl;
        ScriptBuilder sb;
        
        std::cout << "Step 2: Emitting PUSH2..." << std::endl;
        sb.Emit(OpCode::PUSH2);
        
        std::cout << "Step 3: Converting to array..." << std::endl;
        auto scriptBytes = sb.ToArray();
        std::cout << "Script bytes size: " << scriptBytes.Size() << std::endl;
        
        std::cout << "Step 4: Creating Script object..." << std::endl;
        Script script(internal::ByteSpan(scriptBytes.Data(), scriptBytes.Size()));
        std::cout << "Script length: " << script.GetLength() << std::endl;
        
        std::cout << "Step 5: Creating ExecutionEngine..." << std::endl;
        ExecutionEngine engine;
        
        std::cout << "Step 6: Loading script..." << std::endl;
        engine.LoadScript(script);
        
        std::cout << "Step 7: Getting current context..." << std::endl;
        auto& context = engine.GetCurrentContext();
        
        std::cout << "Step 8: Getting current instruction..." << std::endl;
        
        // Let's debug the instruction creation directly
        std::cout << "Debug: Testing instruction creation directly..." << std::endl;
        try {
            auto span = script.GetScript().AsSpan();
            std::cout << "Debug: Script span size: " << span.Size() << std::endl;
            std::cout << "Debug: Byte at position 0: 0x" << std::hex << static_cast<int>(span[0]) << std::dec << std::endl;
            
            std::cout << "Debug: Creating instruction manually..." << std::endl;
            Instruction testInstr(span, 0);
            std::cout << "Debug: Manual instruction created successfully! Opcode: 0x" << std::hex << static_cast<int>(testInstr.opcode) << std::dec << std::endl;
            
        } catch (const std::exception& e) {
            std::cout << "Debug: Manual instruction creation failed: " << e.what() << std::endl;
        }
        
        // Now test script.GetInstruction directly
        std::cout << "Debug: Testing script.GetInstruction(0)..." << std::endl;
        auto scriptInstr = script.GetInstruction(0);
        if (scriptInstr) {
            std::cout << "Debug: script.GetInstruction(0) succeeded! Opcode: 0x" << std::hex << static_cast<int>(scriptInstr->opcode) << std::dec << std::endl;
        } else {
            std::cout << "Debug: script.GetInstruction(0) returned nullptr!" << std::endl;
        }
        
        // Test again to see if caching is the issue
        std::cout << "Debug: Testing script.GetInstruction(0) again..." << std::endl;
        auto scriptInstr2 = script.GetInstruction(0);
        if (scriptInstr2) {
            std::cout << "Debug: script.GetInstruction(0) 2nd time succeeded! Opcode: 0x" << std::hex << static_cast<int>(scriptInstr2->opcode) << std::dec << std::endl;
        } else {
            std::cout << "Debug: script.GetInstruction(0) 2nd time returned nullptr!" << std::endl;
        }
        
        // Now check what's wrong with context.GetCurrentInstruction()
        std::cout << "Debug: Checking context script..." << std::endl;
        auto& contextScript = context.GetScript();
        std::cout << "Debug: Context script length: " << contextScript.GetLength() << std::endl;
        auto contextSpan = contextScript.GetScript().AsSpan();
        std::cout << "Debug: Context script byte at 0: 0x" << std::hex << static_cast<int>(contextSpan[0]) << std::dec << std::endl;
        
        std::cout << "Debug: Calling contextScript.GetInstruction(0)..." << std::endl;
        auto contextScriptInstr = contextScript.GetInstruction(0);
        if (contextScriptInstr) {
            std::cout << "Debug: contextScript.GetInstruction(0) succeeded! Opcode: 0x" << std::hex << static_cast<int>(contextScriptInstr->opcode) << std::dec << std::endl;
        } else {
            std::cout << "Debug: contextScript.GetInstruction(0) returned nullptr!" << std::endl;
        }
        
        auto instruction = context.GetCurrentInstruction();
        
        if (instruction) {
            std::cout << "Current instruction opcode: 0x" << std::hex << static_cast<int>(instruction->opcode) << std::dec << std::endl;
            
            // Check if it's the same pointer
            std::cout << "Debug: Are pointers the same? " << (instruction.get() == contextScriptInstr.get()) << std::endl;
        } else {
            std::cout << "Current instruction is nullptr!" << std::endl;
        }
        
        std::cout << "Step 9: Check instruction pointer..." << std::endl;
        std::cout << "Instruction pointer: " << context.GetInstructionPointer() << std::endl;
        
        std::cout << "Done. Not calling Execute() to avoid segfault." << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
}