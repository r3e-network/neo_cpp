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
        std::cout << "Testing TransactionAttribute serialization (no debug calls)...\n";
        
        // Create a transaction attribute exactly like the test
        TransactionAttribute::Usage usage = TransactionAttribute::Usage::Script;
        ByteVector data = ByteVector::Parse("0102030405060708090a0b0c0d0e0f1011121314");
        TransactionAttribute attribute(usage, data);
        
        // Serialize
        std::stringstream stream;
        BinaryWriter writer(stream);
        attribute.Serialize(writer);
        
        // Deserialize without any Available() calls
        stream.seekg(0);
        BinaryReader reader(stream);
        TransactionAttribute attribute2;
        attribute2.Deserialize(reader);
        
        // Check result
        auto result_data = attribute2.GetData();
        std::cout << "Original data size: " << data.Size() << " bytes\n";
        std::cout << "Deserialized data size: " << result_data.Size() << " bytes\n";
        std::cout << "Data match: " << (attribute2.GetData() == data ? "YES" : "NO") << "\n";
        
        if (attribute2.GetData() != data) {
            std::cout << "Original: " << data.ToHexString() << "\n";
            std::cout << "Result:   " << result_data.ToHexString() << "\n";
        }
        
        return (attribute2.GetData() == data) ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
}