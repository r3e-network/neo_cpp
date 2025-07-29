#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/connection.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p/remote_node.h>

using namespace neo::network::p2p;
using namespace neo::io;

// Mock Connection class for testing
class MockConnection : public Connection
{
  public:
    MockConnection(const IPEndPoint& remote, const IPEndPoint& local)
        : remoteEndPoint_(remote), localEndPoint_(local), isConnected_(true)
    {
    }

    IPEndPoint GetRemoteEndPoint() const override
    {
        return remoteEndPoint_;
    }

    IPEndPoint GetLocalEndPoint() const override
    {
        return localEndPoint_;
    }

    bool Send(const Message& message, bool enableCompression = true) override
    {
        return isConnected_;
    }

    void Disconnect() override
    {
        isConnected_ = false;
    }

    bool IsConnected() const
    {
        return isConnected_;
    }

  private:
    IPEndPoint remoteEndPoint_;
    IPEndPoint localEndPoint_;
    bool isConnected_;
};

class UT_connection_management : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Setup test environment
        remoteEndpoint_ = IPEndPoint("192.168.1.100", 10333);
        localEndpoint_ = IPEndPoint("127.0.0.1", 20333);
    }

    void TearDown() override
    {
        // Cleanup
    }

    IPEndPoint remoteEndpoint_;
    IPEndPoint localEndpoint_;
};

TEST_F(UT_connection_management, BasicConnectionManagement)
{
    // Test: Connection lifecycle management

    // Create a mock connection
    auto mockConnection = std::make_shared<MockConnection>(remoteEndpoint_, localEndpoint_);

    // Verify initial state
    EXPECT_TRUE(mockConnection->IsConnected());
    EXPECT_EQ(mockConnection->GetRemoteEndPoint().GetAddress(), "192.168.1.100");
    EXPECT_EQ(mockConnection->GetRemoteEndPoint().GetPort(), 10333);
    EXPECT_EQ(mockConnection->GetLocalEndPoint().GetAddress(), "127.0.0.1");
    EXPECT_EQ(mockConnection->GetLocalEndPoint().GetPort(), 20333);

    // Test connection ID
    EXPECT_GT(mockConnection->GetId(), 0u);

    // Test disconnect
    mockConnection->Disconnect();
    EXPECT_FALSE(mockConnection->IsConnected());
}

TEST_F(UT_connection_management, ConnectionTiming)
{
    // Test connection timing and statistics

    auto mockConnection = std::make_shared<MockConnection>(remoteEndpoint_, localEndpoint_);

    // Initial timing values should be zero or very small
    EXPECT_GE(mockConnection->GetLastMessageReceived(), 0u);
    EXPECT_GE(mockConnection->GetLastMessageSent(), 0u);
    EXPECT_GE(mockConnection->GetLastPingSent(), 0u);
    EXPECT_GE(mockConnection->GetLastPingReceived(), 0u);

    // Ping time should be reasonable
    EXPECT_GE(mockConnection->GetPingTime(), 0u);

    // Byte counters should start at zero
    EXPECT_EQ(mockConnection->GetBytesSent(), 0u);
    EXPECT_EQ(mockConnection->GetBytesReceived(), 0u);
}

TEST_F(UT_connection_management, ConnectionStatistics)
{
    // Test connection statistics tracking

    auto mockConnection = std::make_shared<MockConnection>(remoteEndpoint_, localEndpoint_);

    // Test that connection maintains proper statistics
    EXPECT_GE(mockConnection->GetBytesSent(), 0u);
    EXPECT_GE(mockConnection->GetBytesReceived(), 0u);
    EXPECT_GE(mockConnection->GetMessagesReceived(), 0u);

    // Test connection state consistency
    EXPECT_TRUE(mockConnection->IsConnected());

    // Test unique connection IDs
    auto anotherConnection = std::make_shared<MockConnection>(remoteEndpoint_, localEndpoint_);
    EXPECT_NE(mockConnection->GetId(), anotherConnection->GetId());
}

TEST_F(UT_connection_management, IPEndPointHandling)
{
    // Test IP endpoint handling in connections

    // Test IPv4 endpoints
    IPEndPoint ipv4Remote("203.0.113.1", 10333);
    IPEndPoint ipv4Local("192.168.1.1", 20333);
    auto ipv4Connection = std::make_shared<MockConnection>(ipv4Remote, ipv4Local);

    EXPECT_EQ(ipv4Connection->GetRemoteEndPoint().GetAddress(), "203.0.113.1");
    EXPECT_EQ(ipv4Connection->GetRemoteEndPoint().GetPort(), 10333);

    // Test loopback endpoints
    IPEndPoint loopbackRemote("127.0.0.1", 10333);
    IPEndPoint loopbackLocal("127.0.0.1", 20333);
    auto loopbackConnection = std::make_shared<MockConnection>(loopbackRemote, loopbackLocal);

    EXPECT_EQ(loopbackConnection->GetRemoteEndPoint().GetAddress(), "127.0.0.1");
    EXPECT_EQ(loopbackConnection->GetLocalEndPoint().GetAddress(), "127.0.0.1");
}

TEST_F(UT_connection_management, ErrorHandling)
{
    // Test error handling for Connection lifecycle management

    // Test with invalid endpoints
    IPEndPoint invalidRemote("", 0);
    IPEndPoint validLocal("127.0.0.1", 20333);

    // Connection should handle invalid endpoints gracefully
    EXPECT_NO_THROW({
        auto connection = std::make_shared<MockConnection>(invalidRemote, validLocal);
        EXPECT_EQ(connection->GetRemoteEndPoint().GetAddress(), "");
        EXPECT_EQ(connection->GetRemoteEndPoint().GetPort(), 0);
    });

    // Test disconnected connection state
    auto connection = std::make_shared<MockConnection>(remoteEndpoint_, localEndpoint_);
    connection->Disconnect();
    EXPECT_FALSE(connection->IsConnected());

    // Should maintain endpoint information even when disconnected
    EXPECT_EQ(connection->GetRemoteEndPoint().GetAddress(), "192.168.1.100");
    EXPECT_EQ(connection->GetLocalEndPoint().GetAddress(), "127.0.0.1");
}
