#include <iostream>
#include <sstream>
#include <iomanip>

int main() {
    std::cout << "Testing specific byte values...\n";
    
    // Test writing each byte individually
    for (int test_val = 0x0e; test_val <= 0x16; test_val++) {
        std::stringstream stream;
        stream.write(reinterpret_cast<const char*>(&test_val), 1);
        
        std::string content = stream.str();
        std::cout << "Writing 0x" << std::hex << test_val 
                  << " - String length: " << content.length();
        
        if (content.length() > 0) {
            std::cout << " - Read back: 0x" << std::hex 
                      << static_cast<int>(static_cast<unsigned char>(content[0]));
        }
        std::cout << "\n";
    }
    
    // Test writing specific sequence
    std::cout << "\nTesting sequence writing:\n";
    std::stringstream stream;
    
    // Write bytes one by one and check
    for (int i = 1; i <= 20; i++) {
        char byte = static_cast<char>(i);
        stream.write(&byte, 1);
        std::cout << "After writing byte " << i << " (0x" << std::hex << i 
                  << "), stream size: " << stream.str().length() << "\n";
    }
    
    return 0;
}