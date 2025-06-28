#include <iostream>
#include <sstream>
#include <cstdint>
#include <cstdio>

int main() {
    std::cout << "Testing raw stream reading...\n";
    
    // Create test data
    std::stringstream stream;
    uint8_t test_data[] = {0x20, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 
                          0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14};
    stream.write(reinterpret_cast<char*>(test_data), sizeof(test_data));
    
    // Check stream size
    stream.seekg(0, std::ios::end);
    size_t total_size = stream.tellg();
    std::cout << "Total stream size: " << total_size << " bytes\n";
    
    // Read usage byte
    stream.seekg(0);
    uint8_t usage;
    stream.read(reinterpret_cast<char*>(&usage), 1);
    std::cout << "Usage byte: 0x" << std::hex << (int)usage << std::dec << "\n";
    std::cout << "gcount after reading usage: " << stream.gcount() << "\n";
    std::cout << "Stream position: " << stream.tellg() << "\n";
    
    // Read 20 bytes
    uint8_t data[20];
    stream.read(reinterpret_cast<char*>(data), 20);
    std::cout << "gcount after reading 20 bytes: " << stream.gcount() << "\n";
    std::cout << "Stream position: " << stream.tellg() << "\n";
    
    std::cout << "Data read: ";
    for (int i = 0; i < stream.gcount(); ++i) {
        printf("%02x", data[i]);
    }
    std::cout << "\n";
    
    // Check remaining
    stream.seekg(0, std::ios::end);
    size_t end_pos = stream.tellg();
    std::cout << "Stream end position: " << end_pos << "\n";
    
    return 0;
}