#include <gtest/gtest.h>
#include <neo/ledger/transaction.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_cache.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native_contract.h>
#include <neo/vm/script.h>
#include <neo/vm/stack_item.h>

using namespace neo::smartcontract;
using namespace neo::smartcontract::native;
using namespace neo::persistence;
using namespace neo::vm;
using namespace neo::io;
using namespace neo::ledger;

class TestNativeContract : public NativeContract
{
  public:
    TestNativeContract() : NativeContract("Test", 999)
    {
        RegisterMethod("test", std::bind(&TestNativeContract::OnTest, this, std::placeholders::_1),
                       CallFlags::ReadStates);
    }

    void Initialize(std::shared_ptr<DataCache> snapshot) override
    {
        // Do nothing
    }

    uint8_t GetStoragePrefix() const override
    {
        return 0x01;
    }

    bool OnTest(ApplicationEngine& engine)
    {
        engine.GetCurrentContext().Push(StackItem::Create(true));
        return true;
    }

  protected:
    std::string CreateManifest() const override
    {
        return R"({"name":"Test"})";
    }
};

class NativeContractTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        store_ = std::make_shared<MemoryStore>();
        snapshot_ = std::make_shared<StoreCache>(store_);
        transaction_ = std::make_shared<Transaction>();
        engine_ = std::make_shared<ApplicationEngine>(TriggerType::Application, transaction_.get(), snapshot_);
        contract_ = std::make_shared<TestNativeContract>();
    }

    std::shared_ptr<MemoryStore> store_;
    std::shared_ptr<StoreCache> snapshot_;
    std::shared_ptr<Transaction> transaction_;
    std::shared_ptr<ApplicationEngine> engine_;
    std::shared_ptr<TestNativeContract> contract_;
};

TEST_F(NativeContractTest, Constructor)
{
    EXPECT_EQ(contract_->GetName(), "Test");
    EXPECT_EQ(contract_->GetId(), 999);
    EXPECT_FALSE(contract_->GetScriptHash().IsZero());
    EXPECT_EQ(contract_->GetContractState().GetId(), 999);
    EXPECT_EQ(contract_->GetContractState().GetScriptHash(), contract_->GetScriptHash());
    EXPECT_FALSE(contract_->GetContractState().GetScript().IsEmpty());
    EXPECT_EQ(contract_->GetContractState().GetManifest(), R"({"name":"Test"})");
}

TEST_F(NativeContractTest, Invoke)
{
    // Invoke a method
    bool result = contract_->Invoke(*engine_, "test");

    // Check the result
    EXPECT_TRUE(result);

    // Check the stack
    EXPECT_EQ(engine_->GetCurrentContext().GetStackSize(), 1);
    EXPECT_TRUE(engine_->GetCurrentContext().Peek()->GetBoolean());

    // Invoke a non-existent method
    result = contract_->Invoke(*engine_, "nonexistent");

    // Check the result
    EXPECT_FALSE(result);
}

TEST_F(NativeContractTest, CreateStorageKey)
{
    // Create a storage key with prefix only
    auto key1 = contract_->CreateStorageKey(0x01);

    // Check the key
    EXPECT_EQ(key1.GetScriptHash(), contract_->GetScriptHash());
    EXPECT_EQ(key1.GetKey().Size(), 1);
    EXPECT_EQ(key1.GetKey()[0], 0x01);

    // Create a storage key with prefix and key
    ByteVector keyData = ByteVector::Parse("0102030405");
    auto key2 = contract_->CreateStorageKey(0x02, keyData);

    // Check the key
    EXPECT_EQ(key2.GetScriptHash(), contract_->GetScriptHash());
    EXPECT_EQ(key2.GetKey().Size(), 6);
    EXPECT_EQ(key2.GetKey()[0], 0x02);
    EXPECT_EQ(key2.GetKey().Slice(1), keyData);
}

TEST_F(NativeContractTest, NativeContractManager)
{
    // Get the instance
    auto& manager = NativeContractManager::GetInstance();

    // Register a contract
    manager.RegisterContract(contract_);

    // Get the contract by script hash
    auto contract1 = manager.GetContract(contract_->GetScriptHash());
    EXPECT_EQ(contract1, contract_);

    // Get the contract by name
    auto contract2 = manager.GetContract("Test");
    EXPECT_EQ(contract2, contract_);

    // Get a non-existent contract
    auto contract3 = manager.GetContract(UInt160());
    EXPECT_EQ(contract3, nullptr);

    auto contract4 = manager.GetContract("NonExistent");
    EXPECT_EQ(contract4, nullptr);

    // Get all contracts
    auto contracts = manager.GetContracts();
    EXPECT_TRUE(std::find(contracts.begin(), contracts.end(), contract_) != contracts.end());

    // Initialize all contracts
    manager.Initialize(snapshot_);
}

TEST(NeoTokenTest, GetInstance)
{
    auto neoToken = NeoToken::GetInstance();
    EXPECT_EQ(neoToken->GetName(), "Neo");
    EXPECT_EQ(neoToken->GetId(), 0);
    EXPECT_FALSE(neoToken->GetScriptHash().IsZero());
}

TEST(NeoTokenTest, Initialize)
{
    auto store = std::make_shared<MemoryStore>();
    auto snapshot = std::make_shared<StoreCache>(store);

    auto neoToken = NeoToken::GetInstance();
    neoToken->Initialize(snapshot);

    // Check total supply
    auto totalSupply = neoToken->GetTotalSupply(snapshot);
    EXPECT_EQ(totalSupply, io::Fixed8(100000000));

    // Check creator balance
    io::UInt160 creator;
    std::memset(creator.Data(), 0, creator.Size());
    auto balance = neoToken->GetBalance(snapshot, creator);
    EXPECT_EQ(balance, io::Fixed8(100000000));
}

TEST(NeoTokenTest, Transfer)
{
    auto store = std::make_shared<MemoryStore>();
    auto snapshot = std::make_shared<StoreCache>(store);

    auto neoToken = NeoToken::GetInstance();
    neoToken->Initialize(snapshot);

    // Create accounts
    io::UInt160 from;
    std::memset(from.Data(), 0, from.Size());

    io::UInt160 to;
    std::memset(to.Data(), 0, to.Size());
    to.Data()[0] = 1;

    // Check initial balances
    auto fromBalance = neoToken->GetBalance(snapshot, from);
    auto toBalance = neoToken->GetBalance(snapshot, to);
    EXPECT_EQ(fromBalance, io::Fixed8(100000000));
    EXPECT_EQ(toBalance, io::Fixed8(0));

    // Transfer
    bool result = neoToken->Transfer(snapshot, from, to, io::Fixed8(1000));
    EXPECT_TRUE(result);

    // Check balances after transfer
    fromBalance = neoToken->GetBalance(snapshot, from);
    toBalance = neoToken->GetBalance(snapshot, to);
    EXPECT_EQ(fromBalance, io::Fixed8(100000000 - 1000));
    EXPECT_EQ(toBalance, io::Fixed8(1000));

    // Transfer too much
    result = neoToken->Transfer(snapshot, from, to, io::Fixed8(100000000));
    EXPECT_FALSE(result);

    // Transfer negative amount
    result = neoToken->Transfer(snapshot, from, to, io::Fixed8(-1000));
    EXPECT_FALSE(result);
}

TEST(NeoTokenTest, RegisterCandidate)
{
    auto store = std::make_shared<MemoryStore>();
    auto snapshot = std::make_shared<StoreCache>(store);

    auto neoToken = NeoToken::GetInstance();
    neoToken->Initialize(snapshot);

    // Create a key pair
    auto keyPair = neo::cryptography::ecc::Secp256r1::GenerateKeyPair();

    // Register candidate
    bool result = neoToken->RegisterCandidate(snapshot, keyPair.PublicKey);
    EXPECT_TRUE(result);

    // Register the same candidate again
    result = neoToken->RegisterCandidate(snapshot, keyPair.PublicKey);
    EXPECT_FALSE(result);
}

TEST(NeoTokenTest, UnregisterCandidate)
{
    auto store = std::make_shared<MemoryStore>();
    auto snapshot = std::make_shared<StoreCache>(store);

    auto neoToken = NeoToken::GetInstance();
    neoToken->Initialize(snapshot);

    // Create a key pair
    auto keyPair = neo::cryptography::ecc::Secp256r1::GenerateKeyPair();

    // Unregister a non-existent candidate
    bool result = neoToken->UnregisterCandidate(snapshot, keyPair.PublicKey);
    EXPECT_FALSE(result);

    // Register candidate
    result = neoToken->RegisterCandidate(snapshot, keyPair.PublicKey);
    EXPECT_TRUE(result);

    // Unregister candidate
    result = neoToken->UnregisterCandidate(snapshot, keyPair.PublicKey);
    EXPECT_TRUE(result);

    // Unregister the same candidate again
    result = neoToken->UnregisterCandidate(snapshot, keyPair.PublicKey);
    EXPECT_FALSE(result);
}

TEST(NeoTokenTest, Vote)
{
    auto store = std::make_shared<MemoryStore>();
    auto snapshot = std::make_shared<StoreCache>(store);

    auto neoToken = NeoToken::GetInstance();
    neoToken->Initialize(snapshot);

    // Create accounts
    io::UInt160 account;
    std::memset(account.Data(), 0, account.Size());

    // Create key pairs
    auto keyPair1 = neo::cryptography::ecc::Secp256r1::GenerateKeyPair();
    auto keyPair2 = neo::cryptography::ecc::Secp256r1::GenerateKeyPair();

    // Register candidates
    neoToken->RegisterCandidate(snapshot, keyPair1.PublicKey);
    neoToken->RegisterCandidate(snapshot, keyPair2.PublicKey);

    // Vote for candidates
    bool result = neoToken->Vote(snapshot, account, {keyPair1.PublicKey, keyPair2.PublicKey});
    EXPECT_TRUE(result);

    // Vote for non-existent candidate
    auto keyPair3 = neo::cryptography::ecc::Secp256r1::GenerateKeyPair();
    result = neoToken->Vote(snapshot, account, {keyPair3.PublicKey});
    EXPECT_FALSE(result);

    // Vote with account that has no NEO
    io::UInt160 account2;
    std::memset(account2.Data(), 0, account2.Size());
    account2.Data()[0] = 1;
    result = neoToken->Vote(snapshot, account2, {keyPair1.PublicKey});
    EXPECT_FALSE(result);
}
