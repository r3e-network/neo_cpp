#include <iostream>
#include <sstream>
#include <iomanip>
#include <neo/ledger/transaction.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>

using namespace neo;
using namespace neo::ledger;
using namespace neo::io;

void printHex(const std::string& data, size_t limit = 100) {
    std::cout << "Hex (" << data.size() << " bytes): ";
    for (size_t i = 0; i < data.size() && i < limit; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(static_cast<unsigned char>(data[i])) << " ";
    }
    if (data.size() > limit) std::cout << "...";
    std::cout << std::dec << "\n";
}

int main() {
    try {
        // Create an InvocationTransaction
        Transaction tx;
        tx.SetType(Transaction::Type::InvocationTransaction);
        tx.SetVersion(1);
        
        // Serialize
        std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
        BinaryWriter writer(stream);
        tx.Serialize(writer);
        
        std::string serialized = stream.str();
        std::cout << "Serialized transaction:\n";
        printHex(serialized);
        
        // Try to deserialize
        stream.seekg(0);
        BinaryReader reader(stream);
        
        // Read manually
        uint8_t type = reader.ReadUInt8();
        std::cout << "\nType: 0x" << std::hex << static_cast<int>(type) << " (expected 0xd1)\n";
        
        uint8_t version = reader.ReadUInt8();
        std::cout << "Version: " << static_cast<int>(version) << "\n";
        
        // For InvocationTransaction, we should see script and gas
        if (type == 0xd1) {
            std::cout << "\nReading InvocationTransaction fields...\n";
            
            // Read script length
            std::cout << "Position before reading script: " << stream.tellg() << "\n";
            
            try {
                auto scriptSize = reader.ReadVarInt();
                std::cout << "Script size: " << scriptSize << "\n";
                
                if (scriptSize > 0) {
                    auto script = reader.ReadBytes(scriptSize);
                    std::cout << "Script read successfully\n";
                }
                
                // Read gas
                Fixed8 gas = reader.Read<Fixed8>();
                std::cout << "Gas: " << gas.Value() << "\n";
                
            } catch (const std::exception& e) {
                std::cout << "Error reading InvocationTransaction fields: " << e.what() << "\n";
                return 1;
            }
        }
        
        // Now deserialize normally
        stream.seekg(0);
        BinaryReader reader2(stream);
        Transaction tx2;
        
        std::cout << "\nDeserializing with Transaction::Deserialize...\n";
        tx2.Deserialize(reader2);
        std::cout << "Success!\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
}