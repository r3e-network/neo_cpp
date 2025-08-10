#include <neo/common/logging.h>
#include <neo/cryptography/crypto.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/get_addr_payload.h>
#include <neo/network/p2p/payloads/verack_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p_server.h>

using namespace neo::network::p2p::payloads;
#include <neo/io/binary_writer.h>
#include <neo/logging/logger.h>
#include <neo/network/network_address.h>
#include <neo/network/peer_discovery_service.h>
#include <neo/network/upnp.h>

#include <boost/asio/io_context.hpp>
#include <chrono>
#include <memory>
#include <random>
#include <sstream>

namespace neo::network
{
// P2PPeer implementation
P2PPeer::P2PPeer(std::shared_ptr<TcpConnection> connection)
    : connection_(std::move(connection)), version_(0), services_(0), startHeight_(0), relay_(false)
{
    UpdateLastSeen();
}

std::shared_ptr<TcpConnection> P2PPeer::GetConnection() const { return connection_; }

uint32_t P2PPeer::GetVersion() const { return version_; }

void P2PPeer::SetVersion(uint32_t version) { version_ = version; }

uint64_t P2PPeer::GetServices() const { return services_; }

void P2PPeer::SetServices(uint64_t services) { services_ = services; }

const std::string& P2PPeer::GetUserAgent() const { return userAgent_; }

void P2PPeer::SetUserAgent(const std::string& userAgent) { userAgent_ = userAgent; }

uint32_t P2PPeer::GetStartHeight() const { return startHeight_; }

void P2PPeer::SetStartHeight(uint32_t startHeight) { startHeight_ = startHeight; }

bool P2PPeer::GetRelay() const { return relay_; }

void P2PPeer::SetRelay(bool relay) { relay_ = relay; }

std::chrono::system_clock::time_point P2PPeer::GetLastSeen() const { return lastSeen_; }

void P2PPeer::UpdateLastSeen() { lastSeen_ = std::chrono::system_clock::now(); }

bool P2PPeer::IsConnected() const { return connection_ != nullptr; }

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
P2PServer::P2PServer(boost::asio::io_context& ioContext, const IPEndPoint& endpoint, const std::string& userAgent,
                     uint32_t startHeight)
    : ioContext_(ioContext), endpoint_(endpoint), userAgent_(userAgent), startHeight_(startHeight)
{
    // Create the TCP server
    server_ = std::make_unique<TcpServer>(endpoint);

    // Set the connection accepted callback
    server_->SetConnectionAcceptedCallback([this](std::shared_ptr<TcpConnection> connection)
                                           { HandleConnectionAccepted(std::move(connection)); });

    // Create the TCP client
    client_ = std::make_unique<TcpClient>(ioContext_);

    // Create the peer discovery service
    peerDiscovery_ = std::make_shared<PeerDiscoveryService>(ioContext_, shared_from_this());
}

std::shared_ptr<PeerDiscoveryService> P2PServer::GetPeerDiscovery() const { return peerDiscovery_; }

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
            [this, peer](const Message& message)
            {
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
            [this, peer]()
            {
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
        NEO_LOG(NEO_ERROR, "Failed to connect to " << endpoint.ToString() << ": " << ex.what());
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
            [this, peer](const Message& message)
            {
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
            [this, peer]()
            {
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

std::shared_ptr<PeerDiscoveryService> P2PServer::GetPeerDiscoveryService() const { return peerDiscovery_; }

void P2PServer::HandleGetAddrMessage(std::shared_ptr<P2PPeer> peer, const Message& message)
{
    if (!peer || !peer->IsConnected())
    {
        NEO_LOG(NEO_WARNING, "Received getaddr from invalid or disconnected peer");
        return;
    }

    if (peerDiscovery_)
    {
        try
        {
            auto payload = message.GetPayload();
            if (payload)
            {
                auto getAddrPayload = std::dynamic_pointer_cast<neo::network::p2p::payloads::GetAddrPayload>(payload);
                if (getAddrPayload)
                {
                    // Implement HandleGetAddrMessage in PeerDiscoveryService
                    peerDiscovery_->HandleGetAddrMessage(peer, getAddrPayload);
                }
                else
                {
                    NEO_LOG(NEO_WARNING, "Invalid getaddr payload from peer: " << peer->GetUserAgent());
                }
            }
            else
            {
                // Handle case where payload is null (shouldn't happen for getaddr)
                // GetAddr messages don't have payload, so this is normal
                // Use the implementation above for handling GetAddr messages
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
        auto addrPayload = std::dynamic_pointer_cast<neo::network::p2p::payloads::AddrPayload>(payload);
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
        NEO_LOG(NEO_ERROR, "Error handling Addr message from peer " << peer->GetUserAgent() << ": " << ex.what());
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
            Message verackMessage(p2p::MessageCommand::Verack, verackPayload);
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
    else if (command == "getaddr")
    {
        // Implement HandleGetAddrMessage matching C# OnGetAddrMessageReceived
        try
        {
            // Randomly select connected peers with listener ports
            std::vector<std::shared_ptr<P2PPeer>> eligiblePeers;

            for (const auto& [id, node] : peers_)
            {
                if (node && node->GetListenerPort() > 0)
                {
                    eligiblePeers.push_back(node);
                }
            }

            if (eligiblePeers.empty()) return;  // No peers to send

            // Group by address and take first from each group (remove duplicates)
            std::map<std::string, std::shared_ptr<P2PPeer>> uniquePeers;
            for (const auto& peer : eligiblePeers)
            {
                std::string address = peer->GetRemoteEndpoint().GetAddress().ToString();
                if (uniquePeers.find(address) == uniquePeers.end())
                {
                    uniquePeers[address] = peer;
                }
            }

            // Convert to vector and shuffle
            std::vector<std::shared_ptr<RemoteNode>> selectedPeers;
            for (const auto& [addr, peer] : uniquePeers)
            {
                selectedPeers.push_back(peer);
            }

            // Randomly shuffle and take up to MaxCountToSend
            std::random_shuffle(selectedPeers.begin(), selectedPeers.end());
            const size_t maxCount = 200;  // AddrPayload.MaxCountToSend
            if (selectedPeers.size() > maxCount)
            {
                selectedPeers.resize(maxCount);
            }

            // Create AddrPayload with network addresses
            auto addrPayload = std::make_shared<payloads::AddrPayload>();
            std::vector<payloads::NetworkAddressWithTime> addresses;

            for (const auto& peer : selectedPeers)
            {
                payloads::NetworkAddressWithTime addr;
                addr.SetAddress(peer->GetListenerEndpoint().GetAddress());
                addr.SetPort(peer->GetListenerPort());
                addr.SetTimestamp(static_cast<uint32_t>(std::time(nullptr)));
                addr.SetServices(peer->GetCapabilities());
                addresses.push_back(addr);
            }

            if (addresses.empty()) return;  // No valid addresses to send

            addrPayload->SetAddresses(addresses);

            // Create and send Addr message
            auto response = std::make_shared<Message>();
            response->SetCommand(MessageCommand::Addr);
            response->SetPayload(addrPayload);

            peer->SendMessage(response);
        }
        catch (const std::exception& e)
        {
            // Log error but don't disconnect peer
            std::cerr << "Error handling GetAddr message: " << e.what() << std::endl;
        }
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
    std::istringstream stream(
        std::string(reinterpret_cast<const char*>(message.GetPayload().Data()), message.GetPayload().Size()));
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
    // Implement GetData message handling matching C# OnGetDataMessageReceived
    try
    {
        auto payload = std::dynamic_pointer_cast<payloads::InvPayload>(message.GetPayload());
        if (!payload) return;

        std::vector<io::UInt256> notFound;

        for (const auto& hash : payload->GetHashes())
        {
            // Check if we've already sent this hash to avoid spam
            if (sentHashes_.find(hash) != sentHashes_.end()) continue;

            sentHashes_.insert(hash);

            switch (payload->GetType())
            {
                case payloads::InventoryType::TX:
                {
                    // Try to get transaction from memory pool
                    auto tx = memoryPool_.GetTransaction(hash);
                    if (tx)
                    {
                        auto response = std::make_shared<Message>();
                        response->SetCommand(MessageCommand::Transaction);
                        response->SetPayload(tx);
                        peer->SendMessage(response);
                    }
                    else
                    {
                        notFound.push_back(hash);
                    }
                    break;
                }
                case payloads::InventoryType::Block:
                {
                    // Try to get block from blockchain
                    auto block = blockchain_.GetBlock(hash);
                    if (block)
                    {
                        // Check if peer has bloom filter for merkle block
                        if (peer->HasBloomFilter())
                        {
                            // Create merkle block with filtered transactions
                            auto merkleBlock = CreateMerkleBlock(block, peer->GetBloomFilter());
                            auto response = std::make_shared<Message>();
                            response->SetCommand(MessageCommand::MerkleBlock);
                            response->SetPayload(merkleBlock);
                            peer->SendMessage(response);
                        }
                        else
                        {
                            // Send full block
                            auto response = std::make_shared<Message>();
                            response->SetCommand(MessageCommand::Block);
                            response->SetPayload(block);
                            peer->SendMessage(response);
                        }
                    }
                    else
                    {
                        notFound.push_back(hash);
                    }
                    break;
                }
                case payloads::InventoryType::Extensible:
                {
                    // Try to get extensible payload from relay cache
                    auto extensible = relayCache_.Get(hash);
                    if (extensible)
                    {
                        auto response = std::make_shared<Message>();
                        response->SetCommand(MessageCommand::Extensible);
                        response->SetPayload(extensible);
                        peer->SendMessage(response);
                    }
                    else
                    {
                        notFound.push_back(hash);
                    }
                    break;
                }
                default:
                {
                    // Try to get from relay cache for other inventory types
                    auto inventory = relayCache_.Get(hash);
                    if (inventory)
                    {
                        auto response = std::make_shared<Message>();
                        response->SetCommand(static_cast<MessageCommand>(payload->GetType()));
                        response->SetPayload(inventory);
                        peer->SendMessage(response);
                    }
                    else
                    {
                        notFound.push_back(hash);
                    }
                    break;
                }
            }
        }

        // Send NotFound message for items we don't have
        if (!notFound.empty())
        {
            // Create NotFound payload groups (max hashes per message)
            const size_t maxHashesPerMessage = 500;  // InvPayload.MaxHashesCount

            for (size_t i = 0; i < notFound.size(); i += maxHashesPerMessage)
            {
                size_t endIndex = std::min(i + maxHashesPerMessage, notFound.size());
                std::vector<io::UInt256> batch(notFound.begin() + i, notFound.begin() + endIndex);

                auto notFoundPayload = std::make_shared<payloads::InvPayload>();
                notFoundPayload->SetType(payload->GetType());
                notFoundPayload->SetHashes(batch);

                auto response = std::make_shared<Message>();
                response->SetCommand(MessageCommand::NotFound);
                response->SetPayload(notFoundPayload);
                peer->SendMessage(response);
            }
        }
    }
    catch (const std::exception& e)
    {
        // Log error but don't disconnect peer
        std::cerr << "Error handling GetData message: " << e.what() << std::endl;
    }
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

        // Create the version payload with complete node capabilities
        std::vector<NodeCapability> capabilities;

        // Add standard Neo node capabilities
        try
        {
            // TCP Server capability - indicates this node accepts incoming connections
            if (IsListening())
            {
                NodeCapability tcpCapability;
                tcpCapability.type = NodeCapabilityType::TcpServer;
                tcpCapability.data.tcp_server.port = GetListenPort();
                capabilities.push_back(tcpCapability);
            }

            // Websocket Server capability - if RPC is enabled
            if (IsRpcEnabled())
            {
                NodeCapability wsCapability;
                wsCapability.type = NodeCapabilityType::WsServer;
                wsCapability.data.ws_server.port = GetRpcPort();
                capabilities.push_back(wsCapability);
            }

            // Full Node capability - indicates we maintain full blockchain state
            NodeCapability fullNodeCapability;
            fullNodeCapability.type = NodeCapabilityType::FullNode;
            fullNodeCapability.data.full_node.start_height = GetBlockchainHeight();
            capabilities.push_back(fullNodeCapability);
        }
        catch (const std::exception& e)
        {
            // Error building capabilities - use minimal capability set
            NodeCapability basicCapability;
            basicCapability.type = NodeCapabilityType::FullNode;
            basicCapability.data.full_node.start_height = 0;
            capabilities.push_back(basicCapability);
        }

        auto versionPayload = VersionPayload::Create(GetNetworkMagic(),  // Use proper network magic
                                                     nonce, userAgent_, capabilities);
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

// Missing P2PServer method implementations
P2PServer::~P2PServer() = default;

const IPEndPoint& P2PServer::GetEndpoint() const { return endpoint_; }

const std::string& P2PServer::GetUserAgent() const { return userAgent_; }

uint32_t P2PServer::GetStartHeight() const { return startHeight_.load(); }

void P2PServer::SetStartHeight(uint32_t startHeight) { startHeight_.store(startHeight); }

std::vector<std::shared_ptr<P2PPeer>> P2PServer::GetConnectedPeers() const
{
    std::lock_guard<std::mutex> lock(peersMutex_);
    std::vector<std::shared_ptr<P2PPeer>> peers;
    peers.reserve(peers_.size());

    for (const auto& [endpoint, peer] : peers_)
    {
        if (peer && peer->IsConnected())
        {
            peers.push_back(peer);
        }
    }

    return peers;
}

uint16_t P2PServer::GetPort() const { return endpoint_.GetPort(); }

uint32_t P2PServer::GetNonce() const
{
    // Generate a random nonce
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis;
    return dis(gen);
}

size_t P2PServer::GetConnectedPeersCount() const
{
    std::lock_guard<std::mutex> lock(peersMutex_);
    size_t count = 0;
    for (const auto& [endpoint, peer] : peers_)
    {
        if (peer && peer->IsConnected())
        {
            count++;
        }
    }
    return count;
}

void P2PServer::Broadcast(const Message& message)
{
    std::lock_guard<std::mutex> lock(peersMutex_);
    for (const auto& [endpoint, peer] : peers_)
    {
        if (peer && peer->IsConnected())
        {
            try
            {
                peer->Send(message);
            }
            catch (const std::exception& ex)
            {
                NEO_LOG(NEO_WARNING, "Failed to send message to peer " << endpoint << ": " << ex.what());
            }
        }
    }
}

void P2PServer::SetInventoryReceivedCallback(
    std::function<void(std::shared_ptr<P2PPeer>, InventoryType, const std::vector<io::UInt256>&)> callback)
{
    inventoryReceivedCallback_ = std::move(callback);
}
}  // namespace neo::network
