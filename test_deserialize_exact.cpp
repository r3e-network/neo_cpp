#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <vector>

// Minimal enum to test
enum class Usage : uint8_t {
    HighPriority = 0x01,
    OracleResponse = 0x11,
    Script = 0x20,
    NotValidBefore = 0x20,  // Same value as Script!
    Conflicts = 0x21
};

int main() {
    std::cout << "Testing exact deserialization logic...\n";
    
    // Create test stream
    std::stringstream stream;
    
    // Write usage byte (0x20)
    stream.put(0x20);
    
    // Write 20 bytes of test data
    for (uint8_t i = 1; i <= 20; i++) {
        stream.put(i);
    }
    
    std::cout << "Stream content size: " << stream.str().size() << " bytes\n";
    
    // Reset for reading
    stream.seekg(0);
    
    // Read usage byte
    uint8_t usage_byte;
    stream.read(reinterpret_cast<char*>(&usage_byte), 1);
    Usage usage = static_cast<Usage>(usage_byte);
    
    std::cout << "Read usage: 0x" << std::hex << static_cast<int>(usage_byte) << "\n";
    
    // Now test the exact if-else chain from TransactionAttribute::Deserialize
    std::vector<uint8_t> data;
    
    if (usage == Usage::HighPriority) {
        std::cout << "Branch: HighPriority\n";
        // No data
    }
    else if (usage == Usage::OracleResponse) {
        std::cout << "Branch: OracleResponse\n";
    }
    // Legacy Neo 2.x attribute handling - check Script first due to value collision with NotValidBefore
    else if (usage == Usage::Script) {
        std::cout << "Branch: Script (reading 20 bytes)\n";
        data.resize(20);
        stream.read(reinterpret_cast<char*>(data.data()), 20);
        std::cout << "Actually read: " << stream.gcount() << " bytes\n";
    }
    else if (usage == Usage::NotValidBefore) {
        std::cout << "Branch: NotValidBefore (reading 4 bytes)\n";
        data.resize(4);
        stream.read(reinterpret_cast<char*>(data.data()), 4);
        std::cout << "Actually read: " << stream.gcount() << " bytes\n";
    }
    else {
        std::cout << "Branch: Other\n";
    }
    
    // Print what was read
    std::cout << "Data read: ";
    for (size_t i = 0; i < data.size(); i++) {
        if (i < stream.gcount()) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
        } else {
            std::cout << "?? ";
        }
    }
    std::cout << "\n";
    
    return 0;
}