#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdint>

int main() {
    std::cout << "Investigating stringstream truncation issue...\n";
    
    // Original test data that's failing
    uint8_t data[] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
        0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14
    };
    
    std::cout << "Original test data:\n";
    for (int i = 0; i < 20; i++) {
        std::cout << "data[" << i << "] = 0x" << std::hex << static_cast<int>(data[i]) << "\n";
    }
    
    // Test with stringstream
    std::stringstream ss;
    
    // Write usage byte first
    ss.put(0x20);
    
    // Write data
    ss.write(reinterpret_cast<const char*>(data), 20);
    
    // Get the string
    std::string str = ss.str();
    std::cout << "\nString size from str(): " << str.size() << "\n";
    
    // Check stream positions
    std::cout << "tellp (write position): " << ss.tellp() << "\n";
    
    // Try reading back
    ss.seekg(0);
    std::cout << "\nReading back:\n";
    
    char c;
    int count = 0;
    while (ss.get(c)) {
        std::cout << "Byte " << count << ": 0x" << std::hex 
                  << static_cast<int>(static_cast<unsigned char>(c)) << "\n";
        count++;
        if (count > 25) break; // Safety limit
    }
    std::cout << "Total bytes read: " << count << "\n";
    
    // Let's also check if the issue is related to how str() works
    std::cout << "\nDirect access to string data:\n";
    for (size_t i = 0; i < str.size(); i++) {
        std::cout << "str[" << i << "] = 0x" << std::hex 
                  << static_cast<int>(static_cast<unsigned char>(str[i])) << "\n";
    }
    
    return 0;
}