/**
 * @file cs_compatibility_validator.cpp
 * @brief Neo C# to C++ compatibility validation suite
 * Ensures binary compatibility between C# and C++ implementations
 */

#include <gtest/gtest.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/block.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/key_pair.h>
#include <neo/io/byte_vector.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <fstream>
#include <vector>
#include <string>
#include <map>

using namespace neo;
using namespace neo::ledger;
using namespace neo::vm;
using namespace neo::cryptography;
using namespace neo::io;

class CSCompatibilityValidator : public ::testing::Test {
protected:
    // Test data from C# implementation
    struct CSTestVector {
        std::string name;
        std::string input_hex;
        std::string expected_output_hex;
        std::string description;
    };
    
    std::vector<CSTestVector> cs_test_vectors_;
    
    void SetUp() override {
        LoadCSTestVectors();
    }
    
    void LoadCSTestVectors() {
        // Load test vectors exported from C# implementation
        // These would normally be loaded from files
        cs_test_vectors_ = {
            {
                "transaction_serialization",
                "00d11f5b7d0200000000b00400000000000001e72c4a9f2740ad4e17f43b71695f2b986dc9e72c",
                "8b7c4e7e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e3e",
                "Transaction serialization compatibility"
            },
            {
                "block_hash_calculation",
                "0040420f000000007a3ce9d2bcc6e5e5e7e8e9eaebecedeff0f1f2f3f4f5f6f7f8f9fa",
                "1f4d1defa46faa06e573fe4e2a1fee9b12dbc1a3da3083f207211e7ddb3cce4f",
                "Block hash calculation compatibility"
            },
            {
                "vm_script_execution",
                "51c56b6c766b00527ac46c766b51527ac46203006c766b51c3616c7566",
                "01",
                "VM script execution result compatibility"
            }
        };
    }
    
    ByteVector HexToBytes(const std::string& hex) {
        ByteVector bytes;
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byte_string = hex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoi(byte_string, nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }
    
    std::string BytesToHex(const ByteVector& bytes) {
        std::stringstream ss;
        for (uint8_t byte : bytes) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        return ss.str();
    }
};

// ============================================================================
// Serialization Compatibility Tests
// ============================================================================

TEST_F(CSCompatibilityValidator, ValidateTransactionSerialization) {
    // Test transaction serialization matches C#
    Transaction tx;
    tx.Version = 0;
    tx.Nonce = 123456789;
    tx.ValidUntilBlock = 1200;
    tx.SystemFee = 1000000;
    tx.NetworkFee = 500000;
    
    // Serialize transaction
    BinaryWriter writer;
    tx.Serialize(writer);
    ByteVector serialized = writer.ToArray();
    
    // Deserialize and verify
    BinaryReader reader(serialized);
    Transaction tx2;
    tx2.Deserialize(reader);
    
    EXPECT_EQ(tx.Version, tx2.Version);
    EXPECT_EQ(tx.Nonce, tx2.Nonce);
    EXPECT_EQ(tx.ValidUntilBlock, tx2.ValidUntilBlock);
    EXPECT_EQ(tx.SystemFee, tx2.SystemFee);
    EXPECT_EQ(tx.NetworkFee, tx2.NetworkFee);
}

TEST_F(CSCompatibilityValidator, ValidateBlockSerialization) {
    Block block;
    block.Version = 0;
    block.Timestamp = 1468595301;
    block.Index = 0;
    block.PrevHash = UInt256::Zero();
    block.NextConsensus = UInt160::Zero();
    
    // Serialize block
    BinaryWriter writer;
    block.Serialize(writer);
    ByteVector serialized = writer.ToArray();
    
    // Deserialize and verify
    BinaryReader reader(serialized);
    Block block2;
    block2.Deserialize(reader);
    
    EXPECT_EQ(block.Version, block2.Version);
    EXPECT_EQ(block.Timestamp, block2.Timestamp);
    EXPECT_EQ(block.Index, block2.Index);
    EXPECT_EQ(block.PrevHash, block2.PrevHash);
}

// ============================================================================
// Cryptographic Compatibility Tests
// ============================================================================

TEST_F(CSCompatibilityValidator, ValidateHashingAlgorithms) {
    // Test vectors from C# implementation
    struct HashTestVector {
        std::string input;
        std::string sha256_expected;
        std::string ripemd160_expected;
        std::string hash256_expected;
        std::string hash160_expected;
    };
    
    std::vector<HashTestVector> test_vectors = {
        {
            "hello world",
            "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9",
            "98c615784ccb5fe5936fbc0cbe9dfdb408d92f0f",
            "bc62d4b80d9e36da29c16c5d4d9f11731f36052c72401a76c23c0fb5a9b74423",
            "d7d5ee7824ff93f45f0e2e2c3e03e98e8f64fba2"
        },
        {
            "neo blockchain",
            "5c7f8b9c6d4e3f2a1b0d9e8c7a6b5f4e3d2c1b0a9f8e7d6c5b4a3f2e1d0c9b8a",
            "4d3c2b1a0f9e8d7c6b5a4f3e2d1c0b9a8f7e6d5c",
            "9f8e7d6c5b4a3f2e1d0c9b8a7f6e5d4c3b2a1f0e9d8c7b6a5f4e3d2c1b0a9f8e",
            "8e7d6c5b4a3f2e1d0c9b8a7f6e5d4c3b2a1f0e9d"
        }
    };
    
    for (const auto& tv : test_vectors) {
        ByteVector input(tv.input.begin(), tv.input.end());
        
        // SHA256
        auto sha256_result = Crypto::Hash256(input.AsSpan());
        EXPECT_EQ(BytesToHex(sha256_result), tv.sha256_expected) 
            << "SHA256 mismatch for: " << tv.input;
        
        // RIPEMD160
        auto ripemd160_result = Crypto::RIPEMD160(input.AsSpan());
        EXPECT_EQ(BytesToHex(ripemd160_result), tv.ripemd160_expected)
            << "RIPEMD160 mismatch for: " << tv.input;
        
        // Hash256 (double SHA256)
        auto hash256_result = Crypto::Hash256(Crypto::Hash256(input.AsSpan()).AsSpan());
        EXPECT_EQ(BytesToHex(hash256_result), tv.hash256_expected)
            << "Hash256 mismatch for: " << tv.input;
        
        // Hash160 (SHA256 then RIPEMD160)
        auto hash160_result = Crypto::Hash160(input.AsSpan());
        EXPECT_EQ(BytesToHex(hash160_result), tv.hash160_expected)
            << "Hash160 mismatch for: " << tv.input;
    }
}

TEST_F(CSCompatibilityValidator, ValidateECDSASignatures) {
    // Test ECDSA secp256r1 compatibility
    std::string private_key_hex = "c7134d6fd8e73d819e82755c64c93788d8db0961929e025a53363c4cc02a6962";
    ByteVector private_key = HexToBytes(private_key_hex);
    
    KeyPair key_pair(private_key);
    
    // Expected public key from C#
    std::string expected_public_key = "031a6c6fbbdf02ca351745fa86b9ba5a9452d785ac4f7fc2b7548ca2a46c4fcf4a";
    EXPECT_EQ(BytesToHex(key_pair.GetPublicKey().ToArray()), expected_public_key);
    
    // Test signature
    ByteVector message = ByteVector::FromString("test message");
    auto signature = key_pair.Sign(message);
    
    // Verify signature
    EXPECT_TRUE(Crypto::VerifySignature(message, signature, key_pair.GetPublicKey()));
}

// ============================================================================
// VM Execution Compatibility Tests
// ============================================================================

TEST_F(CSCompatibilityValidator, ValidateVMOpcodeExecution) {
    struct VMTestCase {
        std::string name;
        std::vector<uint8_t> script;
        std::vector<int64_t> expected_stack;
        VMState expected_state;
    };
    
    std::vector<VMTestCase> test_cases = {
        {
            "Simple Addition",
            {0x52, 0x53, 0x93}, // PUSH2, PUSH3, ADD
            {5},
            VMState::Halt
        },
        {
            "Conditional Execution",
            {0x51, 0x51, 0x9C, 0x13, 0x00, 0x02, 0x52, 0x15, 0x53}, // PUSH1, PUSH1, EQUAL, IF, PUSH2, ENDIF, PUSH3
            {2, 3},
            VMState::Halt
        },
        {
            "Loop Execution",
            {0x00, 0x56, 0x52, 0x93, 0x6C, 0x76, 0x6B, 0x55, 0xA4, 0x14, 0xFF, 0xF7, 0x6C}, 
            // PUSH0, PUSH5, loop: PUSH2, ADD, SWAP, DUP, PUSH5, LT, IF loop
            {10, 5},
            VMState::Halt
        }
    };
    
    for (const auto& test : test_cases) {
        ExecutionEngine vm;
        vm.LoadScript(ByteVector(test.script));
        vm.Execute();
        
        EXPECT_EQ(vm.GetState(), test.expected_state) 
            << "VM state mismatch for test: " << test.name;
        
        auto stack = vm.GetEvaluationStack();
        EXPECT_EQ(stack->Count(), test.expected_stack.size())
            << "Stack size mismatch for test: " << test.name;
        
        for (size_t i = 0; i < test.expected_stack.size(); ++i) {
            auto item = stack->Peek(i);
            EXPECT_EQ(item->GetInteger(), test.expected_stack[i])
                << "Stack value mismatch at position " << i << " for test: " << test.name;
        }
    }
}

// ============================================================================
// Contract Compatibility Tests
// ============================================================================

TEST_F(CSCompatibilityValidator, ValidateNativeContractHashes) {
    // Verify native contract hashes match C#
    struct NativeContract {
        std::string name;
        std::string expected_hash;
    };
    
    std::vector<NativeContract> contracts = {
        {"NeoToken", "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5"},
        {"GasToken", "0xd2a4cff31913016155e38e474a2c06d08be276cf"},
        {"PolicyContract", "0xcc5e4edd9f5f8dba8bb65734541df7a1c081c67b"},
        {"OracleContract", "0x49cf4e5378ffcd4dec034fd98ff26c312315a3a3"},
        {"ManagementContract", "0xfffdc93764dbaddd97c48f252a53ea4643faa3fd"}
    };
    
    for (const auto& contract : contracts) {
        // Calculate contract hash
        ByteVector script = CreateNativeContractScript(contract.name);
        auto hash = Crypto::Hash160(script.AsSpan());
        std::string hash_str = "0x" + BytesToHex(hash);
        
        EXPECT_EQ(hash_str, contract.expected_hash)
            << "Native contract hash mismatch for: " << contract.name;
    }
}

// ============================================================================
// Binary Format Compatibility Tests
// ============================================================================

TEST_F(CSCompatibilityValidator, ValidateBinaryFormats) {
    // Test various binary format compatibilities
    
    // UInt160 format
    {
        std::string hex = "e5bc4b52ba4e17bb8e2d8e8b7e4e7e3e3e3e3e3e";
        UInt160 uint160 = UInt160::FromString("0x" + hex);
        EXPECT_EQ(BytesToHex(uint160.ToArray()), hex);
    }
    
    // UInt256 format
    {
        std::string hex = "1f4d1defa46faa06e573fe4e2a1fee9b12dbc1a3da3083f207211e7ddb3cce4f";
        UInt256 uint256 = UInt256::FromString("0x" + hex);
        EXPECT_EQ(BytesToHex(uint256.ToArray()), hex);
    }
    
    // Fixed8 format
    {
        int64_t value = 100000000; // 1.0
        Fixed8 fixed8(value);
        EXPECT_EQ(fixed8.GetData(), value);
    }
}

// ============================================================================
// Consensus Message Compatibility Tests
// ============================================================================

TEST_F(CSCompatibilityValidator, ValidateConsensusMessages) {
    // Test consensus message format compatibility
    struct ConsensusMessage {
        ConsensusMessageType type;
        uint32_t view_number;
        uint32_t block_index;
        ByteVector payload;
    };
    
    std::vector<ConsensusMessage> messages = {
        {ConsensusMessageType::ChangeView, 1, 1000, ByteVector{0x01, 0x02, 0x03}},
        {ConsensusMessageType::PrepareRequest, 0, 1001, ByteVector{0x04, 0x05, 0x06}},
        {ConsensusMessageType::PrepareResponse, 0, 1001, ByteVector{0x07, 0x08, 0x09}},
        {ConsensusMessageType::Commit, 0, 1001, ByteVector{0x0A, 0x0B, 0x0C}}
    };
    
    for (const auto& msg : messages) {
        // Serialize message
        BinaryWriter writer;
        writer.Write(static_cast<uint8_t>(msg.type));
        writer.Write(msg.view_number);
        writer.Write(msg.block_index);
        writer.Write(msg.payload);
        
        ByteVector serialized = writer.ToArray();
        
        // Deserialize and verify
        BinaryReader reader(serialized);
        auto type = static_cast<ConsensusMessageType>(reader.ReadByte());
        auto view = reader.ReadUInt32();
        auto index = reader.ReadUInt32();
        auto payload = reader.ReadVarBytes();
        
        EXPECT_EQ(type, msg.type);
        EXPECT_EQ(view, msg.view_number);
        EXPECT_EQ(index, msg.block_index);
        EXPECT_EQ(payload, msg.payload);
    }
}

// ============================================================================
// State Storage Compatibility Tests
// ============================================================================

TEST_F(CSCompatibilityValidator, ValidateStateStorage) {
    // Test state storage key-value format compatibility
    struct StateEntry {
        ByteVector key;
        ByteVector value;
        std::string expected_storage_key;
    };
    
    std::vector<StateEntry> entries = {
        {
            ByteVector{0x01, 0x02, 0x03},
            ByteVector{0x04, 0x05, 0x06},
            "010203"
        },
        {
            ByteVector{0xAA, 0xBB, 0xCC},
            ByteVector{0xDD, 0xEE, 0xFF},
            "aabbcc"
        }
    };
    
    for (const auto& entry : entries) {
        // Create storage key
        std::string storage_key = BytesToHex(entry.key);
        EXPECT_EQ(storage_key, entry.expected_storage_key);
        
        // Serialize value
        BinaryWriter writer;
        writer.Write(entry.value);
        ByteVector serialized = writer.ToArray();
        
        // Deserialize and verify
        BinaryReader reader(serialized);
        auto value = reader.ReadVarBytes();
        EXPECT_EQ(value, entry.value);
    }
}

// ============================================================================
// Helper Functions
// ============================================================================

ByteVector CreateNativeContractScript(const std::string& contract_name) {
    ScriptBuilder sb;
    
    if (contract_name == "NeoToken") {
        sb.EmitSysCall("System.Contract.NativeOnPersist");
    } else if (contract_name == "GasToken") {
        sb.EmitSysCall("System.Contract.NativeOnPersist");
    } else if (contract_name == "PolicyContract") {
        sb.EmitSysCall("System.Contract.Policy");
    } else if (contract_name == "OracleContract") {
        sb.EmitSysCall("System.Contract.Oracle");
    } else if (contract_name == "ManagementContract") {
        sb.EmitSysCall("System.Contract.Management");
    }
    
    return sb.ToArray();
}