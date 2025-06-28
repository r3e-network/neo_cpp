#include <iostream>
#include <sstream>
#include <iomanip>

int main() {
    std::cout << "Testing stream size issue...\n";
    
    // Test 1: Using put()
    {
        std::stringstream stream;
        std::cout << "\nTest 1 - Using put():\n";
        
        for (int i = 1; i <= 20; i++) {
            stream.put(i);
        }
        
        std::string content = stream.str();
        std::cout << "String length: " << content.length() << "\n";
        std::cout << "Bytes: ";
        for (unsigned char c : content) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c) << " ";
        }
        std::cout << "\n";
    }
    
    // Test 2: Using write()
    {
        std::stringstream stream;
        std::cout << "\nTest 2 - Using write():\n";
        
        char data[20];
        for (int i = 0; i < 20; i++) {
            data[i] = i + 1;
        }
        stream.write(data, 20);
        
        std::string content = stream.str();
        std::cout << "String length: " << content.length() << "\n";
        std::cout << "Bytes: ";
        for (unsigned char c : content) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c) << " ";
        }
        std::cout << "\n";
    }
    
    // Test 3: Check what BinaryWriter does
    {
        std::stringstream stream;
        std::cout << "\nTest 3 - Simulating BinaryWriter:\n";
        
        // Write usage byte
        stream.write("\x20", 1);
        
        // Write 20 bytes
        char data[20];
        for (int i = 0; i < 20; i++) {
            data[i] = i + 1;
        }
        stream.write(data, 20);
        
        std::string content = stream.str();
        std::cout << "String length: " << content.length() << "\n";
        std::cout << "Stream tellp: " << stream.tellp() << "\n";
        
        // Now try to read it back
        stream.seekg(0);
        char usage;
        stream.read(&usage, 1);
        std::cout << "Read usage: 0x" << std::hex << static_cast<int>(static_cast<unsigned char>(usage)) << "\n";
        
        char buffer[20];
        stream.read(buffer, 20);
        std::cout << "Read " << stream.gcount() << " bytes\n";
    }
    
    return 0;
}