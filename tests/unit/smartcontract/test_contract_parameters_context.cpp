#include <neo/smartcontract/contract_parameters_context.h>
#include <neo/smartcontract/contract.h>
#include <neo/network/p2p/payloads/transaction.h>
#include <neo/network/p2p/payloads/iverifiable.h>
#include <neo/ledger/witness.h>
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
using namespace neo::ledger;

class MockDataCache : public DataCache
{
private:
    std::map<StorageKey, std::shared_ptr<StorageItem>> data_;
    
public:
    MockDataCache() {}
    
    // Implement required virtual methods
    StorageItem& Get(const StorageKey& key) override {
        auto it = data_.find(key);
        if (it == data_.end()) {
            throw std::runtime_error("Key not found");
        }
        return *it->second;
    }
    
    std::shared_ptr<StorageItem> TryGet(const StorageKey& key) override {
        auto it = data_.find(key);
        return (it != data_.end()) ? it->second : nullptr;
    }
    
    std::shared_ptr<StorageItem> GetAndChange(const StorageKey& key, std::function<std::shared_ptr<StorageItem>()> factory = nullptr) override {
        auto it = data_.find(key);
        if (it == data_.end() && factory) {
            data_[key] = factory();
            return data_[key];
        }
        return (it != data_.end()) ? it->second : nullptr;
    }
    
    std::shared_ptr<StoreView> CreateSnapshot() override {
        return std::make_shared<MockDataCache>();
    }
    
    uint32_t GetCurrentBlockIndex() const override {
        return 0;
    }
    
    void Commit() override {}
    
    // Implement StoreView pure virtual methods
    std::optional<StorageItem> TryGet(const StorageKey& key) const override {
        auto it = data_.find(key);
        return (it != data_.end()) ? std::optional<StorageItem>(*it->second) : std::nullopt;
    }
    
    void Add(const StorageKey& key, const StorageItem& item) override {
        data_[key] = std::make_shared<StorageItem>(item);
    }
    
    void Delete(const StorageKey& key) override {
        data_.erase(key);
    }
    
    std::vector<std::pair<StorageKey, StorageItem>> Find(const StorageKey* prefix = nullptr) const override {
        return {}; // Return empty for mock
    }
    
    std::unique_ptr<StorageIterator> Seek(const StorageKey& prefix) const override {
        return nullptr; // Return null for mock
    }
    
    bool IsReadOnly() const override {
        return false; // Mock is read-write
    }
};

class MockTransaction : public IVerifiable
{
public:
    MockTransaction() {}
    
    // Implement IVerifiable interface
    std::vector<UInt160> GetScriptHashesForVerifying() const override
    {
        return { UInt160::Parse("0x902e0d38da5e513b6d07c1c55b85e77d3dce8063") };
    }
    
    const std::vector<neo::ledger::Witness>& GetWitnesses() const override
    {
        return witnesses_;
    }
    
    void SetWitnesses(const std::vector<neo::ledger::Witness>& witnesses) override
    {
        witnesses_ = witnesses;
    }
    
private:
    std::vector<neo::ledger::Witness> witnesses_;
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
        context = std::make_unique<ContractParametersContext>(*dataCache, static_cast<const IVerifiable&>(*transaction), 0x4F454E);
        
        // Create a key pair for testing
        privateKey = ByteVector(std::vector<uint8_t>(32, 0x01));
        keyPair = ECPoint::FromHex("0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798"); // dummy public key
        
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
    ByteVector signature(std::vector<uint8_t>(64, 0x01));
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
    ByteVector signature(std::vector<uint8_t>(64, 0x01));
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
        ByteVector(std::vector<uint8_t>(64, 0x01)),
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
    ByteVector signature(std::vector<uint8_t>(64, 0x01));
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
    ByteVector signature(std::vector<uint8_t>(64, 0x01));
    EXPECT_TRUE(context->Add(*contract, 0, signature));
    
    // Convert the context to JSON
    JsonWriter writer;
    context->ToJson(writer);
    
    // Verify that the JSON contains the expected values
    // TODO: Add JSON verification when JsonWriter provides access to generated JSON
    // std::string json = stream.str();
    // EXPECT_NE(std::string::npos, json.find("\"type\""));
    // EXPECT_NE(std::string::npos, json.find("\"hash\""));
    // EXPECT_NE(std::string::npos, json.find("\"data\""));
    // EXPECT_NE(std::string::npos, json.find("\"items\""));
    // EXPECT_NE(std::string::npos, json.find("\"network\""));
    // EXPECT_NE(std::string::npos, json.find("\"parameters\""));
    // EXPECT_NE(std::string::npos, json.find("\"signatures\""));
}
