#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

int main() {
    std::cout << "Testing stringstream byte writing...\n";
    
    // Create a stringstream and write 21 bytes
    std::stringstream stream;
    
    // Write usage byte
    stream.write("\x20", 1);
    
    // Write 20 bytes
    char data[20];
    for (int i = 0; i < 20; i++) {
        data[i] = i + 1;
    }
    stream.write(data, 20);
    
    // Get the string content
    std::string content = stream.str();
    std::cout << "String length: " << content.length() << "\n";
    std::cout << "Content bytes: ";
    for (unsigned char c : content) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c) << " ";
    }
    std::cout << "\n";
    
    // Check if there are any zero bytes
    for (size_t i = 0; i < content.length(); i++) {
        if (content[i] == 0) {
            std::cout << "Found zero byte at position " << i << "\n";
        }
    }
    
    return 0;
}