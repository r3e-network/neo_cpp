#include <gtest/gtest.h>
#include <neo/smartcontract/native/native_contract.h>
#include <neo/smartcontract/native/native_contract_manager.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/std_lib.h>
#include <neo/smartcontract/native/crypto_lib.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/policy_contract.h>
#include <neo/smartcontract/native/oracle_contract.h>
#include <neo/smartcontract/native/role_management.h>
#include <neo/smartcontract/native/name_service.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/data_cache.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>

using namespace neo::smartcontract::native;
using namespace neo::smartcontract;
using namespace neo::persistence;
using namespace neo::io;
using namespace neo::vm;
using namespace neo::ledger;

class NativeContractTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        store = std::make_shared<MemoryStore>();
        snapshot = std::make_shared<DataCache>(store.get());
        engine = std::make_shared<ApplicationEngine>(TriggerType::Application, nullptr, snapshot);
    }

    std::shared_ptr<MemoryStore> store;
    std::shared_ptr<DataCache> snapshot;
    std::shared_ptr<ApplicationEngine> engine;
};

TEST_F(NativeContractTest, NativeContractManager)
{
    auto& manager = NativeContractManager::GetInstance();
    
    // Check if all native contracts are registered
    EXPECT_EQ(manager.GetContracts().size(), 10);
    
    // Check if contracts can be retrieved by name
    EXPECT_NE(manager.GetContract(ContractManagement::NAME), nullptr);
    EXPECT_NE(manager.GetContract(StdLib::NAME), nullptr);
    EXPECT_NE(manager.GetContract(CryptoLib::NAME), nullptr);
    EXPECT_NE(manager.GetContract(LedgerContract::NAME), nullptr);
    EXPECT_NE(manager.GetContract(NeoToken::NAME), nullptr);
    EXPECT_NE(manager.GetContract(GasToken::NAME), nullptr);
    EXPECT_NE(manager.GetContract(PolicyContract::NAME), nullptr);
    EXPECT_NE(manager.GetContract(OracleContract::NAME), nullptr);
    EXPECT_NE(manager.GetContract(RoleManagement::NAME), nullptr);
    EXPECT_NE(manager.GetContract(NameService::NAME), nullptr);
    
    // Check if contracts can be retrieved by ID
    EXPECT_NE(manager.GetContract(ContractManagement::ID), nullptr);
    EXPECT_NE(manager.GetContract(StdLib::ID), nullptr);
    EXPECT_NE(manager.GetContract(CryptoLib::ID), nullptr);
    EXPECT_NE(manager.GetContract(LedgerContract::ID), nullptr);
    EXPECT_NE(manager.GetContract(NeoToken::ID), nullptr);
    EXPECT_NE(manager.GetContract(GasToken::ID), nullptr);
    EXPECT_NE(manager.GetContract(PolicyContract::ID), nullptr);
    EXPECT_NE(manager.GetContract(OracleContract::ID), nullptr);
    EXPECT_NE(manager.GetContract(RoleManagement::ID), nullptr);
    EXPECT_NE(manager.GetContract(NameService::ID), nullptr);
    
    // Check if contracts can be retrieved by script hash
    EXPECT_NE(manager.GetContract(manager.GetContract(ContractManagement::NAME)->GetScriptHash()), nullptr);
    EXPECT_NE(manager.GetContract(manager.GetContract(StdLib::NAME)->GetScriptHash()), nullptr);
    EXPECT_NE(manager.GetContract(manager.GetContract(CryptoLib::NAME)->GetScriptHash()), nullptr);
    EXPECT_NE(manager.GetContract(manager.GetContract(LedgerContract::NAME)->GetScriptHash()), nullptr);
    EXPECT_NE(manager.GetContract(manager.GetContract(NeoToken::NAME)->GetScriptHash()), nullptr);
    EXPECT_NE(manager.GetContract(manager.GetContract(GasToken::NAME)->GetScriptHash()), nullptr);
    EXPECT_NE(manager.GetContract(manager.GetContract(PolicyContract::NAME)->GetScriptHash()), nullptr);
    EXPECT_NE(manager.GetContract(manager.GetContract(OracleContract::NAME)->GetScriptHash()), nullptr);
    EXPECT_NE(manager.GetContract(manager.GetContract(RoleManagement::NAME)->GetScriptHash()), nullptr);
    EXPECT_NE(manager.GetContract(manager.GetContract(NameService::NAME)->GetScriptHash()), nullptr);
}

TEST_F(NativeContractTest, ContractManagement)
{
    auto contract = std::make_shared<ContractManagement>();
    
    // Check contract properties
    EXPECT_EQ(contract->GetName(), ContractManagement::NAME);
    EXPECT_EQ(contract->GetId(), ContractManagement::ID);
    
    // Initialize contract
    contract->Initialize();
    
    // Check if methods are registered
    EXPECT_TRUE(contract->CheckCallFlags("deploy", CallFlags::All));
    EXPECT_TRUE(contract->CheckCallFlags("update", CallFlags::All));
    EXPECT_TRUE(contract->CheckCallFlags("destroy", CallFlags::All));
    EXPECT_TRUE(contract->CheckCallFlags("getContract", CallFlags::ReadStates));
}

TEST_F(NativeContractTest, StdLib)
{
    auto contract = std::make_shared<StdLib>();
    
    // Check contract properties
    EXPECT_EQ(contract->GetName(), StdLib::NAME);
    EXPECT_EQ(contract->GetId(), StdLib::ID);
    
    // Initialize contract
    contract->Initialize();
    
    // Check if methods are registered
    EXPECT_TRUE(contract->CheckCallFlags("serialize", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("deserialize", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("jsonSerialize", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("jsonDeserialize", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("itoa", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("atoi", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("base64Encode", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("base64Decode", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("base58Encode", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("base58Decode", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("memoryCompare", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("memoryCopy", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("memorySearch", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("stringCompare", CallFlags::None));
}

TEST_F(NativeContractTest, CryptoLib)
{
    auto contract = std::make_shared<CryptoLib>();
    
    // Check contract properties
    EXPECT_EQ(contract->GetName(), CryptoLib::NAME);
    EXPECT_EQ(contract->GetId(), CryptoLib::ID);
    
    // Initialize contract
    contract->Initialize();
    
    // Check if methods are registered
    EXPECT_TRUE(contract->CheckCallFlags("sha256", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("ripemd160", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("hash160", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("hash256", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("verifySignature", CallFlags::None));
    EXPECT_TRUE(contract->CheckCallFlags("verifyWithECDsa", CallFlags::None));
}

TEST_F(NativeContractTest, LedgerContract)
{
    auto contract = std::make_shared<LedgerContract>();
    
    // Check contract properties
    EXPECT_EQ(contract->GetName(), LedgerContract::NAME);
    EXPECT_EQ(contract->GetId(), LedgerContract::ID);
    
    // Initialize contract
    contract->Initialize();
    
    // Check if methods are registered
    EXPECT_TRUE(contract->CheckCallFlags("getHash", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("getBlock", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("getTransaction", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("getTransactionHeight", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("getCurrentIndex", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("getCurrentHash", CallFlags::ReadStates));
}

TEST_F(NativeContractTest, NeoToken)
{
    auto contract = std::make_shared<NeoToken>();
    
    // Check contract properties
    EXPECT_EQ(contract->GetName(), NeoToken::NAME);
    EXPECT_EQ(contract->GetId(), NeoToken::ID);
    
    // Initialize contract
    contract->Initialize();
    
    // Check if methods are registered
    EXPECT_TRUE(contract->CheckCallFlags("symbol", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("decimals", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("totalSupply", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("balanceOf", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("transfer", CallFlags::All));
}

TEST_F(NativeContractTest, GasToken)
{
    auto contract = std::make_shared<GasToken>();
    
    // Check contract properties
    EXPECT_EQ(contract->GetName(), GasToken::NAME);
    EXPECT_EQ(contract->GetId(), GasToken::ID);
    
    // Initialize contract
    contract->Initialize();
    
    // Check if methods are registered
    EXPECT_TRUE(contract->CheckCallFlags("symbol", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("decimals", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("totalSupply", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("balanceOf", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("transfer", CallFlags::All));
}

TEST_F(NativeContractTest, PolicyContract)
{
    auto contract = std::make_shared<PolicyContract>();
    
    // Check contract properties
    EXPECT_EQ(contract->GetName(), PolicyContract::NAME);
    EXPECT_EQ(contract->GetId(), PolicyContract::ID);
    
    // Initialize contract
    contract->Initialize();
    
    // Check if methods are registered
    EXPECT_TRUE(contract->CheckCallFlags("getMaxTransactionsPerBlock", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("setMaxTransactionsPerBlock", CallFlags::States));
    EXPECT_TRUE(contract->CheckCallFlags("getFeePerByte", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("setFeePerByte", CallFlags::States));
    EXPECT_TRUE(contract->CheckCallFlags("getExecutionFeeFactor", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("setExecutionFeeFactor", CallFlags::States));
    EXPECT_TRUE(contract->CheckCallFlags("getStoragePrice", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("setStoragePrice", CallFlags::States));
    EXPECT_TRUE(contract->CheckCallFlags("isBlocked", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("blockAccount", CallFlags::States));
    EXPECT_TRUE(contract->CheckCallFlags("unblockAccount", CallFlags::States));
}

TEST_F(NativeContractTest, OracleContract)
{
    auto contract = std::make_shared<OracleContract>();
    
    // Check contract properties
    EXPECT_EQ(contract->GetName(), OracleContract::NAME);
    EXPECT_EQ(contract->GetId(), OracleContract::ID);
    
    // Initialize contract
    contract->Initialize();
    
    // Check if methods are registered
    EXPECT_TRUE(contract->CheckCallFlags("getPrice", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("setPrice", CallFlags::States));
    EXPECT_TRUE(contract->CheckCallFlags("getOracles", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("setOracles", CallFlags::States));
    EXPECT_TRUE(contract->CheckCallFlags("request", CallFlags::All));
    EXPECT_TRUE(contract->CheckCallFlags("finish", CallFlags::States));
}

TEST_F(NativeContractTest, RoleManagement)
{
    auto contract = std::make_shared<RoleManagement>();
    
    // Check contract properties
    EXPECT_EQ(contract->GetName(), RoleManagement::NAME);
    EXPECT_EQ(contract->GetId(), RoleManagement::ID);
    
    // Initialize contract
    contract->Initialize();
    
    // Check if methods are registered
    EXPECT_TRUE(contract->CheckCallFlags("getDesignatedByRole", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("designateAsRole", CallFlags::States));
}

TEST_F(NativeContractTest, NameService)
{
    auto contract = std::make_shared<NameService>();
    
    // Check contract properties
    EXPECT_EQ(contract->GetName(), NameService::NAME);
    EXPECT_EQ(contract->GetId(), NameService::ID);
    
    // Initialize contract
    contract->Initialize();
    
    // Check if methods are registered
    EXPECT_TRUE(contract->CheckCallFlags("getPrice", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("setPrice", CallFlags::States));
    EXPECT_TRUE(contract->CheckCallFlags("isAvailable", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("register", CallFlags::All));
    EXPECT_TRUE(contract->CheckCallFlags("renew", CallFlags::All));
    EXPECT_TRUE(contract->CheckCallFlags("transfer", CallFlags::States));
    EXPECT_TRUE(contract->CheckCallFlags("delete", CallFlags::States));
    EXPECT_TRUE(contract->CheckCallFlags("resolve", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("getOwner", CallFlags::ReadStates));
    EXPECT_TRUE(contract->CheckCallFlags("getExpiration", CallFlags::ReadStates));
}
