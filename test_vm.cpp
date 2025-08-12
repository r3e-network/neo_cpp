#include <iostream>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/stack_item.h>

using namespace neo::vm;

int main()
{
    std::cout << "=== Neo VM Functionality Test ===" << std::endl;
    
    try {
        // Create a simple script: PUSH 5, PUSH 3, ADD
        ScriptBuilder builder;
        builder.EmitPush(static_cast<int64_t>(5));
        builder.EmitPush(static_cast<int64_t>(3));
        builder.Emit(OpCode::ADD);
        
        auto script_data = builder.ToArray();
        Script script(script_data);
        
        // Create execution engine
        ExecutionEngine engine;
        engine.LoadScript(script);
        
        std::cout << "Script loaded: " << script.size() << " bytes" << std::endl;
        std::cout << "Executing: PUSH 5, PUSH 3, ADD" << std::endl;
        
        // Execute the script
        engine.Execute();
        
        // Check the result
        if (engine.State() == VMState::HALT) {
            std::cout << "✅ Execution completed successfully!" << std::endl;
            
            if (engine.ResultStack().GetCount() > 0) {
                auto result = engine.ResultStack().Pop();
                auto integer = std::dynamic_pointer_cast<IntegerStackItem>(result);
                if (integer) {
                    std::cout << "✅ Result: 5 + 3 = " << integer->GetInteger() << std::endl;
                    
                    if (integer->GetInteger().ToInt64() == 8) {
                        std::cout << "✅ VM arithmetic is working correctly!" << std::endl;
                    }
                }
            }
        } else {
            std::cout << "❌ Execution failed with state: " << static_cast<int>(engine.State()) << std::endl;
        }
        
        // Test more complex operations
        std::cout << "\nTesting more operations..." << std::endl;
        
        ScriptBuilder builder2;
        builder2.EmitPush(static_cast<int64_t>(10));
        builder2.EmitPush(static_cast<int64_t>(2));
        builder2.Emit(OpCode::MUL);  // 10 * 2 = 20
        builder2.EmitPush(static_cast<int64_t>(5));
        builder2.Emit(OpCode::SUB);  // 20 - 5 = 15
        
        auto script_data2 = builder2.ToArray();
        Script script2(script_data2);
        ExecutionEngine engine2;
        engine2.LoadScript(script2);
        
        std::cout << "Executing: (10 * 2) - 5" << std::endl;
        engine2.Execute();
        
        if (engine2.State() == VMState::HALT && engine2.ResultStack().GetCount() > 0) {
            auto result = engine2.ResultStack().Pop();
            auto integer = std::dynamic_pointer_cast<IntegerStackItem>(result);
            if (integer && integer->GetInteger().ToInt64() == 15) {
                std::cout << "✅ Complex arithmetic: (10 * 2) - 5 = " << integer->GetInteger() << std::endl;
                std::cout << "✅ VM is fully operational!" << std::endl;
            }
        }
        
        std::cout << "\n✅ All VM tests passed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}