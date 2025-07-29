// Disabled due to API mismatches - needs to be updated
#include <gtest/gtest.h>
#include <memory>
#include <neo/io/uint160.h>
#include <neo/network/payloads/notary_assisted.h>
#include <neo/network/payloads/transaction.h>
#include <neo/network/payloads/transaction_attribute.h>
#include <neo/persistence/memory_store_view.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/notary.h>
#include <neo/smartcontract/native/policy_contract.h>
#include <neo/vm/stack_item.h>

using namespace neo;
using namespace neo::smartcontract;
using namespace neo::smartcontract::native;
using namespace neo::persistence;
using namespace neo::io;
using namespace neo::vm;
using namespace neo::network::payloads;

class NotaryTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MemoryStoreView> snapshot;
    std::shared_ptr<Notary> notary;
    std::shared_ptr<GasToken> gasToken;
    std::shared_ptr<NeoToken> neoToken;
    std::shared_ptr<PolicyContract> policyContract;
    std::shared_ptr<LedgerContract> ledgerContract;
    std::shared_ptr<ApplicationEngine> engine;
    std::shared_ptr<Block> block;
    UInt160 account1;
    UInt160 account2;

    void SetUp() override
    {
        snapshot = std::make_shared<MemoryStoreView>();
        notary = Notary::GetInstance();
        gasToken = GasToken::GetInstance();
        neoToken = NeoToken::GetInstance();
        policyContract = PolicyContract::GetInstance();
        ledgerContract = LedgerContract::GetInstance();
        engine = std::make_shared<ApplicationEngine>(TriggerType::Application, nullptr, snapshot, 0, false);
        block = std::make_shared<Block>();
        block->SetIndex(1000);
        engine->SetPersistingBlock(block);

        // Create test accounts
        std::memset(account1.Data(), 1, account1.Size());
        std::memset(account2.Data(), 2, account2.Size());

        // Initialize contracts
        notary->Initialize();
        gasToken->Initialize();
        neoToken->Initialize();
        policyContract->Initialize();
        ledgerContract->Initialize();

        // Initialize contract state
        notary->InitializeContract(*engine, Hardfork::Echidna);
        policyContract->InitializeContract(*engine, 0);

        // Set current block index
        auto key = ledgerContract->GetStorageKey(LedgerContract::PREFIX_BLOCK_STATE, io::ByteVector{});
        auto value = io::ByteVector::FromHexString(
            "0000000000000000000000000000000000000000000000000000000000000000e703000000000000");
        snapshot->Put(key, std::make_shared<StorageItem>(value));
    }
};

TEST_F(NotaryTest, TestGetMaxNotValidBeforeDelta)
{
    ASSERT_EQ(notary->GetMaxNotValidBeforeDelta(snapshot), Notary::DEFAULT_MAX_NOT_VALID_BEFORE_DELTA);
}

TEST_F(NotaryTest, TestSetMaxNotValidBeforeDelta)
{
    // Set committee address to account1
    neoToken->SetCommitteeAddress(snapshot, account1);

    // Set current script hash to account1
    engine->SetCurrentScriptHash(account1);

    // Set max not valid before delta
    ASSERT_TRUE(notary->SetMaxNotValidBeforeDelta(*engine, 100));

    // Check max not valid before delta
    ASSERT_EQ(notary->GetMaxNotValidBeforeDelta(snapshot), 100);
}

TEST_F(NotaryTest, TestExpirationOf)
{
    // Check expiration of empty deposit
    ASSERT_EQ(notary->ExpirationOf(snapshot, account1), 0);

    // Create deposit
    auto deposit = std::make_shared<Notary::Deposit>(1000, 2000);
    notary->PutDepositFor(*engine, account1, deposit);

    // Check expiration
    ASSERT_EQ(notary->ExpirationOf(snapshot, account1), 2000);
}

TEST_F(NotaryTest, TestBalanceOf)
{
    // Check balance of empty deposit
    ASSERT_EQ(notary->BalanceOf(snapshot, account1), 0);

    // Create deposit
    auto deposit = std::make_shared<Notary::Deposit>(1000, 2000);
    notary->PutDepositFor(*engine, account1, deposit);

    // Check balance
    ASSERT_EQ(notary->BalanceOf(snapshot, account1), 1000);
}

TEST_F(NotaryTest, TestLockDepositUntil)
{
    // Create deposit
    auto deposit = std::make_shared<Notary::Deposit>(1000, 2000);
    notary->PutDepositFor(*engine, account1, deposit);

    // Set current script hash to account1
    engine->SetCurrentScriptHash(account1);

    // Lock deposit until
    ASSERT_TRUE(notary->LockDepositUntil(*engine, account1, 3000));

    // Check expiration
    ASSERT_EQ(notary->ExpirationOf(snapshot, account1), 3000);
}

TEST_F(NotaryTest, TestWithdraw)
{
    // Create deposit
    auto deposit = std::make_shared<Notary::Deposit>(1000, 500);
    notary->PutDepositFor(*engine, account1, deposit);

    // Set current script hash to account1
    engine->SetCurrentScriptHash(account1);

    // Mint some GAS to the notary contract
    gasToken->Mint(snapshot, notary->GetScriptHash(), 1000);

    // Withdraw
    ASSERT_TRUE(notary->Withdraw(*engine, account1, account2));

    // Check balance
    ASSERT_EQ(notary->BalanceOf(snapshot, account1), 0);
    ASSERT_EQ(gasToken->GetBalance(snapshot, account2), 1000);
}

TEST_F(NotaryTest, TestOnNEP17Payment)
{
    // Set current script hash to GAS token
    engine->SetCurrentScriptHash(gasToken->GetScriptHash());

    // Set transaction sender to account1
    auto tx = std::make_shared<Transaction>();
    tx->SetSender(account1);
    engine->SetScriptContainer(tx.get());

    // Create data
    auto data = StackItem::CreateArray();
    data->Add(StackItem::Null());
    data->Add(StackItem::Create(static_cast<int64_t>(2000)));

    // Set attribute fee
    policyContract->SetAttributeFee(*engine, TransactionAttributeType::NotaryAssisted, 1000);

    // Call OnNEP17Payment
    notary->OnNEP17Payment(*engine, account1, 3000, data);

    // Check deposit
    ASSERT_EQ(notary->BalanceOf(snapshot, account1), 3000);
    ASSERT_EQ(notary->ExpirationOf(snapshot, account1), 2000);
}

TEST_F(NotaryTest, TestOnPersist)
{
    // Create deposit
    auto deposit = std::make_shared<Notary::Deposit>(10000, 2000);
    notary->PutDepositFor(*engine, account1, deposit);

    // Create transaction with notary assisted attribute
    auto tx = std::make_shared<Transaction>();
    tx->SetSender(notary->GetScriptHash());
    tx->SetSystemFee(1000);
    tx->SetNetworkFee(2000);

    // Add signers
    std::vector<Signer> signers;
    signers.push_back(Signer(notary->GetScriptHash()));
    signers.push_back(Signer(account1));
    tx->SetSigners(signers);

    // Add notary assisted attribute
    auto attr = std::make_shared<NotaryAssisted>();
    attr->SetNKeys(4);
    tx->AddAttribute(attr);

    // Add transaction to block
    std::vector<std::shared_ptr<Transaction>> transactions;
    transactions.push_back(tx);
    block->SetTransactions(transactions);

    // Set committee
    std::vector<ECPoint> committee;
    committee.push_back(ECPoint::FromBytes(
        ByteVector::FromHexString("03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c").AsSpan(),
        ECCurve::Secp256r1));
    neoToken->SetCommittee(snapshot, committee);

    // Call OnPersist
    ASSERT_TRUE(notary->OnPersist(*engine));

    // Check deposit
    ASSERT_EQ(notary->BalanceOf(snapshot, account1), 7000);
}

TEST_F(NotaryTest, TestDeposit)
{
    // Test Deposit constructor
    auto deposit = std::make_shared<Notary::Deposit>(1000, 2000);
    ASSERT_EQ(deposit->Amount, 1000);
    ASSERT_EQ(deposit->Till, 2000);

    // Test ToStackItem and FromStackItem
    auto referenceCounter = vm::ReferenceCounter();
    auto stackItem = deposit->ToStackItem(referenceCounter);
    auto deposit2 = std::make_shared<Notary::Deposit>();
    deposit2->FromStackItem(stackItem);
    ASSERT_EQ(deposit2->Amount, 1000);
    ASSERT_EQ(deposit2->Till, 2000);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
