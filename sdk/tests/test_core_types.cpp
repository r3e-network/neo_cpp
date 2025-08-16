/**
 * @file test_core_types.cpp
 * @brief Unit tests for SDK core types
 * @author Neo C++ Team
 * @date 2025
 */

#include <gtest/gtest.h>
#include <neo/sdk/core/types.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>

using namespace neo::sdk::core;
using namespace neo::io;

class CoreTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

// Test Block structure
TEST_F(CoreTypesTest, BlockConstruction) {
    Block block;
    block.Index = 100;
    block.Timestamp = 1640000000;
    block.Version = 0;
    block.MerkleRoot = UInt256::Zero();
    block.PrevHash = UInt256::Zero();
    block.NextConsensus = UInt160::Zero();
    
    EXPECT_EQ(block.Index, 100);
    EXPECT_EQ(block.Timestamp, 1640000000);
    EXPECT_EQ(block.Version, 0);
    EXPECT_EQ(block.MerkleRoot, UInt256::Zero());
    EXPECT_EQ(block.PrevHash, UInt256::Zero());
    EXPECT_EQ(block.NextConsensus, UInt160::Zero());
}

TEST_F(CoreTypesTest, BlockHash) {
    Block block;
    block.Index = 0;
    block.Timestamp = 1468595301;
    block.Version = 0;
    
    // Genesis block should have a deterministic hash
    auto hash = block.GetHash();
    EXPECT_NE(hash, UInt256::Zero());
}

// Test Transaction structure
TEST_F(CoreTypesTest, TransactionConstruction) {
    Transaction tx;
    tx.Version = 0;
    tx.Nonce = 12345;
    tx.SystemFee = 1000000;
    tx.NetworkFee = 500000;
    tx.ValidUntilBlock = 1000;
    
    EXPECT_EQ(tx.Version, 0);
    EXPECT_EQ(tx.Nonce, 12345);
    EXPECT_EQ(tx.SystemFee, 1000000);
    EXPECT_EQ(tx.NetworkFee, 500000);
    EXPECT_EQ(tx.ValidUntilBlock, 1000);
}

TEST_F(CoreTypesTest, TransactionSerialization) {
    Transaction tx;
    tx.Version = 0;
    tx.Nonce = 42;
    tx.SystemFee = 1000000;
    tx.NetworkFee = 500000;
    tx.ValidUntilBlock = 999;
    
    // Serialize
    BinaryWriter writer;
    tx.Serialize(writer);
    auto data = writer.ToArray();
    
    // Deserialize
    BinaryReader reader(data);
    Transaction tx2;
    tx2.Deserialize(reader);
    
    EXPECT_EQ(tx.Version, tx2.Version);
    EXPECT_EQ(tx.Nonce, tx2.Nonce);
    EXPECT_EQ(tx.SystemFee, tx2.SystemFee);
    EXPECT_EQ(tx.NetworkFee, tx2.NetworkFee);
    EXPECT_EQ(tx.ValidUntilBlock, tx2.ValidUntilBlock);
}

TEST_F(CoreTypesTest, TransactionHash) {
    Transaction tx;
    tx.Version = 0;
    tx.Nonce = 12345;
    
    auto hash1 = tx.GetHash();
    tx.Nonce = 54321;
    auto hash2 = tx.GetHash();
    
    // Different nonces should produce different hashes
    EXPECT_NE(hash1, hash2);
}

// Test ContractParameter types
TEST_F(CoreTypesTest, ContractParameterBoolean) {
    ContractParameter param(true);
    
    EXPECT_EQ(param.GetType(), ContractParameterType::Boolean);
    EXPECT_TRUE(param.GetBoolean());
    
    ContractParameter param2(false);
    EXPECT_FALSE(param2.GetBoolean());
}

TEST_F(CoreTypesTest, ContractParameterInteger) {
    ContractParameter param(int64_t(12345));
    
    EXPECT_EQ(param.GetType(), ContractParameterType::Integer);
    EXPECT_EQ(param.GetInteger(), 12345);
}

TEST_F(CoreTypesTest, ContractParameterByteArray) {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    ContractParameter param(data);
    
    EXPECT_EQ(param.GetType(), ContractParameterType::ByteArray);
    EXPECT_EQ(param.GetByteArray(), data);
}

TEST_F(CoreTypesTest, ContractParameterString) {
    std::string str = "Hello, Neo!";
    ContractParameter param(str);
    
    EXPECT_EQ(param.GetType(), ContractParameterType::String);
    EXPECT_EQ(param.GetString(), str);
}

TEST_F(CoreTypesTest, ContractParameterHash160) {
    UInt160 hash = UInt160::Zero();
    ContractParameter param(hash);
    
    EXPECT_EQ(param.GetType(), ContractParameterType::Hash160);
    EXPECT_EQ(param.GetHash160(), hash);
}

TEST_F(CoreTypesTest, ContractParameterHash256) {
    UInt256 hash = UInt256::Zero();
    ContractParameter param(hash);
    
    EXPECT_EQ(param.GetType(), ContractParameterType::Hash256);
    EXPECT_EQ(param.GetHash256(), hash);
}

TEST_F(CoreTypesTest, ContractParameterArray) {
    std::vector<ContractParameter> array;
    array.emplace_back(int64_t(1));
    array.emplace_back(int64_t(2));
    array.emplace_back(int64_t(3));
    
    ContractParameter param(array);
    
    EXPECT_EQ(param.GetType(), ContractParameterType::Array);
    EXPECT_EQ(param.GetArray().size(), 3);
    EXPECT_EQ(param.GetArray()[0].GetInteger(), 1);
    EXPECT_EQ(param.GetArray()[1].GetInteger(), 2);
    EXPECT_EQ(param.GetArray()[2].GetInteger(), 3);
}

TEST_F(CoreTypesTest, ContractParameterMap) {
    std::map<ContractParameter, ContractParameter> map;
    map[ContractParameter(std::string("key1"))] = ContractParameter(int64_t(100));
    map[ContractParameter(std::string("key2"))] = ContractParameter(int64_t(200));
    
    ContractParameter param(map);
    
    EXPECT_EQ(param.GetType(), ContractParameterType::Map);
    EXPECT_EQ(param.GetMap().size(), 2);
}

// Test Script structure
TEST_F(CoreTypesTest, ScriptConstruction) {
    std::vector<uint8_t> scriptData = {0x00, 0x01, 0x02, 0x03};
    Script script(scriptData);
    
    EXPECT_EQ(script.GetData(), scriptData);
    EXPECT_EQ(script.GetSize(), 4);
}

TEST_F(CoreTypesTest, ScriptHash) {
    std::vector<uint8_t> scriptData = {0x00, 0x01, 0x02, 0x03};
    Script script(scriptData);
    
    auto hash = script.GetScriptHash();
    EXPECT_NE(hash, UInt160::Zero());
    
    // Same script should produce same hash
    Script script2(scriptData);
    EXPECT_EQ(script.GetScriptHash(), script2.GetScriptHash());
    
    // Different script should produce different hash
    std::vector<uint8_t> scriptData2 = {0x04, 0x05, 0x06, 0x07};
    Script script3(scriptData2);
    EXPECT_NE(script.GetScriptHash(), script3.GetScriptHash());
}

// Test Witness structure
TEST_F(CoreTypesTest, WitnessConstruction) {
    std::vector<uint8_t> invocation = {0x01, 0x02, 0x03};
    std::vector<uint8_t> verification = {0x04, 0x05, 0x06};
    
    Witness witness;
    witness.InvocationScript = invocation;
    witness.VerificationScript = verification;
    
    EXPECT_EQ(witness.InvocationScript, invocation);
    EXPECT_EQ(witness.VerificationScript, verification);
}

TEST_F(CoreTypesTest, WitnessSerialization) {
    Witness witness;
    witness.InvocationScript = {0x01, 0x02, 0x03};
    witness.VerificationScript = {0x04, 0x05, 0x06};
    
    // Serialize
    BinaryWriter writer;
    witness.Serialize(writer);
    auto data = writer.ToArray();
    
    // Deserialize
    BinaryReader reader(data);
    Witness witness2;
    witness2.Deserialize(reader);
    
    EXPECT_EQ(witness.InvocationScript, witness2.InvocationScript);
    EXPECT_EQ(witness.VerificationScript, witness2.VerificationScript);
}

// Test Signer structure
TEST_F(CoreTypesTest, SignerConstruction) {
    Signer signer;
    signer.Account = UInt160::Zero();
    signer.Scopes = WitnessScope::CalledByEntry;
    
    EXPECT_EQ(signer.Account, UInt160::Zero());
    EXPECT_EQ(signer.Scopes, WitnessScope::CalledByEntry);
}

TEST_F(CoreTypesTest, SignerWithContracts) {
    Signer signer;
    signer.Account = UInt160::Zero();
    signer.Scopes = WitnessScope::CustomContracts;
    
    UInt160 contract1 = UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678");
    UInt160 contract2 = UInt160::Parse("0xabcdef1234567890abcdef1234567890abcdef12");
    
    signer.AllowedContracts.push_back(contract1);
    signer.AllowedContracts.push_back(contract2);
    
    EXPECT_EQ(signer.AllowedContracts.size(), 2);
    EXPECT_EQ(signer.AllowedContracts[0], contract1);
    EXPECT_EQ(signer.AllowedContracts[1], contract2);
}

// Test TransactionAttribute
TEST_F(CoreTypesTest, TransactionAttributeHighPriority) {
    TransactionAttribute attr;
    attr.Type = TransactionAttributeType::HighPriority;
    
    EXPECT_EQ(attr.Type, TransactionAttributeType::HighPriority);
}

TEST_F(CoreTypesTest, TransactionAttributeOracleResponse) {
    TransactionAttribute attr;
    attr.Type = TransactionAttributeType::OracleResponse;
    attr.Data = {0x01, 0x02, 0x03, 0x04}; // Oracle response data
    
    EXPECT_EQ(attr.Type, TransactionAttributeType::OracleResponse);
    EXPECT_EQ(attr.Data.size(), 4);
}

// Test helper functions
TEST_F(CoreTypesTest, AddressFromScriptHash) {
    UInt160 scriptHash = UInt160::Zero();
    std::string address = AddressFromScriptHash(scriptHash);
    
    // Should produce a valid Neo address
    EXPECT_FALSE(address.empty());
    EXPECT_EQ(address[0], 'N'); // Neo mainnet addresses start with 'N'
}

TEST_F(CoreTypesTest, ScriptHashFromAddress) {
    std::string address = "NUVPACMnKFhpuHjsRjhUvXz1GhqfGWx2CT";
    auto scriptHash = ScriptHashFromAddress(address);
    
    EXPECT_TRUE(scriptHash.has_value());
    EXPECT_NE(scriptHash.value(), UInt160::Zero());
    
    // Invalid address should return nullopt
    auto invalidHash = ScriptHashFromAddress("invalid_address");
    EXPECT_FALSE(invalidHash.has_value());
}

// Test BlockHeader
TEST_F(CoreTypesTest, BlockHeaderBasics) {
    BlockHeader header;
    header.Version = 0;
    header.PrevHash = UInt256::Zero();
    header.MerkleRoot = UInt256::Zero();
    header.Timestamp = 1640000000;
    header.Index = 100;
    header.NextConsensus = UInt160::Zero();
    
    EXPECT_EQ(header.Version, 0);
    EXPECT_EQ(header.Index, 100);
    EXPECT_EQ(header.Timestamp, 1640000000);
}

// Test comprehensive Transaction building
TEST_F(CoreTypesTest, BuildCompleteTransaction) {
    Transaction tx;
    tx.Version = 0;
    tx.Nonce = 1234567890;
    tx.SystemFee = 1000000;
    tx.NetworkFee = 500000;
    tx.ValidUntilBlock = 999999;
    
    // Add signers
    Signer signer;
    signer.Account = UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678");
    signer.Scopes = WitnessScope::CalledByEntry;
    tx.Signers.push_back(signer);
    
    // Add attributes
    TransactionAttribute attr;
    attr.Type = TransactionAttributeType::HighPriority;
    tx.Attributes.push_back(attr);
    
    // Add script
    tx.Script = {0x00, 0x01, 0x02, 0x03};
    
    // Add witness
    Witness witness;
    witness.InvocationScript = {0x40}; // Signature placeholder
    witness.VerificationScript = {0x21}; // Public key placeholder
    tx.Witnesses.push_back(witness);
    
    // Verify transaction structure
    EXPECT_EQ(tx.Signers.size(), 1);
    EXPECT_EQ(tx.Attributes.size(), 1);
    EXPECT_EQ(tx.Script.size(), 4);
    EXPECT_EQ(tx.Witnesses.size(), 1);
    
    // Get transaction hash
    auto hash = tx.GetHash();
    EXPECT_NE(hash, UInt256::Zero());
}

// Test error conditions
TEST_F(CoreTypesTest, InvalidAddressHandling) {
    // Empty address
    auto hash1 = ScriptHashFromAddress("");
    EXPECT_FALSE(hash1.has_value());
    
    // Invalid characters
    auto hash2 = ScriptHashFromAddress("!@#$%^&*()");
    EXPECT_FALSE(hash2.has_value());
    
    // Wrong length
    auto hash3 = ScriptHashFromAddress("N123");
    EXPECT_FALSE(hash3.has_value());
    
    // Invalid checksum
    auto hash4 = ScriptHashFromAddress("NUVPACMnKFhpuHjsRjhUvXz1GhqfGWx2XX");
    EXPECT_FALSE(hash4.has_value());
}

TEST_F(CoreTypesTest, ContractParameterTypeConversion) {
    // Test type conversion and validation
    ContractParameter boolParam(true);
    EXPECT_THROW(boolParam.GetInteger(), std::runtime_error);
    EXPECT_THROW(boolParam.GetString(), std::runtime_error);
    
    ContractParameter intParam(int64_t(42));
    EXPECT_THROW(intParam.GetBoolean(), std::runtime_error);
    EXPECT_THROW(intParam.GetByteArray(), std::runtime_error);
    
    ContractParameter strParam(std::string("test"));
    EXPECT_THROW(strParam.GetInteger(), std::runtime_error);
    EXPECT_THROW(strParam.GetHash160(), std::runtime_error);
}

// Test edge cases
TEST_F(CoreTypesTest, MaxValueHandling) {
    // Test maximum values
    Transaction tx;
    tx.SystemFee = UINT64_MAX;
    tx.NetworkFee = UINT64_MAX;
    tx.ValidUntilBlock = UINT32_MAX;
    
    EXPECT_EQ(tx.SystemFee, UINT64_MAX);
    EXPECT_EQ(tx.NetworkFee, UINT64_MAX);
    EXPECT_EQ(tx.ValidUntilBlock, UINT32_MAX);
    
    // Test serialization with max values
    BinaryWriter writer;
    tx.Serialize(writer);
    auto data = writer.ToArray();
    
    BinaryReader reader(data);
    Transaction tx2;
    tx2.Deserialize(reader);
    
    EXPECT_EQ(tx.SystemFee, tx2.SystemFee);
    EXPECT_EQ(tx.NetworkFee, tx2.NetworkFee);
    EXPECT_EQ(tx.ValidUntilBlock, tx2.ValidUntilBlock);
}

TEST_F(CoreTypesTest, EmptyCollections) {
    Transaction tx;
    
    // Empty collections should be valid
    EXPECT_TRUE(tx.Signers.empty());
    EXPECT_TRUE(tx.Attributes.empty());
    EXPECT_TRUE(tx.Witnesses.empty());
    EXPECT_TRUE(tx.Script.empty());
    
    // Should still produce a valid hash
    auto hash = tx.GetHash();
    EXPECT_NE(hash, UInt256::Zero());
}

// Benchmark tests
TEST_F(CoreTypesTest, SerializationPerformance) {
    Transaction tx;
    tx.Version = 0;
    tx.Nonce = 12345;
    tx.SystemFee = 1000000;
    tx.NetworkFee = 500000;
    tx.ValidUntilBlock = 999999;
    
    // Add multiple signers
    for (int i = 0; i < 10; i++) {
        Signer signer;
        signer.Account = UInt160::Zero();
        signer.Scopes = WitnessScope::CalledByEntry;
        tx.Signers.push_back(signer);
    }
    
    // Measure serialization time
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; i++) {
        BinaryWriter writer;
        tx.Serialize(writer);
        auto data = writer.ToArray();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete 1000 serializations in reasonable time
    EXPECT_LT(duration.count(), 1000); // Less than 1 second
}