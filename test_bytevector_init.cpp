#include <iostream>
#include <neo/io/byte_vector.h>

using namespace neo::io;

int main() {
    std::cout << "Testing ByteVector initialization...\n";
    
    // Test 1: Create ByteVector with size
    ByteVector vec1(20);
    std::cout << "ByteVector(20) size: " << vec1.Size() << "\n";
    std::cout << "ByteVector(20) data: " << vec1.ToHexString() << "\n";
    
    // Test 2: Check if it's zero-initialized
    bool all_zeros = true;
    for (size_t i = 0; i < vec1.Size(); i++) {
        if (vec1[i] != 0) {
            all_zeros = false;
            break;
        }
    }
    std::cout << "All zeros: " << (all_zeros ? "YES" : "NO") << "\n";
    
    // Test 3: Test ReadRawBytes behavior
    uint8_t buffer[20];
    ByteVector vec2(20);
    
    // Simulate what ReadRawBytes does
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
                          0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14};
    std::memcpy(vec2.Data(), test_data, 20);
    
    std::cout << "After memcpy: " << vec2.ToHexString() << "\n";
    
    return 0;
}