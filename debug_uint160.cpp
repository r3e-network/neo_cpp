#include <neo/io/uint160.h>
#include <iostream>
#include <iomanip>

int main() {
    auto hash1 = neo::io::UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    auto hash2 = neo::io::UInt160::Parse("1102030405060708090a0b0c0d0e0f1011121314");
    
    std::cout << "Hash1 bytes: ";
    for (int i = 0; i < 20; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)hash1.Data()[i] << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Hash2 bytes: ";
    for (int i = 0; i < 20; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)hash2.Data()[i] << " ";
    }
    std::cout << std::endl;
    
    int32_t id1 = *reinterpret_cast<const int32_t*>(hash1.Data());
    int32_t id2 = *reinterpret_cast<const int32_t*>(hash2.Data());
    
    std::cout << "ID1: 0x" << std::hex << id1 << std::endl;
    std::cout << "ID2: 0x" << std::hex << id2 << std::endl;
    
    return 0;
}
