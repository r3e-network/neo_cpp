#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/remote_node.h>
#include <neo/network/p2p/tcp_connection.h>
#include <neo/network/ip_endpoint.h>
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <thread>
#include <chrono>
#include <algorithm>

namespace neo::network::p2p
{
    bool LocalNode::Start(uint16_t port, size_t maxConnections)
    {
        if (running_)
            return false;

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
            acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(ioContext_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));

            // Start accepting connections
            StartAccept();

            // Create the work object to keep the io_context running
            work_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(boost::asio::make_work_guard(ioContext_));

            // Start the io_context thread
            ioThread_ = std::thread([this]() {
                ioContext_.run();
            });

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
        if (running_)
            return false;

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
            acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(ioContext_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config.GetTcp().GetPort()));

            // Start accepting connections
            StartAccept();

            // Create the work object to keep the io_context running
            work_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(boost::asio::make_work_guard(ioContext_));

            // Start the io_context thread
            ioThread_ = std::thread([this]() {
                ioContext_.run();
            });

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
        if (!running_)
            return;

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
        if (!running_)
            return false;

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
            boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(
                endpoint.GetAddress().ToString(), std::to_string(endpoint.GetPort()));

            // Connect to the endpoint
            boost::asio::async_connect(socket, endpoints,
                std::bind(&LocalNode::HandleConnect, this,
                    std::placeholders::_1, std::move(socket), endpoint));

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
        if (connectionLifecycleRunning_)
            return;

        connectionLifecycleRunning_ = true;

        connectionLifecycleThread_ = std::thread([this]() {
            ManageConnectionLifecycle();
        });
    }

    void LocalNode::StopConnectionLifecycle()
    {
        if (!connectionLifecycleRunning_)
            return;

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
                std::sort(unconnectedPeers.begin(), unconnectedPeers.end(), [](const Peer& a, const Peer& b) {
                    return a.GetLastConnectionTime() < b.GetLastConnectionTime();
                });

                // Connect to peers
                for (const auto& peer : unconnectedPeers)
                {
                    if (GetConnectedCount() >= maxConnections_)
                        break;

                    Connect(peer.GetEndPoint());
                }
            }

            // Sleep for a while
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    }

    void LocalNode::StartAccept()
    {
        if (!acceptor_)
            return;

        acceptor_->async_accept(
            std::bind(&LocalNode::HandleAccept, this,
                std::placeholders::_1, std::placeholders::_2));
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

    void LocalNode::HandleConnect(const std::error_code& error, boost::asio::ip::tcp::socket socket, const IPEndPoint& endpoint)
    {
        if (!error)
        {
            // Create a TCP connection
            auto connection = TcpConnection::Create(std::move(socket));

            // Create a remote node
            auto remoteNode = std::make_unique<RemoteNode>(this, connection);

            // Add the remote node to the connected nodes
            AddConnectedNode(std::move(remoteNode));

            // Start receiving messages
            connection->StartReceiving();

            // Send version message
            remoteNode->SendVersion();
        }
        else
        {
            // Connection failed, mark the peer as bad
            MarkPeerBad(endpoint);
        }
    }

    void LocalNode::AddConnectedNode(std::unique_ptr<RemoteNode> remoteNode)
    {
        std::lock_guard<std::mutex> lock(connectedNodesMutex_);

        uint32_t id = remoteNode->GetConnection()->GetId();
        auto node = remoteNode.get();

        connectedNodes_[id] = std::move(remoteNode);

        // Notify that a remote node has connected
        OnRemoteNodeConnected(node);
    }

    void LocalNode::RemoveConnectedNode(uint32_t id)
    {
        std::lock_guard<std::mutex> lock(connectedNodesMutex_);
        connectedNodes_.erase(id);
    }
}
