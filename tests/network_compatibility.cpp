#include <iostream>
#include <fstream>
#include <neo/network/message.h>
#include <neo/network/tcp_server.h>
#include <neo/network/tcp_client.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/ip_endpoint.h>
#include <neo/logging/logger.h>
#include <thread>
#include <chrono>
#include <ctime>

using namespace neo::network;
using namespace neo::network::p2p::payloads;

/**
 * @brief Tests network compatibility and message exchange with Neo nodes
 */
class ConnectionTracker
{
private:
    std::shared_ptr<TcpServer> server_;
    std::shared_ptr<TcpClient> client_;
    
public:
    ConnectionTracker() = default;
    
    void SetServer(std::shared_ptr<TcpServer> server)
    {
        server_ = server;
    }
    
    void SetClient(std::shared_ptr<TcpClient> client)
    {
        client_ = client;
    }
    
    void HandleMessage(std::shared_ptr<TcpConnection> connection, const Message& message)
    {
        std::cout << "Received message: " << static_cast<int>(message.GetCommand()) << std::endl;
        
        switch (message.GetCommand())
        {
            case MessageCommand::Version:
                HandleVersionMessage(connection, message);
                break;
                
            case MessageCommand::Verack:
                HandleVerackMessage(connection, message);
                break;
                
            case MessageCommand::GetAddr:
                HandleGetAddrMessage(connection, message);
                break;
                
            case MessageCommand::Addr:
                HandleAddrMessage(connection, message);
                break;
                
            case MessageCommand::Ping:
                HandlePingMessage(connection, message);
                break;
                
            default:
                std::cout << "Unhandled message command: " << static_cast<int>(message.GetCommand()) << std::endl;
        }
    }
    
    void HandleVersionMessage(std::shared_ptr<TcpConnection> connection, const Message& message)
    {
        // Deserialize version payload
        auto versionPayload = std::dynamic_pointer_cast<VersionPayload>(message.GetPayload());
        if (!versionPayload)
        {
            std::cerr << "Failed to deserialize version payload" << std::endl;
            return;
        }
        
        std::cout << "Version message received:" << std::endl;
        std::cout << "  Network: " << std::hex << versionPayload->GetNetwork() << std::dec << std::endl;
        std::cout << "  Version: " << versionPayload->GetVersion() << std::endl;
        std::cout << "  User Agent: " << versionPayload->GetUserAgent() << std::endl;
        std::cout << "  Timestamp: " << versionPayload->GetTimestamp() << std::endl;
        std::cout << "  Nonce: " << versionPayload->GetNonce() << std::endl;
        
        // Send Verack
        Message verackMessage(MessageCommand::Verack);
        connection->Send(verackMessage);
        
        std::cout << "Sent Verack message" << std::endl;
    }
    
    void HandleVerackMessage(std::shared_ptr<TcpConnection> connection, const Message& message)
    {
        std::cout << "Verack message received - handshake complete" << std::endl;
    }
    
    void HandleGetAddrMessage(std::shared_ptr<TcpConnection> connection, const Message& message)
    {
        std::cout << "GetAddr message received - sending known addresses" << std::endl;
        
        // Send some known addresses
        SendKnownAddresses(connection);
    }
    
    void HandleAddrMessage(std::shared_ptr<TcpConnection> connection, const Message& message)
    {
        // Deserialize addr payload
        auto addrPayload = std::dynamic_pointer_cast<AddrPayload>(message.GetPayload());
        if (!addrPayload)
        {
            std::cerr << "Failed to deserialize addr payload" << std::endl;
            return;
        }
        
        std::cout << "Addr message received with " << addrPayload->GetAddresses().size() << " addresses" << std::endl;
    }
    
    void SendKnownAddresses(std::shared_ptr<TcpConnection> connection)
    {
        auto payload = std::make_shared<AddrPayload>();
        std::vector<neo::network::p2p::NetworkAddressWithTime> addresses;
        
        // Add some known mainnet nodes
        neo::network::p2p::NetworkAddressWithTime addr1;
        addr1.SetAddress(IPAddress::Parse("35.187.20.172"));
        addr1.SetPort(10333);
        addr1.SetTimestamp(std::time(nullptr));
        addresses.push_back(addr1);
        
        neo::network::p2p::NetworkAddressWithTime addr2;
        addr2.SetAddress(IPAddress::Parse("13.59.75.23"));
        addr2.SetPort(10333);
        addr2.SetTimestamp(std::time(nullptr));
        addresses.push_back(addr2);
        
        payload->SetAddresses(addresses);
        Message addrMessage(MessageCommand::Addr, payload);
        connection->Send(addrMessage);
        
        std::cout << "Sent Addr message with " << addresses.size() << " addresses" << std::endl;
    }
    
    void HandlePingMessage(std::shared_ptr<TcpConnection> connection, const Message& message)
    {
        // Respond with Pong
        Message pongMessage(MessageCommand::Pong);
        connection->Send(pongMessage);
        
        std::cout << "Sent Pong message in response to Ping" << std::endl;
    }
};

/**
 * @brief Runs the server side of the compatibility test
 */
void RunServer(const IPEndPoint& endpoint)
{
    ConnectionTracker tracker;
    auto server = std::make_shared<TcpServer>(endpoint);
    tracker.SetServer(server);
    
    server->OnConnectionReceived = [&tracker](std::shared_ptr<TcpConnection> connection) {
        std::cout << "Client connected from: " << connection->GetRemoteEndPoint().ToString() << std::endl;
        
        connection->OnMessageReceived = [&tracker, connection](const Message& message) {
            tracker.HandleMessage(connection, message);
        };
        
        connection->OnDisconnected = [](DisconnectReason reason) {
            std::cout << "Client disconnected. Reason: " << static_cast<int>(reason) << std::endl;
        };
    };
    
    server->Start();
    std::cout << "Server listening on: " << endpoint.ToString() << std::endl;
    
    // Keep server running
    std::this_thread::sleep_for(std::chrono::minutes(5));
    
    server->Stop();
}

/**
 * @brief Runs the client side of the compatibility test
 */
void RunClient(const IPEndPoint& endpoint)
{
    ConnectionTracker tracker;
    auto client = std::make_shared<TcpClient>();
    tracker.SetClient(client);
    
    client->OnMessageReceived = [&tracker](const Message& message) {
        tracker.HandleMessage(nullptr, message);
    };
    
    client->OnDisconnected = [](DisconnectReason reason) {
        std::cout << "Disconnected from server. Reason: " << static_cast<int>(reason) << std::endl;
    };
    
    std::cout << "Connecting to: " << endpoint.ToString() << std::endl;
    
    if (!client->Connect(endpoint))
    {
        std::cerr << "Failed to connect to server" << std::endl;
        return;
    }
    
    std::cout << "Connected to server" << std::endl;
    
    // Get a proper connection handle
    auto connection = client->GetConnection();
    if (!connection)
    {
        std::cerr << "Failed to get connection handle" << std::endl;
        return;
    }
    
    // Send Version message
    auto versionPayload = std::make_shared<VersionPayload>();
    versionPayload->SetVersion(0);
    versionPayload->SetNetwork(0x334E454F); // Neo mainnet magic
    versionPayload->SetTimestamp(std::time(nullptr));
    versionPayload->SetNonce(rand());
    versionPayload->SetUserAgent("/Neo:3.0/C++/");
    
    // Add capabilities (TCP server capability)
    std::vector<NodeCapability> capabilities;
    NodeCapability tcpCapability;
    tcpCapability.type = NodeCapabilityType::TcpServer;
    tcpCapability.data.push_back(10333 & 0xFF);
    tcpCapability.data.push_back((10333 >> 8) & 0xFF);
    capabilities.push_back(tcpCapability);
    versionPayload->SetCapabilities(capabilities);
    
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
    
    // Keep client running to receive responses
    std::this_thread::sleep_for(std::chrono::seconds(30));
    
    client->Disconnect();
}

int main(int argc, char* argv[])
{
    // Initialize logging
    neo::logging::Logger::Instance().SetLevel(neo::logging::Logger::Level::Debug);
    
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <server|client> [host] [port]" << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  " << argv[0] << " server 127.0.0.1 10333    # Run as server" << std::endl;
        std::cout << "  " << argv[0] << " client seed1.neo.org 10333 # Connect to mainnet" << std::endl;
        return 1;
    }
    
    std::string mode = argv[1];
    std::string host = (argc > 2) ? argv[2] : "127.0.0.1";
    uint16_t port = (argc > 3) ? std::stoi(argv[3]) : 10333;
    
    try
    {
        IPEndPoint endpoint(IPAddress::Parse(host), port);
        
        if (mode == "server")
        {
            RunServer(endpoint);
        }
        else if (mode == "client")
        {
            RunClient(endpoint);
        }
        else
        {
            std::cerr << "Invalid mode. Use 'server' or 'client'" << std::endl;
            return 1;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}