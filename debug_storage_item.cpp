#include <neo/persistence/storage_item.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/byte_vector.h>
#include <sstream>
#include <iostream>
#include <iomanip>

int main() {
    try {
        // Create a storage item
        auto value = neo::io::ByteVector::Parse("0102030405");
        neo::persistence::StorageItem item(value);
        
        std::cout << "Original value size: " << value.Size() << std::endl;
        std::cout << "Original value: ";
        for (size_t i = 0; i < value.Size(); i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)value[i] << " ";
        }
        std::cout << std::endl;
        
        // Serialize
        std::stringstream stream;
        neo::io::BinaryWriter writer(stream);
        item.Serialize(writer);
        
        std::cout << "Stream size after write: " << stream.str().size() << std::endl;
        std::cout << "Stream content: ";
        for (char c : stream.str()) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)(uint8_t)c << " ";
        }
        std::cout << std::endl;
        
        // Reset stream position
        stream.seekg(0);
        std::cout << "Stream position after reset: " << stream.tellg() << std::endl;
        
        // Deserialize
        neo::io::BinaryReader reader(stream);
        neo::persistence::StorageItem item2;
        item2.Deserialize(reader);
        
        std::cout << "Deserialized successfully!" << std::endl;
        std::cout << "Deserialized value size: " << item2.GetValue().Size() << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    
    return 0;
}
