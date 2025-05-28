#include <neo/vm/opcode.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/stack_item.h>
#include <iostream>

using namespace neo::vm;

int main()
{
    // Test LE opcode (which replaces LTE)
    {
        ScriptBuilder builder;
        builder.EmitPush(10);  // Push 10
        builder.EmitPush(5);   // Push 5
        builder.Emit(OpCode::LE);  // 10 <= 5 should be false
        
        ExecutionEngine engine;
        auto script = builder.ToScript();
        engine.LoadScript(script);
        
        bool result = engine.Execute();
        if (!result)
        {
            std::cerr << "Execution failed for LE test 1" << std::endl;
            return 1;
        }
        
        auto item = engine.Pop();
        if (item->GetBoolean())
        {
            std::cerr << "LE test 1 failed: expected false, got true" << std::endl;
            return 1;
        }
        
        std::cout << "LE test 1 passed" << std::endl;
    }
    
    // Test LE opcode with equal values
    {
        ScriptBuilder builder;
        builder.EmitPush(5);  // Push 5
        builder.EmitPush(5);  // Push 5
        builder.Emit(OpCode::LE);  // 5 <= 5 should be true
        
        ExecutionEngine engine;
        auto script = builder.ToScript();
        engine.LoadScript(script);
        
        bool result = engine.Execute();
        if (!result)
        {
            std::cerr << "Execution failed for LE test 2" << std::endl;
            return 1;
        }
        
        auto item = engine.Pop();
        if (!item->GetBoolean())
        {
            std::cerr << "LE test 2 failed: expected true, got false" << std::endl;
            return 1;
        }
        
        std::cout << "LE test 2 passed" << std::endl;
    }
    
    // Test GE opcode (which replaces GTE)
    {
        ScriptBuilder builder;
        builder.EmitPush(5);   // Push 5
        builder.EmitPush(10);  // Push 10
        builder.Emit(OpCode::GE);  // 5 >= 10 should be false
        
        ExecutionEngine engine;
        auto script = builder.ToScript();
        engine.LoadScript(script);
        
        bool result = engine.Execute();
        if (!result)
        {
            std::cerr << "Execution failed for GE test 1" << std::endl;
            return 1;
        }
        
        auto item = engine.Pop();
        if (item->GetBoolean())
        {
            std::cerr << "GE test 1 failed: expected false, got true" << std::endl;
            return 1;
        }
        
        std::cout << "GE test 1 passed" << std::endl;
    }
    
    // Test GE opcode with equal values
    {
        ScriptBuilder builder;
        builder.EmitPush(5);  // Push 5
        builder.EmitPush(5);  // Push 5
        builder.Emit(OpCode::GE);  // 5 >= 5 should be true
        
        ExecutionEngine engine;
        auto script = builder.ToScript();
        engine.LoadScript(script);
        
        bool result = engine.Execute();
        if (!result)
        {
            std::cerr << "Execution failed for GE test 2" << std::endl;
            return 1;
        }
        
        auto item = engine.Pop();
        if (!item->GetBoolean())
        {
            std::cerr << "GE test 2 failed: expected true, got false" << std::endl;
            return 1;
        }
        
        std::cout << "GE test 2 passed" << std::endl;
    }
    
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
