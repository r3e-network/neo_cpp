#include <gtest/gtest.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/persistence/memory_store_view.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>

using namespace neo::smartcontract::native;
using namespace neo::smartcontract;
using namespace neo::persistence;
using namespace neo::io;
using namespace neo::vm;

class GasTokenTest : public ::testing::Test
{
protected:
    std::shared_ptr<MemoryStoreView> snapshot;
    std::shared_ptr<GasToken> gasToken;
    std::shared_ptr<ApplicationEngine> engine;
    
    void SetUp() override
    {
        snapshot = std::make_shared<MemoryStoreView>();
        gasToken = GasToken::GetInstance();
        // engine = std::make_shared<ApplicationEngine>(TriggerType::Application, nullptr, snapshot, nullptr);
    }
};

TEST_F(GasTokenTest, TestSymbol)
{
    // Call the symbol method
    auto result = gasToken->Call(*engine, "symbol", {});
    
    // Check the result
    ASSERT_TRUE(result->IsString());
    ASSERT_EQ(result->GetString(), "GAS");
}

TEST_F(GasTokenTest, TestDecimals)
{
    // Call the decimals method
    auto result = gasToken->Call(*engine, "decimals", {});
    
    // Check the result
    ASSERT_TRUE(result->IsInteger());
    ASSERT_EQ(result->GetInteger(), 8);
}

TEST_F(GasTokenTest, TestTotalSupply)
{
    // Call the totalSupply method
    auto result = gasToken->Call(*engine, "totalSupply", {});
    
    // Check the result
    ASSERT_TRUE(result->IsInteger());
    ASSERT_EQ(result->GetInteger(), gasToken->GetTotalSupply(snapshot));
}

TEST_F(GasTokenTest, TestBalanceOf)
{
    // Create an account
    UInt160 account;
    std::memset(account.Data(), 1, account.Size());
    
    // Mint some GAS
    gasToken->Mint(snapshot, account, 100);
    
    // Call the balanceOf method
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(account));
    auto result = gasToken->Call(*engine, "balanceOf", args);
    
    // Check the result
    ASSERT_TRUE(result->IsInteger());
    ASSERT_EQ(result->GetInteger(), 100);
}

TEST_F(GasTokenTest, TestTransfer)
{
    // Create accounts
    UInt160 from;
    std::memset(from.Data(), 1, from.Size());
    
    UInt160 to;
    std::memset(to.Data(), 2, to.Size());
    
    // Mint some GAS
    gasToken->Mint(snapshot, from, 100);
    
    // Set the current script hash to the from account
    engine->SetCurrentScriptHash(from);
    
    // Call the transfer method
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(from));
    args.push_back(StackItem::Create(to));
    args.push_back(StackItem::Create(50));
    auto result = gasToken->Call(*engine, "transfer", args);
    
    // Check the result
    ASSERT_TRUE(result->IsBoolean());
    ASSERT_TRUE(result->GetBoolean());
    
    // Check the balances
    ASSERT_EQ(gasToken->GetBalance(snapshot, from), 50);
    ASSERT_EQ(gasToken->GetBalance(snapshot, to), 50);
}

TEST_F(GasTokenTest, TestMint)
{
    // Create an account
    UInt160 account;
    std::memset(account.Data(), 1, account.Size());
    
    // Mint some GAS
    bool result = gasToken->Mint(snapshot, account, 100);
    
    // Check the result
    ASSERT_TRUE(result);
    
    // Check the balance
    ASSERT_EQ(gasToken->GetBalance(snapshot, account), 100);
    
    // Check the total supply
    ASSERT_EQ(gasToken->GetTotalSupply(snapshot), 100);
}

TEST_F(GasTokenTest, TestBurn)
{
    // Create an account
    UInt160 account;
    std::memset(account.Data(), 1, account.Size());
    
    // Mint some GAS
    gasToken->Mint(snapshot, account, 100);
    
    // Burn some GAS
    bool result = gasToken->Burn(snapshot, account, 50);
    
    // Check the result
    ASSERT_TRUE(result);
    
    // Check the balance
    ASSERT_EQ(gasToken->GetBalance(snapshot, account), 50);
    
    // Check the total supply
    ASSERT_EQ(gasToken->GetTotalSupply(snapshot), 50);
}

TEST_F(GasTokenTest, TestGasPerBlock)
{
    // Get the default gas per block
    int64_t gasPerBlock = gasToken->GetGasPerBlock(snapshot);
    
    // Check the default value
    ASSERT_EQ(gasPerBlock, 5 * GasToken::FACTOR);
    
    // Set a new gas per block
    gasToken->SetGasPerBlock(snapshot, 10 * GasToken::FACTOR);
    
    // Check the new value
    gasPerBlock = gasToken->GetGasPerBlock(snapshot);
    ASSERT_EQ(gasPerBlock, 10 * GasToken::FACTOR);
}

TEST_F(GasTokenTest, TestPostTransfer)
{
    // Create accounts
    UInt160 from;
    std::memset(from.Data(), 1, from.Size());
    
    UInt160 to;
    std::memset(to.Data(), 2, to.Size());
    
    // Mint some GAS
    gasToken->Mint(snapshot, from, 100);
    
    // Set the current script hash to the from account
    engine->SetCurrentScriptHash(from);
    
    // Call the PostTransfer method
    bool result = gasToken->PostTransfer(*engine, from, to, 50, StackItem::Null(), false);
    
    // Check the result
    ASSERT_TRUE(result);
    
    // Check that a notification was sent
    auto notifications = engine->GetNotifications();
    ASSERT_EQ(notifications.size(), 1);
    ASSERT_EQ(notifications[0].GetEventName(), "Transfer");
    ASSERT_EQ(notifications[0].GetScriptHash(), gasToken->GetScriptHash());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
