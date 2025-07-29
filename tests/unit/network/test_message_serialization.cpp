#include <gtest/gtest.h>
#include <memory>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/node_capability.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <sstream>

using namespace neo::network::p2p;
using namespace neo::io;

class UT_message_serialization : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Setup test environment
    }

    void TearDown() override
    {
        // Cleanup
    }
};

TEST_F(UT_message_serialization, BasicMessageSerialization)
{
    // Test: Message serialization/deserialization

    // Create a simple message without payload (Verack)
    auto message = Message::Create(MessageCommand::Verack);

    // Serialize to byte array
    auto serialized = message.ToArray(false);  // Disable compression for testing
    EXPECT_FALSE(serialized.empty());

    // Deserialize back
    Message deserializedMessage;
    uint32_t bytesRead = Message::TryDeserialize(serialized.AsSpan(), deserializedMessage);
    EXPECT_GT(bytesRead, 0u);
    EXPECT_EQ(deserializedMessage.GetCommand(), MessageCommand::Verack);
    EXPECT_EQ(deserializedMessage.GetPayload(), nullptr);
}

TEST_F(UT_message_serialization, MessageWithPayloadSerialization)
{
    // Test: Message with payload serialization/deserialization

    // Create a version payload
    auto versionPayload = std::make_shared<payloads::VersionPayload>();
    versionPayload->SetNetwork(0x334F454E);  // Neo MainNet magic
    versionPayload->SetVersion(0);
    versionPayload->SetNonce(12345);
    versionPayload->SetUserAgent("NEO:3.6.0");

    // Create capabilities
    std::vector<NodeCapability> capabilities;
    FullNodeCapability fullNode(1000000);
    capabilities.push_back(fullNode);
    versionPayload->SetCapabilities(capabilities);

    // Create message with payload
    auto message = Message::Create(MessageCommand::Version, versionPayload);

    // Serialize to byte array
    auto serialized = message.ToArray(false);
    EXPECT_FALSE(serialized.empty());

    // Deserialize back
    Message deserializedMessage;
    uint32_t bytesRead = Message::TryDeserialize(serialized.AsSpan(), deserializedMessage);
    EXPECT_GT(bytesRead, 0u);
    EXPECT_EQ(deserializedMessage.GetCommand(), MessageCommand::Version);

    // Verify payload
    auto deserializedPayload = std::dynamic_pointer_cast<payloads::VersionPayload>(deserializedMessage.GetPayload());
    ASSERT_NE(deserializedPayload, nullptr);
    EXPECT_EQ(deserializedPayload->GetNetwork(), 0x334F454E);
    EXPECT_EQ(deserializedPayload->GetNonce(), 12345u);
    EXPECT_EQ(deserializedPayload->GetUserAgent(), "NEO:3.6.0");
}

TEST_F(UT_message_serialization, CompressionFeature)
{
    // Test: Message compression functionality

    // Create a ping payload with data
    auto pingPayload = std::make_shared<payloads::PingPayload>();
    pingPayload->SetNonce(987654321);
    pingPayload->SetTimestamp(1234567890);
    pingPayload->SetLastBlockIndex(500000);

    auto message = Message::Create(MessageCommand::Ping, pingPayload);

    // Test with compression enabled
    auto compressedData = message.ToArray(true);
    EXPECT_FALSE(compressedData.empty());

    // Test with compression disabled
    auto uncompressedData = message.ToArray(false);
    EXPECT_FALSE(uncompressedData.empty());

    // Both should deserialize correctly
    Message compressedMsg, uncompressedMsg;
    EXPECT_GT(Message::TryDeserialize(compressedData.AsSpan(), compressedMsg), 0u);
    EXPECT_GT(Message::TryDeserialize(uncompressedData.AsSpan(), uncompressedMsg), 0u);

    EXPECT_EQ(compressedMsg.GetCommand(), MessageCommand::Ping);
    EXPECT_EQ(uncompressedMsg.GetCommand(), MessageCommand::Ping);
}

TEST_F(UT_message_serialization, MultipleMessageCommands)
{
    // Test: Various message commands serialization

    std::vector<MessageCommand> commands = {MessageCommand::Version, MessageCommand::Verack, MessageCommand::GetAddr,
                                            MessageCommand::Ping,    MessageCommand::Pong,   MessageCommand::GetHeaders,
                                            MessageCommand::Mempool};

    for (auto cmd : commands)
    {
        auto message = Message::Create(cmd);
        auto serialized = message.ToArray();
        EXPECT_FALSE(serialized.empty()) << "Failed for command: " << static_cast<int>(cmd);

        Message deserialized;
        uint32_t bytesRead = Message::TryDeserialize(serialized.AsSpan(), deserialized);
        EXPECT_GT(bytesRead, 0u) << "Failed to deserialize command: " << static_cast<int>(cmd);
        EXPECT_EQ(deserialized.GetCommand(), cmd);
    }
}

TEST_F(UT_message_serialization, ErrorHandling)
{
    // Test error handling for Message serialization/deserialization

    // Test with empty data
    ByteVector emptyData;
    Message message;
    uint32_t bytesRead = Message::TryDeserialize(emptyData.AsSpan(), message);
    EXPECT_EQ(bytesRead, 0u);

    // Test with corrupted data
    ByteVector corruptedData = {0xFF, 0xFF, 0xFF, 0xFF};
    bytesRead = Message::TryDeserialize(corruptedData.AsSpan(), message);
    EXPECT_EQ(bytesRead, 0u);

    // Test with partial data
    auto validMessage = Message::Create(MessageCommand::Verack);
    auto validData = validMessage.ToArray();
    if (validData.Size() > 1)
    {
        ByteVector partialData(validData.Data(), validData.Size() - 1);
        bytesRead = Message::TryDeserialize(partialData.AsSpan(), message);
        EXPECT_EQ(bytesRead, 0u);
    }
}

TEST_F(UT_message_serialization, MessageSize)
{
    // Test: Message size calculations

    // Empty message
    auto emptyMessage = Message::Create(MessageCommand::Verack);
    EXPECT_GT(emptyMessage.GetSize(), 0u);

    // Message with payload
    auto payload = std::make_shared<payloads::VersionPayload>();
    payload->SetUserAgent("Test Agent");
    auto messageWithPayload = Message::Create(MessageCommand::Version, payload);
    EXPECT_GT(messageWithPayload.GetSize(), emptyMessage.GetSize());
}
