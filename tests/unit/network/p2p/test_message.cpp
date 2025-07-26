#include <gtest/gtest.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/byte_vector.h>
#include <sstream>
#include <memory>

using namespace neo::network::p2p;
using namespace neo::network::p2p::payloads;
using namespace neo::io;

TEST(P2PMessageTest, Constructor)
{
    // Default constructor
    Message message1;
    EXPECT_EQ(message1.GetCommand(), MessageCommand::Version);
    EXPECT_EQ(message1.GetFlags(), MessageFlags::None);
    EXPECT_EQ(message1.GetPayload(), nullptr);
    
    // Command and payload constructor
    auto pingPayload = std::make_shared<PingPayload>();
    pingPayload->SetLastBlockIndex(123);
    Message message2(MessageCommand::Ping, pingPayload);
    EXPECT_EQ(message2.GetCommand(), MessageCommand::Ping);
    EXPECT_EQ(message2.GetFlags(), MessageFlags::None);
    EXPECT_NE(message2.GetPayload(), nullptr);
}

TEST(P2PMessageTest, Create)
{
    // Create with payload
    auto pingPayload = std::make_shared<PingPayload>();
    pingPayload->SetLastBlockIndex(123);
    Message message1 = Message::Create(MessageCommand::Ping, pingPayload);
    EXPECT_EQ(message1.GetCommand(), MessageCommand::Ping);
    EXPECT_EQ(message1.GetFlags(), MessageFlags::None);
    EXPECT_NE(message1.GetPayload(), nullptr);
    
    // Create without payload
    Message message2 = Message::Create(MessageCommand::GetAddr);
    EXPECT_EQ(message2.GetCommand(), MessageCommand::GetAddr);
    EXPECT_EQ(message2.GetFlags(), MessageFlags::None);
    EXPECT_EQ(message2.GetPayload(), nullptr);
}

TEST(P2PMessageTest, Serialize_Deserialize)
{
    // Create a message with a ping payload
    auto pingPayload = std::make_shared<PingPayload>();
    pingPayload->SetLastBlockIndex(123);
    pingPayload->SetNonce(456);
    pingPayload->SetTimestamp(789);
    Message message = Message::Create(MessageCommand::Ping, pingPayload);
    
    // Serialize the message
    std::ostringstream stream;
    BinaryWriter writer(stream);
    message.Serialize(writer);
    std::string data = stream.str();
    
    // Deserialize the message
    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    Message deserializedMessage;
    deserializedMessage.Deserialize(reader);
    
    // Check the deserialized message
    EXPECT_EQ(deserializedMessage.GetCommand(), MessageCommand::Ping);
    EXPECT_EQ(deserializedMessage.GetFlags(), MessageFlags::None);
    
    // NOTE: Payload deserialization is not yet implemented - this is a future enhancement
}

TEST(P2PMessageTest, ToArray)
{
    // Create a message with a ping payload
    auto pingPayload = std::make_shared<PingPayload>();
    pingPayload->SetLastBlockIndex(123);
    pingPayload->SetNonce(456);
    pingPayload->SetTimestamp(789);
    Message message = Message::Create(MessageCommand::Ping, pingPayload);
    
    // Convert the message to a byte array
    ByteVector bytes = message.ToArray();
    
    // Deserialize the message from the byte array
    Message deserializedMessage;
    uint32_t bytesRead = Message::TryDeserialize(bytes.AsSpan(), deserializedMessage);
    
    // Check the deserialized message
    EXPECT_EQ(bytesRead, bytes.Size());
    EXPECT_EQ(deserializedMessage.GetCommand(), MessageCommand::Ping);
    EXPECT_EQ(deserializedMessage.GetFlags(), MessageFlags::None);
    
    // NOTE: Payload deserialization is not yet implemented - this is a future enhancement
}

TEST(P2PMessageTest, SerializeJson_DeserializeJson)
{
    // Create a message with a ping payload
    auto pingPayload = std::make_shared<PingPayload>();
    pingPayload->SetLastBlockIndex(123);
    pingPayload->SetNonce(456);
    pingPayload->SetTimestamp(789);
    Message message = Message::Create(MessageCommand::Ping, pingPayload);
    
    // Serialize the message to JSON
    nlohmann::json json = message.ToJson();
    
    // Check the JSON
    EXPECT_EQ(json["command"], static_cast<uint8_t>(MessageCommand::Ping));
    EXPECT_EQ(json["flags"], static_cast<uint8_t>(MessageFlags::None));
    EXPECT_TRUE(json.contains("payload"));
    
    // Deserialize the message from JSON
    Message deserializedMessage;
    deserializedMessage.DeserializeFromJson(json);
    
    // Check the deserialized message
    EXPECT_EQ(deserializedMessage.GetCommand(), MessageCommand::Ping);
    EXPECT_EQ(deserializedMessage.GetFlags(), MessageFlags::None);
    
    // NOTE: Payload deserialization is not yet implemented - this is a future enhancement
}
