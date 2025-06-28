#include <iostream>
#include <sstream>
#include <iomanip>

int main() {
    std::cout << "Testing stringstream behavior...\n";
    
    // Create a stringstream and write 21 bytes (1 usage + 20 data)
    std::stringstream stream;
    
    // Write usage byte
    stream.put(0x20);
    
    // Write 20 bytes
    for (int i = 1; i <= 20; i++) {
        stream.put(i);
    }
    
    // Check stream state
    std::cout << "Stream tellp (write position): " << stream.tellp() << "\n";
    
    // Reset to beginning
    stream.seekg(0);
    std::cout << "Stream tellg (read position) after seekg(0): " << stream.tellg() << "\n";
    
    // Read usage byte
    char usage;
    stream.get(usage);
    std::cout << "Read usage: 0x" << std::hex << static_cast<int>(static_cast<unsigned char>(usage)) << "\n";
    std::cout << "Stream tellg after reading usage: " << stream.tellg() << "\n";
    
    // Try to read 20 bytes
    char buffer[20];
    stream.read(buffer, 20);
    std::cout << "Stream gcount (bytes actually read): " << stream.gcount() << "\n";
    std::cout << "Stream state: good=" << stream.good() << " eof=" << stream.eof() << " fail=" << stream.fail() << "\n";
    
    // Print what was read
    std::cout << "Read data: ";
    for (int i = 0; i < stream.gcount(); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(buffer[i])) << " ";
    }
    std::cout << "\n";
    
    // Try Available calculation
    stream.seekg(1); // Go back to after usage byte
    std::streampos current = stream.tellg();
    stream.seekg(0, std::ios::end);
    std::streampos end = stream.tellg();
    std::cout << "Current position: " << current << "\n";
    std::cout << "End position: " << end << "\n";
    std::cout << "Available bytes: " << (end - current) << "\n";
    
    return 0;
}