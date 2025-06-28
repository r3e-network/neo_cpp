#include <iostream>
#include <sstream>
#include <iomanip>
#include <neo/ledger/transaction.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/byte_vector.h>

using namespace neo;
using namespace neo::ledger;
using namespace neo::io;

int main() {
    try {
        // Test 1: ContractTransaction (no extra fields)
        {
            std::cout << "Test 1: ContractTransaction\n";
            Transaction tx;
            tx.SetType(Transaction::Type::ContractTransaction);
            tx.SetVersion(1);
            
            std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
            BinaryWriter writer(stream);
            tx.Serialize(writer);
            
            std::cout << "Serialized " << stream.tellp() << " bytes\n";
            
            stream.seekg(0);
            BinaryReader reader(stream);
            Transaction tx2;
            tx2.Deserialize(reader);
            std::cout << "Deserialized successfully!\n\n";
        }
        
        // Test 2: InvocationTransaction (with extra fields)
        {
            std::cout << "Test 2: InvocationTransaction\n";
            Transaction tx;
            tx.SetType(Transaction::Type::InvocationTransaction);
            tx.SetVersion(1);
            
            std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
            BinaryWriter writer(stream);
            
            // Serialize manually to see what's expected
            writer.Write(static_cast<uint8_t>(0xd1)); // InvocationTransaction
            writer.Write(static_cast<uint8_t>(1));     // Version
            
            // Expected: script and gas fields here
            writer.WriteVarInt(0);  // Empty script
            writer.Write(Fixed8(0)); // Zero gas
            
            // Then attributes, inputs, outputs, witnesses
            writer.WriteVarInt(0);  // No attributes
            writer.WriteVarInt(0);  // No inputs
            writer.WriteVarInt(0);  // No outputs
            writer.WriteVarInt(0);  // No witnesses
            
            std::cout << "Manually serialized " << stream.tellp() << " bytes\n";
            
            // Try to deserialize
            stream.seekg(0);
            BinaryReader reader(stream);
            Transaction tx2;
            
            try {
                tx2.Deserialize(reader);
                std::cout << "Deserialized successfully!\n";
            } catch (const std::exception& e) {
                std::cout << "Failed: " << e.what() << "\n";
            }
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << "\n";
        return 1;
    }
}