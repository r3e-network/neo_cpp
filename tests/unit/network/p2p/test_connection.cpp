#include <gtest/gtest.h>
#include <neo/network/p2p/connection.h>
#include <neo/network/p2p/tcp_connection.h>
#include <neo/network/p2p/remote_node.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/ip_endpoint.h>
#include <asio.hpp>
#include <memory>
#include <thread>
#include <chrono>

using namespace neo::network::p2p;
using namespace neo::network;
using namespace neo::io;

class MockConnection : public Connection
{
public:
    MockConnection() = default;
    
    IPEndPoint GetRemoteEndPoint() const override
    {
        return IPEndPoint(IPAddress::Parse("127.0.0.1"), 10333);
    }
    
    IPEndPoint GetLocalEndPoint() const override
    {
        return IPEndPoint(IPAddress::Parse("127.0.0.1"), 0);
    }
    
    bool Send(const Message& message, bool enableCompression = true) override
    {
        lastSentMessage_ = message;
        return true;
    }
    
    void Disconnect() override
    {
        OnDisconnected();
    }
    
    const Message& GetLastSentMessage() const
    {
        return lastSentMessage_;
    }
    
    void SimulateMessageReceived(const Message& message)
    {
        OnMessageReceived(message);
    }
    
private:
    Message lastSentMessage_;
};

TEST(ConnectionTest, Constructor)
{
    MockConnection connection;
    
    EXPECT_EQ(connection.GetLastMessageReceived(), connection.GetLastMessageSent());
    EXPECT_EQ(connection.GetLastPingSent(), 0);
    EXPECT_EQ(connection.GetLastPingReceived(), 0);
    EXPECT_EQ(connection.GetPingTime(), 0);
    EXPECT_EQ(connection.GetBytesSent(), 0);
    EXPECT_EQ(connection.GetBytesReceived(), 0);
    EXPECT_EQ(connection.GetMessagesSent(), 0);
    EXPECT_EQ(connection.GetMessagesReceived(), 0);
}

TEST(ConnectionTest, MessageReceivedCallback)
{
    MockConnection connection;
    
    bool callbackCalled = false;
    Message receivedMessage;
    
    connection.SetMessageReceivedCallback([&](const Message& message) {
        callbackCalled = true;
        receivedMessage = message;
    });
    
    Message message(MessageCommand::Ping);
    connection.SimulateMessageReceived(message);
    
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(receivedMessage.GetCommand(), MessageCommand::Ping);
    EXPECT_EQ(connection.GetMessagesReceived(), 1);
}

TEST(ConnectionTest, DisconnectedCallback)
{
    MockConnection connection;
    
    bool callbackCalled = false;
    
    connection.SetDisconnectedCallback([&]() {
        callbackCalled = true;
    });
    
    connection.Disconnect();
    
    EXPECT_TRUE(callbackCalled);
}

TEST(RemoteNodeTest, Constructor)
{
    auto connection = std::make_shared<MockConnection>();
    RemoteNode remoteNode(&LocalNode::GetInstance(), connection);
    
    EXPECT_EQ(remoteNode.GetConnection(), connection);
    EXPECT_EQ(remoteNode.GetRemoteEndPoint(), connection->GetRemoteEndPoint());
    EXPECT_EQ(remoteNode.GetLocalEndPoint(), connection->GetLocalEndPoint());
    EXPECT_TRUE(remoteNode.IsConnected());
    EXPECT_FALSE(remoteNode.IsHandshaked());
}

TEST(RemoteNodeTest, SendVersion)
{
    auto connection = std::make_shared<MockConnection>();
    RemoteNode remoteNode(&LocalNode::GetInstance(), connection);
    
    remoteNode.SendVersion();
    
    EXPECT_EQ(connection->GetLastSentMessage().GetCommand(), MessageCommand::Version);
}

TEST(RemoteNodeTest, SendVerack)
{
    auto connection = std::make_shared<MockConnection>();
    RemoteNode remoteNode(&LocalNode::GetInstance(), connection);
    
    remoteNode.SendVerack();
    
    EXPECT_EQ(connection->GetLastSentMessage().GetCommand(), MessageCommand::Verack);
}

TEST(RemoteNodeTest, SendPing)
{
    auto connection = std::make_shared<MockConnection>();
    RemoteNode remoteNode(&LocalNode::GetInstance(), connection);
    
    remoteNode.SendPing();
    
    EXPECT_EQ(connection->GetLastSentMessage().GetCommand(), MessageCommand::Ping);
}

TEST(RemoteNodeTest, ProcessVersionMessage)
{
    auto connection = std::make_shared<MockConnection>();
    RemoteNode remoteNode(&LocalNode::GetInstance(), connection);
    
    // Create a version payload
    auto payload = std::make_shared<payloads::VersionPayload>();
    payload->SetVersion(0);
    payload->SetServices(1);
    payload->SetUserAgent("Test Node");
    
    // Create a version message
    Message message(MessageCommand::Version, payload);
    
    // Process the message
    connection->SimulateMessageReceived(message);
    
    // Check that the remote node sent a verack message
    EXPECT_EQ(connection->GetLastSentMessage().GetCommand(), MessageCommand::Verack);
    
    // Check that the remote node stored the version information
    EXPECT_EQ(remoteNode.GetVersion(), 0);
    EXPECT_EQ(remoteNode.GetServices(), 1);
    EXPECT_EQ(remoteNode.GetUserAgent(), "Test Node");
}

TEST(RemoteNodeTest, ProcessVerackMessage)
{
    auto connection = std::make_shared<MockConnection>();
    RemoteNode remoteNode(&LocalNode::GetInstance(), connection);
    
    // Create a verack message
    Message message(MessageCommand::Verack);
    
    // Process the message
    connection->SimulateMessageReceived(message);
    
    // Check that the remote node is handshaked
    EXPECT_TRUE(remoteNode.IsHandshaked());
}

TEST(RemoteNodeTest, ProcessPingMessage)
{
    auto connection = std::make_shared<MockConnection>();
    RemoteNode remoteNode(&LocalNode::GetInstance(), connection);
    
    // Create a ping payload
    auto payload = std::make_shared<payloads::PingPayload>();
    payload->SetLastBlockIndex(12345);
    
    // Create a ping message
    Message message(MessageCommand::Ping, payload);
    
    // Process the message
    connection->SimulateMessageReceived(message);
    
    // Check that the remote node sent a pong message
    EXPECT_EQ(connection->GetLastSentMessage().GetCommand(), MessageCommand::Pong);
    
    // Check that the remote node stored the last block index
    EXPECT_EQ(remoteNode.GetLastBlockIndex(), 12345);
}
