#include <iostream>
#include <sstream>
#include <neo/ledger/transaction_attribute.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>

using namespace neo::ledger;
using namespace neo::io;

int main() {
    // Create a transaction attribute
    TransactionAttribute::Usage usage = TransactionAttribute::Usage::Script;
    ByteVector data = ByteVector::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    
    std::cout << "Original data size: " << data.Size() << std::endl;
    std::cout << "Original data: " << data.ToHexString() << std::endl;
    
    TransactionAttribute attribute(usage, data);
    
    // Serialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    attribute.Serialize(writer);
    
    // Get serialized data
    std::string serialized = stream.str();
    std::cout << "Serialized size: " << serialized.size() << std::endl;
    std::cout << "Serialized data: ";
    for (unsigned char c : serialized) {
        printf("%02x", c);
    }
    std::cout << std::endl;
    
    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    TransactionAttribute attribute2;
    attribute2.Deserialize(reader);
    
    std::cout << "Deserialized data size: " << attribute2.GetData().Size() << std::endl;
    std::cout << "Deserialized data: " << attribute2.GetData().ToHexString() << std::endl;
    
    return 0;
}