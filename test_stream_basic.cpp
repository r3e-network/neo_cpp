#include <iostream>
#include <sstream>
#include <vector>
#include <cstdint>

int main() {
    try {
        std::cout << "=== Basic stream test ===\n";
        
        // Create test data
        std::vector<uint8_t> test_data = {0x20, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 
                                         0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14};
        
        // Write to stream
        std::stringstream stream;
        stream.write(reinterpret_cast<const char*>(test_data.data()), test_data.size());
        
        std::cout << "Wrote " << test_data.size() << " bytes\n";
        std::cout << "Stream string size: " << stream.str().size() << "\n";
        
        // Reset for reading
        stream.seekg(0);
        
        // Read back manually
        std::vector<uint8_t> read_data(test_data.size());
        stream.read(reinterpret_cast<char*>(read_data.data()), test_data.size());
        
        std::cout << "Read " << stream.gcount() << " bytes\n";
        
        // Compare
        bool match = true;
        for (size_t i = 0; i < test_data.size(); ++i) {
            if (test_data[i] != read_data[i]) {
                std::cout << "Mismatch at byte " << i << ": expected 0x" 
                          << std::hex << (int)test_data[i] << ", got 0x" 
                          << (int)read_data[i] << std::dec << "\n";
                match = false;
            }
        }
        
        if (match) {
            std::cout << "Basic stream test PASSED\n";
        } else {
            std::cout << "Basic stream test FAILED\n";
        }
        
        return match ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
}