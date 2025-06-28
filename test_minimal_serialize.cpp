#include <iostream>
#include <sstream>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>

int main() {
    try {
        // Test basic serialization/deserialization
        std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
        neo::io::BinaryWriter writer(stream);
        
        // Write some basic data
        writer.Write(static_cast<uint8_t>(0xd1)); // Type
        writer.Write(static_cast<uint8_t>(1));      // Version
        writer.WriteVarInt(0);  // No attributes
        writer.WriteVarInt(0);  // No inputs
        writer.WriteVarInt(0);  // No outputs
        writer.WriteVarInt(0);  // No witnesses
        
        std::cout << "Wrote " << stream.tellp() << " bytes\n";
        
        // Read it back
        stream.seekg(0);
        neo::io::BinaryReader reader(stream);
        
        uint8_t type = reader.ReadUInt8();
        std::cout << "Type: 0x" << std::hex << static_cast<int>(type) << "\n";
        
        uint8_t version = reader.ReadUInt8();
        std::cout << "Version: " << static_cast<int>(version) << "\n";
        
        int64_t attrCount = reader.ReadVarInt();
        std::cout << "Attributes: " << std::dec << attrCount << "\n";
        
        int64_t inputCount = reader.ReadVarInt();
        std::cout << "Inputs: " << inputCount << "\n";
        
        int64_t outputCount = reader.ReadVarInt();
        std::cout << "Outputs: " << outputCount << "\n";
        
        int64_t witnessCount = reader.ReadVarInt();
        std::cout << "Witnesses: " << witnessCount << "\n";
        
        std::cout << "Position after reading: " << stream.tellg() << "\n";
        std::cout << "EOF: " << stream.eof() << "\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
}