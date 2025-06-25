#include <gtest/gtest.h>
#include <neo/smartcontract/contract.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <sstream>

using namespace neo::smartcontract;
using namespace neo::io;
using namespace neo::cryptography::ecc;

TEST(ContractParameterTest, Constructor)
{
    // Default constructor
    ContractParameter param1;
    EXPECT_EQ(param1.GetType(), ContractParameterType::Void);
    EXPECT_FALSE(param1.GetValue().has_value());
    EXPECT_TRUE(param1.GetArray().empty());
    EXPECT_TRUE(param1.GetMap().empty());
    
    // Type constructor
    ContractParameter param2(ContractParameterType::String);
    EXPECT_EQ(param2.GetType(), ContractParameterType::String);
    EXPECT_FALSE(param2.GetValue().has_value());
    EXPECT_TRUE(param2.GetArray().empty());
    EXPECT_TRUE(param2.GetMap().empty());
}

TEST(ContractParameterTest, SettersAndGetters)
{
    ContractParameter param;
    
    // Type
    param.SetType(ContractParameterType::Integer);
    EXPECT_EQ(param.GetType(), ContractParameterType::Integer);
    
    // Value
    ByteVector value = ByteVector::Parse("0102030405");
    param.SetValue(value);
    EXPECT_TRUE(param.GetValue().has_value());
    EXPECT_EQ(*param.GetValue(), value);
    
    // Array
    std::vector<ContractParameter> array = {
        ContractParameter(ContractParameterType::Boolean),
        ContractParameter(ContractParameterType::Integer)
    };
    param.SetArray(array);
    EXPECT_EQ(param.GetArray().size(), 2);
    EXPECT_EQ(param.GetArray()[0].GetType(), ContractParameterType::Boolean);
    EXPECT_EQ(param.GetArray()[1].GetType(), ContractParameterType::Integer);
    
    // Map
    std::vector<std::pair<ContractParameter, ContractParameter>> map = {
        { ContractParameter(ContractParameterType::String), ContractParameter(ContractParameterType::ByteArray) }
    };
    param.SetMap(map);
    EXPECT_EQ(param.GetMap().size(), 1);
    EXPECT_EQ(param.GetMap()[0].first.GetType(), ContractParameterType::String);
    EXPECT_EQ(param.GetMap()[0].second.GetType(), ContractParameterType::ByteArray);
}

TEST(ContractParameterTest, CreateMethods)
{
    // CreateSignature
    ByteVector signature = ByteVector::Parse("0102030405");
    auto param1 = ContractParameter::CreateSignature(signature);
    EXPECT_EQ(param1.GetType(), ContractParameterType::Signature);
    EXPECT_TRUE(param1.GetValue().has_value());
    EXPECT_EQ(*param1.GetValue(), signature);
    
    // CreateBoolean
    auto param2 = ContractParameter::CreateBoolean(true);
    EXPECT_EQ(param2.GetType(), ContractParameterType::Boolean);
    EXPECT_TRUE(param2.GetValue().has_value());
    EXPECT_EQ(param2.GetValue()->Size(), sizeof(bool));
    EXPECT_EQ(param2.GetValue()->operator[](0), 1);
    
    // CreateInteger
    auto param3 = ContractParameter::CreateInteger(123);
    EXPECT_EQ(param3.GetType(), ContractParameterType::Integer);
    EXPECT_TRUE(param3.GetValue().has_value());
    EXPECT_EQ(param3.GetValue()->Size(), sizeof(int64_t));
    
    // CreateHash160
    UInt160 hash160 = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    auto param4 = ContractParameter::CreateHash160(hash160);
    EXPECT_EQ(param4.GetType(), ContractParameterType::Hash160);
    EXPECT_TRUE(param4.GetValue().has_value());
    EXPECT_EQ(param4.GetValue()->Size(), 20);
    EXPECT_EQ(ByteVector(ByteSpan(hash160.Data(), UInt160::Size)), *param4.GetValue());
    
    // CreateHash256
    UInt256 hash256 = UInt256::Parse("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    auto param5 = ContractParameter::CreateHash256(hash256);
    EXPECT_EQ(param5.GetType(), ContractParameterType::Hash256);
    EXPECT_TRUE(param5.GetValue().has_value());
    EXPECT_EQ(param5.GetValue()->Size(), 32);
    EXPECT_EQ(ByteVector(ByteSpan(hash256.Data(), UInt256::Size)), *param5.GetValue());
    
    // CreateByteArray
    ByteVector byteArray = ByteVector::Parse("0102030405");
    auto param6 = ContractParameter::CreateByteArray(byteArray);
    EXPECT_EQ(param6.GetType(), ContractParameterType::ByteArray);
    EXPECT_TRUE(param6.GetValue().has_value());
    EXPECT_EQ(*param6.GetValue(), byteArray);
    
    // CreatePublicKey
    auto keyPair = Secp256r1::GenerateKeyPair();
    auto param7 = ContractParameter::CreatePublicKey(keyPair.PublicKey);
    EXPECT_EQ(param7.GetType(), ContractParameterType::PublicKey);
    EXPECT_TRUE(param7.GetValue().has_value());
    EXPECT_EQ(*param7.GetValue(), keyPair.PublicKey.ToArray());
    
    // CreateString
    std::string str = "Hello, world!";
    auto param8 = ContractParameter::CreateString(str);
    EXPECT_EQ(param8.GetType(), ContractParameterType::String);
    EXPECT_TRUE(param8.GetValue().has_value());
    EXPECT_EQ(param8.GetValue()->Size(), str.size());
    EXPECT_EQ(std::string(reinterpret_cast<const char*>(param8.GetValue()->Data()), param8.GetValue()->Size()), str);
    
    // CreateArray
    std::vector<ContractParameter> array = {
        ContractParameter(ContractParameterType::Boolean),
        ContractParameter(ContractParameterType::Integer)
    };
    auto param9 = ContractParameter::CreateArray(array);
    EXPECT_EQ(param9.GetType(), ContractParameterType::Array);
    EXPECT_EQ(param9.GetArray().size(), 2);
    EXPECT_EQ(param9.GetArray()[0].GetType(), ContractParameterType::Boolean);
    EXPECT_EQ(param9.GetArray()[1].GetType(), ContractParameterType::Integer);
    
    // CreateMap
    std::vector<std::pair<ContractParameter, ContractParameter>> map = {
        { ContractParameter(ContractParameterType::String), ContractParameter(ContractParameterType::ByteArray) }
    };
    auto param10 = ContractParameter::CreateMap(map);
    EXPECT_EQ(param10.GetType(), ContractParameterType::Map);
    EXPECT_EQ(param10.GetMap().size(), 1);
    EXPECT_EQ(param10.GetMap()[0].first.GetType(), ContractParameterType::String);
    EXPECT_EQ(param10.GetMap()[0].second.GetType(), ContractParameterType::ByteArray);
    
    // CreateVoid
    auto param11 = ContractParameter::CreateVoid();
    EXPECT_EQ(param11.GetType(), ContractParameterType::Void);
    EXPECT_FALSE(param11.GetValue().has_value());
    EXPECT_TRUE(param11.GetArray().empty());
    EXPECT_TRUE(param11.GetMap().empty());
}

TEST(ContractTest, Constructor)
{
    // Default constructor
    Contract contract1;
    EXPECT_TRUE(contract1.GetScript().IsEmpty());
    EXPECT_TRUE(contract1.GetParameterList().empty());
    
    // Parameter constructor
    ByteVector script = ByteVector::Parse("0102030405");
    std::vector<ContractParameterType> parameterList = {
        ContractParameterType::Signature,
        ContractParameterType::Boolean
    };
    Contract contract2(script, parameterList);
    EXPECT_EQ(contract2.GetScript(), script);
    EXPECT_EQ(contract2.GetParameterList().size(), 2);
    EXPECT_EQ(contract2.GetParameterList()[0], ContractParameterType::Signature);
    EXPECT_EQ(contract2.GetParameterList()[1], ContractParameterType::Boolean);
}

TEST(ContractTest, SettersAndGetters)
{
    Contract contract;
    
    // Script
    ByteVector script = ByteVector::Parse("0102030405");
    contract.SetScript(script);
    EXPECT_EQ(contract.GetScript(), script);
    
    // Parameter list
    std::vector<ContractParameterType> parameterList = {
        ContractParameterType::Signature,
        ContractParameterType::Boolean
    };
    contract.SetParameterList(parameterList);
    EXPECT_EQ(contract.GetParameterList().size(), 2);
    EXPECT_EQ(contract.GetParameterList()[0], ContractParameterType::Signature);
    EXPECT_EQ(contract.GetParameterList()[1], ContractParameterType::Boolean);
}

TEST(ContractTest, GetScriptHash)
{
    ByteVector script = ByteVector::Parse("0102030405");
    Contract contract(script, {});
    
    UInt160 scriptHash = contract.GetScriptHash();
    EXPECT_EQ(scriptHash, neo::cryptography::Hash::Hash160(script.AsSpan()));
}

TEST(ContractTest, Serialization)
{
    // Create a contract
    ByteVector script = ByteVector::Parse("0102030405");
    std::vector<ContractParameterType> parameterList = {
        ContractParameterType::Signature,
        ContractParameterType::Boolean
    };
    Contract contract(script, parameterList);
    
    // Serialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    contract.Serialize(writer);
    
    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    Contract contract2;
    contract2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(contract2.GetScript(), script);
    EXPECT_EQ(contract2.GetParameterList().size(), 2);
    EXPECT_EQ(contract2.GetParameterList()[0], ContractParameterType::Signature);
    EXPECT_EQ(contract2.GetParameterList()[1], ContractParameterType::Boolean);
}

TEST(ContractTest, CreateSignatureContract)
{
    auto keyPair = Secp256r1::GenerateKeyPair();
    Contract contract = Contract::CreateSignatureContract(keyPair.PublicKey);
    
    EXPECT_FALSE(contract.GetScript().IsEmpty());
    EXPECT_EQ(contract.GetParameterList().size(), 1);
    EXPECT_EQ(contract.GetParameterList()[0], ContractParameterType::Signature);
}

TEST(ContractTest, CreateMultiSigContract)
{
    auto keyPair1 = Secp256r1::GenerateKeyPair();
    auto keyPair2 = Secp256r1::GenerateKeyPair();
    auto keyPair3 = Secp256r1::GenerateKeyPair();
    
    std::vector<ECPoint> pubKeys = {
        keyPair1.PublicKey,
        keyPair2.PublicKey,
        keyPair3.PublicKey
    };
    
    Contract contract = Contract::CreateMultiSigContract(2, pubKeys);
    
    EXPECT_FALSE(contract.GetScript().IsEmpty());
    EXPECT_EQ(contract.GetParameterList().size(), 2);
    EXPECT_EQ(contract.GetParameterList()[0], ContractParameterType::Signature);
    EXPECT_EQ(contract.GetParameterList()[1], ContractParameterType::Signature);
}

TEST(ContractStateTest, Constructor)
{
    ContractState state;
    EXPECT_EQ(state.GetId(), 0);
    EXPECT_EQ(state.GetScriptHash(), UInt160());
    EXPECT_TRUE(state.GetScript().IsEmpty());
    EXPECT_TRUE(state.GetManifest().empty());
}

TEST(ContractStateTest, SettersAndGetters)
{
    ContractState state;
    
    // Id
    state.SetId(123);
    EXPECT_EQ(state.GetId(), 123);
    
    // Script hash
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    state.SetScriptHash(scriptHash);
    EXPECT_EQ(state.GetScriptHash(), scriptHash);
    
    // Script
    ByteVector script = ByteVector::Parse("0102030405");
    state.SetScript(script);
    EXPECT_EQ(state.GetScript(), script);
    
    // Manifest
    std::string manifest = R"({"name":"Test"})";
    state.SetManifest(manifest);
    EXPECT_EQ(state.GetManifest(), manifest);
}

TEST(ContractStateTest, Serialization)
{
    // Create a contract state
    ContractState state;
    state.SetId(123);
    state.SetScriptHash(UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));
    state.SetScript(ByteVector::Parse("0102030405"));
    state.SetManifest(R"({"name":"Test"})");
    
    // Serialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    state.Serialize(writer);
    
    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    ContractState state2;
    state2.Deserialize(reader);
    
    // Check
    EXPECT_EQ(state2.GetId(), 123);
    EXPECT_EQ(state2.GetScriptHash(), UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314"));
    EXPECT_EQ(state2.GetScript(), ByteVector::Parse("0102030405"));
    EXPECT_EQ(state2.GetManifest(), R"({"name":"Test"})");
}
