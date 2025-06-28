#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdint>
#include <vector>

// Minimal reproduction of the issue
class MinimalByteVector {
public:
    MinimalByteVector(size_t size) : data_(size) {}
    
    uint8_t* Data() { return data_.data(); }
    size_t Size() const { return data_.size(); }
    
    void PrintHex() const {
        for (uint8_t b : data_) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
        }
    }
    
private:
    std::vector<uint8_t> data_;
};

int main() {
    std::cout << "Minimal reproduction test...\n";
    
    // Create stream with test data
    std::stringstream stream;
    
    // Write 20 bytes
    for (uint8_t i = 1; i <= 20; i++) {
        stream.put(i);
    }
    
    // Reset stream
    stream.seekg(0);
    
    // Method 1: Direct read
    {
        std::cout << "\nMethod 1 - Direct read:\n";
        char buffer[20];
        stream.read(buffer, 20);
        std::cout << "Read " << stream.gcount() << " bytes: ";
        for (int i = 0; i < 20; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(static_cast<unsigned char>(buffer[i]));
        }
        std::cout << "\n";
    }
    
    // Reset stream
    stream.clear();
    stream.seekg(0);
    
    // Method 2: Like BinaryReader::ReadBytes
    {
        std::cout << "\nMethod 2 - Like BinaryReader:\n";
        MinimalByteVector vec(20);
        stream.read(reinterpret_cast<char*>(vec.Data()), 20);
        std::cout << "Read " << stream.gcount() << " bytes: ";
        vec.PrintHex();
        std::cout << "\n";
    }
    
    // Reset stream  
    stream.clear();
    stream.seekg(0);
    
    // Method 3: Check if ByteVector constructor initializes
    {
        std::cout << "\nMethod 3 - Check ByteVector initialization:\n";
        MinimalByteVector vec(20);
        std::cout << "Before read: ";
        vec.PrintHex();
        std::cout << "\n";
        
        // Only read 4 bytes to simulate the issue
        stream.read(reinterpret_cast<char*>(vec.Data()), 4);
        std::cout << "After reading 4 bytes: ";
        vec.PrintHex();
        std::cout << "\n";
    }
    
    return 0;
}