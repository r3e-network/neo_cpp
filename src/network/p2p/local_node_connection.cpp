#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/remote_node.h>
#include <neo/network/p2p/tcp_connection.h>

#include <algorithm>
#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>

namespace neo::network::p2p
{
bool LocalNode::Start(uint16_t port, size_t maxConnections)
{
    if (running_) return false;

    maxConnections_ = maxConnections;

    try
    {
        // Update the port in the capabilities
        for (auto& capability : capabilities_)
        {
            if (capability.GetType() == NodeCapabilityType::TcpServer)
            {
                static_cast<ServerCapability&>(capability).SetPort(port);
                break;
            }
        }

        // Create the acceptor
        acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(
            ioContext_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));

        // Start accepting connections
        StartAccept();

        // Create the work object to keep the io_context running
        work_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
            boost::asio::make_work_guard(ioContext_));

        // Start the io_context thread
        ioThread_ = std::thread([this]() { ioContext_.run(); });

        // Load the peer list
        LoadPeerList();

        // Start the connection lifecycle management
        StartConnectionLifecycle();

        running_ = true;
        return true;
    }
    catch (const std::exception&)
    {
        Stop();
        return false;
    }
}

bool LocalNode::Start(const ChannelsConfig& config)
{
    if (running_) return false;

    maxConnections_ = config.GetMaxConnections();

    try
    {
        // Update the port in the capabilities
        for (auto& capability : capabilities_)
        {
            if (capability.GetType() == NodeCapabilityType::TcpServer)
            {
                static_cast<ServerCapability&>(capability).SetPort(config.GetTcp().GetPort());
                break;
            }
        }

        // Create the acceptor
        acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(
            ioContext_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config.GetTcp().GetPort()));

        // Start accepting connections
        StartAccept();

        // Create the work object to keep the io_context running
        work_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
            boost::asio::make_work_guard(ioContext_));

        // Start the io_context thread
        ioThread_ = std::thread([this]() { ioContext_.run(); });

        // Load the peer list
        LoadPeerList();

        // Add seed list to peer list
        AddPeers(config.GetSeedList());

        // Start the connection lifecycle management
        StartConnectionLifecycle();

        running_ = true;
        return true;
    }
    catch (const std::exception&)
    {
        Stop();
        return false;
    }
}

void LocalNode::Stop()
{
    if (!running_) return;

    running_ = false;

    // Stop the connection lifecycle management
    StopConnectionLifecycle();

    // Close the acceptor
    if (acceptor_)
    {
        acceptor_->close();
        acceptor_.reset();
    }

    // Disconnect all connected nodes
    {
        std::lock_guard<std::mutex> lock(connectedNodesMutex_);
        connectedNodes_.clear();
    }

    // Stop the io_context
    work_.reset();
    ioContext_.stop();

    // Wait for the io_context thread to finish
    if (ioThread_.joinable())
    {
        ioThread_.join();
    }

    // Save the peer list
    SavePeerList();
}

bool LocalNode::Connect(const IPEndPoint& endpoint)
{
    if (!running_) return false;

    try
    {
        // Check if we're already connected to this endpoint
        for (const auto& node : GetConnectedNodes())
        {
            if (node->GetRemoteEndPoint() == endpoint)
            {
                return true;
            }
        }

        // Add the peer to the peer list
        AddPeer(endpoint);

        // Increment the connection attempts
        auto peer = peerList_.GetPeer(endpoint);
        if (peer)
        {
            peer->IncrementConnectionAttempts();

            // If the peer has too many connection attempts, mark it as bad
            if (peer->GetConnectionAttempts() > 10)
            {
                peer->SetBad(true);
                return false;
            }
        }

        // Create a socket
        boost::asio::ip::tcp::socket socket(ioContext_);

        // Resolve the endpoint
        boost::asio::ip::tcp::resolver resolver(ioContext_);
        boost::asio::ip::tcp::resolver::results_type endpoints =
            resolver.resolve(endpoint.GetAddress().ToString(), std::to_string(endpoint.GetPort()));

        // Connect to the endpoint
        boost::asio::async_connect(socket, endpoints,
                                   [this, endpoint, socket = std::move(socket)](
                                       const std::error_code& error, const boost::asio::ip::tcp::endpoint&) mutable
                                   { HandleConnect(error, std::move(socket), endpoint); });

        return true;
    }
    catch (const std::exception&)
    {
        // Mark the peer as bad
        MarkPeerBad(endpoint);
        return false;
    }
}

void LocalNode::StartConnectionLifecycle()
{
    if (connectionLifecycleRunning_) return;

    connectionLifecycleRunning_ = true;

    connectionLifecycleThread_ = std::thread([this]() { ManageConnectionLifecycle(); });
}

void LocalNode::StopConnectionLifecycle()
{
    if (!connectionLifecycleRunning_) return;

    connectionLifecycleRunning_ = false;

    if (connectionLifecycleThread_.joinable())
    {
        connectionLifecycleThread_.join();
    }
}

void LocalNode::ManageConnectionLifecycle()
{
    while (connectionLifecycleRunning_)
    {
        // Check if we need to connect to more peers
        if (GetConnectedCount() < maxConnections_)
        {
            // Get unconnected peers
            auto unconnectedPeers = peerList_.GetUnconnectedPeers();

            // Sort by last connection time (oldest first)
            std::sort(unconnectedPeers.begin(), unconnectedPeers.end(), [](const Peer& a, const Peer& b)
                      { return a.GetLastConnectionTime() < b.GetLastConnectionTime(); });

            // Connect to peers
            for (const auto& peer : unconnectedPeers)
            {
                if (GetConnectedCount() >= maxConnections_) break;

                Connect(peer.GetEndPoint());
            }
        }

        // Sleep for a while
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

void LocalNode::StartAccept()
{
    if (!acceptor_) return;

    acceptor_->async_accept(std::bind(&LocalNode::HandleAccept, this, std::placeholders::_1, std::placeholders::_2));
}

void LocalNode::HandleAccept(const std::error_code& error, boost::asio::ip::tcp::socket socket)
{
    if (!error)
    {
        // Check if we have reached the maximum number of connections
        if (GetConnectedCount() < maxConnections_)
        {
            // Get the remote endpoint
            IPEndPoint endpoint;
            try
            {
                auto asioEndpoint = socket.remote_endpoint();
                endpoint = IPEndPoint(IPAddress(asioEndpoint.address().to_string()), asioEndpoint.port());
            }
            catch (const std::exception&)
            {
                // Failed to get the remote endpoint, close the socket
                socket.close();

                // Continue accepting connections
                StartAccept();
                return;
            }

            // Add the peer to the peer list
            AddPeer(endpoint);

            // Create a TCP connection
            auto connection = TcpConnection::Create(std::move(socket));

            // Create a remote node
            auto remoteNode = std::make_unique<RemoteNode>(this, connection);

            // Add the remote node to the connected nodes
            AddConnectedNode(std::move(remoteNode));

            // Start receiving messages
            connection->StartReceiving();
        }
        else
        {
            // We have reached the maximum number of connections, close the socket
            socket.close();
        }

        // Continue accepting connections
        StartAccept();
    }
}

void LocalNode::HandleConnect(const std::error_code& error, boost::asio::ip::tcp::socket socket,
                              const IPEndPoint& endpoint)
{
    if (!error)
    {
        LOG_INFO("Connected to " + endpoint.ToString());

        // Create a TCP connection
        auto connection = TcpConnection::Create(std::move(socket));

        // Create a remote node
        auto remoteNode = std::make_unique<RemoteNode>(this, connection);

        // Send version message BEFORE moving the remoteNode
        LOG_INFO("Sending Version message to newly connected peer");
        bool versionSent = remoteNode->SendVersion();
        LOG_INFO("Version message send result: " + std::string(versionSent ? "success" : "failed"));

        // Start receiving messages
        connection->StartReceiving();

        // Add the remote node to the connected nodes (moves remoteNode)
        AddConnectedNode(std::move(remoteNode));
    }
    else
    {
        // Connection failed, mark the peer as bad
        MarkPeerBad(endpoint);
    }
}

void LocalNode::AddConnectedNode(std::unique_ptr<RemoteNode> remoteNode)
{
    try
    {
        std::lock_guard<std::mutex> lock(connectedNodesMutex_);

        uint32_t id = remoteNode->GetConnection()->GetId();
        auto node = remoteNode.get();

        LOG_DEBUG("Adding connected node with ID: " + std::to_string(id));

        connectedNodes_[id] = std::move(remoteNode);

        LOG_DEBUG("About to call OnRemoteNodeConnected");
        // Notify that a remote node has connected
        OnRemoteNodeConnected(node);
        LOG_DEBUG("OnRemoteNodeConnected completed");
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("AddConnectedNode failed: " + std::string(e.what()));
    }
}

void LocalNode::RemoveConnectedNode(uint32_t id)
{
    std::lock_guard<std::mutex> lock(connectedNodesMutex_);
    connectedNodes_.erase(id);
}

void LocalNode::RelayTransaction(std::shared_ptr<ledger::Transaction> transaction)
{
    // Implement transaction relay matching C# LocalNode.RelayDirectly
    if (!transaction) return;

    try
    {
        // Create inventory message for the transaction
        auto inventoryPayload = std::make_shared<payloads::InventoryPayload>();
        inventoryPayload->SetType(payloads::InventoryType::TX);
        inventoryPayload->AddHash(transaction->GetHash());

        // Create message
        auto message = std::make_shared<Message>();
        message->SetCommand(MessageCommand::Inv);
        message->SetPayload(inventoryPayload);

        // Broadcast to connected peers
        BroadcastMessage(message);

        // Store transaction in memory pool for serving GetData requests
        memoryPool_[transaction->GetHash()] = transaction;
    }
    catch (const std::exception& e)
    {
        // Log error but don't throw
        std::cerr << "Failed to relay transaction: " << e.what() << std::endl;
    }
}

void LocalNode::RelayBlock(std::shared_ptr<ledger::Block> block)
{
    // Implement block relay matching C# LocalNode.RelayDirectly
    if (!block) return;

    try
    {
        // Create inventory message for the block
        auto inventoryPayload = std::make_shared<payloads::InventoryPayload>();
        inventoryPayload->SetType(payloads::InventoryType::Block);
        inventoryPayload->AddHash(block->GetHash());

        // Create message
        auto message = std::make_shared<Message>();
        message->SetCommand(MessageCommand::Inv);
        message->SetPayload(inventoryPayload);

        // Broadcast to connected peers
        BroadcastMessage(message);

        // Store block for serving GetData requests
        blockCache_[block->GetHash()] = block;
    }
    catch (const std::exception& e)
    {
        // Log error but don't throw
        std::cerr << "Failed to relay block: " << e.what() << std::endl;
    }
}

void LocalNode::DiscoverPeers()
{
    // Implement peer discovery matching C# LocalNode.ConnectToPeers
    try
    {
        // Get seed nodes from protocol settings
        auto protocolSettings = ProtocolSettings::GetDefault();
        auto seedNodes = protocolSettings.GetSeedList();

        for (const auto& seedNode : seedNodes)
        {
            try
            {
                // Parse seed node address
                size_t colonPos = seedNode.find(':');
                if (colonPos == std::string::npos) continue;

                std::string host = seedNode.substr(0, colonPos);
                std::string portStr = seedNode.substr(colonPos + 1);
                uint16_t port = static_cast<uint16_t>(std::stoi(portStr));

                // Create endpoint
                boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(host), port);

                // Check if we're already connected to this endpoint
                bool alreadyConnected = false;
                for (const auto& [id, node] : connectedNodes_)
                {
                    if (node->GetEndpoint() == endpoint)
                    {
                        alreadyConnected = true;
                        break;
                    }
                }

                if (!alreadyConnected && connectedNodes_.size() < maxConnections_)
                {
                    // Attempt to connect
                    ConnectToPeer(endpoint);
                }
            }
            catch (const std::exception& e)
            {
                // Log error and continue with next seed node
                std::cerr << "Failed to connect to seed node " << seedNode << ": " << e.what() << std::endl;
            }
        }

        // Request addresses from connected peers
        RequestPeerAddresses();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Peer discovery failed: " << e.what() << std::endl;
    }
}

void LocalNode::RequestPeerAddresses()
{
    // Implement address request matching C# LocalNode.RequestPeers
    try
    {
        // Create GetAddr message
        auto message = std::make_shared<Message>();
        message->SetCommand(MessageCommand::GetAddr);
        message->SetPayload(nullptr);  // GetAddr has no payload

        // Send to all connected peers
        for (const auto& [id, node] : connectedNodes_)
        {
            try
            {
                node->SendMessage(message);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Failed to request addresses from peer " << id << ": " << e.what() << std::endl;
            }
        }
    }
    catch (const std::exception& e)
    {