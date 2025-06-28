#include <iostream>
#include <neo/vm/script.h>
#include <neo/vm/instruction.h>
#include <neo/vm/internal/byte_vector.h>

using namespace neo::vm;
using namespace neo::vm::internal;

int main() {
    try {
        std::cout << "Testing VM Script GetInstruction...\n";
        
        // Test 1: PUSH0 (0x10) - using internal ByteVector like the tests do
        ByteVector bytes1 = ByteVector::Parse("10");
        std::cout << "Created ByteVector with size: " << bytes1.Size() << "\n";
        std::cout << "Bytes: " << bytes1.ToHexString() << "\n";
        
        Script script1(bytes1);
        std::cout << "Created Script with length: " << script1.GetLength() << "\n";
        
        auto instruction1 = script1.GetInstruction(0);
        if (instruction1) {
            std::cout << "✓ GetInstruction(0) succeeded\n";
            std::cout << "  Opcode: " << static_cast<int>(instruction1->opcode) << "\n";
            std::cout << "  Operand size: " << instruction1->Operand.Size() << "\n";
        } else {
            std::cout << "✗ GetInstruction(0) returned nullptr\n";
        }
        
        // Test 2: Out of bounds
        auto instruction2 = script1.GetInstruction(1);
        if (instruction2) {
            std::cout << "✗ GetInstruction(1) should have returned nullptr\n";
        } else {
            std::cout << "✓ GetInstruction(1) correctly returned nullptr for out of bounds\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}