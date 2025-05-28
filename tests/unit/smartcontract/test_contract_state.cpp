#include <gtest/gtest.h>
#include <neo/smartcontract/contract_state.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/cryptography/hash.h>
#include <sstream>

using namespace neo::smartcontract;
using namespace neo::io;
using namespace neo::cryptography;

TEST(ContractStateTest, Constructor)
{
    ContractState state;
    EXPECT_EQ(state.GetId(), 0);
    EXPECT_TRUE(state.GetScriptHash().IsZero());
    EXPECT_TRUE(state.GetScript().IsEmpty());
    EXPECT_TRUE(state.GetManifest().empty());
}

TEST(ContractStateTest, GettersAndSetters)
{
    ContractState state;
    
    // Set and get ID
    state.SetId(123);
    EXPECT_EQ(state.GetId(), 123);
    
    // Set and get script hash
    UInt160 scriptHash;
    scriptHash.FromString("0x1234567890abcdef1234567890abcdef12345678");
    state.SetScriptHash(scriptHash);
    EXPECT_EQ(state.GetScriptHash(), scriptHash);
    
    // Set and get script
    ByteVector script{1, 2, 3, 4, 5};
    state.SetScript(script);
    EXPECT_EQ(state.GetScript(), script);
    
    // Set and get manifest
    std::string manifest = R"({"name":"Test","groups":[],"features":{},"abi":{"methods":[]},"permissions":[{"contract":"*","methods":"*"}],"trusts":[],"safeMethods":[]})";
    state.SetManifest(manifest);
    EXPECT_EQ(state.GetManifest(), manifest);
}

TEST(ContractStateTest, SerializeAndDeserialize)
{
    ContractState state;
    state.SetId(123);
    
    UInt160 scriptHash;
    scriptHash.FromString("0x1234567890abcdef1234567890abcdef12345678");
    state.SetScriptHash(scriptHash);
    
    ByteVector script{1, 2, 3, 4, 5};
    state.SetScript(script);
    
    std::string manifest = R"({"name":"Test","groups":[],"features":{},"abi":{"methods":[]},"permissions":[{"contract":"*","methods":"*"}],"trusts":[],"safeMethods":[]})";
    state.SetManifest(manifest);
    
    // Serialize
    std::ostringstream stream;
    BinaryWriter writer(stream);
    state.Serialize(writer);
    std::string data = stream.str();
    
    // Deserialize
    std::istringstream stream2(data);
    BinaryReader reader(stream2);
    ContractState state2;
    state2.Deserialize(reader);
    
    // Check if deserialized state is equal to original state
    EXPECT_EQ(state2.GetId(), state.GetId());
    EXPECT_EQ(state2.GetScriptHash(), state.GetScriptHash());
    EXPECT_EQ(state2.GetScript(), state.GetScript());
    EXPECT_EQ(state2.GetManifest(), state.GetManifest());
}

TEST(ContractStateTest, ScriptHashCalculation)
{
    ContractState state;
    
    // Set script
    ByteVector script{1, 2, 3, 4, 5};
    state.SetScript(script);
    
    // Calculate script hash
    UInt160 scriptHash = Hash::Hash160(script.AsSpan());
    
    // Set script hash
    state.SetScriptHash(scriptHash);
    
    // Check if script hash is correct
    EXPECT_EQ(state.GetScriptHash(), scriptHash);
}
