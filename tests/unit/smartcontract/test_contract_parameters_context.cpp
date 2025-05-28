#include <neo/smartcontract/contract_parameters_context.h>
#include <neo/smartcontract/contract.h>
#include <neo/network/p2p/payloads/transaction.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/crypto.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <gtest/gtest.h>
#include <sstream>
#include <memory>

using namespace neo::smartcontract;
using namespace neo::network::p2p::payloads;
using namespace neo::cryptography::ecc;
using namespace neo::io;
using namespace neo::persistence;

class MockDataCache : public DataCache
{
public:
    MockDataCache() {}
    
    // Implement required virtual methods
    void Put(const UInt160& key, const ByteVector& value) override {}
    bool TryGet(const UInt160& key, ByteVector& value) const override { return false; }
    bool Contains(const UInt160& key) const override { return false; }
    void Delete(const UInt160& key) override {}
    std::vector<UInt160> GetKeys() const override { return {}; }
    void Commit() override {}
};

class MockTransaction : public Transaction
{
public:
    MockTransaction() : Transaction() {}
    
    // Override GetScriptHashesForVerifying to return a known script hash
    std::vector<UInt160> GetScriptHashesForVerifying(const DataCache& snapshot) const override
    {
        return { UInt160::Parse("0x902e0d38da5e513b6d07c1c55b85e77d3dce8063") };
    }
};

class UT_ContractParametersContext : public testing::Test
{
protected:
    void SetUp() override
    {
        // Create a mock transaction
        transaction = std::make_unique<MockTransaction>();
        
        // Create a mock data cache
        dataCache = std::make_unique<MockDataCache>();
        
        // Create a contract parameters context
        context = std::make_unique<ContractParametersContext>(*dataCache, *transaction, 0x4F454E);
        
        // Create a key pair for testing
        privateKey = ByteVector(32, 0x01);
        keyPair = ECPoint::FromPrivateKey(privateKey);
        
        // Create a contract for testing
        contract = std::make_unique<Contract>();
        contract->SetScript(ByteVector::FromHexString("21" + keyPair.ToString() + "ac"));
        contract->SetParameterList({ ContractParameterType::Signature });
    }
    
    std::unique_ptr<MockTransaction> transaction;
    std::unique_ptr<MockDataCache> dataCache;
    std::unique_ptr<ContractParametersContext> context;
    ByteVector privateKey;
    ECPoint keyPair;
    std::unique_ptr<Contract> contract;
};

TEST_F(UT_ContractParametersContext, TestIsCompleted)
{
    // Initially, the context is not completed
    EXPECT_FALSE(context->IsCompleted());
    
    // Add a parameter to the contract
    ByteVector signature(64, 0x01);
    EXPECT_TRUE(context->Add(*contract, 0, signature));
    
    // Now the context should be completed
    EXPECT_TRUE(context->IsCompleted());
}

TEST_F(UT_ContractParametersContext, TestGetScriptHashes)
{
    // Get the script hashes
    const auto& scriptHashes = context->GetScriptHashes();
    
    // Verify that the script hashes match the expected value
    EXPECT_EQ(1, scriptHashes.size());
    EXPECT_EQ(UInt160::Parse("0x902e0d38da5e513b6d07c1c55b85e77d3dce8063"), scriptHashes[0]);
}

TEST_F(UT_ContractParametersContext, TestAdd)
{
    // Add a parameter to the contract
    ByteVector signature(64, 0x01);
    EXPECT_TRUE(context->Add(*contract, 0, signature));
    
    // Verify that the parameter was added
    const auto* parameter = context->GetParameter(contract->GetScriptHash(), 0);
    EXPECT_NE(nullptr, parameter);
    EXPECT_EQ(ContractParameterType::Signature, parameter->GetType());
    EXPECT_EQ(signature, parameter->GetValue().value());
    
    // Try to add a parameter to a non-existent contract
    Contract invalidContract;
    invalidContract.SetScript(ByteVector::FromHexString("00"));
    invalidContract.SetParameterList({ ContractParameterType::Signature });
    EXPECT_FALSE(context->Add(invalidContract, 0, signature));
}

TEST_F(UT_ContractParametersContext, TestAddMultipleParameters)
{
    // Create a contract with multiple parameters
    Contract multiParamContract;
    multiParamContract.SetScript(ByteVector::FromHexString("21" + keyPair.ToString() + "ac"));
    multiParamContract.SetParameterList({ 
        ContractParameterType::Signature, 
        ContractParameterType::Boolean, 
        ContractParameterType::Integer 
    });
    
    // Add multiple parameters to the contract
    std::vector<ByteVector> parameters = {
        ByteVector(64, 0x01),
        ByteVector::FromHexString("01"),
        ByteVector::FromHexString("0102030405")
    };
    EXPECT_TRUE(context->Add(multiParamContract, parameters));
    
    // Verify that the parameters were added
    const auto* params = context->GetParameters(multiParamContract.GetScriptHash());
    EXPECT_NE(nullptr, params);
    EXPECT_EQ(3, params->size());
    EXPECT_EQ(ContractParameterType::Signature, (*params)[0].GetType());
    EXPECT_EQ(parameters[0], (*params)[0].GetValue().value());
    EXPECT_EQ(ContractParameterType::Boolean, (*params)[1].GetType());
    EXPECT_EQ(parameters[1], (*params)[1].GetValue().value());
    EXPECT_EQ(ContractParameterType::Integer, (*params)[2].GetType());
    EXPECT_EQ(parameters[2], (*params)[2].GetValue().value());
}

TEST_F(UT_ContractParametersContext, TestAddSignature)
{
    // Add a signature to the contract
    ByteVector signature(64, 0x01);
    EXPECT_TRUE(context->AddSignature(*contract, keyPair, signature));
    
    // Verify that the signature was added
    const auto* signatures = context->GetSignatures(contract->GetScriptHash());
    EXPECT_NE(nullptr, signatures);
    EXPECT_EQ(1, signatures->size());
    EXPECT_EQ(signature, signatures->at(keyPair));
    
    // Verify that the parameter was set
    const auto* parameter = context->GetParameter(contract->GetScriptHash(), 0);
    EXPECT_NE(nullptr, parameter);
    EXPECT_EQ(ContractParameterType::Signature, parameter->GetType());
    EXPECT_EQ(signature, parameter->GetValue().value());
}

TEST_F(UT_ContractParametersContext, TestToJson)
{
    // Add a parameter to the contract
    ByteVector signature(64, 0x01);
    EXPECT_TRUE(context->Add(*contract, 0, signature));
    
    // Convert the context to JSON
    std::stringstream stream;
    JsonWriter writer(stream);
    context->ToJson(writer);
    
    // Verify that the JSON contains the expected values
    std::string json = stream.str();
    EXPECT_NE(std::string::npos, json.find("\"type\""));
    EXPECT_NE(std::string::npos, json.find("\"hash\""));
    EXPECT_NE(std::string::npos, json.find("\"data\""));
    EXPECT_NE(std::string::npos, json.find("\"items\""));
    EXPECT_NE(std::string::npos, json.find("\"network\""));
    EXPECT_NE(std::string::npos, json.find("\"parameters\""));
    EXPECT_NE(std::string::npos, json.find("\"signatures\""));
}
