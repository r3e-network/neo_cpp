#include <gtest/gtest.h>
#include <neo/smartcontract/contract.h>
#include <neo/smartcontract/manifest.h>
#include <neo/smartcontract/nef_file.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/key_builder.h>
#include <neo/smartcontract/notify_event_args.h>
#include <neo/vm/stack_item.h>

using namespace neo::smartcontract;
using namespace neo::vm;

class SmartContractExtendedTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(SmartContractExtendedTest, TestContract)
{
    // Create a simple contract
    Contract contract;
    contract.Id = 1;
    contract.UpdateCounter = 0;
    contract.Hash = UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678");
    
    // Set NEF
    NefFile nef;
    nef.Compiler = "neo-cpp-compiler";
    nef.Version = "3.5.0";
    nef.Script = ByteVector::Parse("0102030405");
    contract.Nef = nef;
    
    // Set Manifest
    ContractManifest manifest;
    manifest.Name = "TestContract";
    manifest.Groups = {};
    manifest.Features = {};
    manifest.SupportedStandards = {"NEP-17"};
    contract.Manifest = manifest;
    
    EXPECT_EQ(contract.Id, 1);
    EXPECT_EQ(contract.Manifest.Name, "TestContract");
    EXPECT_EQ(contract.Nef.Script.Size(), 5);
}

TEST_F(SmartContractExtendedTest, TestManifest)
{
    ContractManifest manifest;
    manifest.Name = "MyToken";
    manifest.Groups = {};
    manifest.Features = {"Storage", "Payable"};
    manifest.SupportedStandards = {"NEP-17", "NEP-11"};
    
    // Add ABI
    ContractAbi abi;
    
    // Add method
    ContractMethodDescriptor method;
    method.Name = "transfer";
    method.Parameters = {
        {"from", ContractParameterType::Hash160},
        {"to", ContractParameterType::Hash160},
        {"amount", ContractParameterType::Integer},
        {"data", ContractParameterType::Any}
    };
    method.ReturnType = ContractParameterType::Boolean;
    method.Offset = 0;
    method.Safe = false;
    abi.Methods.push_back(method);
    
    // Add event
    ContractEventDescriptor event;
    event.Name = "Transfer";
    event.Parameters = {
        {"from", ContractParameterType::Hash160},
        {"to", ContractParameterType::Hash160},
        {"amount", ContractParameterType::Integer}
    };
    abi.Events.push_back(event);
    
    manifest.Abi = abi;
    
    EXPECT_EQ(manifest.Name, "MyToken");
    EXPECT_EQ(manifest.SupportedStandards.size(), 2);
    EXPECT_EQ(manifest.Abi.Methods.size(), 1);
    EXPECT_EQ(manifest.Abi.Events.size(), 1);
    EXPECT_EQ(manifest.Abi.Methods[0].Name, "transfer");
}

TEST_F(SmartContractExtendedTest, TestNefFile)
{
    NefFile nef;
    nef.Magic = 0x3346454E; // "NEF3"
    nef.Compiler = "neo-cpp-compiler 1.0";
    nef.Version = "3.5.0";
    
    // Set script
    ByteVector script;
    script.push_back(OpCode::PUSH1);
    script.push_back(OpCode::PUSH2);
    script.push_back(OpCode::ADD);
    script.push_back(OpCode::RET);
    nef.Script = script;
    
    // Calculate checksum
    nef.UpdateCheckSum();
    
    EXPECT_EQ(nef.Magic, 0x3346454E);
    EXPECT_EQ(nef.Script.Size(), 4);
    EXPECT_NE(nef.CheckSum, 0);
    
    // Serialize and deserialize
    ByteVector serialized = nef.Serialize();
    NefFile deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(deserialized.Compiler, nef.Compiler);
    EXPECT_EQ(deserialized.Version, nef.Version);
    EXPECT_EQ(deserialized.Script, nef.Script);
    EXPECT_EQ(deserialized.CheckSum, nef.CheckSum);
}

TEST_F(SmartContractExtendedTest, TestApplicationEngine)
{
    // Create application engine
    ApplicationEngine engine(TriggerType::Application, nullptr, nullptr, 10000000);
    
    EXPECT_EQ(engine.GetTrigger(), TriggerType::Application);
    EXPECT_EQ(engine.GasConsumed, 0);
    EXPECT_GT(engine.GasLeft, 0);
    
    // Push values to stack
    engine.Push(StackItem::FromInteger(5));
    engine.Push(StackItem::FromInteger(3));
    
    // Execute ADD operation
    engine.ExecuteOp(OpCode::ADD);
    
    // Check result
    auto result = engine.Pop();
    EXPECT_EQ(result->GetInteger(), 8);
    
    // Check gas consumption
    EXPECT_GT(engine.GasConsumed, 0);
}

TEST_F(SmartContractExtendedTest, TestKeyBuilder)
{
    // Create key builder for storage
    KeyBuilder builder(1); // Contract ID 1
    
    // Add prefix
    builder.Add(0x01); // Storage prefix
    
    // Add key components
    builder.Add("balance");
    builder.Add(UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678"));
    
    // Get final key
    ByteVector key = builder.ToArray();
    
    EXPECT_GT(key.Size(), 0);
    EXPECT_EQ(key[0], 1); // Contract ID
    EXPECT_EQ(key[1], 0x01); // Storage prefix
}

TEST_F(SmartContractExtendedTest, TestNotifyEventArgs)
{
    // Create notification event
    NotifyEventArgs args;
    args.ScriptHash = UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678");
    args.EventName = "Transfer";
    
    // Add event data
    std::vector<StackItem::Ptr> state;
    state.push_back(StackItem::FromByteArray(UInt160::Zero().ToArray()));
    state.push_back(StackItem::FromByteArray(UInt160::Parse("0xabcdef1234567890abcdef1234567890abcdef12").ToArray()));
    state.push_back(StackItem::FromInteger(1000000));
    args.State = StackItem::FromArray(state);
    
    EXPECT_EQ(args.EventName, "Transfer");
    EXPECT_FALSE(args.ScriptHash.IsZero());
    EXPECT_EQ(args.State->GetArray().size(), 3);
}

TEST_F(SmartContractExtendedTest, TestStackItem)
{
    // Test various stack item types
    
    // Boolean
    auto boolItem = StackItem::FromBoolean(true);
    EXPECT_TRUE(boolItem->GetBoolean());
    EXPECT_EQ(boolItem->GetType(), StackItemType::Boolean);
    
    // Integer
    auto intItem = StackItem::FromInteger(42);
    EXPECT_EQ(intItem->GetInteger(), 42);
    EXPECT_EQ(intItem->GetType(), StackItemType::Integer);
    
    // ByteArray
    ByteVector data = ByteVector::Parse("0102030405");
    auto byteItem = StackItem::FromByteArray(data);
    EXPECT_EQ(byteItem->GetByteArray(), data);
    EXPECT_EQ(byteItem->GetType(), StackItemType::ByteString);
    
    // Array
    std::vector<StackItem::Ptr> array;
    array.push_back(StackItem::FromInteger(1));
    array.push_back(StackItem::FromInteger(2));
    array.push_back(StackItem::FromInteger(3));
    auto arrayItem = StackItem::FromArray(array);
    EXPECT_EQ(arrayItem->GetArray().size(), 3);
    EXPECT_EQ(arrayItem->GetType(), StackItemType::Array);
    
    // Map
    std::map<StackItem::Ptr, StackItem::Ptr> map;
    map[StackItem::FromString("key1")] = StackItem::FromInteger(100);
    map[StackItem::FromString("key2")] = StackItem::FromInteger(200);
    auto mapItem = StackItem::FromMap(map);
    EXPECT_EQ(mapItem->GetMap().size(), 2);
    EXPECT_EQ(mapItem->GetType(), StackItemType::Map);
}