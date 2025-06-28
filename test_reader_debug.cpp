#include <iostream>
#include <sstream>
#include <neo/ledger/transaction_attribute.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/byte_vector.h>

using namespace neo;
using namespace neo::ledger;
using namespace neo::io;

int main() {
    try {
        std::cout << "Testing BinaryReader with Script attribute...\n";
        
        // Create test data
        ByteVector data = ByteVector::Parse("0102030405060708090a0b0c0d0e0f1011121314");
        std::cout << "Original data: " << data.ToHexString() << " (size: " << data.Size() << ")\n";
        
        // Write to stream
        std::stringstream stream;
        BinaryWriter writer(stream);
        
        // Write usage byte (Script = 0x20)
        writer.Write(static_cast<uint8_t>(0x20));
        
        // Write 20 bytes of data
        writer.Write(data.AsSpan());
        
        // Check what was written
        std::string written = stream.str();
        std::cout << "Written bytes: ";
        for (unsigned char c : written) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c) << " ";
        }
        std::cout << "\n";
        std::cout << "Total written: " << written.size() << " bytes\n";
        
        // Now read it back
        stream.seekg(0);
        BinaryReader reader(stream);
        
        // Read usage
        uint8_t usage = reader.ReadUInt8();
        std::cout << "Read usage: 0x" << std::hex << static_cast<int>(usage) << "\n";
        
        // Check stream position
        std::cout << "Stream position after usage: " << reader.GetPosition() << "\n";
        
        // Read 20 bytes
        std::cout << "About to read 20 bytes...\n";
        ByteVector read_data = reader.ReadBytes(20);
        std::cout << "Read data: " << read_data.ToHexString() << " (size: " << read_data.Size() << ")\n";
        
        // Try reading directly from stream
        stream.seekg(1); // Skip usage byte
        char buffer[20];
        stream.read(buffer, 20);
        std::cout << "Direct stream read: ";
        for (int i = 0; i < 20; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(buffer[i])) << " ";
        }
        std::cout << "\n";
        std::cout << "Stream gcount: " << stream.gcount() << "\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
}