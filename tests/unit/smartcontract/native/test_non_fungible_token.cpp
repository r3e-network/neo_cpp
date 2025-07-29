// Disabled due to API mismatches - needs to be updated
#include <gtest/gtest.h>
#include <memory>
#include <neo/io/uint160.h>
#include <neo/persistence/memory_store_view.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/non_fungible_token.h>
#include <neo/vm/stack_item.h>

using namespace neo;
using namespace neo::smartcontract;
using namespace neo::smartcontract::native;
using namespace neo::persistence;
using namespace neo::io;
using namespace neo::vm;

// Mock NonFungibleToken implementation for testing
class MockNonFungibleToken : public NonFungibleToken
{
  public:
    MockNonFungibleToken() : NonFungibleToken("Mock", 100) {}

    std::string GetSymbol() const override
    {
        return "MNFT";
    }

    static std::shared_ptr<MockNonFungibleToken> GetInstance()
    {
        static std::shared_ptr<MockNonFungibleToken> instance = std::make_shared<MockNonFungibleToken>();
        return instance;
    }

    bool MintToken(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& tokenId,
                   const io::UInt160& owner, const std::map<std::string, std::shared_ptr<vm::StackItem>>& properties)
    {
        return Mint(snapshot, tokenId, owner, properties);
    }

    bool MintToken(ApplicationEngine& engine, const io::ByteVector& tokenId, const io::UInt160& owner,
                   const std::map<std::string, std::shared_ptr<vm::StackItem>>& properties,
                   std::shared_ptr<vm::StackItem> data, bool callOnPayment)
    {
        return Mint(engine, tokenId, owner, properties, data, callOnPayment);
    }

    bool BurnToken(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& tokenId)
    {
        return Burn(snapshot, tokenId);
    }

    bool BurnToken(ApplicationEngine& engine, const io::ByteVector& tokenId)
    {
        return Burn(engine, tokenId);
    }
};

class NonFungibleTokenTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MemoryStoreView> snapshot;
    std::shared_ptr<MockNonFungibleToken> token;
    std::shared_ptr<ApplicationEngine> engine;
    UInt160 account1;
    UInt160 account2;
    ByteVector tokenId1;
    ByteVector tokenId2;
    std::map<std::string, std::shared_ptr<StackItem>> properties1;
    std::map<std::string, std::shared_ptr<StackItem>> properties2;

    void SetUp() override
    {
        snapshot = std::make_shared<MemoryStoreView>();
        token = MockNonFungibleToken::GetInstance();
        engine = std::make_shared<ApplicationEngine>(TriggerType::Application, nullptr, snapshot, 0, false);

        // Create test accounts
        std::memset(account1.Data(), 1, account1.Size());
        std::memset(account2.Data(), 2, account2.Size());

        // Create test token IDs
        tokenId1 = ByteVector::FromHexString("0102030405");
        tokenId2 = ByteVector::FromHexString("0607080910");

        // Create test properties
        properties1["name"] = StackItem::Create("Token 1");
        properties1["image"] = StackItem::Create("https://example.com/token1.jpg");
        properties1["description"] = StackItem::Create("This is token 1");

        properties2["name"] = StackItem::Create("Token 2");
        properties2["image"] = StackItem::Create("https://example.com/token2.jpg");
        properties2["description"] = StackItem::Create("This is token 2");
    }
};

TEST_F(NonFungibleTokenTest, TestGetSymbol)
{
    ASSERT_EQ(token->GetSymbol(), "MNFT");
}

TEST_F(NonFungibleTokenTest, TestGetDecimals)
{
    ASSERT_EQ(token->GetDecimals(), 0);
}

TEST_F(NonFungibleTokenTest, TestGetTotalSupply)
{
    // Initial total supply should be 0
    ASSERT_EQ(token->GetTotalSupply(snapshot), 0);

    // Mint a token
    ASSERT_TRUE(token->MintToken(snapshot, tokenId1, account1, properties1));

    // Total supply should be updated
    ASSERT_EQ(token->GetTotalSupply(snapshot), 1);
}

TEST_F(NonFungibleTokenTest, TestGetBalanceOf)
{
    // Initial balance should be 0
    ASSERT_EQ(token->GetBalanceOf(snapshot, account1), 0);

    // Mint a token
    ASSERT_TRUE(token->MintToken(snapshot, tokenId1, account1, properties1));

    // Balance should be updated
    ASSERT_EQ(token->GetBalanceOf(snapshot, account1), 1);
}

TEST_F(NonFungibleTokenTest, TestGetOwnerOf)
{
    // Initial owner should be null
    ASSERT_TRUE(token->GetOwnerOf(snapshot, tokenId1).IsZero());

    // Mint a token
    ASSERT_TRUE(token->MintToken(snapshot, tokenId1, account1, properties1));

    // Owner should be updated
    ASSERT_EQ(token->GetOwnerOf(snapshot, tokenId1), account1);
}

TEST_F(NonFungibleTokenTest, TestGetProperties)
{
    // Initial properties should be empty
    ASSERT_TRUE(token->GetProperties(snapshot, tokenId1).empty());

    // Mint a token
    ASSERT_TRUE(token->MintToken(snapshot, tokenId1, account1, properties1));

    // Properties should be updated
    auto props = token->GetProperties(snapshot, tokenId1);
    ASSERT_EQ(props.size(), 3);
    ASSERT_EQ(props["name"]->GetString(), "Token 1");
    ASSERT_EQ(props["image"]->GetString(), "https://example.com/token1.jpg");
    ASSERT_EQ(props["description"]->GetString(), "This is token 1");
}

TEST_F(NonFungibleTokenTest, TestGetTokens)
{
    // Initial tokens should be empty
    ASSERT_TRUE(token->GetTokens(snapshot).empty());

    // Mint tokens
    ASSERT_TRUE(token->MintToken(snapshot, tokenId1, account1, properties1));
    ASSERT_TRUE(token->MintToken(snapshot, tokenId2, account1, properties2));

    // Tokens should be updated
    auto tokens = token->GetTokens(snapshot);
    ASSERT_EQ(tokens.size(), 2);
    ASSERT_TRUE(std::find(tokens.begin(), tokens.end(), tokenId1) != tokens.end());
    ASSERT_TRUE(std::find(tokens.begin(), tokens.end(), tokenId2) != tokens.end());
}

TEST_F(NonFungibleTokenTest, TestGetTokensOf)
{
    // Initial tokens should be empty
    ASSERT_TRUE(token->GetTokensOf(snapshot, account1).empty());

    // Mint tokens
    ASSERT_TRUE(token->MintToken(snapshot, tokenId1, account1, properties1));
    ASSERT_TRUE(token->MintToken(snapshot, tokenId2, account1, properties2));

    // Tokens should be updated
    auto tokens = token->GetTokensOf(snapshot, account1);
    ASSERT_EQ(tokens.size(), 2);
    ASSERT_TRUE(std::find(tokens.begin(), tokens.end(), tokenId1) != tokens.end());
    ASSERT_TRUE(std::find(tokens.begin(), tokens.end(), tokenId2) != tokens.end());
}

TEST_F(NonFungibleTokenTest, TestTransfer)
{
    // Mint a token to account1
    ASSERT_TRUE(token->MintToken(snapshot, tokenId1, account1, properties1));

    // Transfer token from account1 to account2
    ASSERT_TRUE(token->Transfer(snapshot, account1, account2, tokenId1));

    // Check owner
    ASSERT_EQ(token->GetOwnerOf(snapshot, tokenId1), account2);

    // Check balances
    ASSERT_EQ(token->GetBalanceOf(snapshot, account1), 0);
    ASSERT_EQ(token->GetBalanceOf(snapshot, account2), 1);

    // Check tokens of accounts
    ASSERT_TRUE(token->GetTokensOf(snapshot, account1).empty());
    auto tokens = token->GetTokensOf(snapshot, account2);
    ASSERT_EQ(tokens.size(), 1);
    ASSERT_EQ(tokens[0], tokenId1);
}

TEST_F(NonFungibleTokenTest, TestTransferWithEngine)
{
    // Mint a token to account1
    ASSERT_TRUE(token->MintToken(snapshot, tokenId1, account1, properties1));

    // Set current script hash to account1
    engine->SetCurrentScriptHash(account1);

    // Transfer token from account1 to account2
    ASSERT_TRUE(token->Transfer(*engine, account1, account2, tokenId1, StackItem::Null(), true));

    // Check owner
    ASSERT_EQ(token->GetOwnerOf(snapshot, tokenId1), account2);

    // Check balances
    ASSERT_EQ(token->GetBalanceOf(snapshot, account1), 0);
    ASSERT_EQ(token->GetBalanceOf(snapshot, account2), 1);
}

TEST_F(NonFungibleTokenTest, TestMint)
{
    // Mint a token
    ASSERT_TRUE(token->MintToken(snapshot, tokenId1, account1, properties1));

    // Check owner
    ASSERT_EQ(token->GetOwnerOf(snapshot, tokenId1), account1);

    // Check balance
    ASSERT_EQ(token->GetBalanceOf(snapshot, account1), 1);

    // Check total supply
    ASSERT_EQ(token->GetTotalSupply(snapshot), 1);

    // Check properties
    auto props = token->GetProperties(snapshot, tokenId1);
    ASSERT_EQ(props.size(), 3);
    ASSERT_EQ(props["name"]->GetString(), "Token 1");
}

TEST_F(NonFungibleTokenTest, TestMintWithEngine)
{
    // Mint a token
    ASSERT_TRUE(token->MintToken(*engine, tokenId1, account1, properties1, StackItem::Null(), true));

    // Check owner
    ASSERT_EQ(token->GetOwnerOf(snapshot, tokenId1), account1);

    // Check balance
    ASSERT_EQ(token->GetBalanceOf(snapshot, account1), 1);

    // Check total supply
    ASSERT_EQ(token->GetTotalSupply(snapshot), 1);
}

TEST_F(NonFungibleTokenTest, TestBurn)
{
    // Mint a token
    ASSERT_TRUE(token->MintToken(snapshot, tokenId1, account1, properties1));

    // Burn the token
    ASSERT_TRUE(token->BurnToken(snapshot, tokenId1));

    // Check owner
    ASSERT_TRUE(token->GetOwnerOf(snapshot, tokenId1).IsZero());

    // Check balance
    ASSERT_EQ(token->GetBalanceOf(snapshot, account1), 0);

    // Check total supply
    ASSERT_EQ(token->GetTotalSupply(snapshot), 0);

    // Check tokens
    ASSERT_TRUE(token->GetTokens(snapshot).empty());
    ASSERT_TRUE(token->GetTokensOf(snapshot, account1).empty());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
