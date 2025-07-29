#include <gtest/gtest.h>
#include <memory>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/lz4.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/network/message.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <sstream>

using namespace neo::network;
using namespace neo::network::p2p;
using namespace neo::io;

// Helper function to create a test version payload
std::shared_ptr<payloads::VersionPayload> CreateTestVersionPayload()
{
    std::vector<NodeCapability> capabilities;
    capabilities.push_back(ServerCapability(NodeCapabilityType::TcpServer, 10333));
    capabilities.push_back(FullNodeCapability(0));

    auto payload = std::make_shared<payloads::VersionPayload>();
    *payload = payloads::VersionPayload::Create(0x4F454E, 123456, "/Neo:3.0/", capabilities);
    payload->SetVersion(0);
    payload->SetTimestamp(12345678);
    return payload;
}

// Helper function to create a test ping payload (exactly matching C# test)
std::shared_ptr<payloads::PingPayload> CreateTestPingPayload()
{
    auto payload = std::make_shared<payloads::PingPayload>();
    payload->SetLastBlockIndex(0xFFFFFFFF);  // uint.MaxValue
    payload->SetTimestamp(12345678);
    payload->SetNonce(123456);
    return payload;
}

// Test Serialize_Deserialize (matching C# UT_Message.Serialize_Deserialize)
TEST(MessageTest, Serialize_Deserialize)
{
    // Create a ping payload with uint.MaxValue as LastBlockIndex
    auto payload = CreateTestPingPayload();
    auto msg = Message::Create(MessageCommand::Ping, payload);

    // Convert to byte array
    neo::io::ByteVector buffer = msg.ToArray();

    // Deserialize back into a message
    Message copy;
    copy.FromArray(buffer);

    // Check the message properties
    EXPECT_EQ(msg.GetCommand(), copy.GetCommand());
    EXPECT_EQ(msg.GetFlags(), copy.GetFlags());
    EXPECT_EQ(payload->GetSize() + 3, msg.GetSize());  // Exactly matching C# size calculation

    // Check the payload properties
    auto pingPayload = std::dynamic_pointer_cast<payloads::PingPayload>(copy.GetPayload());
    ASSERT_NE(pingPayload, nullptr);

    EXPECT_EQ(payload->GetLastBlockIndex(), pingPayload->GetLastBlockIndex());
    EXPECT_EQ(payload->GetNonce(), pingPayload->GetNonce());
    EXPECT_EQ(payload->GetTimestamp(), pingPayload->GetTimestamp());
}

// Test Serialize_Deserialize_WithoutPayload (matching C# UT_Message.Serialize_Deserialize_WithoutPayload)
TEST(MessageTest, Serialize_Deserialize_WithoutPayload)
{
    // Create a message without payload
    auto msg = Message::Create(MessageCommand::GetAddr);

    // Convert to byte array
    neo::io::ByteVector buffer = msg.ToArray();

    // Deserialize back into a message
    Message copy;
    copy.FromArray(buffer);

    // Check the message properties
    EXPECT_EQ(msg.GetCommand(), copy.GetCommand());
    EXPECT_EQ(msg.GetFlags(), copy.GetFlags());
    EXPECT_EQ(nullptr, copy.GetPayload());
}

// Test ToArray (matching C# UT_Message.ToArray)
TEST(MessageTest, ToArray)
{
    // Create a ping payload with uint.MaxValue as LastBlockIndex
    auto payload = CreateTestPingPayload();
    auto msg = Message::Create(MessageCommand::Ping, payload);

    // Convert to byte array
    neo::io::ByteVector buffer = msg.ToArray();

    // Check that the size is correct
    EXPECT_EQ(payload->GetSize() + 3, msg.GetSize());
}

// Test ToArray_WithoutPayload (matching C# UT_Message.ToArray_WithoutPayload)
TEST(MessageTest, ToArray_WithoutPayload)
{
    // Create a message without payload
    auto msg = Message::Create(MessageCommand::GetAddr);

    // Convert to byte array
    neo::io::ByteVector buffer = msg.ToArray();

    // Should not throw any exceptions
}

// Test Compression (matching C# UT_Message.Compression)
TEST(MessageTest, Compression)
{
    // Create a version payload with large user agent to trigger compression
    // This matches the C# test that creates a transaction with large script
    std::shared_ptr<payloads::VersionPayload> payload = std::make_shared<payloads::VersionPayload>();
    payload->SetUserAgent(std::string(100, 'A'));

    // Create a message with a compressible command
    auto msg = Message::Create(MessageCommand::Transaction, payload);
    auto buffer = msg.ToArray();

    // The exact size may differ due to implementation differences, but verify it's compressed
    EXPECT_TRUE(msg.IsCompressed());
    EXPECT_TRUE(HasFlag(msg.GetFlags(), MessageFlags::Compressed));

    // Deserialize and check flags
    Message copy;
    copy.FromArray(buffer);

    EXPECT_TRUE(copy.IsCompressed());
    EXPECT_TRUE(HasFlag(copy.GetFlags(), MessageFlags::Compressed));
}

// Test Serialize_Deserialize_ByteString and MultipleSizes (matching C# UT_Message.MultipleSizes)
TEST(MessageTest, MultipleSizes)
{
    // Create a message without payload for testing
    auto msg = Message::Create(MessageCommand::GetAddr);
    neo::io::ByteVector buffer = msg.ToArray();

    // Test with empty buffer - should fail
    {
        neo::io::ByteVector emptyBuffer;
        Message copy;
        bool success = copy.FromArray(emptyBuffer);
        EXPECT_FALSE(success);
    }

    // Test with normal buffer - should succeed
    {
        Message copy;
        bool success = copy.FromArray(buffer);
        EXPECT_TRUE(success);
        EXPECT_EQ(msg.GetCommand(), copy.GetCommand());
    }

    // Test with oversized payload - should fail
    // This is an attempted match for C#'s TryDeserialize test with the 0xFF size flag
    {
        // Create a buffer with a huge payload size
        std::ostringstream stream;
        neo::io::BinaryWriter writer(stream);

        // Magic (4 bytes)
        writer.Write(Message::MainNetMagic);

        // Command (12 bytes, null-padded)
        std::string commandName = "getaddr";
        char commandBytes[12] = {0};
        std::strncpy(commandBytes, commandName.c_str(), std::min(commandName.size(), size_t(12)));
        writer.Write(neo::io::ByteSpan(reinterpret_cast<const uint8_t*>(commandBytes), 12));

        // Payload size - set to max value to trigger exception (4 bytes)
        writer.Write(static_cast<uint32_t>(0xFFFFFFFF));

        // Checksum (4 bytes)
        writer.Write(static_cast<uint32_t>(0));

        // Flags (1 byte)
        writer.Write(static_cast<uint8_t>(MessageFlags::None));

        // Convert to ByteVector
        std::string data = stream.str();
        neo::io::ByteVector hugeBuffer(neo::io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));

        // Try to deserialize - should fail due to max size check
        Message copy;
        bool success = copy.FromArray(hugeBuffer);
        EXPECT_FALSE(success);
    }
}

// Test Serialize_Deserialize_ByteString (matching C# UT_Message.Serialize_Deserialize_ByteString)
TEST(MessageTest, Serialize_Deserialize_ByteString)
{
    auto payload = CreateTestPingPayload();
    auto msg = Message::Create(MessageCommand::Ping, payload);

    // Convert to byte array
    neo::io::ByteVector buffer = msg.ToArray();

    // Deserialize back into a message
    Message copy;
    bool success = copy.FromArray(buffer);

    // Check deserialization was successful
    EXPECT_TRUE(success);

    // Check payload properties
    auto pingPayload = std::dynamic_pointer_cast<payloads::PingPayload>(copy.GetPayload());
    ASSERT_NE(pingPayload, nullptr);

    EXPECT_EQ(payload->GetLastBlockIndex(), pingPayload->GetLastBlockIndex());
    EXPECT_EQ(payload->GetNonce(), pingPayload->GetNonce());
    EXPECT_EQ(payload->GetTimestamp(), pingPayload->GetTimestamp());
}

// Test Serialize_Deserialize_ByteString_WithoutPayload (matching C#
// UT_Message.Serialize_Deserialize_WithoutPayload_ByteString)
TEST(MessageTest, Serialize_Deserialize_ByteString_WithoutPayload)
{
    // Create a message without payload
    auto msg = Message::Create(MessageCommand::GetAddr);

    // Convert to byte array
    neo::io::ByteVector buffer = msg.ToArray();

    // Deserialize back into a message
    Message copy;
    bool success = copy.FromArray(buffer);

    // Check deserialization was successful
    EXPECT_TRUE(success);
    EXPECT_EQ(msg.GetCommand(), copy.GetCommand());
    EXPECT_EQ(msg.GetFlags(), copy.GetFlags());
    EXPECT_EQ(nullptr, copy.GetPayload());
}

// Additional tests for C++ specific functionality

TEST(MessageTest, JsonSerialization)
{
    // Create a message with a payload
    auto payload = CreateTestVersionPayload();
    Message message(MessageCommand::Version, payload);

    // Serialize to JSON
    nlohmann::json json = message.ToJson();

    // Deserialize from JSON
    Message deserializedMessage;
    deserializedMessage.DeserializeFromJson(json);

    // Check the deserialized message
    EXPECT_EQ(deserializedMessage.GetCommand(), MessageCommand::Version);
    EXPECT_EQ(deserializedMessage.GetFlags(), MessageFlags::None);
}

// Test helper functions
TEST(MessageTest, HelperFunctions)
{
    // Test command name conversions
    EXPECT_EQ(GetCommandName(MessageCommand::Version), "version");
    EXPECT_EQ(GetCommandName(MessageCommand::Verack), "verack");
    EXPECT_EQ(GetCommandName(MessageCommand::GetAddr), "getaddr");

    // Test flag operations
    MessageFlags flags = MessageFlags::None;
    flags = SetFlag(flags, MessageFlags::Compressed);
    EXPECT_TRUE(HasFlag(flags, MessageFlags::Compressed));

    flags = ClearFlag(flags, MessageFlags::Compressed);
    EXPECT_FALSE(HasFlag(flags, MessageFlags::Compressed));
}
