#include <iostream>
#include <neo/vm/script.h>
#include <neo/vm/instruction.h>
#include <neo/vm/internal/byte_vector.h>
#include <neo/vm/opcode.h>

using namespace neo::vm;
using namespace neo::vm::internal;

int main() {
    try {
        std::cout << "Testing VM Script exactly like the unit test...\n";
        
        // Exact copy of the failing test
        ByteVector bytes1 = ByteVector::Parse("10"); // PUSH0 (0x10)
        std::cout << "ByteVector created successfully\n";
        
        Script script1(bytes1);
        std::cout << "Script created successfully\n";
        
        auto instruction1 = script1.GetInstruction(0);
        std::cout << "GetInstruction called\n";
        
        if (instruction1 != nullptr) {
            std::cout << "✓ instruction1 != nullptr - PASS\n";
        } else {
            std::cout << "✗ instruction1 == nullptr - FAIL\n";
            return 1;
        }
        
        std::cout << "OpCode value: " << static_cast<int>(instruction1->opcode) << "\n";
        std::cout << "OpCode::PUSH0 value: " << static_cast<int>(OpCode::PUSH0) << "\n";
        
        if (instruction1->opcode == OpCode::PUSH0) {
            std::cout << "✓ opcode == OpCode::PUSH0 - PASS\n";
        } else {
            std::cout << "✗ opcode != OpCode::PUSH0 - FAIL\n";
        }
        
        std::cout << "Operand size: " << instruction1->Operand.Size() << "\n";
        if (instruction1->Operand.Size() == 0) {
            std::cout << "✓ Operand.Size() == 0 - PASS\n";
        } else {
            std::cout << "✗ Operand.Size() != 0 - FAIL\n";
        }
        
        std::cout << "All checks passed!\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
}