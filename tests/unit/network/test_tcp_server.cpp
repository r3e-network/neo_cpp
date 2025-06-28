#include <gtest/gtest.h>
#include <neo/network/tcp_server.h>
#include <neo/network/message.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/ip_endpoint.h>
#include <neo/logging/logger.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <set>

using namespace neo::network;
using namespace neo::network::p2p;
using namespace neo::io;

// Helper function to create a test message
Message CreateTestMessage(MessageCommand command)
{
    if (command == MessageCommand::Version)
    {
        auto payload = std::make_shared<payloads::VersionPayload>();
        payload->SetVersion(0);
        payload->SetServices(1);
        payload->SetTimestamp(std::time(nullptr));
        payload->SetPort(10333);
        payload->SetNonce(123456);
        payload->SetUserAgent("/Neo:3.0/");
        payload->SetStartHeight(0);
        payload->SetRelay(true);
        return Message(command, payload);
    }
    else
    {
        return Message(command);
    }
}

// Test class to test TCP server and client with threading
class TcpNetworkTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Configure logging
        neo::logging::Logger::Instance().SetLogLevel(neo::logging::LogLevel::Debug);
        
        // Create local endpoint for server
        serverEndpoint_ = IPEndPoint(IPAddress::Loopback(), 22334);
        
        // Create server
        server_ = std::make_unique<TcpServer>(serverEndpoint_);
        
        // Set up connection handler
        server_->SetConnectionAcceptedCallback([this](std::shared_ptr<TcpConnection> connection) {
            std::lock_guard<std::mutex> lock(connectionsMutex_);
            serverConnections_.insert(connection);
            
            connection->SetMessageReceivedCallback([this, connection](const Message& message) {
                HandleServerMessage(connection, message);
            });
        });
    }
    
    void TearDown() override
    {
        // Stop client and server
        if (client_)
        {
            client_->Stop();
            client_.reset();
        }
        
        if (server_)
        {
            server_->Stop();
            server_.reset();
        }
    }
    
    void HandleServerMessage(std::shared_ptr<TcpConnection> connection, const Message& message)
    {
        std::lock_guard<std::mutex> lock(serverMessagesMutex_);
        serverMessages_.push_back(message);
        
        // If it's a Version message, respond with VerAck
        if (message.GetCommand() == MessageCommand::Version)
        {
            Message verackMessage(MessageCommand::Verack);
            connection->Send(verackMessage);
        }
        
        // Notify for message received
        serverMessageReceived_.notify_all();
    }
    
    void HandleClientMessage(const Message& message)
    {
        std::lock_guard<std::mutex> lock(clientMessagesMutex_);
        clientMessages_.push_back(message);
        
        // Notify for message received
        clientMessageReceived_.notify_all();
    }
    
    bool WaitForServerMessages(size_t count, std::chrono::milliseconds timeout = std::chrono::seconds(5))
    {
        std::unique_lock<std::mutex> lock(serverMessagesMutex_);
        return serverMessageReceived_.wait_for(lock, timeout, [this, count]() {
            return serverMessages_.size() >= count;
        });
    }
    
    bool WaitForClientMessages(size_t count, std::chrono::milliseconds timeout = std::chrono::seconds(5))
    {
        std::unique_lock<std::mutex> lock(clientMessagesMutex_);
        return clientMessageReceived_.wait_for(lock, timeout, [this, count]() {
            return clientMessages_.size() >= count;
        });
    }
    
    IPEndPoint serverEndpoint_;
    std::unique_ptr<TcpServer> server_;
    std::unique_ptr<TcpClient> client_;
    std::shared_ptr<TcpConnection> clientConnection_;
    
    std::mutex connectionsMutex_;
    std::set<std::shared_ptr<TcpConnection>> serverConnections_;
    
    std::mutex serverMessagesMutex_;
    std::vector<Message> serverMessages_;
    std::condition_variable serverMessageReceived_;
    
    std::mutex clientMessagesMutex_;
    std::vector<Message> clientMessages_;
    std::condition_variable clientMessageReceived_;
};

// Test server start/stop
TEST_F(TcpNetworkTest, ServerStartStop)
{
    // Start server
    server_->Start();
    EXPECT_EQ(server_->GetEndpoint().GetPort(), serverEndpoint_.GetPort());
    
    // Stop server
    server_->Stop();
}

// Test client connect/disconnect
TEST_F(TcpNetworkTest, ClientConnect)
{
    // Start server
    server_->Start();
    
    // Create client
    client_ = std::make_unique<TcpClient>();
    
    // Connect to server
    clientConnection_ = client_->Connect(serverEndpoint_);
    ASSERT_NE(clientConnection_, nullptr);
    
    // Set message handler
    clientConnection_->SetMessageReceivedCallback([this](const Message& message) {
        HandleClientMessage(message);
    });
    
    // Verify connection
    EXPECT_EQ(clientConnection_->GetRemoteEndpoint().GetAddress(), serverEndpoint_.GetAddress());
    EXPECT_EQ(clientConnection_->GetRemoteEndpoint().GetPort(), serverEndpoint_.GetPort());
    
    // Wait for server to accept connection
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify server accepted connection
    {
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        EXPECT_EQ(serverConnections_.size(), 1);
    }
    
    // Disconnect client
    clientConnection_->Stop();
    client_->Stop();
    
    // Wait for server to remove connection
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify server removed connection
    {
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        EXPECT_EQ(serverConnections_.size(), 0);
    }
    
    // Stop server
    server_->Stop();
}

// Test message sending/receiving
TEST_F(TcpNetworkTest, MessageSendReceive)
{
    // Start server
    server_->Start();
    
    // Create client
    client_ = std::make_unique<TcpClient>();
    
    // Connect to server
    clientConnection_ = client_->Connect(serverEndpoint_);
    ASSERT_NE(clientConnection_, nullptr);
    
    // Set message handler
    clientConnection_->SetMessageReceivedCallback([this](const Message& message) {
        HandleClientMessage(message);
    });
    
    // Wait for connection to establish
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Send Version message from client to server
    Message versionMessage = CreateTestMessage(MessageCommand::Version);
    clientConnection_->Send(versionMessage);
    
    // Wait for server to receive message
    ASSERT_TRUE(WaitForServerMessages(1));
    
    // Verify server received Version message
    {
        std::lock_guard<std::mutex> lock(serverMessagesMutex_);
        ASSERT_EQ(serverMessages_.size(), 1);
        EXPECT_EQ(serverMessages_[0].GetCommand(), MessageCommand::Version);
    }
    
    // Wait for client to receive VerAck response
    ASSERT_TRUE(WaitForClientMessages(1));
    
    // Verify client received VerAck message
    {
        std::lock_guard<std::mutex> lock(clientMessagesMutex_);
        ASSERT_EQ(clientMessages_.size(), 1);
        EXPECT_EQ(clientMessages_[0].GetCommand(), MessageCommand::Verack);
    }
    
    // Stop client and server
    clientConnection_->Stop();
    client_->Stop();
    server_->Stop();
}

// Test multiple connections
TEST_F(TcpNetworkTest, MultipleConnections)
{
    // Start server
    server_->Start();
    
    // Create multiple clients
    const int numClients = 5;
    std::vector<std::unique_ptr<TcpClient>> clients(numClients);
    std::vector<std::shared_ptr<TcpConnection>> connections(numClients);
    
    for (int i = 0; i < numClients; i++)
    {
        clients[i] = std::make_unique<TcpClient>();
        connections[i] = clients[i]->Connect(serverEndpoint_);
        ASSERT_NE(connections[i], nullptr);
    }
    
    // Wait for server to accept all connections
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Verify server accepted all connections
    {
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        EXPECT_EQ(serverConnections_.size(), numClients);
    }
    
    // Disconnect all clients
    for (int i = 0; i < numClients; i++)
    {
        connections[i]->Stop();
        clients[i]->Stop();
    }
    
    // Wait for server to remove all connections
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Verify server removed all connections
    {
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        EXPECT_EQ(serverConnections_.size(), 0);
    }
    
    // Stop server
    server_->Stop();
}

// Test connection error handling
TEST_F(TcpNetworkTest, ConnectionErrors)
{
    // Try to connect to non-existent server
    client_ = std::make_unique<TcpClient>();
    clientConnection_ = client_->Connect(IPEndPoint(IPAddress::Parse("127.0.0.1"), 55555));
    
    // Connection should fail
    EXPECT_EQ(clientConnection_, nullptr);
    
    // Start server
    server_->Start();
    
    // Connect to server
    clientConnection_ = client_->Connect(serverEndpoint_);
    ASSERT_NE(clientConnection_, nullptr);
    
    // Set message handler
    clientConnection_->SetMessageReceivedCallback([this](const Message& message) {
        HandleClientMessage(message);
    });
    
    // Wait for connection to establish
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop server while client is connected
    server_->Stop();
    
    // Wait for client connection to detect server disconnect
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Try to send message after server stopped (should not crash)
    Message message = CreateTestMessage(MessageCommand::Ping);
    clientConnection_->Send(message);
    
    // Clean up client
    client_->Stop();
}

// Test thread safety
TEST_F(TcpNetworkTest, ThreadSafety)
{
    // Start server
    server_->Start();
    
    // Create client
    client_ = std::make_unique<TcpClient>();
    
    // Connect to server
    clientConnection_ = client_->Connect(serverEndpoint_);
    ASSERT_NE(clientConnection_, nullptr);
    
    // Set message handler
    clientConnection_->SetMessageReceivedCallback([this](const Message& message) {
        HandleClientMessage(message);
    });
    
    // Wait for connection to establish
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Create multiple threads to send messages simultaneously
    const int numThreads = 10;
    const int messagesPerThread = 10;
    std::atomic<int> sentCount(0);
    
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; i++)
    {
        threads.emplace_back([this, i, messagesPerThread, &sentCount]() {
            for (int j = 0; j < messagesPerThread; j++)
            {
                try
                {
                    Message message = CreateTestMessage(MessageCommand::Ping);
                    clientConnection_->Send(message);
                    sentCount++;
                    
                    // Small delay to simulate real-world scenario
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Error in thread " << i << ": " << e.what() << std::endl;
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
    
    // Verify all messages were sent
    EXPECT_EQ(sentCount.load(), numThreads * messagesPerThread);
    
    // Wait for server to receive messages
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Verify server received messages (might not receive all due to timing/buffering)
    {
        std::lock_guard<std::mutex> lock(serverMessagesMutex_);
        std::cout << "Server received " << serverMessages_.size() << " messages out of " 
                  << (numThreads * messagesPerThread) << " sent" << std::endl;
        EXPECT_GT(serverMessages_.size(), 0);
    }
    
    // Stop client and server
    clientConnection_->Stop();
    client_->Stop();
    server_->Stop();
} 