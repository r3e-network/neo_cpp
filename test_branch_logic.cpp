#include <iostream>
#include <sstream>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>

using namespace neo::io;

int main() {
    std::cout << "Testing BinaryReader behavior with script attribute pattern...\n";
    
    // Create a stream with usage byte 0x20 followed by 20 bytes
    std::stringstream stream;
    BinaryWriter writer(stream);
    
    // Write usage byte (0x20)
    writer.Write(static_cast<uint8_t>(0x20));
    
    // Write 20 bytes
    for (uint8_t i = 1; i <= 20; i++) {
        writer.Write(i);
    }
    
    // Check stream content
    std::string content = stream.str();
    std::cout << "Stream size: " << content.size() << " bytes\n";
    std::cout << "Stream content: ";
    for (unsigned char c : content) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c) << " ";
    }
    std::cout << "\n\n";
    
    // Now read it back
    stream.seekg(0);
    BinaryReader reader(stream);
    
    // Read usage
    uint8_t usage = reader.ReadUInt8();
    std::cout << "Read usage: 0x" << std::hex << static_cast<int>(usage) << "\n";
    
    // Based on the TransactionAttribute::Deserialize logic:
    // It should check Script (0x20) first, which should read 20 bytes
    ByteVector data;
    
    if (usage == 0x20) {  // Both Script and NotValidBefore
        std::cout << "Usage is 0x20 - ambiguous (Script or NotValidBefore)\n";
        
        // The code checks Script first, so let's read 20 bytes
        std::cout << "Reading 20 bytes (Script branch)...\n";
        data = reader.ReadBytes(20);
        std::cout << "Read " << data.Size() << " bytes: " << data.ToHexString() << "\n";
    }
    
    return 0;
}