/**
 * @file test_message_handler.cpp
 * @brief Network message handler test suite
 */

#include <gtest/gtest.h>
#include <neo/network/message.h>
#include <neo/network/p2p/message_handler.h>
#include <neo/io/memory_stream.h>

namespace neo::network::tests {

class MessageHandlerTest : public ::testing::Test {
protected:
    std::unique_ptr<MessageHandler> handler;
    
    void SetUp() override {
        handler = std::make_unique<MessageHandler>();
    }
    
    void TearDown() override {
        handler.reset();
    }
};

TEST_F(MessageHandlerTest, CreateVersionMessage) {
    VersionPayload payload;
    payload.Version = 0;
    payload.Services = 1;
    payload.Timestamp = 1234567890;
    payload.Port = 10333;
    payload.Nonce = 0x12345678;
    payload.UserAgent = "/Neo:3.0.0/";
    payload.StartHeight = 0;
    payload.Relay = true;
    
    auto message = Message::Create(MessageType::Version, payload);
    EXPECT_EQ(message.Type, MessageType::Version);
    EXPECT_FALSE(message.Payload.empty());
}

TEST_F(MessageHandlerTest, CreateVerAckMessage) {
    auto message = Message::Create(MessageType::Verack);
    EXPECT_EQ(message.Type, MessageType::Verack);
    EXPECT_TRUE(message.Payload.empty());
}

TEST_F(MessageHandlerTest, CreateGetAddrMessage) {
    auto message = Message::Create(MessageType::GetAddr);
    EXPECT_EQ(message.Type, MessageType::GetAddr);
    EXPECT_TRUE(message.Payload.empty());
}

TEST_F(MessageHandlerTest, CreatePingMessage) {
    PingPayload payload;
    payload.LastBlockIndex = 12345;
    payload.Timestamp = 1234567890;
    payload.Nonce = 0xABCDEF;
    
    auto message = Message::Create(MessageType::Ping, payload);
    EXPECT_EQ(message.Type, MessageType::Ping);
    EXPECT_FALSE(message.Payload.empty());
}

TEST_F(MessageHandlerTest, CreatePongMessage) {
    PingPayload pingPayload;
    pingPayload.LastBlockIndex = 12345;
    pingPayload.Nonce = 0xABCDEF;
    
    auto message = Message::Create(MessageType::Pong, pingPayload);
    EXPECT_EQ(message.Type, MessageType::Pong);
    EXPECT_FALSE(message.Payload.empty());
}

TEST_F(MessageHandlerTest, HandleVersionMessage) {
    VersionPayload payload;
    payload.Version = 0;
    payload.Services = 1;
    payload.UserAgent = "/Neo:3.0.0/";
    
    auto result = handler->HandleVersion(payload);
    EXPECT_TRUE(result.ShouldSendVerack);
    EXPECT_FALSE(result.ShouldDisconnect);
}

TEST_F(MessageHandlerTest, HandleInvalidVersion) {
    VersionPayload payload;
    payload.Version = 999; // Invalid version
    
    auto result = handler->HandleVersion(payload);
    EXPECT_FALSE(result.ShouldSendVerack);
    EXPECT_TRUE(result.ShouldDisconnect);
}

TEST_F(MessageHandlerTest, HandlePingMessage) {
    PingPayload payload;
    payload.LastBlockIndex = 12345;
    payload.Nonce = 0xABCDEF;
    
    auto response = handler->HandlePing(payload);
    EXPECT_TRUE(response.has_value());
    EXPECT_EQ(response->Nonce, payload.Nonce);
}

TEST_F(MessageHandlerTest, MessageSerialization) {
    Message original;
    original.Type = MessageType::Ping;
    original.Payload = {0x01, 0x02, 0x03, 0x04};
    
    // Serialize
    io::MemoryStream stream;
    original.Serialize(stream);
    
    // Deserialize
    stream.Seek(0, io::SeekOrigin::Begin);
    Message deserialized;
    deserialized.Deserialize(stream);
    
    EXPECT_EQ(original.Type, deserialized.Type);
    EXPECT_EQ(original.Payload, deserialized.Payload);
}

TEST_F(MessageHandlerTest, HandleGetBlocks) {
    GetBlocksPayload payload;
    payload.HashStart = io::UInt256::Zero();
    payload.Count = 500;
    
    auto response = handler->HandleGetBlocks(payload);
    EXPECT_TRUE(response.empty()); // No blocks in test environment
}

TEST_F(MessageHandlerTest, HandleGetHeaders) {
    GetBlocksPayload payload;
    payload.HashStart = io::UInt256::Zero();
    payload.Count = 2000;
    
    auto response = handler->HandleGetHeaders(payload);
    EXPECT_TRUE(response.empty()); // No headers in test environment
}

} // namespace neo::network::tests