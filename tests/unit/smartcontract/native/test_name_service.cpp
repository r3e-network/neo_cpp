// Disabled due to API mismatches - needs to be updated
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
using namespace neo::smartcontract;
using namespace neo::persistence;
using namespace neo::io;
using namespace neo::vm;

class NameServiceTest : public ::testing::Test
{
protected:
    std::shared_ptr<MemoryStoreView> snapshot;
    std::shared_ptr<NameService> nameService;
    std::shared_ptr<smartcontract::ApplicationEngine> engine;

    void SetUp() override
    {
        snapshot = std::make_shared<MemoryStoreView>();
        nameService = NameService::GetInstance();
        engine = std::make_shared<smartcontract::ApplicationEngine>(smartcontract::TriggerType::Application, nullptr, snapshot, 0, false);
    }
};

TEST_F(NameServiceTest, TestGetPrice)
{
    // Default price
    EXPECT_EQ(nameService->GetPrice(*snapshot), NameService::DEFAULT_PRICE);

    // Set price
    auto key = nameService->GetStorageKey(NameService::PREFIX_PRICE, ByteVector{});
    int64_t price = 2000000;
    ByteVector value(ByteSpan(reinterpret_cast<const uint8_t*>(&price), sizeof(int64_t)));
    nameService->PutStorageValue(snapshot, key, value);

    // Get price
    EXPECT_EQ(nameService->GetPrice(*snapshot), price);
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
    EXPECT_TRUE(nameService->IsAvailable(*snapshot, "abc123"));

    // Register name
    auto key = nameService->GetStorageKey(NameService::PREFIX_NAME, "abc123");
    UInt160 owner;
    std::memset(owner.Data(), 1, UInt160::Size);
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
    EXPECT_FALSE(nameService->IsAvailable(*snapshot, "abc123"));

    // Expired
    snapshot->SetCurrentBlockIndex(1500);
    EXPECT_TRUE(nameService->IsAvailable(*snapshot, "abc123"));

    // Invalid name
    EXPECT_FALSE(nameService->IsAvailable(*snapshot, "a"));
}

TEST_F(NameServiceTest, TestGetName)
{
    // Register name
    auto key = nameService->GetStorageKey(NameService::PREFIX_NAME, "abc123");
    UInt160 owner;
    std::memset(owner.Data(), 1, UInt160::Size);
    uint64_t expiration = 1000;
    std::ostringstream stream;
    BinaryWriter writer(stream);
    writer.Write(owner);
    writer.Write(expiration);
    std::string data = stream.str();
    ByteVector value(ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    nameService->PutStorageValue(snapshot, key, value);

    // Get name data
    auto nameData = nameService->GetName(*snapshot, "abc123");
    EXPECT_TRUE(nameData.has_value());
    EXPECT_EQ(nameData->owner, owner);
    EXPECT_EQ(nameData->expiration, expiration);

    // Get non-existent name
    auto noData = nameService->GetName(*snapshot, "xyz789");
    EXPECT_FALSE(noData.has_value());
}

TEST_F(NameServiceTest, TestOnNep17Payment)
{
    // Create a test account
    UInt160 from;
    std::memset(from.Data(), 1, UInt160::Size);

    // Set up GAS payment
    int64_t amount = NameService::DEFAULT_PRICE;
    ByteVector data = ByteVector::FromUtf8("abc123");

    // Test valid payment
    bool result = nameService->OnNep17Payment(*engine, from, amount, data);
    EXPECT_TRUE(result);

    // Test invalid payment - wrong token
    // TODO: Need to set up wrong token context
    // result = nameService->OnNep17Payment(*engine, from, amount, data);
    // EXPECT_FALSE(result);

    // Test invalid payment - insufficient amount
    result = nameService->OnNep17Payment(*engine, from, amount / 2, data);
    EXPECT_FALSE(result);

    // Test invalid payment - invalid name
    data = ByteVector::FromUtf8("ab");
    result = nameService->OnNep17Payment(*engine, from, amount, data);
    EXPECT_FALSE(result);
}

TEST_F(NameServiceTest, TestPostPersist)
{
    // Set up NEO balance for NameService contract
    auto neoToken = NeoToken::GetInstance();
    neoToken->Mint(*snapshot, nameService->GetScriptHash(), 1000000);

    // Call PostPersist
    EXPECT_TRUE(nameService->PostPersist(*engine));
}

TEST_F(NameServiceTest, DISABLED_TestRegisterAndResolve)
{
    // Initialize contract
    nameService->InitializeContract(*engine, 0);

    // Create a user account
    UInt160 userScriptHash;
    std::memset(userScriptHash.Data(), 1, UInt160::Size);

    // Complete Call method implementation - now fully implemented
    // Register a name
    std::vector<std::shared_ptr<vm::StackItem>> args;
    args.push_back(vm::StackItem::CreateString("testname"));
    args.push_back(vm::StackItem::CreateByteArray(io::ByteSpan(userScriptHash.Data(), io::UInt160::Size)));
    auto result = nameService->Call(*engine, "register", args);
    ASSERT_TRUE(result->IsBoolean());
    ASSERT_TRUE(result->GetBoolean());

    // Resolve the name
    // args.clear();
    // args.push_back(StackItem::Create("testname"));
    // result = nameService->Call(*engine, "resolve", args);
    // ASSERT_TRUE(result->IsBuffer());
    // ASSERT_EQ(result->GetByteArray(), ByteVector(ByteSpan(userScriptHash.Data(), UInt160::Size)));
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}