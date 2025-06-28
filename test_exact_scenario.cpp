#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdint>

// Simulate ByteVector::Parse
std::vector<uint8_t> ParseHex(const std::string& hex) {
    std::vector<uint8_t> result;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte = hex.substr(i, 2);
        result.push_back(static_cast<uint8_t>(std::stoul(byte, nullptr, 16)));
    }
    return result;
}

int main() {
    std::cout << "Testing exact TransactionAttribute scenario...\n";
    
    // Parse the exact data from the test
    std::string hex_data = "0102030405060708090a0b0c0d0e0f1011121314";
    std::vector<uint8_t> data = ParseHex(hex_data);
    
    std::cout << "Original data (" << data.size() << " bytes): ";
    for (uint8_t b : data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b) << " ";
    }
    std::cout << "\n";
    
    // Serialize like TransactionAttribute
    std::stringstream stream;
    
    // Write usage (0x20 for Script)
    uint8_t usage = 0x20;
    stream.write(reinterpret_cast<const char*>(&usage), 1);
    
    // Write the 20 bytes of data
    stream.write(reinterpret_cast<const char*>(data.data()), data.size());
    
    // Check what was written
    std::string content = stream.str();
    std::cout << "\nSerialized (" << content.size() << " bytes): ";
    for (unsigned char c : content) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c) << " ";
    }
    std::cout << "\n";
    
    // Now deserialize
    stream.seekg(0);
    
    // Read usage
    uint8_t read_usage;
    stream.read(reinterpret_cast<char*>(&read_usage), 1);
    std::cout << "\nRead usage: 0x" << std::hex << static_cast<int>(read_usage) << "\n";
    
    // Check stream state before reading data
    std::cout << "Stream position: " << stream.tellg() << "\n";
    std::cout << "Stream state: good=" << stream.good() << " eof=" << stream.eof() << " fail=" << stream.fail() << "\n";
    
    // Try to read 20 bytes (Script branch)
    std::vector<uint8_t> read_data(20);
    stream.read(reinterpret_cast<char*>(read_data.data()), 20);
    size_t bytes_read = stream.gcount();
    
    std::cout << "Requested 20 bytes, got " << bytes_read << " bytes\n";
    std::cout << "Stream state after read: good=" << stream.good() << " eof=" << stream.eof() << " fail=" << stream.fail() << "\n";
    
    std::cout << "Read data: ";
    for (size_t i = 0; i < bytes_read; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(read_data[i]) << " ";
    }
    std::cout << "\n";
    
    // Also try reading 4 bytes (NotValidBefore branch) to see what would happen
    stream.clear();
    stream.seekg(1); // Skip usage
    std::vector<uint8_t> read_data_4(4);
    stream.read(reinterpret_cast<char*>(read_data_4.data()), 4);
    
    std::cout << "\nIf NotValidBefore branch (4 bytes): ";
    for (int i = 0; i < 4; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(read_data_4[i]) << " ";
    }
    std::cout << "\n";
    
    return 0;
}