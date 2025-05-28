#include <gtest/gtest.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/persistence/memory_store_view.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/cryptography/hash.h>
#include <sstream>

using namespace neo::smartcontract::native;
using namespace neo::persistence;
using namespace neo::io;
using namespace neo::vm;
using namespace neo::cryptography;

class ContractManagementTest : public ::testing::Test
{
protected:
    std::shared_ptr<MemoryStoreView> snapshot;
    std::shared_ptr<ContractManagement> contractManagement;
    std::shared_ptr<ApplicationEngine> engine;
    
    void SetUp() override
    {
        snapshot = std::make_shared<MemoryStoreView>();
        contractManagement = ContractManagement::GetInstance();
        engine = std::make_shared<ApplicationEngine>(TriggerType::Application, nullptr, snapshot, 0, false);
    }
};

TEST_F(ContractManagementTest, TestGetMinimumDeploymentFee)
{
    // Call the getMinimumDeploymentFee method
    auto result = contractManagement->Call(*engine, "getMinimumDeploymentFee", {});
    
    // Check the result
    ASSERT_TRUE(result->IsInteger());
    ASSERT_EQ(result->GetInteger(), 10 * 100000000); // 10 GAS
}

TEST_F(ContractManagementTest, TestDeployAndGetContract)
{
    // Create a simple contract
    ByteVector script = ByteVector::Parse("010203");
    std::string manifest = R"({
        "name": "TestContract",
        "groups": [],
        "supportedstandards": [],
        "abi": {
            "methods": [
                {
                    "name": "test",
                    "parameters": [],
                    "returntype": "Void",
                    "offset": 0
                }
            ],
            "events": []
        },
        "permissions": [
            {
                "contract": "*",
                "methods": "*"
            }
        ],
        "trusts": [],
        "features": {},
        "extra": null
    })";
    
    // Calculate the script hash
    auto hash = Hash::Hash160(script.AsSpan());
    
    // Deploy the contract
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(script));
    args.push_back(StackItem::Create(manifest));
    auto deployResult = contractManagement->Call(*engine, "deploy", args);
    
    // Check the result
    ASSERT_TRUE(deployResult->IsBoolean());
    ASSERT_TRUE(deployResult->GetBoolean());
    
    // Get the contract
    args.clear();
    args.push_back(StackItem::Create(hash));
    auto getContractResult = contractManagement->Call(*engine, "getContract", args);
    
    // Check the result
    ASSERT_TRUE(getContractResult->IsArray());
    auto contractArray = getContractResult->GetArray();
    ASSERT_EQ(contractArray.size(), 5);
    ASSERT_TRUE(contractArray[0]->IsInteger()); // ID
    ASSERT_TRUE(contractArray[1]->IsInteger()); // UpdateCounter
    ASSERT_TRUE(contractArray[2]->IsBuffer()); // ScriptHash
    ASSERT_TRUE(contractArray[3]->IsBuffer()); // Script
    ASSERT_TRUE(contractArray[4]->IsString()); // Manifest
    
    // Check the contract values
    ASSERT_EQ(contractArray[0]->GetInteger(), 1); // ID
    ASSERT_EQ(contractArray[1]->GetInteger(), 0); // UpdateCounter
    ASSERT_EQ(contractArray[2]->GetByteArray(), ByteVector(ByteSpan(hash.Data(), hash.Size()))); // ScriptHash
    ASSERT_EQ(contractArray[3]->GetByteArray(), script); // Script
    ASSERT_EQ(contractArray[4]->GetString(), manifest); // Manifest
}

TEST_F(ContractManagementTest, TestUpdateContract)
{
    // Create a simple contract
    ByteVector script = ByteVector::Parse("010203");
    std::string manifest = R"({
        "name": "TestContract",
        "groups": [],
        "supportedstandards": [],
        "abi": {
            "methods": [
                {
                    "name": "test",
                    "parameters": [],
                    "returntype": "Void",
                    "offset": 0
                }
            ],
            "events": []
        },
        "permissions": [
            {
                "contract": "*",
                "methods": "*"
            }
        ],
        "trusts": [],
        "features": {},
        "extra": null
    })";
    
    // Calculate the script hash
    auto hash = Hash::Hash160(script.AsSpan());
    
    // Deploy the contract
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(script));
    args.push_back(StackItem::Create(manifest));
    contractManagement->Call(*engine, "deploy", args);
    
    // Set the current script hash to the contract hash
    engine->SetCurrentScriptHash(hash);
    
    // Update the contract
    ByteVector newScript = ByteVector::Parse("010204");
    std::string newManifest = R"({
        "name": "TestContract",
        "groups": [],
        "supportedstandards": [],
        "abi": {
            "methods": [
                {
                    "name": "test",
                    "parameters": [],
                    "returntype": "Void",
                    "offset": 0
                },
                {
                    "name": "test2",
                    "parameters": [],
                    "returntype": "Void",
                    "offset": 0
                }
            ],
            "events": []
        },
        "permissions": [
            {
                "contract": "*",
                "methods": "*"
            }
        ],
        "trusts": [],
        "features": {},
        "extra": null
    })";
    
    args.clear();
    args.push_back(StackItem::Create(newScript));
    args.push_back(StackItem::Create(newManifest));
    auto updateResult = contractManagement->Call(*engine, "update", args);
    
    // Check the result
    ASSERT_TRUE(updateResult->IsBoolean());
    ASSERT_TRUE(updateResult->GetBoolean());
    
    // Get the contract
    args.clear();
    args.push_back(StackItem::Create(hash));
    auto getContractResult = contractManagement->Call(*engine, "getContract", args);
    
    // Check the result
    ASSERT_TRUE(getContractResult->IsArray());
    auto contractArray = getContractResult->GetArray();
    
    // Check the contract values
    ASSERT_EQ(contractArray[3]->GetByteArray(), newScript); // Script
    ASSERT_EQ(contractArray[4]->GetString(), newManifest); // Manifest
}

TEST_F(ContractManagementTest, TestHasMethod)
{
    // Create a simple contract
    ByteVector script = ByteVector::Parse("010203");
    std::string manifest = R"({
        "name": "TestContract",
        "groups": [],
        "supportedstandards": [],
        "abi": {
            "methods": [
                {
                    "name": "test",
                    "parameters": [],
                    "returntype": "Void",
                    "offset": 0
                }
            ],
            "events": []
        },
        "permissions": [
            {
                "contract": "*",
                "methods": "*"
            }
        ],
        "trusts": [],
        "features": {},
        "extra": null
    })";
    
    // Calculate the script hash
    auto hash = Hash::Hash160(script.AsSpan());
    
    // Deploy the contract
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(script));
    args.push_back(StackItem::Create(manifest));
    contractManagement->Call(*engine, "deploy", args);
    
    // Check if the contract has the test method
    args.clear();
    args.push_back(StackItem::Create(hash));
    args.push_back(StackItem::Create("test"));
    args.push_back(StackItem::Create(0));
    auto hasMethodResult = contractManagement->Call(*engine, "hasMethod", args);
    
    // Check the result
    ASSERT_TRUE(hasMethodResult->IsBoolean());
    ASSERT_TRUE(hasMethodResult->GetBoolean());
    
    // Check if the contract has a non-existent method
    args.clear();
    args.push_back(StackItem::Create(hash));
    args.push_back(StackItem::Create("nonexistent"));
    args.push_back(StackItem::Create(0));
    hasMethodResult = contractManagement->Call(*engine, "hasMethod", args);
    
    // Check the result
    ASSERT_TRUE(hasMethodResult->IsBoolean());
    ASSERT_FALSE(hasMethodResult->GetBoolean());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
