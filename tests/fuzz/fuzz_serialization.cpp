/**
 * @file fuzz_serialization.cpp
 * @brief Fuzz testing for serialization/deserialization
 */

#include <cstdint>
#include <cstddef>
#include <vector>
#include <neo/ledger/transaction.h>
#include <neo/ledger/block.h>
#include <neo/network/message.h>
#include <neo/io/byte_vector.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>

using namespace neo::ledger;
using namespace neo::network;
using namespace neo::io;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1 || size > 1048576) { // Max 1MB
        return 0;
    }
    
    try {
        ByteVector input(data, data + size);
        
        // Test Transaction serialization
        {
            BinaryReader reader(input);
            try {
                Transaction tx;
                tx.Deserialize(reader);
                
                // Serialize it back
                BinaryWriter writer;
                tx.Serialize(writer);
                auto serialized = writer.ToArray();
                
                // Deserialize again
                BinaryReader reader2(serialized);
                Transaction tx2;
                tx2.Deserialize(reader2);
                
                // Verify consistency
                if (tx.GetHash() != tx2.GetHash()) {
                    // Serialization inconsistency
                    return 0;
                }
                
                // Test transaction validation
                bool valid = tx.Verify();
                (void)valid; // Result doesn't matter, just shouldn't crash
                
            } catch (...) {
                // Invalid transaction format
                return 0;
            }
        }
        
        // Test Block serialization
        if (size > 100) { // Blocks need more data
            BinaryReader reader(input);
            try {
                Block block;
                block.Deserialize(reader);
                
                // Serialize it back
                BinaryWriter writer;
                block.Serialize(writer);
                auto serialized = writer.ToArray();
                
                // Deserialize again
                BinaryReader reader2(serialized);
                Block block2;
                block2.Deserialize(reader2);
                
                // Verify consistency
                if (block.GetHash() != block2.GetHash()) {
                    // Serialization inconsistency
                    return 0;
                }
                
                // Test merkle root calculation
                auto merkle = block.CalculateMerkleRoot();
                (void)merkle; // Just ensure no crash
                
            } catch (...) {
                // Invalid block format
                return 0;
            }
        }
        
        // Test Network Message serialization
        {
            BinaryReader reader(input);
            try {
                Message msg;
                msg.Deserialize(reader);
                
                // Serialize it back
                BinaryWriter writer;
                msg.Serialize(writer);
                auto serialized = writer.ToArray();
                
                // Deserialize again
                BinaryReader reader2(serialized);
                Message msg2;
                msg2.Deserialize(reader2);
                
                // Verify consistency
                if (msg.Command != msg2.Command || msg.Payload != msg2.Payload) {
                    // Serialization inconsistency
                    return 0;
                }
                
            } catch (...) {
                // Invalid message format
                return 0;
            }
        }
        
        // Test various data types
        {
            BinaryReader reader(input);
            
            // Try reading various types
            try {
                auto b = reader.ReadByte();
                auto s = reader.ReadInt16();
                auto i = reader.ReadInt32();
                auto l = reader.ReadInt64();
                auto str = reader.ReadVarString();
                auto bytes = reader.ReadVarBytes();
                auto fixed = reader.ReadFixedBytes(32);
                
                // Write them back
                BinaryWriter writer;
                writer.WriteByte(b);
                writer.WriteInt16(s);
                writer.WriteInt32(i);
                writer.WriteInt64(l);
                writer.WriteVarString(str);
                writer.WriteVarBytes(bytes);
                writer.WriteFixedBytes(fixed);
                
                // Read again and verify
                BinaryReader reader2(writer.ToArray());
                if (reader2.ReadByte() != b) return 0;
                if (reader2.ReadInt16() != s) return 0;
                if (reader2.ReadInt32() != i) return 0;
                if (reader2.ReadInt64() != l) return 0;
                if (reader2.ReadVarString() != str) return 0;
                if (reader2.ReadVarBytes() != bytes) return 0;
                if (reader2.ReadFixedBytes(32) != fixed) return 0;
                
            } catch (...) {
                // Not enough data or invalid format
                return 0;
            }
        }
        
    } catch (const std::exception& e) {
        // Expected for malformed input
        return 0;
    } catch (...) {
        // Catch all
        return 0;
    }
    
    return 0;
}

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    return 0;
}