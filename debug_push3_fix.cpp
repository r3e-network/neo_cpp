#include <iostream>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/opcode.h>

using namespace neo::vm;

int main() {
    try {
        std::cout << "Testing different push approaches..." << std::endl;
        
        // Test 1: String literal
        {
            std::cout << "\nTest 1: String literal" << std::endl;
            ScriptBuilder sb;
            sb.EmitPush("Hello");
            auto bytes = sb.ToArray();
            std::cout << "Bytes: ";
            for (size_t i = 0; i < bytes.Size(); ++i) {
                std::cout << "0x" << std::hex << static_cast<int>(bytes[i]) << " ";
            }
            std::cout << std::dec << std::endl;
        }
        
        // Test 2: std::string object
        {
            std::cout << "\nTest 2: std::string object" << std::endl;
            ScriptBuilder sb;
            std::string hello = "Hello";
            sb.EmitPush(hello);
            auto bytes = sb.ToArray();
            std::cout << "Bytes: ";
            for (size_t i = 0; i < bytes.Size(); ++i) {
                std::cout << "0x" << std::hex << static_cast<int>(bytes[i]) << " ";
            }
            std::cout << std::dec << std::endl;
        }
        
        // Test 3: ByteSpan directly
        {
            std::cout << "\nTest 3: ByteSpan directly" << std::endl;
            ScriptBuilder sb;
            std::string hello = "Hello";
            neo::io::ByteSpan span(reinterpret_cast<const uint8_t*>(hello.data()), hello.size());
            sb.EmitPush(span);
            auto bytes = sb.ToArray();
            std::cout << "Bytes: ";
            for (size_t i = 0; i < bytes.Size(); ++i) {
                std::cout << "0x" << std::hex << static_cast<int>(bytes[i]) << " ";
            }
            std::cout << std::dec << std::endl;
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
}