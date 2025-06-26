#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cstdint>

void parseUInt160(const std::string& hex) {
    std::cout << "Parsing: " << hex << std::endl;
    
    std::string cleanHex = hex;
    if (cleanHex.size() < 40) {
        cleanHex = std::string(40 - cleanHex.size(), '0') + cleanHex;
    }
    
    uint8_t data[20];
    
    // Parse hex string (big-endian to little-endian)
    for (size_t i = 0; i < 20; ++i) {
        std::string byteStr = cleanHex.substr((20 - 1 - i) * 2, 2);
        unsigned long byteVal = std::strtoul(byteStr.c_str(), nullptr, 16);
        data[i] = static_cast<uint8_t>(byteVal);
    }
    
    std::cout << "Bytes: ";
    for (int i = 0; i < 20; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << " ";
    }
    std::cout << std::endl;
    
    int32_t id = *reinterpret_cast<const int32_t*>(data);
    std::cout << "ID: 0x" << std::hex << id << std::endl;
    std::cout << std::endl;
}

int main() {
    parseUInt160("0102030405060708090a0b0c0d0e0f1011121314");
    parseUInt160("1102030405060708090a0b0c0d0e0f1011121314");
    return 0;
}
