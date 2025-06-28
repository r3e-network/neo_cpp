#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <vector>

std::vector<uint8_t> ParseHex(const std::string& hex) {
    std::vector<uint8_t> result;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte = hex.substr(i, 2);
        result.push_back(static_cast<uint8_t>(std::stoul(byte, nullptr, 16)));
    }
    return result;
}

void TestWithMode(const std::string& mode_name, std::ios_base::openmode mode) {
    std::cout << "\nTesting with " << mode_name << ":\n";
    
    // Create stream with specified mode
    std::stringstream stream(mode);
    
    // Write usage byte (0x20)
    stream.write("\x20", 1);
    
    // Write test data
    std::vector<uint8_t> data = ParseHex("0102030405060708090a0b0c0d0e0f1011121314");
    stream.write(reinterpret_cast<const char*>(data.data()), data.size());
    
    // Check what was written
    std::cout << "Stream tellp: " << stream.tellp() << "\n";
    std::cout << "String size: " << stream.str().size() << "\n";
    
    // Try to read back
    stream.seekg(0);
    
    // Read usage
    char usage;
    stream.read(&usage, 1);
    std::cout << "Read usage: 0x" << std::hex << static_cast<int>(static_cast<unsigned char>(usage)) << "\n";
    
    // Read data
    std::vector<char> read_data(20);
    stream.read(read_data.data(), 20);
    std::cout << "Requested 20 bytes, got " << stream.gcount() << " bytes\n";
    
    // If we got less than expected, try reading 4 bytes instead
    if (stream.gcount() < 20) {
        stream.clear();
        stream.seekg(1);
        stream.read(read_data.data(), 4);
        std::cout << "Reading 4 bytes instead, got " << stream.gcount() << " bytes: ";
        for (int i = 0; i < 4 && i < stream.gcount(); i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(static_cast<unsigned char>(read_data[i])) << " ";
        }
        std::cout << "\n";
    }
}

int main() {
    std::cout << "Testing stringstream modes with TransactionAttribute data...\n";
    
    // Test different stream modes
    TestWithMode("default mode", std::ios::in | std::ios::out);
    TestWithMode("binary mode", std::ios::binary | std::ios::in | std::ios::out);
    
    return 0;
}