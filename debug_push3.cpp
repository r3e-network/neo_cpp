#include <iostream>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/opcode.h>

using namespace neo::vm;

int main() {
    try {
        std::cout << "Testing string push operation..." << std::endl;
        
        ScriptBuilder sb;
        sb.EmitPush("Hello");
        
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
            std::cout << "Result type: " << typeid(*result).name() << std::endl;
            
            auto byteStringItem = std::dynamic_pointer_cast<ByteStringItem>(result);
            if (byteStringItem) {
                auto bytes = byteStringItem->GetByteArray();
                std::string str((char*)bytes.Data(), bytes.Size());
                std::cout << "String result: " << str << std::endl;
            } else {
                std::cout << "Result is not a ByteStringItem" << std::endl;
                
                auto intItem = std::dynamic_pointer_cast<IntegerItem>(result);
                if (intItem) {
                    std::cout << "Result is IntegerItem: " << intItem->GetInteger() << std::endl;
                }
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