#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <cstring>

int main() {
    std::cout << "Testing write issue with byte 0x14 and beyond...\n";
    
    // Create test data with problematic bytes
    uint8_t test_data[25] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
        0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
        0x24, 0x25, 0x26, 0x27, 0x28
    };
    
    // Test 1: Write using write()
    {
        std::cout << "\nTest 1 - Using write():\n";
        std::stringstream stream;
        stream.write(reinterpret_cast<const char*>(test_data), 25);
        
        std::string content = stream.str();
        std::cout << "Written " << content.size() << " bytes\n";
        std::cout << "Data: ";
        for (size_t i = 0; i < content.size(); i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(static_cast<unsigned char>(content[i])) << " ";
        }
        std::cout << "\n";
    }
    
    // Test 2: Write byte by byte
    {
        std::cout << "\nTest 2 - Writing byte by byte:\n";
        std::stringstream stream;
        
        for (int i = 0; i < 25; i++) {
            stream.write(reinterpret_cast<const char*>(&test_data[i]), 1);
            if (stream.str().size() != i + 1) {
                std::cout << "ERROR: After writing byte " << i << " (0x" 
                          << std::hex << static_cast<int>(test_data[i]) 
                          << "), stream size is " << stream.str().size() 
                          << " instead of " << (i + 1) << "\n";
                break;
            }
        }
        
        std::cout << "Final size: " << stream.str().size() << "\n";
    }
    
    // Test 3: Check if it's a specific byte value issue
    {
        std::cout << "\nTest 3 - Testing specific problem bytes:\n";
        
        // Test each potentially problematic byte
        uint8_t problem_bytes[] = {0x14, 0x15, 0x00, 0x0a, 0x0d};
        
        for (uint8_t byte : problem_bytes) {
            std::stringstream stream;
            stream.write(reinterpret_cast<const char*>(&byte), 1);
            std::cout << "Writing 0x" << std::hex << static_cast<int>(byte) 
                      << " - String size: " << stream.str().size() << "\n";
        }
    }
    
    // Test 4: Binary mode
    {
        std::cout << "\nTest 4 - Using binary mode:\n";
        std::stringstream stream(std::ios::binary | std::ios::in | std::ios::out);
        stream.write(reinterpret_cast<const char*>(test_data), 25);
        
        std::string content = stream.str();
        std::cout << "Written " << content.size() << " bytes in binary mode\n";
    }
    
    return 0;
}