// Disabled due to API mismatches - needs to be updated
#include <gtest/gtest.h>
#include <neo/smartcontract/native/policy_contract.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/persistence/memory_store_view.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>

using namespace neo::smartcontract::native;
using namespace neo::persistence;
using namespace neo::io;
using namespace neo::vm;
using namespace neo::cryptography;

class PolicyContractTest : public ::testing::Test
{
protected:
    std::shared_ptr<MemoryStoreView> snapshot;
    std::shared_ptr<PolicyContract> policyContract;
    std::shared_ptr<NeoToken> neoToken;
    std::shared_ptr<ApplicationEngine> engine;

    void SetUp() override
    {
        snapshot = std::make_shared<MemoryStoreView>();
        policyContract = PolicyContract::GetInstance();
        neoToken = NeoToken::GetInstance();
        engine = std::make_shared<ApplicationEngine>(TriggerType::Application, nullptr, snapshot, 0, false);

        // Initialize contracts
        policyContract->Initialize();
        neoToken->Initialize();

        // Set current block index
        snapshot->SetCurrentBlockIndex(0);
    }
};

TEST_F(PolicyContractTest, TestGetMaxTransactionsPerBlock)
{
    // Default value should be 512
    ASSERT_EQ(policyContract->GetMaxTransactionsPerBlock(snapshot), 512);

    // Set a new value
    auto key = policyContract->GetStorageKey(PolicyContract::PREFIX_MAX_TRANSACTIONS_PER_BLOCK, io::ByteVector{});
    uint32_t value = 1024;
    io::ByteVector valueBytes(io::ByteSpan(reinterpret_cast<const uint8_t*>(&value), sizeof(uint32_t)));
    policyContract->PutStorageValue(snapshot, key, valueBytes);

    // Check if the value was updated
    ASSERT_EQ(policyContract->GetMaxTransactionsPerBlock(snapshot), 1024);
}

TEST_F(PolicyContractTest, TestGetFeePerByte)
{
    // Default value should be 1000
    ASSERT_EQ(policyContract->GetFeePerByte(snapshot), 1000);

    // Set a new value
    auto key = policyContract->GetStorageKey(PolicyContract::PREFIX_FEE_PER_BYTE, io::ByteVector{});
    int64_t value = 2000;
    io::ByteVector valueBytes(io::ByteSpan(reinterpret_cast<const uint8_t*>(&value), sizeof(int64_t)));
    policyContract->PutStorageValue(snapshot, key, valueBytes);

    // Check if the value was updated
    ASSERT_EQ(policyContract->GetFeePerByte(snapshot), 2000);
}

TEST_F(PolicyContractTest, TestGetExecutionFeeFactor)
{
    // Default value should be 30
    ASSERT_EQ(policyContract->GetExecutionFeeFactor(snapshot), 30);

    // Set a new value
    auto key = policyContract->GetStorageKey(PolicyContract::PREFIX_EXECUTION_FEE_FACTOR, io::ByteVector{});
    uint32_t value = 50;
    io::ByteVector valueBytes(io::ByteSpan(reinterpret_cast<const uint8_t*>(&value), sizeof(uint32_t)));
    policyContract->PutStorageValue(snapshot, key, valueBytes);

    // Check if the value was updated
    ASSERT_EQ(policyContract->GetExecutionFeeFactor(snapshot), 50);
}

TEST_F(PolicyContractTest, TestGetStoragePrice)
{
    // Default value should be 100000
    ASSERT_EQ(policyContract->GetStoragePrice(snapshot), 100000);

    // Set a new value
    auto key = policyContract->GetStorageKey(PolicyContract::PREFIX_STORAGE_PRICE, io::ByteVector{});
    uint32_t value = 200000;
    io::ByteVector valueBytes(io::ByteSpan(reinterpret_cast<const uint8_t*>(&value), sizeof(uint32_t)));
    policyContract->PutStorageValue(snapshot, key, valueBytes);

    // Check if the value was updated
    ASSERT_EQ(policyContract->GetStoragePrice(snapshot), 200000);
}

TEST_F(PolicyContractTest, TestIsBlocked)
{
    // Create an account
    io::UInt160 account;
    std::memset(account.Data(), 1, account.Size());

    // Account should not be blocked by default
    ASSERT_FALSE(policyContract->IsBlocked(snapshot, account));

    // Block the account
    auto key = policyContract->GetStorageKey(PolicyContract::PREFIX_BLOCKED_ACCOUNT, account);
    policyContract->PutStorageValue(snapshot, key, io::ByteVector{1});

    // Account should be blocked
    ASSERT_TRUE(policyContract->IsBlocked(snapshot, account));

    // Unblock the account
    policyContract->DeleteStorageValue(snapshot, key);

    // Account should not be blocked
    ASSERT_FALSE(policyContract->IsBlocked(snapshot, account));
}

TEST_F(PolicyContractTest, TestGetAttributeFee)
{
    // Default value for regular attribute should be 0
    ASSERT_EQ(policyContract->GetAttributeFee(snapshot, 0x01), 0);

    // Default value for NotaryAssisted attribute should be 1000'0000
    ASSERT_EQ(policyContract->GetAttributeFee(snapshot, 0x20), 1000'0000);

    // Set a new value for regular attribute
    auto key = policyContract->GetStorageKey(PolicyContract::PREFIX_ATTRIBUTE_FEE, io::ByteVector{0x01});
    uint32_t value = 1000;
    io::ByteVector valueBytes(io::ByteSpan(reinterpret_cast<const uint8_t*>(&value), sizeof(uint32_t)));
    policyContract->PutStorageValue(snapshot, key, valueBytes);

    // Check if the value was updated
    ASSERT_EQ(policyContract->GetAttributeFee(snapshot, 0x01), 1000);
}

TEST_F(PolicyContractTest, TestGetMillisecondsPerBlock)
{
    // Default value should be 15000
    ASSERT_EQ(policyContract->GetMillisecondsPerBlock(snapshot), 15000);

    // Set a new value
    auto key = policyContract->GetStorageKey(PolicyContract::PREFIX_MILLISECONDS_PER_BLOCK, io::ByteVector{});
    uint32_t value = 20000;
    io::ByteVector valueBytes(io::ByteSpan(reinterpret_cast<const uint8_t*>(&value), sizeof(uint32_t)));
    policyContract->PutStorageValue(snapshot, key, valueBytes);

    // Check if the value was updated
    ASSERT_EQ(policyContract->GetMillisecondsPerBlock(snapshot), 20000);
}

TEST_F(PolicyContractTest, TestGetMaxValidUntilBlockIncrement)
{
    // Default value should be 5760
    ASSERT_EQ(policyContract->GetMaxValidUntilBlockIncrement(snapshot), 5760);

    // Set a new value
    auto key = policyContract->GetStorageKey(PolicyContract::PREFIX_MAX_VALID_UNTIL_BLOCK_INCREMENT, io::ByteVector{});
    uint32_t value = 10000;
    io::ByteVector valueBytes(io::ByteSpan(reinterpret_cast<const uint8_t*>(&value), sizeof(uint32_t)));
    policyContract->PutStorageValue(snapshot, key, valueBytes);

    // Check if the value was updated
    ASSERT_EQ(policyContract->GetMaxValidUntilBlockIncrement(snapshot), 10000);
}

TEST_F(PolicyContractTest, TestGetMaxTraceableBlocks)
{
    // Default value should be 2102400
    ASSERT_EQ(policyContract->GetMaxTraceableBlocks(snapshot), 2102400);

    // Set a new value
    auto key = policyContract->GetStorageKey(PolicyContract::PREFIX_MAX_TRACEABLE_BLOCKS, io::ByteVector{});
    uint32_t value = 3000000;
    io::ByteVector valueBytes(io::ByteSpan(reinterpret_cast<const uint8_t*>(&value), sizeof(uint32_t)));
    policyContract->PutStorageValue(snapshot, key, valueBytes);

    // Check if the value was updated
    ASSERT_EQ(policyContract->GetMaxTraceableBlocks(snapshot), 3000000);
}

TEST_F(PolicyContractTest, TestInitializeContract)
{
    // Clear all policy settings
    auto feePerByteKey = policyContract->GetStorageKey(PolicyContract::PREFIX_FEE_PER_BYTE, io::ByteVector{});
    auto execFeeFactorKey = policyContract->GetStorageKey(PolicyContract::PREFIX_EXECUTION_FEE_FACTOR, io::ByteVector{});
    auto storagePriceKey = policyContract->GetStorageKey(PolicyContract::PREFIX_STORAGE_PRICE, io::ByteVector{});
    auto millisecondsPerBlockKey = policyContract->GetStorageKey(PolicyContract::PREFIX_MILLISECONDS_PER_BLOCK, io::ByteVector{});
    auto maxValidUntilBlockIncrementKey = policyContract->GetStorageKey(PolicyContract::PREFIX_MAX_VALID_UNTIL_BLOCK_INCREMENT, io::ByteVector{});
    auto maxTraceableBlocksKey = policyContract->GetStorageKey(PolicyContract::PREFIX_MAX_TRACEABLE_BLOCKS, io::ByteVector{});

    snapshot->Delete(feePerByteKey);
    snapshot->Delete(execFeeFactorKey);
    snapshot->Delete(storagePriceKey);
    snapshot->Delete(millisecondsPerBlockKey);
    snapshot->Delete(maxValidUntilBlockIncrementKey);
    snapshot->Delete(maxTraceableBlocksKey);

    // Initialize the contract
    ASSERT_TRUE(policyContract->InitializeContract(*engine, 0));

    // Check if all values were initialized
    ASSERT_EQ(policyContract->GetFeePerByte(snapshot), PolicyContract::DEFAULT_FEE_PER_BYTE);
    ASSERT_EQ(policyContract->GetExecutionFeeFactor(snapshot), PolicyContract::DEFAULT_EXECUTION_FEE_FACTOR);
    ASSERT_EQ(policyContract->GetStoragePrice(snapshot), PolicyContract::DEFAULT_STORAGE_PRICE);
    ASSERT_EQ(policyContract->GetMillisecondsPerBlock(snapshot), PolicyContract::DEFAULT_MILLISECONDS_PER_BLOCK);
    ASSERT_EQ(policyContract->GetMaxValidUntilBlockIncrement(snapshot), PolicyContract::DEFAULT_MAX_VALID_UNTIL_BLOCK_INCREMENT);
    ASSERT_EQ(policyContract->GetMaxTraceableBlocks(snapshot), PolicyContract::DEFAULT_MAX_TRACEABLE_BLOCKS);
}

TEST_F(PolicyContractTest, TestOnPersist)
{
    // Clear all policy settings
    auto feePerByteKey = policyContract->GetStorageKey(PolicyContract::PREFIX_FEE_PER_BYTE, io::ByteVector{});
    auto execFeeFactorKey = policyContract->GetStorageKey(PolicyContract::PREFIX_EXECUTION_FEE_FACTOR, io::ByteVector{});
    auto storagePriceKey = policyContract->GetStorageKey(PolicyContract::PREFIX_STORAGE_PRICE, io::ByteVector{});
    auto millisecondsPerBlockKey = policyContract->GetStorageKey(PolicyContract::PREFIX_MILLISECONDS_PER_BLOCK, io::ByteVector{});
    auto maxValidUntilBlockIncrementKey = policyContract->GetStorageKey(PolicyContract::PREFIX_MAX_VALID_UNTIL_BLOCK_INCREMENT, io::ByteVector{});
    auto maxTraceableBlocksKey = policyContract->GetStorageKey(PolicyContract::PREFIX_MAX_TRACEABLE_BLOCKS, io::ByteVector{});

    snapshot->Delete(feePerByteKey);
    snapshot->Delete(execFeeFactorKey);
    snapshot->Delete(storagePriceKey);
    snapshot->Delete(millisecondsPerBlockKey);
    snapshot->Delete(maxValidUntilBlockIncrementKey);
    snapshot->Delete(maxTraceableBlocksKey);

    // Call OnPersist
    ASSERT_TRUE(policyContract->OnPersist(*engine));

    // Check if all values were initialized
    ASSERT_EQ(policyContract->GetFeePerByte(snapshot), PolicyContract::DEFAULT_FEE_PER_BYTE);
    ASSERT_EQ(policyContract->GetExecutionFeeFactor(snapshot), PolicyContract::DEFAULT_EXECUTION_FEE_FACTOR);
    ASSERT_EQ(policyContract->GetStoragePrice(snapshot), PolicyContract::DEFAULT_STORAGE_PRICE);
    ASSERT_EQ(policyContract->GetMillisecondsPerBlock(snapshot), PolicyContract::DEFAULT_MILLISECONDS_PER_BLOCK);
    ASSERT_EQ(policyContract->GetMaxValidUntilBlockIncrement(snapshot), PolicyContract::DEFAULT_MAX_VALID_UNTIL_BLOCK_INCREMENT);
    ASSERT_EQ(policyContract->GetMaxTraceableBlocks(snapshot), PolicyContract::DEFAULT_MAX_TRACEABLE_BLOCKS);
}

TEST_F(PolicyContractTest, TestPostPersist)
{
    // Call PostPersist
    ASSERT_TRUE(policyContract->PostPersist(*engine));
}

TEST_F(PolicyContractTest, TestSetMillisecondsPerBlockWithEchidnaHardfork)
{
    // Create application engine with Echidna hardfork enabled
    auto engineWithHardfork = std::make_shared<ApplicationEngine>(TriggerType::Application, nullptr, snapshot, 0, false);
    engineWithHardfork->SetHardforkEnabled(Hardfork::Echidna, true);

    // Set the committee address to the current script hash
    auto committeeAddress = engineWithHardfork->GetCurrentScriptHash();
    neoToken->SetCommitteeAddress(snapshot, committeeAddress);

    // Track notifications
    std::vector<std::tuple<io::UInt160, std::string, std::shared_ptr<StackItem>>> notifications;
    engineWithHardfork->SetNotificationCallback([&notifications](const io::UInt160& scriptHash, const std::string& eventName, const std::shared_ptr<StackItem>& state) {
        notifications.emplace_back(scriptHash, eventName, state);
    });

    // Call setMillisecondsPerBlock
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(20000));
    auto result = policyContract->Call(*engineWithHardfork, "setMillisecondsPerBlock", args);

    // Check result
    ASSERT_TRUE(result->GetBoolean());

    // Check notifications
    ASSERT_EQ(notifications.size(), 1);
    ASSERT_EQ(std::get<1>(notifications[0]), "MillisecondsPerBlockChanged");

    // Check notification state
    auto state = std::get<2>(notifications[0]);
    ASSERT_TRUE(state->IsArray());
    auto stateArray = state->GetArray();
    ASSERT_EQ(stateArray.size(), 2); // old, new

    // Check old value
    ASSERT_TRUE(stateArray[0]->IsInteger());
    ASSERT_EQ(stateArray[0]->GetInteger(), 15000);

    // Check new value
    ASSERT_TRUE(stateArray[1]->IsInteger());
    ASSERT_EQ(stateArray[1]->GetInteger(), 20000);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
