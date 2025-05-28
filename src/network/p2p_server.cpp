#include <neo/network/p2p_server.h>
#include <neo/cryptography/crypto.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p/payloads/verack_payload.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/get_addr_payload.h>
#include <neo/common/logging.h>

using namespace neo::network::p2p::payloads;
#include <neo/network/network_address.h>
#include <neo/network/peer_discovery_service.h>
#include <neo/network/upnp.h>
#include <neo/logging/logger.h>
#include <neo/io/binary_writer.h>
#include <sstream>
#include <random>
#include <chrono>
#include <memory>
#include <boost/asio/io_context.hpp>

namespace neo::network
{
    // P2PPeer implementation
    P2PPeer::P2PPeer(std::shared_ptr<TcpConnection> connection)
        : connection_(std::move(connection)), version_(0), services_(0), startHeight_(0), relay_(false)
    {
        UpdateLastSeen();
    }

    std::shared_ptr<TcpConnection> P2PPeer::GetConnection() const
    {
        return connection_;
    }

    uint32_t P2PPeer::GetVersion() const
    {
        return version_;
    }

    void P2PPeer::SetVersion(uint32_t version)
    {
        version_ = version;
    }

    uint64_t P2PPeer::GetServices() const
    {
        return services_;
    }

    void P2PPeer::SetServices(uint64_t services)
    {
        services_ = services;
    }

    const std::string& P2PPeer::GetUserAgent() const
    {
        return userAgent_;
    }

    void P2PPeer::SetUserAgent(const std::string& userAgent)
    {
        userAgent_ = userAgent;
    }

    uint32_t P2PPeer::GetStartHeight() const
    {
        return startHeight_;
    }

    void P2PPeer::SetStartHeight(uint32_t startHeight)
    {
        startHeight_ = startHeight;
    }

    bool P2PPeer::GetRelay() const
    {
        return relay_;
    }

    void P2PPeer::SetRelay(bool relay)
    {
        relay_ = relay;
    }

    std::chrono::system_clock::time_point P2PPeer::GetLastSeen() const
    {
        return lastSeen_;
    }

    void P2PPeer::UpdateLastSeen()
    {
        lastSeen_ = std::chrono::system_clock::now();
    }

    bool P2PPeer::IsConnected() const
    {
        return connection_ != nullptr;
    }

    void P2PPeer::Disconnect()
    {
        if (connection_)
        {
            connection_->Stop();
            connection_ = nullptr;
        }
    }

    void P2PPeer::Send(const Message& message)
    {
        if (connection_)
        {
            connection_->Send(message);
        }
    }

    // P2PServer implementation
    P2PServer::P2PServer(
        boost::asio::io_context& ioContext,
        const IPEndPoint& endpoint,
        const std::string& userAgent,
        uint32_t startHeight)
        : ioContext_(ioContext)
        , endpoint_(endpoint)
        , userAgent_(userAgent)
        , startHeight_(startHeight)
    {
        // Create the TCP server
        server_ = std::make_unique<TcpServer>(
            ioContext_,
            endpoint,
            [this](std::shared_ptr<TcpConnection> connection) {
                HandleConnectionAccepted(std::move(connection));
            });

        // Create the TCP client
        client_ = std::make_unique<TcpClient>(ioContext_);

        // Create the peer discovery service
        peerDiscovery_ = std::make_shared<PeerDiscoveryService>(ioContext_, shared_from_this());
    }

    std::shared_ptr<PeerDiscoveryService> P2PServer::GetPeerDiscovery() const
    {
        return peerDiscovery_;
    }

    void P2PServer::Start()
    {
        // Start the TCP server
        server_->Start();

        // Try to set up UPnP port forwarding
        if (endpoint_.GetPort() > 0)
        {
            try
            {
                if (UPnP::Discover())
                {
                    try
                    {
                        // Add the external IP address to the list of local addresses
                        IPAddress externalIP = UPnP::GetExternalIP();
                        NEO_LOG(NEO_INFO, "P2P server UPnP external IP: " << externalIP.ToString());

                        // Forward the TCP port
                        UPnP::ForwardPort(endpoint_.GetPort(), "TCP", "NEO P2P TCP");
                        NEO_LOG(NEO_INFO, "P2P server UPnP port forwarding set up for TCP port " << endpoint_.GetPort());
                    }
                    catch (const std::exception& e)
                    {
                        NEO_LOG(NEO_WARNING, "P2P server failed to set up UPnP: " << e.what());
                    }
                }
                else
                {
                    NEO_LOG(NEO_INFO, "P2P server UPnP device not found");
                }
            }
            catch (const std::exception& e)
            {
                NEO_LOG(NEO_WARNING, "P2P server error discovering UPnP device: " << e.what());
            }
        }

        // Start the peer discovery service
        if (peerDiscovery_)
        {
            peerDiscovery_->Start();
        }

        NEO_LOG(NEO_INFO, "P2P server started on " << endpoint_.ToString());
    }

    void P2PServer::Stop()
    {
        // Stop the peer discovery service
        if (peerDiscovery_)
        {
            peerDiscovery_->Stop();
        }

        // Stop the TCP server
        if (server_)
        {
            server_->Stop();
        }

        // Close all connections
        std::lock_guard<std::mutex> lock(peersMutex_);
        for (auto& [endpoint, peer] : peers_)
        {
            if (auto conn = peer->GetConnection())
            {
                conn->Close();
            }
        }
        peers_.clear();

        // Remove UPnP port forwarding
        if (endpoint_.GetPort() > 0)
        {
            try
            {
                UPnP::DeleteForwardingRule(endpoint_.GetPort(), "TCP");
                NEO_LOG(NEO_INFO, "P2P server UPnP port forwarding removed for TCP port " << endpoint_.GetPort());
            }
            catch (const std::exception& e)
            {
                NEO_LOG(NEO_WARNING, "P2P server failed to remove UPnP port forwarding: " << e.what());
            }
        }

        NEO_LOG(NEO_INFO, "P2P server stopped");
    }

    std::shared_ptr<P2PPeer> P2PServer::ConnectToPeer(const IPEndPoint& endpoint)
    {
        try
        {
            // Create connection
            auto connection = client_->Connect(endpoint);

            // Create peer
            auto peer = std::make_shared<P2PPeer>(connection);
            std::string endpointStr = endpoint.ToString();

            // Add to connected peers
            {
                std::lock_guard<std::mutex> lock(peersMutex_);
                // Don't connect if already connected
                if (peers_.find(endpointStr) != peers_.end())
                {
                    throw std::runtime_error("Already connected to " + endpointStr);
                }
                peers_[endpointStr] = peer;
            }

            // Notify peer discovery
            if (peerDiscovery_)
            {
                peerDiscovery_->OnPeerConnected(endpointStr);
            }

            // Set up message handler
            connection->SetMessageReceivedCallback(
                [this, peer](const Message& message) {
                    try
                    {
                        HandleMessageReceived(peer, message);

                        // Update last seen time
                        peer->UpdateLastSeen();
                    }
                    catch (const std::exception& ex)
                    {
                        NEO_LOG(NEO_ERROR, "Error handling message: " << ex.what());
                        peer->GetConnection()->Close();
                    }
                });

            // Set up close handler
            connection->SetConnectionClosedCallback(
                [this, peer]() {
                    std::string endpoint = peer->GetConnection()->GetRemoteEndpoint().ToString();

                    // Notify peer discovery
                    if (peerDiscovery_)
                    {
                        peerDiscovery_->OnPeerDisconnected(endpoint);
                    }

                    // Remove from connected peers
                    std::lock_guard<std::mutex> lock(peersMutex_);
                    peers_.erase(endpoint);
                });

            // Send version message
            SendVersionMessage(peer);

            return peer;
        }
        catch (const std::exception& ex)
        {
            NEO_LOG(NEO_ERROR, "Failed to connect to " << endpoint.ToString()
                               << ": " << ex.what());
            throw;
        }
    }

    void P2PServer::HandleConnectionAccepted(std::shared_ptr<TcpConnection> connection)
    {
        try
        {
            auto peer = std::make_shared<P2PPeer>(connection);
            std::string endpoint = connection->GetRemoteEndpoint().ToString();

            // Add to connected peers
            {
                std::lock_guard<std::mutex> lock(peersMutex_);
                peers_[endpoint] = peer;
            }

            // Notify peer discovery
            if (peerDiscovery_)
            {
                peerDiscovery_->OnPeerConnected(endpoint);
            }

            // Set up message handler
            connection->SetMessageReceivedCallback(
                [this, peer](const Message& message) {
                    try
                    {
                        HandleMessageReceived(peer, message);

                        // Update last seen time
                        peer->UpdateLastSeen();
                    }
                    catch (const std::exception& ex)
                    {
                        NEO_LOG(NEO_ERROR, "Error handling message: " << ex.what());
                        peer->GetConnection()->Close();
                    }
                });

            // Set up close handler
            connection->SetConnectionClosedCallback(
                [this, peer]() {
                    std::string endpoint = peer->GetConnection()->GetRemoteEndpoint().ToString();

                    // Notify peer discovery
                    if (peerDiscovery_)
                    {
                        peerDiscovery_->OnPeerDisconnected(endpoint);
                    }

                    // Remove from connected peers
                    std::lock_guard<std::mutex> lock(peersMutex_);
                    peers_.erase(endpoint);
                });

            // Send version message
            SendVersionMessage(peer);
        }
        catch (const std::exception& ex)
        {
            NEO_LOG(NEO_ERROR, "Error handling connection: " << ex.what());
            connection->Close();
        }
    }

    std::shared_ptr<PeerDiscoveryService> P2PServer::GetPeerDiscoveryService() const
    {
        return peerDiscovery_;
    }

    void P2PServer::HandleGetAddrMessage(std::shared_ptr<P2PPeer> peer, const Message& message)
    {
        if (!peer || !peer->IsConnected())
        {
            NEO_LOG(NEO_WARNING, "Received getaddr from invalid or disconnected peer");
            return;
        }

        NEO_LOG(NEO_DEBUG, "Received getaddr from peer: " << peer->GetUserAgent());

        // Forward to peer discovery service if available
        if (peerDiscovery_)
        {
            try
            {
                auto payload = message.GetPayload();
                if (payload)
                {
                    auto getAddrPayload = std::dynamic_pointer_cast<GetAddrPayload>(payload);
                    if (getAddrPayload)
                    {
                        // TODO: Implement HandleGetAddrMessage in PeerDiscoveryService
                        // peerDiscovery_->HandleGetAddrMessage(peer, getAddrPayload);
                    }
                    else
                    {
                        NEO_LOG(NEO_WARNING, "Invalid getaddr payload from peer: " << peer->GetUserAgent());
                    }
                }
                else
                {
                    // Handle case where payload is null (shouldn't happen for getaddr)
                    // TODO: Implement HandleGetAddrMessage in PeerDiscoveryService
                    // peerDiscovery_->HandleGetAddrMessage(peer, std::make_shared<GetAddrPayload>());
                }
            }
            catch (const std::exception& ex)
            {
                NEO_LOG(NEO_ERROR, "Error processing getaddr message: " << ex.what());
                peer->Disconnect();
            }
        }
        else
        {
            NEO_LOG(NEO_WARNING, "Received getaddr but peer discovery service is not available");
        }
    }

    void P2PServer::HandleAddrMessage(std::shared_ptr<P2PPeer> peer, const Message& message)
    {
        if (!peer || !peer->IsConnected())
        {
            NEO_LOG(NEO_WARNING, "Received addr from invalid or disconnected peer");
            return;
        }

        try
        {
            // Deserialize the address payload
            auto payload = message.GetPayload();
            if (!payload)
            {
                NEO_LOG(NEO_WARNING, "Received addr message with null payload from peer: " << peer->GetUserAgent());
                return;
            }

            // Cast to AddrPayload
            auto addrPayload = std::dynamic_pointer_cast<AddrPayload>(payload);
            if (!addrPayload)
            {
                NEO_LOG(NEO_WARNING, "Failed to cast addr payload from peer: " << peer->GetUserAgent());
                return;
            }

            NEO_LOG(NEO_DEBUG, "Received Addr message with " << addrPayload->GetAddresses().size()
                                 << " addresses from peer: " << peer->GetUserAgent());

            // Forward to peer discovery service if available
            if (peerDiscovery_)
            {
                peerDiscovery_->HandleAddrMessage(peer, addrPayload);
            }
            else
            {
                NEO_LOG(NEO_WARNING, "Received Addr but peer discovery service is not available");
            }
        }
        catch (const std::exception& ex)
        {
            NEO_LOG(NEO_ERROR, "Error handling Addr message from peer "
                                 << peer->GetUserAgent() << ": " << ex.what());
            peer->Disconnect();
        }
    }

    void P2PServer::HandleMessageReceived(std::shared_ptr<P2PPeer> peer, const Message& message)
    {
        // Update the last seen time
        peer->UpdateLastSeen();

        // Handle the message
        const std::string command = p2p::GetCommandName(message.GetCommand());
        if (command == "getaddr")
        {
            HandleGetAddrMessage(peer, message);
        }
        else if (command == "addr")
        {
            HandleAddrMessage(peer, message);
        }
        else if (command == "version")
        {
            try
            {
                // Deserialize the version payload
                auto payload = message.GetPayload();
                if (!payload)
                {
                    NEO_LOG(NEO_WARNING, "Received version message with null payload");
                    peer->Disconnect();
                    return;
                }

                // Cast to VersionPayload
                auto versionPayload = std::dynamic_pointer_cast<VersionPayload>(payload);
                if (!versionPayload)
                {
                    NEO_LOG(NEO_WARNING, "Failed to cast version payload");
                    peer->Disconnect();
                    return;
                }

                // Update peer information (only using properties that exist in C# VersionPayload)
                peer->SetVersion(versionPayload->GetVersion());
                peer->SetUserAgent(versionPayload->GetUserAgent());
                peer->UpdateLastSeen();

                NEO_LOG(NEO_INFO, "Received version: " << versionPayload->GetUserAgent()
                                    << " (version: " << versionPayload->GetVersion()
                                    << ", network: " << versionPayload->GetNetwork() << ")");

                // Send verack message
                auto verackPayload = std::make_shared<VerAckPayload>();
                Message verackMessage(MessageCommand::Verack, verackPayload);
                peer->Send(verackMessage);

                NEO_LOG(NEO_DEBUG, "Sent verack to peer");
            }
            catch (const std::exception& ex)
            {
                NEO_LOG(NEO_ERROR, "Error handling version message: " << ex.what());
                peer->Disconnect();
            }
        }
        else if (command == "verack")
        {
            HandleVerackMessage(peer, message);
        }
        else if (command == "inv")
        {
            HandleInventoryMessage(peer, message);
        }
        else if (command == "getdata")
        {
            HandleGetDataMessage(peer, message);
        }
        else if (command == "ping")
        {
            HandlePingMessage(peer, message);
        }
        else if (command == "pong")
        {
            HandlePongMessage(peer, message);
        }
    }

    void P2PServer::HandleVerackMessage(std::shared_ptr<P2PPeer> peer, const Message& message)
    {
        try
        {
            NEO_LOG(NEO_DEBUG, "Received verack from peer: " << peer->GetUserAgent());

            // The handshake is now complete
            // You can add any additional logic here for when the handshake completes

            // For example, you might want to request the peer's address list
            // or start syncing blocks
        }
        catch (const std::exception& ex)
        {
            NEO_LOG(NEO_ERROR, "Error handling verack message: " << ex.what());
            peer->Disconnect();
        }
    }

    void P2PServer::HandleInventoryMessage(std::shared_ptr<P2PPeer> peer, const Message& message)
    {
        // Deserialize the inventory payload
        std::istringstream stream(std::string(reinterpret_cast<const char*>(message.GetPayload().Data()), message.GetPayload().Size()));
        io::BinaryReader reader(stream);
        InventoryPayload payload;
        payload.Deserialize(reader);

        // Call the callback
        if (inventoryReceivedCallback_)
        {
            inventoryReceivedCallback_(peer, payload.GetType(), payload.GetHashes());
        }
    }

    void P2PServer::HandleGetDataMessage(std::shared_ptr<P2PPeer> peer, const Message& message)
    {
        // Deserialize the inventory payload
        std::istringstream stream(std::string(reinterpret_cast<const char*>(message.GetPayload().Data()), message.GetPayload().Size()));
        io::BinaryReader reader(stream);
        InventoryPayload payload;
        payload.Deserialize(reader);

        // TODO: Send the requested data
    }

    void P2PServer::HandlePingMessage(std::shared_ptr<P2PPeer> peer, const Message& message)
    {
        try
        {
            NEO_LOG(NEO_DEBUG, "Received ping from peer: " << peer->GetUserAgent());

            // Forward the same payload back as a pong
            Message pong(MessageCommand::Pong, message.GetPayload());
            peer->Send(pong);

            NEO_LOG(NEO_DEBUG, "Sent pong to peer: " << peer->GetUserAgent());
        }
        catch (const std::exception& ex)
        {
            NEO_LOG(NEO_ERROR, "Error handling ping message: " << ex.what());
            peer->Disconnect();
        }
    }

    void P2PServer::HandlePongMessage(std::shared_ptr<P2PPeer> peer, const Message& message)
    {
        try
        {
            NEO_LOG(NEO_DEBUG, "Received pong from peer: " << peer->GetUserAgent());

            // Update the last seen time to indicate the peer is still responsive
            peer->UpdateLastSeen();

            // You could update any latency measurements here if needed
        }
        catch (const std::exception& ex)
        {
            NEO_LOG(NEO_ERROR, "Error handling pong message: " << ex.what());
            peer->Disconnect();
        }
    }

    void P2PServer::SendVersionMessage(std::shared_ptr<P2PPeer> peer)
    {
        try
        {
            // Generate a random nonce
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<uint32_t> dis(0, std::numeric_limits<uint32_t>::max());
            uint32_t nonce = dis(gen);

            // Create the version payload using the C# VersionPayload.Create method equivalent
            std::vector<NodeCapability> capabilities; // Empty capabilities for now
            auto versionPayload = VersionPayload::Create(
                0, // network magic - should be set properly
                nonce,
                userAgent_,
                capabilities
            );
            auto payload = std::make_shared<VersionPayload>(versionPayload);

            // Create and send the message
            Message message(MessageCommand::Version, payload);
            peer->Send(message);

            NEO_LOG(NEO_DEBUG, "Sent version message to peer");
        }
        catch (const std::exception& ex)
        {
            NEO_LOG(NEO_ERROR, "Error sending version message: " << ex.what());
            peer->Disconnect();
        }
    }

    void P2PServer::RequestAddresses(std::shared_ptr<P2PPeer> peer)
    {
        try
        {
            NEO_LOG(NEO_DEBUG, "Requesting addresses from peer: " << peer->GetUserAgent());

            // Create and send a getaddr message
            Message getAddrMessage(MessageCommand::GetAddr, nullptr);
            peer->Send(getAddrMessage);

            NEO_LOG(NEO_DEBUG, "Sent getaddr to peer: " << peer->GetUserAgent());
        }
        catch (const std::exception& ex)
        {
            NEO_LOG(NEO_ERROR, "Error requesting addresses: " << ex.what());
            peer->Disconnect();
        }
    }
}
