#include <iostream>
#include <fstream>
#include <neo/network/message.h>
#include <neo/network/tcp_server.h>
#include <neo/network/tcp_client.h>
#include <neo/network/payloads/version_payload.h>
#include <neo/network/payloads/addr_payload.h>
#include <neo/network/ip_endpoint.h>
#include <neo/logging/logger.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace neo::network;
using namespace neo::io;
using namespace neo::network::payloads;

/**
 * @brief Network Compatibility Test Utility
 * 
 * This utility tests the compatibility of the Neo N3 C++ implementation with the C# implementation.
 * It can be run in either server or client mode:
 * - Server mode: Listens for connections and logs received messages.
 * - Client mode: Connects to a server and sends test messages.
 */

// Connection state tracker
class ConnectionTracker
{
public:
    ConnectionTracker() : messageCount_(0) {}
    
    void AddConnection(std::shared_ptr<TcpConnection> connection)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.push_back(connection);
        
        connection->SetMessageReceivedCallback([this, connection](const Message& message) {
            HandleMessage(connection, message);
        });
        
        connection->SetConnectionClosedCallback([this, connection]() {
            RemoveConnection(connection);
        });
    }
    
    void RemoveConnection(std::shared_ptr<TcpConnection> connection)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find(connections_.begin(), connections_.end(), connection);
        if (it != connections_.end())
        {
            connections_.erase(it);
            std::cout << "Connection closed, " << connections_.size() << " connections remaining" << std::endl;
        }
    }
    
    void HandleMessage(std::shared_ptr<TcpConnection> connection, const Message& message)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        messageCount_++;
        
        std::cout << "Received message #" << messageCount_ << " - Command: " 
                  << GetCommandName(message.GetCommand()) 
                  << ", Size: " << message.GetSize() << " bytes" << std::endl;
        
        // Process message based on command
        switch (message.GetCommand())
        {
            case MessageCommand::Version:
                HandleVersionMessage(connection, message);
                break;
            case MessageCommand::Verack:
                std::cout << "Received Verack message" << std::endl;
                break;
            case MessageCommand::GetAddr:
                HandleGetAddrMessage(connection);
                break;
            case MessageCommand::Addr:
                std::cout << "Received Addr message" << std::endl;
                break;
            case MessageCommand::Ping:
                HandlePingMessage(connection, message);
                break;
            default:
                std::cout << "Received unhandled message type: " 
                          << static_cast<int>(message.GetCommand()) << std::endl;
                break;
        }
    }
    
    void HandleVersionMessage(std::shared_ptr<TcpConnection> connection, const Message& message)
    {
        // Extract version information
        auto payload = std::dynamic_pointer_cast<VersionPayload>(message.GetPayload());
        if (payload)
        {
            std::cout << "Version: " << payload->GetVersion()
                      << ", User Agent: " << payload->GetUserAgent()
                      << ", Start Height: " << payload->GetStartHeight() << std::endl;
        }
        
        // Send Verack response
        Message verackMessage(MessageCommand::Verack);
        connection->Send(verackMessage);
        std::cout << "Sent Verack message" << std::endl;
    }
    
    void HandleGetAddrMessage(std::shared_ptr<TcpConnection> connection)
    {
        // Send Addr message with sample network addresses
        auto payload = std::make_shared<AddrPayload>();
        std::vector<NetworkAddressWithTime> addresses;
        
        // Add some sample network addresses
        NetworkAddressWithTime addr1;
        addr1.address = IPEndPoint(IPAddress::Parse("35.187.20.172"), 10333);
        addr1.timestamp = std::time(nullptr);
        addr1.capabilities.tcp_server = true;
        addresses.push_back(addr1);
        
        NetworkAddressWithTime addr2;
        addr2.address = IPEndPoint(IPAddress::Parse("13.59.75.23"), 10333);
        addr2.timestamp = std::time(nullptr);
        addr2.capabilities.tcp_server = true;
        addresses.push_back(addr2);
        
        payload->SetAddresses(addresses);
        Message addrMessage(MessageCommand::Addr, payload);
        connection->Send(addrMessage);
        std::cout << "Sent Addr message with " << addresses.size() << " addresses" << std::endl;
    }
    
    void HandlePingMessage(std::shared_ptr<TcpConnection> connection, const Message& message)
    {
        // Send Pong message
        Message pongMessage(MessageCommand::Pong);
        connection->Send(pongMessage);
        std::cout << "Sent Pong message" << std::endl;
    }
    
    int GetMessageCount() const
    {
        return messageCount_;
    }
    
    size_t GetConnectionCount() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return connections_.size();
    }
    
private:
    std::vector<std::shared_ptr<TcpConnection>> connections_;
    mutable std::mutex mutex_;
    int messageCount_;
};

// Server mode
void RunServer(const IPEndPoint& endpoint)
{
    std::cout << "Starting server on " << endpoint.ToString() << std::endl;
    
    // Create server
    TcpServer server(endpoint);
    ConnectionTracker tracker;
    
    // Set up connection handler
    server.SetConnectionAcceptedCallback([&tracker](std::shared_ptr<TcpConnection> connection) {
        std::cout << "Accepted connection from " << connection->GetRemoteEndpoint().ToString() << std::endl;
        tracker.AddConnection(connection);
    });
    
    // Start server
    server.Start();
    
    std::cout << "Server started, press Enter to stop" << std::endl;
    std::cin.get();
    
    std::cout << "Stopping server" << std::endl;
    server.Stop();
}

// Client mode
void RunClient(const IPEndPoint& endpoint)
{
    std::cout << "Connecting to server at " << endpoint.ToString() << std::endl;
    
    // Create client
    TcpClient client;
    auto connection = client.Connect(endpoint);
    
    if (!connection)
    {
        std::cerr << "Failed to connect to server" << std::endl;
        return;
    }
    
    std::cout << "Connected to server" << std::endl;
    
    ConnectionTracker tracker;
    tracker.AddConnection(connection);
    
    // Send Version message
    auto versionPayload = std::make_shared<VersionPayload>();
    versionPayload->SetVersion(0);
    versionPayload->SetServices(1);
    versionPayload->SetTimestamp(std::time(nullptr));
    versionPayload->SetPort(10333);
    versionPayload->SetNonce(rand());
    versionPayload->SetUserAgent("/Neo:3.0/C++/");
    versionPayload->SetStartHeight(0);
    versionPayload->SetRelay(true);
    
    Message versionMessage(MessageCommand::Version, versionPayload);
    connection->Send(versionMessage);
    std::cout << "Sent Version message" << std::endl;
    
    // Wait a bit for the handshake to complete
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Send GetAddr message
    Message getAddrMessage(MessageCommand::GetAddr);
    connection->Send(getAddrMessage);
    std::cout << "Sent GetAddr message" << std::endl;
    
    // Send Ping message
    Message pingMessage(MessageCommand::Ping);
    connection->Send(pingMessage);
    std::cout << "Sent Ping message" << std::endl;
    
    std::cout << "Client running, press Enter to disconnect" << std::endl;
    std::cin.get();
    
    std::cout << "Disconnecting from server" << std::endl;
    connection->Stop();
    client.Stop();
}

void PrintUsage(const char* programName)
{
    std::cout << "Usage: " << programName << " [server|client] [host] [port]" << std::endl;
    std::cout << "  server: Run in server mode, listening on host:port" << std::endl;
    std::cout << "  client: Run in client mode, connecting to host:port" << std::endl;
    std::cout << "  host: Hostname or IP address (default: 127.0.0.1)" << std::endl;
    std::cout << "  port: Port number (default: 10333)" << std::endl;
}

int main(int argc, char* argv[])
{
    // Initialize random seed
    std::srand(std::time(nullptr));
    
    // Set up logging
    neo::logging::Logger::Instance().SetLogLevel(neo::logging::LogLevel::Debug);
    
    // Parse arguments
    std::string mode = "server";
    std::string host = "127.0.0.1";
    int port = 10333;
    
    if (argc > 1)
    {
        mode = argv[1];
        if (mode != "server" && mode != "client")
        {
            PrintUsage(argv[0]);
            return 1;
        }
    }
    
    if (argc > 2)
        host = argv[2];
    
    if (argc > 3)
        port = std::atoi(argv[3]);
    
    // Create endpoint
    IPEndPoint endpoint(IPAddress::Parse(host), port);
    
    // Run in specified mode
    if (mode == "server")
        RunServer(endpoint);
    else
        RunClient(endpoint);
    
    return 0;
} 