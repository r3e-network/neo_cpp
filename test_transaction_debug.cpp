#include <iostream>
#include <sstream>
#include <iomanip>
#include <neo/ledger/transaction.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/ledger/coin_reference.h>
#include <neo/ledger/transaction_output.h>
#include <neo/ledger/witness.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/byte_vector.h>
#include <neo/io/fixed8.h>

using namespace neo;
using namespace neo::ledger;
using namespace neo::io;

int main() {
    try {
        std::cout << "Testing Transaction serialization (InvocationTransaction)...\n";
        
        // Create a transaction
        Transaction tx;
        tx.SetType(Transaction::Type::InvocationTransaction);
        tx.SetVersion(1);
        
        // Add attributes
        TransactionAttribute::Usage usage = TransactionAttribute::Usage::Script;
        ByteVector data = ByteVector::Parse("0102030405");
        TransactionAttribute attribute(usage, data);
        tx.SetAttributes({attribute});
        
        // Add inputs
        UInt256 prevHash = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
        uint16_t prevIndex = 123;
        CoinReference input(prevHash, prevIndex);
        tx.SetInputs({input});
        
        // Add outputs
        UInt256 assetId = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
        Fixed8 value(123);
        UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
        TransactionOutput output(assetId, value, scriptHash);
        tx.SetOutputs({output});
        
        // Add witnesses
        ByteVector invocationScript = ByteVector::Parse("0102030405");
        ByteVector verificationScript = ByteVector::Parse("0607080910");
        Witness witness(invocationScript, verificationScript);
        tx.SetWitnesses({witness});
        
        // Serialize
        std::cout << "\nSerializing transaction...\n";
        std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
        BinaryWriter writer(stream);
        
        std::cout << "Calling tx.Serialize(writer)...\n";
        tx.Serialize(writer);
        
        std::cout << "Serialized " << stream.tellp() << " bytes\n";
        
        // Print serialized data
        std::string serialized = stream.str();
        std::cout << "Serialized data (" << serialized.size() << " bytes): ";
        for (size_t i = 0; i < serialized.size() && i < 50; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(static_cast<unsigned char>(serialized[i])) << " ";
        }
        if (serialized.size() > 50) std::cout << "...";
        std::cout << "\n";
        
        // Deserialize
        std::cout << "\nDeserializing transaction...\n";
        stream.seekg(0);
        
        // Print stream state
        std::cout << "Stream state before deserialization:\n";
        std::cout << "  good: " << stream.good() << "\n";
        std::cout << "  eof: " << stream.eof() << "\n";
        std::cout << "  fail: " << stream.fail() << "\n";
        std::cout << "  bad: " << stream.bad() << "\n";
        std::cout << "  tellg: " << stream.tellg() << "\n";
        
        BinaryReader reader(stream);
        Transaction tx2;
        
        try {
            std::cout << "Calling tx2.Deserialize(reader)...\n";
            tx2.Deserialize(reader);
            std::cout << "Deserialized successfully!\n";
        } catch (const std::exception& e) {
            std::cout << "Exception during deserialization: " << e.what() << "\n";
            
            // Re-read to see what's there
            stream.clear();
            stream.seekg(0);
            std::cout << "\nTrying manual deserialization...\n";
            
            // Read type
            uint8_t type;
            stream.read(reinterpret_cast<char*>(&type), 1);
            std::cout << "Type: 0x" << std::hex << static_cast<int>(type) << " (InvocationTransaction = 0xd1)\n";
            
            // Read version  
            uint8_t version;
            stream.read(reinterpret_cast<char*>(&version), 1);
            std::cout << "Version: " << static_cast<int>(version) << "\n";
            
            // Check if this is InvocationTransaction
            if (type == 0xd1) {
                std::cout << "This is an InvocationTransaction - might need special handling!\n";
                
                // Check stream position
                std::cout << "Stream position: " << stream.tellg() << " / " << serialized.size() << "\n";
            }
            
            return 1;
        }
        
        // Verify
        std::cout << "\nVerifying deserialized transaction...\n";
        std::cout << "Type match: " << (tx2.GetType() == Transaction::Type::InvocationTransaction ? "YES" : "NO") << "\n";
        std::cout << "Version match: " << (tx2.GetVersion() == 1 ? "YES" : "NO") << "\n";
        std::cout << "Attributes count: " << tx2.GetAttributes().size() << "\n";
        std::cout << "Inputs count: " << tx2.GetInputs().size() << "\n";
        std::cout << "Outputs count: " << tx2.GetOutputs().size() << "\n";
        std::cout << "Witnesses count: " << tx2.GetWitnesses().size() << "\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << "\n";
        return 1;
    }
}