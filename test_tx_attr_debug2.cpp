#include <iostream>
#include <sstream>
#include <neo/ledger/transaction_attribute.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/byte_vector.h>

using namespace neo;
using namespace neo::ledger;
using namespace neo::io;

class DebugTransactionAttribute : public TransactionAttribute
{
public:
    void DebugDeserialize(BinaryReader& reader)
    {
        auto usage_byte = reader.ReadUInt8();
        std::cout << "Read usage byte: 0x" << std::hex << static_cast<int>(usage_byte) << "\n";
        
        SetUsage(static_cast<Usage>(usage_byte));
        auto usage = GetUsage();
        
        std::cout << "Usage enum value: 0x" << std::hex << static_cast<int>(usage) << "\n";
        
        // Check which branch would be taken
        if (usage == Usage::HighPriority) {
            std::cout << "Branch: HighPriority (no data)\n";
        }
        else if (usage == Usage::OracleResponse) {
            std::cout << "Branch: OracleResponse\n";
        }
        else if (usage == Usage::Script) {
            std::cout << "Branch: Script (should read 20 bytes)\n";
            SetData(reader.ReadBytes(20));
        }
        else if (usage == Usage::NotValidBefore) {
            std::cout << "Branch: NotValidBefore (should read 4 bytes)\n";
            SetData(reader.ReadBytes(sizeof(uint32_t)));
        }
        else if (usage == Usage::Conflicts) {
            std::cout << "Branch: Conflicts\n";
        }
        else {
            std::cout << "Branch: Other\n";
        }
    }
};

int main() {
    try {
        std::cout << "Testing TransactionAttribute deserialization with debug...\n";
        
        // Create a transaction attribute
        TransactionAttribute::Usage usage = TransactionAttribute::Usage::Script;
        ByteVector data = ByteVector::Parse("0102030405060708090a0b0c0d0e0f1011121314");
        TransactionAttribute attribute(usage, data);
        
        std::cout << "Original usage: 0x" << std::hex << static_cast<int>(usage) << "\n";
        std::cout << "Original data: " << data.ToHexString() << " (" << data.Size() << " bytes)\n";
        
        // Serialize
        std::stringstream stream;
        BinaryWriter writer(stream);
        attribute.Serialize(writer);
        
        // Check serialized data
        std::string serialized = stream.str();
        std::cout << "\nSerialized bytes: ";
        for (unsigned char c : serialized) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c) << " ";
        }
        std::cout << "\n";
        std::cout << "Total serialized: " << serialized.size() << " bytes\n";
        
        // Deserialize with debug
        stream.seekg(0);
        BinaryReader reader(stream);
        DebugTransactionAttribute attribute2;
        attribute2.DebugDeserialize(reader);
        
        // Check result
        auto result_data = attribute2.GetData();
        std::cout << "\nDeserialized data: " << result_data.ToHexString() << " (" << result_data.Size() << " bytes)\n";
        std::cout << "Data match: " << (result_data == data ? "YES" : "NO") << "\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
}