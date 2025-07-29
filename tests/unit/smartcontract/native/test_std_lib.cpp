// Disabled due to API mismatches - needs to be updated
#include <gtest/gtest.h>
#include <neo/cryptography/base58.h>
#include <neo/cryptography/base64.h>
#include <neo/cryptography/base64url.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/persistence/memory_store_view.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/std_lib.h>
#include <sstream>

using namespace neo::smartcontract::native;
using namespace neo::persistence;
using namespace neo::io;
using namespace neo::vm;
using namespace neo::cryptography;

class StdLibTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MemoryStoreView> snapshot;
    std::shared_ptr<StdLib> stdLib;
    std::shared_ptr<ApplicationEngine> engine;

    void SetUp() override
    {
        snapshot = std::make_shared<MemoryStoreView>();
        stdLib = std::make_shared<StdLib>();
        engine = std::make_shared<ApplicationEngine>(TriggerType::Application, nullptr, snapshot, 0, false);
    }
};

TEST_F(StdLibTest, TestSerializeDeserialize)
{
    // Create a stack item
    auto item = StackItem::Create("test");

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(item);

    // Call the serialize method
    auto result = stdLib->Call(*engine, "serialize", args);

    // Check the result
    ASSERT_TRUE(result->IsBuffer());
    auto serialized = result->GetByteArray();

    // Call the deserialize method
    args.clear();
    args.push_back(StackItem::Create(serialized));
    auto deserialized = stdLib->Call(*engine, "deserialize", args);

    // Check the result
    ASSERT_TRUE(deserialized->IsString());
    ASSERT_EQ(deserialized->GetString(), "test");
}

TEST_F(StdLibTest, TestJsonSerializeDeserialize)
{
    // Create a stack item
    auto item = StackItem::Create("test");

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(item);

    // Call the jsonSerialize method
    auto result = stdLib->Call(*engine, "jsonSerialize", args);

    // Check the result
    ASSERT_TRUE(result->IsString());
    auto serialized = result->GetString();

    // Call the jsonDeserialize method
    args.clear();
    args.push_back(StackItem::Create(serialized));
    auto deserialized = stdLib->Call(*engine, "jsonDeserialize", args);

    // Check the result
    ASSERT_TRUE(deserialized->IsString());
    ASSERT_EQ(deserialized->GetString(), "test");
}

TEST_F(StdLibTest, TestItoaAtoi)
{
    // Test itoa with base 10
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(123));
    auto result = stdLib->Call(*engine, "itoa", args);
    ASSERT_TRUE(result->IsString());
    ASSERT_EQ(result->GetString(), "123");

    // Test itoa with base 16
    args.clear();
    args.push_back(StackItem::Create(255));
    args.push_back(StackItem::Create(16));
    result = stdLib->Call(*engine, "itoa", args);
    ASSERT_TRUE(result->IsString());
    ASSERT_EQ(result->GetString(), "ff");

    // Test atoi with base 10
    args.clear();
    args.push_back(StackItem::Create("123"));
    result = stdLib->Call(*engine, "atoi", args);
    ASSERT_TRUE(result->IsInteger());
    ASSERT_EQ(result->GetInteger(), 123);

    // Test atoi with base 16
    args.clear();
    args.push_back(StackItem::Create("ff"));
    args.push_back(StackItem::Create(16));
    result = stdLib->Call(*engine, "atoi", args);
    ASSERT_TRUE(result->IsInteger());
    ASSERT_EQ(result->GetInteger(), 255);
}

TEST_F(StdLibTest, TestBase64EncodeDecode)
{
    // Create a byte array
    ByteVector data = ByteVector::Parse("010203");

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(data));

    // Call the base64Encode method
    auto result = stdLib->Call(*engine, "base64Encode", args);

    // Check the result
    ASSERT_TRUE(result->IsString());
    auto encoded = result->GetString();
    ASSERT_EQ(encoded, "AQID");

    // Call the base64Decode method
    args.clear();
    args.push_back(StackItem::Create(encoded));
    auto decoded = stdLib->Call(*engine, "base64Decode", args);

    // Check the result
    ASSERT_TRUE(decoded->IsBuffer());
    ASSERT_EQ(decoded->GetByteArray(), data);
}

TEST_F(StdLibTest, TestBase64UrlEncodeDecode)
{
    // Create a byte array
    ByteVector data = ByteVector::Parse("010203");

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(data));

    // Call the base64UrlEncode method
    auto result = stdLib->Call(*engine, "base64UrlEncode", args);

    // Check the result
    ASSERT_TRUE(result->IsString());
    auto encoded = result->GetString();
    ASSERT_EQ(encoded, "AQID");

    // Call the base64UrlDecode method
    args.clear();
    args.push_back(StackItem::Create(encoded));
    auto decoded = stdLib->Call(*engine, "base64UrlDecode", args);

    // Check the result
    ASSERT_TRUE(decoded->IsString());
    ASSERT_EQ(decoded->GetString(), std::string(reinterpret_cast<const char*>(data.Data()), data.Size()));
}

TEST_F(StdLibTest, TestBase58EncodeDecode)
{
    // Create a byte array
    ByteVector data = ByteVector::Parse("010203");

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(data));

    // Call the base58Encode method
    auto result = stdLib->Call(*engine, "base58Encode", args);

    // Check the result
    ASSERT_TRUE(result->IsString());
    auto encoded = result->GetString();

    // Call the base58Decode method
    args.clear();
    args.push_back(StackItem::Create(encoded));
    auto decoded = stdLib->Call(*engine, "base58Decode", args);

    // Check the result
    ASSERT_TRUE(decoded->IsBuffer());
    ASSERT_EQ(decoded->GetByteArray(), data);
}

TEST_F(StdLibTest, TestBase58CheckEncodeDecode)
{
    // Create a byte array
    ByteVector data = ByteVector::Parse("010203");

    // Create arguments
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create(data));

    // Call the base58CheckEncode method
    auto result = stdLib->Call(*engine, "base58CheckEncode", args);

    // Check the result
    ASSERT_TRUE(result->IsString());
    auto encoded = result->GetString();

    // Call the base58CheckDecode method
    args.clear();
    args.push_back(StackItem::Create(encoded));
    auto decoded = stdLib->Call(*engine, "base58CheckDecode", args);

    // Check the result
    ASSERT_TRUE(decoded->IsBuffer());
    ASSERT_EQ(decoded->GetByteArray(), data);
}

TEST_F(StdLibTest, TestStrLen)
{
    // Test with ASCII string
    std::vector<std::shared_ptr<StackItem>> args;
    args.push_back(StackItem::Create("abc"));
    auto result = stdLib->Call(*engine, "strLen", args);
    ASSERT_TRUE(result->IsInteger());
    ASSERT_EQ(result->GetInteger(), 3);

    // Test with UTF-8 string
    args.clear();
    args.push_back(StackItem::Create("🦆"));
    result = stdLib->Call(*engine, "strLen", args);
    ASSERT_TRUE(result->IsInteger());
    ASSERT_EQ(result->GetInteger(), 1);

    // Test with combining characters
    args.clear();
    args.push_back(StackItem::Create("ã"));
    result = stdLib->Call(*engine, "strLen", args);
    ASSERT_TRUE(result->IsInteger());
    ASSERT_EQ(result->GetInteger(), 1);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
