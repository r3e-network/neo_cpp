#include <iostream>
#include <sstream>
#include <neo/ledger/transaction_attribute.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/byte_vector.h>

using namespace neo;
using namespace neo::ledger;
using namespace neo::io;

// Custom attribute class to add debugging
class DebugAttribute : public TransactionAttribute {
public:
    void DebugDeserialize(BinaryReader& reader) {
        try {
            std::cout << "Starting deserialization...\n";
            
            // Read usage
            uint8_t usage_byte = reader.ReadUInt8();
            std::cout << "Read usage byte: 0x" << std::hex << static_cast<int>(usage_byte) << "\n";
            
            SetUsage(static_cast<Usage>(usage_byte));
            
            // Print which enum value this maps to
            if (GetUsage() == Usage::Script) {
                std::cout << "Usage is Script\n";
            }
            if (GetUsage() == Usage::NotValidBefore) {
                std::cout << "Usage is NotValidBefore\n";
            }
            
            // Call the actual deserialization
            std::cout << "Calling parent Deserialize...\n";
            
            // We need to re-create the reader at the usage position since we already read it
            std::stringstream temp_stream;
            BinaryWriter temp_writer(temp_stream);
            temp_writer.Write(usage_byte);
            
            // Copy remaining data from original stream
            std::vector<char> buffer(1024);
            reader.GetStream().read(buffer.data(), buffer.size());
            size_t bytes_read = reader.GetStream().gcount();
            temp_stream.write(buffer.data(), bytes_read);
            
            // Reset and deserialize
            temp_stream.seekg(0);
            BinaryReader temp_reader(temp_stream);
            Deserialize(temp_reader);
            
            std::cout << "Deserialization complete. Data size: " << GetData().Size() << "\n";
            
        } catch (const std::exception& e) {
            std::cout << "Exception during deserialization: " << e.what() << "\n";
            throw;
        }
    }
    
    std::istream& GetStream() { return *stream_; }
    
private:
    std::istream* stream_;
};

int main() {
    try {
        std::cout << "Testing TransactionAttribute deserialization directly...\n";
        
        // Create test attribute
        TransactionAttribute::Usage usage = TransactionAttribute::Usage::Script;
        ByteVector data = ByteVector::Parse("0102030405060708090a0b0c0d0e0f1011121314");
        TransactionAttribute attribute(usage, data);
        
        std::cout << "Original: usage=0x" << std::hex << static_cast<int>(usage) 
                  << ", data=" << data.ToHexString() << " (" << data.Size() << " bytes)\n";
        
        // Serialize
        std::stringstream stream;
        BinaryWriter writer(stream);
        std::cout << "\nSerializing...\n";
        attribute.Serialize(writer);
        
        std::cout << "Serialized " << stream.tellp() << " bytes\n";
        std::cout << "Stream str() size: " << stream.str().size() << " bytes\n";
        
        // Print serialized bytes
        std::string serialized = stream.str();
        std::cout << "Serialized data: ";
        for (unsigned char c : serialized) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c) << " ";
        }
        std::cout << "\n";
        
        // Deserialize normally
        std::cout << "\nDeserializing normally...\n";
        stream.seekg(0);
        BinaryReader reader(stream);
        TransactionAttribute attribute2;
        
        try {
            attribute2.Deserialize(reader);
            std::cout << "Deserialized successfully\n";
            std::cout << "Result: usage=0x" << std::hex << static_cast<int>(attribute2.GetUsage())
                      << ", data=" << attribute2.GetData().ToHexString() 
                      << " (" << attribute2.GetData().Size() << " bytes)\n";
        } catch (const std::exception& e) {
            std::cout << "Exception: " << e.what() << "\n";
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << "\n";
        return 1;
    }
}