#include <gtest/gtest.h>
#include <neo/smartcontract/native/name_service.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/persistence/memory_store_view.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>

using namespace neo::smartcontract::native;
using namespace neo::persistence;
using namespace neo::io;
using namespace neo::vm;

class NameServiceTest : public ::testing::Test
{
protected:
    std::shared_ptr<MemoryStoreView> snapshot;
    std::shared_ptr<NameService> nameService;
    std::shared_ptr<ApplicationEngine> engine;

    void SetUp() override
    {
        snapshot = std::make_shared<MemoryStoreView>();
        nameService = NameService::GetInstance();
        engine = std::make_shared<ApplicationEngine>(TriggerType::Application, nullptr, snapshot, 0, false);
    }
};

TEST_F(NameServiceTest, TestGetPrice)
{
    // Default price
    EXPECT_EQ(nameService->GetPrice(snapshot), NameService::DEFAULT_PRICE);

    // Set price
    auto key = nameService->GetStorageKey(NameService::PREFIX_PRICE, ByteVector{});
    int64_t price = 2000000;
    ByteVector value(ByteSpan(reinterpret_cast<const uint8_t*>(&price), sizeof(int64_t)));
    nameService->PutStorageValue(snapshot, key, value);

    // Get price
    EXPECT_EQ(nameService->GetPrice(snapshot), price);
}

TEST_F(NameServiceTest, TestValidateName)
{
    // Valid names
    EXPECT_TRUE(nameService->ValidateName("abc"));
    EXPECT_TRUE(nameService->ValidateName("abc123"));
    EXPECT_TRUE(nameService->ValidateName("a-b-c"));
    EXPECT_TRUE(nameService->ValidateName("123abc"));

    // Invalid names
    EXPECT_FALSE(nameService->ValidateName(""));
    EXPECT_FALSE(nameService->ValidateName("a"));
    EXPECT_FALSE(nameService->ValidateName("ab"));
    EXPECT_FALSE(nameService->ValidateName("-abc"));
    EXPECT_FALSE(nameService->ValidateName("abc-"));
    EXPECT_FALSE(nameService->ValidateName("ABC"));
    EXPECT_FALSE(nameService->ValidateName("abc.def"));
    EXPECT_FALSE(nameService->ValidateName("abc_def"));
    EXPECT_FALSE(nameService->ValidateName("abc def"));
}

TEST_F(NameServiceTest, TestIsAvailable)
{
    // Available name
    EXPECT_TRUE(nameService->IsAvailable(snapshot, "abc123"));

    // Register name
    auto key = nameService->GetStorageKey(NameService::PREFIX_NAME, "abc123");
    UInt160 owner;
    std::memset(owner.Data(), 1, owner.Size());
    uint64_t expiration = 1000;
    std::ostringstream stream;
    BinaryWriter writer(stream);
    writer.Write(owner);
    writer.Write(expiration);
    std::string data = stream.str();
    ByteVector value(ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    nameService->PutStorageValue(snapshot, key, value);

    // Not available
    snapshot->SetCurrentBlockIndex(500);
    EXPECT_FALSE(nameService->IsAvailable(snapshot, "abc123"));

    // Expired
    snapshot->SetCurrentBlockIndex(1500);
    EXPECT_TRUE(nameService->IsAvailable(snapshot, "abc123"));

    // Invalid name
    EXPECT_FALSE(nameService->IsAvailable(snapshot, "a"));
}

TEST_F(NameServiceTest, TestGetName)
{
    // Register name
    auto key = nameService->GetStorageKey(NameService::PREFIX_NAME, "abc123");
    UInt160 owner;
    std::memset(owner.Data(), 1, owner.Size());
    uint64_t expiration = 1000;
    std::ostringstream stream;
    BinaryWriter writer(stream);
    writer.Write(owner);
    writer.Write(expiration);
    std::string data = stream.str();
    ByteVector value(ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    nameService->PutStorageValue(snapshot, key, value);

    // Get name
    auto [retrievedOwner, retrievedExpiration] = nameService->GetName(snapshot, "abc123");
    EXPECT_EQ(retrievedOwner, owner);
    EXPECT_EQ(retrievedExpiration, expiration);

    // Name not found
    EXPECT_THROW(nameService->GetName(snapshot, "def456"), std::runtime_error);
}

TEST_F(NameServiceTest, TestInitializeContract)
{
    // Initialize the contract
    EXPECT_TRUE(nameService->InitializeContract(*engine, 0));

    // Check if the price was set
    EXPECT_EQ(nameService->GetPrice(snapshot), NameService::DEFAULT_PRICE);
}

TEST_F(NameServiceTest, TestOnPersist)
{
    // Clear the price
    auto key = nameService->GetStorageKey(NameService::PREFIX_PRICE, ByteVector{});
    snapshot->Delete(key);

    // Check if the price is empty
    EXPECT_EQ(nameService->GetPrice(snapshot), 0);

    // Call OnPersist
    EXPECT_TRUE(nameService->OnPersist(*engine));

    // Check if the price was set
    EXPECT_EQ(nameService->GetPrice(snapshot), NameService::DEFAULT_PRICE);
}

TEST_F(NameServiceTest, TestPostPersist)
{
    // Call PostPersist
    EXPECT_TRUE(nameService->PostPersist(*engine));
}

TEST_F(NameServiceTest, TestRegisterAndResolve)
{
    // Initialize the contract
    nameService->InitializeContract(*engine, 0);

    // Set up the engine to simulate a user
    UInt160 userScriptHash;
    std::memset(userScriptHash.Data(), 1, userScriptHash.Size());
    engine->SetCurrentScriptHash(userScriptHash);

    // Add some GAS to the user's account
    auto gasToken = GasToken::GetInstance();
    gasToken->Mint(*engine, userScriptHash, 100000000, true); // 1 GAS

    // Call the register method
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create("example"));
    args.push_back(StackItem::Create(ByteVector(ByteSpan(userScriptHash.Data(), userScriptHash.Size()))));
    auto result = nameService->Call(*engine, "register", args);

    // Check the result
    ASSERT_TRUE(result->IsBoolean());
    ASSERT_TRUE(result->GetBoolean());

    // Call the resolve method
    args.clear();
    args.push_back(StackItem::Create("example"));
    result = nameService->Call(*engine, "resolve", args);

    // Check the result
    ASSERT_TRUE(result->IsBuffer());
    ASSERT_EQ(result->GetByteArray(), ByteVector(ByteSpan(userScriptHash.Data(), userScriptHash.Size())));
}
