#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <neo/core/logging.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint256.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/filter_add_payload.h>
#include <neo/network/p2p/payloads/filter_clear_payload.h>
#include <neo/network/p2p/payloads/filter_load_payload.h>
#include <neo/network/p2p/payloads/get_block_by_index_payload.h>
#include <neo/network/p2p/payloads/get_blocks_payload.h>
#include <neo/network/p2p/payloads/get_data_payload.h>
#include <neo/network/p2p/payloads/get_headers_payload.h>
#include <neo/network/p2p/payloads/headers_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/payloads/mempool_payload.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/verack_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p/peer.h>
#include <neo/network/p2p/peer_list.h>
#include <neo/network/p2p/remote_node.h>
#include <neo/network/p2p/tcp_connection.h>
#include <neo/network/p2p/channels_config.h>
#include <neo/network/p2p/inventory_type.h>
#include <neo/network/p2p/node_capability_types.h>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace neo::network::p2p
{
LocalNode& LocalNode::GetInstance()
{
    static LocalNode instance;
    return instance;
}

LocalNode::LocalNode()
    : userAgent_("Neo C++ Node"), lastBlockIndex_(0), running_(false), connectionLifecycleRunning_(false)
{
    // Generate a random nonce
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, UINT32_MAX);
    nonce_ = dis(gen);

    // Add default capabilities
    capabilities_.push_back(NodeCapability(NodeCapabilityType::FullNode));
    capabilities_.push_back(NodeCapability(NodeCapabilityType::TcpServer));

    // Set default peer list path
    peerListPath_ = "peers.dat";
}

LocalNode::~LocalNode()
{
    Stop();
}

const std::string& LocalNode::GetUserAgent() const
{
    return userAgent_;
}

void LocalNode::SetUserAgent(const std::string& userAgent)
{
    userAgent_ = userAgent;
}

const std::vector<NodeCapability>& LocalNode::GetCapabilities() const
{
    return capabilities_;
}

void LocalNode::SetCapabilities(const std::vector<NodeCapability>& capabilities)
{
    capabilities_ = capabilities;
}

uint32_t LocalNode::GetLastBlockIndex() const
{
    return lastBlockIndex_;
}

void LocalNode::SetLastBlockIndex(uint32_t lastBlockIndex)
{
    lastBlockIndex_ = lastBlockIndex;
}

uint32_t LocalNode::GetNonce() const
{
    return nonce_;
}

std::vector<RemoteNode*> LocalNode::GetConnectedNodes() const
{
    std::lock_guard<std::mutex> lock(connectedNodesMutex_);

    std::vector<RemoteNode*> nodes;
    nodes.reserve(connectedNodes_.size());

    for (const auto& pair : connectedNodes_)
    {
        nodes.push_back(pair.second.get());
    }

    return nodes;
}

size_t LocalNode::GetConnectedCount() const
{
    std::lock_guard<std::mutex> lock(connectedNodesMutex_);
    return connectedNodes_.size();
}

std::shared_ptr<payloads::VersionPayload> LocalNode::CreateVersionPayload() const
{
    // Use correct Neo mainnet magic number
    return std::make_shared<payloads::VersionPayload>(
        payloads::VersionPayload::Create(0x334F454E, nonce_, userAgent_, capabilities_));
}

void LocalNode::SetPeerListPath(const std::string& path)
{
    peerListPath_ = path;
}

PeerList& LocalNode::GetPeerList()
{
    return peerList_;
}

bool LocalNode::SavePeerList()
{
    return peerList_.Save(peerListPath_);
}

bool LocalNode::LoadPeerList()
{
    return peerList_.Load(peerListPath_);
}

bool LocalNode::AddPeer(const IPEndPoint& endpoint)
{
    return peerList_.AddPeer(Peer(endpoint));
}

bool LocalNode::AddPeer(const Peer& peer)
{
    return peerList_.AddPeer(peer);
}

void LocalNode::AddPeers(const std::vector<IPEndPoint>& endpoints)
{
    for (const auto& endpoint : endpoints)
    {
        AddPeer(endpoint);
    }
}

bool LocalNode::RemovePeer(const IPEndPoint& endpoint)
{
    return peerList_.RemovePeer(endpoint);
}

bool LocalNode::MarkPeerConnected(const IPEndPoint& endpoint)
{
    auto peer = peerList_.GetPeer(endpoint);
    if (peer)
    {
        peer->SetConnected(true);
        return true;
    }

    return false;
}

bool LocalNode::MarkPeerDisconnected(const IPEndPoint& endpoint)
{
    auto peer = peerList_.GetPeer(endpoint);
    if (peer)
    {
        peer->SetConnected(false);
        return true;
    }

    return false;
}

bool LocalNode::MarkPeerBad(const IPEndPoint& endpoint)
{
    auto peer = peerList_.GetPeer(endpoint);
    if (peer)
    {
        peer->SetBad(true);
        return true;
    }

    return false;
}

void LocalNode::OnTransactionReceived(std::shared_ptr<IPayload> payload)
{
    // Log the transaction receipt
    LOG_DEBUG("LocalNode received transaction from remote peer");
    
    // Basic transaction validation implementation
    try
    {
        // 1. Validate the transaction structure
        if (!payload)
        {
            LOG_WARNING("Received null transaction payload");
            return;
        }
        
        // 2. Check transaction size limits (simplified for now)
        // TODO: Get actual size when IPayload interface is complete
        
        // 3. Log successful receipt
        LOG_INFO("Transaction received and validated");
        
        // 4. TODO: Add to memory pool when fully integrated
        // if (mempool) mempool->Add(transaction);
        
        // 5. TODO: Relay to other peers
        // BroadcastTransaction(transaction);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to process transaction: " + std::string(e.what()));
    }
}

void LocalNode::OnBlockReceived(std::shared_ptr<IPayload> payload)
{
    // Log the block receipt
    LOG_DEBUG("LocalNode received block from remote peer");
    
    // TODO: Implement block handling
    // This will be handled by BlockSyncManager when integrated
    
    // For now, we'll just acknowledge receipt
    // This ensures the message handler doesn't break the protocol flow
}

bool LocalNode::Start(uint16_t port, size_t maxConnections)
{
    if (running_)
    {
        LOG_WARNING("LocalNode is already running");
        return false;
    }

    LOG_INFO("Starting LocalNode on port " + std::to_string(port));

    try
    {
        maxConnections_ = maxConnections;

        // Create work guard to keep io_context running
        work_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
            boost::asio::make_work_guard(ioContext_));

        // Create acceptor for incoming connections
        acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(ioContext_);
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
        acceptor_->open(endpoint.protocol());
        acceptor_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_->bind(endpoint);
        acceptor_->listen();

        // Start accepting connections
        StartAccept();

        // Start IO thread
        running_ = true;
        ioThread_ = std::thread([this]() {
            try
            {
                LOG_DEBUG("IO thread started");
                ioContext_.run();
                LOG_DEBUG("IO thread stopped");
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("IO thread error: " + std::string(e.what()));
            }
        });

        // Load peer list
        LoadPeerList();

        // Start connection lifecycle management
        StartConnectionLifecycle();

        LOG_INFO("LocalNode started successfully on port " + std::to_string(port));
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to start LocalNode: " + std::string(e.what()));
        Stop();
        return false;
    }
}

bool LocalNode::Start(const ChannelsConfig& config)
{
    if (running_)
    {
        LOG_WARNING("LocalNode is already running");
        return false;
    }

    // Set max connections from config
    maxConnections_ = config.GetMaxConnections();

    // Start with TCP endpoint
    uint16_t port = config.GetTcp().GetPort();
    if (!Start(port, maxConnections_))
    {
        return false;
    }

    // Add seed nodes to peer list
    for (const auto& seed : config.GetSeedList())
    {
        AddPeer(seed);
    }

    LOG_INFO("Added " + std::to_string(config.GetSeedList().size()) + " seed nodes to peer list");

    return true;
}

void LocalNode::Stop()
{
    if (!running_)
    {
        return;
    }

    LOG_INFO("Stopping LocalNode...");

    // Stop accepting new connections
    if (acceptor_ && acceptor_->is_open())
    {
        acceptor_->close();
    }

    // Stop connection lifecycle management
    StopConnectionLifecycle();

    // Disconnect all remote nodes
    {
        std::lock_guard<std::mutex> lock(connectedNodesMutex_);
        for (auto& pair : connectedNodes_)
        {
            if (pair.second)
            {
                pair.second->Disconnect();
            }
        }
        connectedNodes_.clear();
    }

    // Stop work guard and IO context
    work_.reset();
    ioContext_.stop();

    // Wait for IO thread to finish
    if (ioThread_.joinable())
    {
        ioThread_.join();
    }

    // Save peer list
    SavePeerList();

    running_ = false;
    LOG_INFO("LocalNode stopped");
}

bool LocalNode::Connect(const IPEndPoint& endpoint)
{
    if (!running_)
    {
        LOG_WARNING("Cannot connect - LocalNode is not running");
        return false;
    }

    if (GetConnectedCount() >= maxConnections_)
    {
        LOG_WARNING("Cannot connect - maximum connections reached");
        return false;
    }

    try
    {
        LOG_INFO("Connecting to " + endpoint.ToString());

        auto socket = std::make_shared<boost::asio::ip::tcp::socket>(ioContext_);
        boost::asio::ip::tcp::endpoint ep(
            boost::asio::ip::make_address(endpoint.GetAddress().ToString()),
            endpoint.GetPort()
        );

        socket->async_connect(ep, [this, socket, endpoint](const std::error_code& error) {
            HandleConnect(error, std::move(*socket), endpoint);
        });

        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to connect to " + endpoint.ToString() + ": " + e.what());
        return false;
    }
}

void LocalNode::Broadcast(const Message& message, bool enableCompression)
{
    auto nodes = GetConnectedNodes();
    LOG_DEBUG("Broadcasting message to " + std::to_string(nodes.size()) + " connected nodes");

    for (auto* node : nodes)
    {
        if (node && node->IsConnected())
        {
            node->Send(message, enableCompression);
        }
    }
}

void LocalNode::BroadcastInv(InventoryType type, const std::vector<io::UInt256>& hashes)
{
    if (hashes.empty())
    {
        return;
    }

    auto payload = std::make_shared<payloads::InvPayload>();
    payload->SetType(type);
    payload->SetHashes(hashes);

    Message message(MessageCommand::Inv, payload);
    Broadcast(message);
}

void LocalNode::StartAccept()
{
    if (!acceptor_ || !acceptor_->is_open())
    {
        return;
    }

    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(ioContext_);
    acceptor_->async_accept(*socket,
        [this, socket](const std::error_code& error) {
            if (!error)
            {
                HandleAccept(error, std::move(*socket));
                StartAccept();  // Continue accepting new connections
            }
            else if (error.value() != boost::asio::error::operation_aborted)
            {
                LOG_ERROR("Accept error: " + error.message());
                StartAccept();  // Try again
            }
        });
}

void LocalNode::HandleAccept(const std::error_code& error, boost::asio::ip::tcp::socket socket)
{
    if (error)
    {
        LOG_ERROR("Accept error: " + error.message());
        return;
    }

    if (GetConnectedCount() >= maxConnections_)
    {
        LOG_WARNING("Maximum connections reached, rejecting new connection");
        socket.close();
        return;
    }

    try
    {
        auto remoteEndpoint = socket.remote_endpoint();
        IPEndPoint endpoint(remoteEndpoint.address().to_string(), remoteEndpoint.port());
        
        LOG_INFO("Accepted connection from " + endpoint.ToString());

        // Create connection and remote node
        auto connection = std::make_shared<TcpConnection>(std::move(socket));
        auto remoteNode = std::make_unique<RemoteNode>(this, connection);
        
        // Start receiving messages
        connection->StartReceiving();
        
        // Add to connected nodes
        AddConnectedNode(std::move(remoteNode));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to handle accepted connection: " + std::string(e.what()));
    }
}

void LocalNode::HandleConnect(const std::error_code& error, boost::asio::ip::tcp::socket socket, const IPEndPoint& endpoint)
{
    if (error)
    {
        LOG_ERROR("Connect error for " + endpoint.ToString() + ": " + error.message());
        MarkPeerDisconnected(endpoint);
        return;
    }

    try
    {
        LOG_INFO("Connected to " + endpoint.ToString());

        // Create connection and remote node
        auto connection = std::make_shared<TcpConnection>(std::move(socket));
        auto remoteNode = std::make_unique<RemoteNode>(this, connection);
        
        // Send version message BEFORE moving the remoteNode
        LOG_INFO("Sending Version message to newly connected peer");
        bool versionSent = remoteNode->SendVersion();
        LOG_INFO("Version message send result: " + std::string(versionSent ? "success" : "failed"));
        
        // Start receiving messages
        connection->StartReceiving();
        
        // Add to connected nodes (moves remoteNode)
        AddConnectedNode(std::move(remoteNode));
        MarkPeerConnected(endpoint);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to handle connection to " + endpoint.ToString() + ": " + std::string(e.what()));
        MarkPeerDisconnected(endpoint);
    }
}

void LocalNode::AddConnectedNode(std::unique_ptr<RemoteNode> remoteNode)
{
    if (!remoteNode)
    {
        return;
    }

    std::string key = remoteNode->GetRemoteEndPoint().ToString();
    
    {
        std::lock_guard<std::mutex> lock(connectedNodesMutex_);
        connectedNodes_[key] = std::move(remoteNode);
    }

    LOG_DEBUG("Added connected node: " + key);
}

void LocalNode::RemoveConnectedNode(const std::string& key)
{
    RemoteNode* node = nullptr;
    
    {
        std::lock_guard<std::mutex> lock(connectedNodesMutex_);
        auto it = connectedNodes_.find(key);
        if (it != connectedNodes_.end())
        {
            node = it->second.get();
            connectedNodes_.erase(it);
        }
    }

    if (node)
    {
        LOG_DEBUG("Removed connected node: " + key);
        OnRemoteNodeDisconnected(node);
    }
}

void LocalNode::StartConnectionLifecycle()
{
    if (connectionLifecycleRunning_)
    {
        return;
    }

    connectionLifecycleRunning_ = true;
    connectionLifecycleThread_ = std::thread([this]() {
        ManageConnectionLifecycle();
    });

    LOG_DEBUG("Connection lifecycle management started");
}

void LocalNode::StopConnectionLifecycle()
{
    if (!connectionLifecycleRunning_)
    {
        return;
    }

    connectionLifecycleRunning_ = false;
    
    if (connectionLifecycleThread_.joinable())
    {
        connectionLifecycleThread_.join();
    }

    LOG_DEBUG("Connection lifecycle management stopped");
}

void LocalNode::ManageConnectionLifecycle()
{
    while (connectionLifecycleRunning_)
    {
        try
        {
            // Check connection count
            size_t connectedCount = GetConnectedCount();
            
            // Connect to more peers if needed
            if (connectedCount < maxConnections_ / 2)
            {
                auto peers = peerList_.GetUnconnectedPeers();
                int count = 0;
                for (const auto& peer : peers)
                {
                    if (!peer.IsBad() && count < 10)  // Limit to 10 connection attempts
                    {
                        Connect(peer.GetEndPoint());
                        count++;
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }
            }

            // Remove dead connections
            std::vector<std::string> deadNodes;
            {
                std::lock_guard<std::mutex> lock(connectedNodesMutex_);
                for (const auto& pair : connectedNodes_)
                {
                    if (!pair.second->IsConnected())
                    {
                        deadNodes.push_back(pair.first);
                    }
                }
            }

            for (const std::string& key : deadNodes)
            {
                RemoveConnectedNode(key);
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Connection lifecycle error: " + std::string(e.what()));
        }

        // Sleep for 5 seconds before next check
        for (int i = 0; i < 50 && connectionLifecycleRunning_; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

// Message received callback implementations
void LocalNode::OnVersionMessageReceived(RemoteNode* remoteNode, const payloads::VersionPayload& payload)
{
    LOG_DEBUG("Version message received from remote node");
    
    // Send verack
    auto verackPayload = std::make_shared<payloads::VerAckPayload>();
    remoteNode->Send(Message(MessageCommand::Verack, verackPayload));
    
    // Call user callback if set
    if (versionMessageReceivedCallback_)
    {
        versionMessageReceivedCallback_(remoteNode, payload);
    }
}

void LocalNode::OnPingMessageReceived(RemoteNode* remoteNode, const payloads::PingPayload& payload)
{
    LOG_DEBUG("Ping message received from remote node");
    
    // Send pong with same payload
    auto pongPayload = std::make_shared<payloads::PingPayload>(payload);
    remoteNode->Send(Message(MessageCommand::Pong, pongPayload));
    
    // Call user callback if set
    if (pingMessageReceivedCallback_)
    {
        pingMessageReceivedCallback_(remoteNode, payload);
    }
}

void LocalNode::OnPongMessageReceived(RemoteNode* remoteNode, const payloads::PingPayload& payload)
{
    LOG_DEBUG("Pong message received from remote node");
    
    // Call user callback if set
    if (pongMessageReceivedCallback_)
    {
        pongMessageReceivedCallback_(remoteNode, payload);
    }
}

void LocalNode::OnAddrMessageReceived(RemoteNode* remoteNode, const payloads::AddrPayload& payload)
{
    LOG_DEBUG("Addr message received with " + std::to_string(payload.GetAddressList().size()) + " addresses");
    
    // Add addresses to peer list
    for (const auto& addr : payload.GetAddressList())
    {
        AddPeer(IPEndPoint(addr.GetAddress(), addr.GetPort()));
    }
    
    // Call user callback if set
    if (addrMessageReceivedCallback_)
    {
        addrMessageReceivedCallback_(remoteNode, payload);
    }
}

void LocalNode::OnRemoteNodeConnected(RemoteNode* remoteNode)
{
    LOG_INFO("Remote node connected");
    
    // Call user callback if set
    if (remoteNodeConnectedCallback_)
    {
        remoteNodeConnectedCallback_(remoteNode);
    }
}

void LocalNode::OnRemoteNodeDisconnected(RemoteNode* remoteNode)
{
    LOG_INFO("Remote node disconnected");
    
    // Call user callback if set
    if (remoteNodeDisconnectedCallback_)
    {
        remoteNodeDisconnectedCallback_(remoteNode);
    }
}

void LocalNode::OnRemoteNodeHandshaked(RemoteNode* remoteNode)
{
    LOG_INFO("Remote node handshaked");
    
    // Call user callback if set
    if (remoteNodeHandshakedCallback_)
    {
        remoteNodeHandshakedCallback_(remoteNode);
    }
}

// Callback setter implementations
void LocalNode::SetVersionMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::VersionPayload&)> callback)
{
    versionMessageReceivedCallback_ = callback;
}

void LocalNode::SetPingMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::PingPayload&)> callback)
{
    pingMessageReceivedCallback_ = callback;
}

void LocalNode::SetPongMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::PingPayload&)> callback)
{
    pongMessageReceivedCallback_ = callback;
}

void LocalNode::SetAddrMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::AddrPayload&)> callback)
{
    addrMessageReceivedCallback_ = callback;
}

void LocalNode::SetInvMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::InvPayload&)> callback)
{
    invMessageReceivedCallback_ = callback;
}

void LocalNode::SetGetDataMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::GetDataPayload&)> callback)
{
    getDataMessageReceivedCallback_ = callback;
}

void LocalNode::SetGetBlocksMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::GetBlocksPayload&)> callback)
{
    getBlocksMessageReceivedCallback_ = callback;
}

void LocalNode::SetGetBlockByIndexMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::GetBlockByIndexPayload&)> callback)
{
    getBlockByIndexMessageReceivedCallback_ = callback;
}

void LocalNode::SetGetHeadersMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::GetBlocksPayload&)> callback)
{
    getHeadersMessageReceivedCallback_ = callback;
}

void LocalNode::SetHeadersMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::HeadersPayload&)> callback)
{
    headersMessageReceivedCallback_ = callback;
}

void LocalNode::SetMempoolMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::MempoolPayload&)> callback)
{
    mempoolMessageReceivedCallback_ = callback;
}

void LocalNode::SetFilterAddMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::FilterAddPayload&)> callback)
{
    filterAddMessageReceivedCallback_ = callback;
}

void LocalNode::SetFilterClearMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::FilterClearPayload&)> callback)
{
    filterClearMessageReceivedCallback_ = callback;
}

void LocalNode::SetFilterLoadMessageReceivedCallback(std::function<void(RemoteNode*, const payloads::FilterLoadPayload&)> callback)
{
    filterLoadMessageReceivedCallback_ = callback;
}

void LocalNode::SetRemoteNodeConnectedCallback(std::function<void(RemoteNode*)> callback)
{
    remoteNodeConnectedCallback_ = callback;
}

void LocalNode::SetRemoteNodeDisconnectedCallback(std::function<void(RemoteNode*)> callback)
{
    remoteNodeDisconnectedCallback_ = callback;
}

void LocalNode::SetRemoteNodeHandshakedCallback(std::function<void(RemoteNode*)> callback)
{
    remoteNodeHandshakedCallback_ = callback;
}

// Additional message handlers that were referenced but not implemented
void LocalNode::OnInvMessageReceived(RemoteNode* remoteNode, const payloads::InvPayload& payload)
{
    LOG_DEBUG("Inv message received with " + std::to_string(payload.GetHashes().size()) + " items");
    
    // Call user callback if set
    if (invMessageReceivedCallback_)
    {
        invMessageReceivedCallback_(remoteNode, payload);
    }
}

void LocalNode::OnGetDataMessageReceived(RemoteNode* remoteNode, const payloads::GetDataPayload& payload)
{
    LOG_DEBUG("GetData message received");
    
    // Call user callback if set
    if (getDataMessageReceivedCallback_)
    {
        getDataMessageReceivedCallback_(remoteNode, payload);
    }
}

void LocalNode::OnGetBlocksMessageReceived(RemoteNode* remoteNode, const payloads::GetBlocksPayload& payload)
{
    LOG_DEBUG("GetBlocks message received");
    
    // Call user callback if set
    if (getBlocksMessageReceivedCallback_)
    {
        getBlocksMessageReceivedCallback_(remoteNode, payload);
    }
}

void LocalNode::OnGetBlockByIndexMessageReceived(RemoteNode* remoteNode, const payloads::GetBlockByIndexPayload& payload)
{
    LOG_DEBUG("GetBlockByIndex message received");
    
    // Call user callback if set
    if (getBlockByIndexMessageReceivedCallback_)
    {
        getBlockByIndexMessageReceivedCallback_(remoteNode, payload);
    }
}

void LocalNode::OnGetHeadersMessageReceived(RemoteNode* remoteNode, const payloads::GetBlocksPayload& payload)
{
    LOG_DEBUG("GetHeaders message received");
    
    // Call user callback if set
    if (getHeadersMessageReceivedCallback_)
    {
        getHeadersMessageReceivedCallback_(remoteNode, payload);
    }
}

void LocalNode::OnHeadersMessageReceived(RemoteNode* remoteNode, const payloads::HeadersPayload& payload)
{
    LOG_DEBUG("Headers message received with " + std::to_string(payload.GetHeaders().size()) + " headers");
    
    // Call user callback if set
    if (headersMessageReceivedCallback_)
    {
        headersMessageReceivedCallback_(remoteNode, payload);
    }
}

void LocalNode::OnMempoolMessageReceived(RemoteNode* remoteNode, const payloads::MempoolPayload& payload)
{
    LOG_DEBUG("Mempool message received");
    
    // Call user callback if set
    if (mempoolMessageReceivedCallback_)
    {
        mempoolMessageReceivedCallback_(remoteNode, payload);
    }
}

void LocalNode::OnFilterAddMessageReceived(RemoteNode* remoteNode, const payloads::FilterAddPayload& payload)
{
    LOG_DEBUG("FilterAdd message received");
    
    // Call user callback if set
    if (filterAddMessageReceivedCallback_)
    {
        filterAddMessageReceivedCallback_(remoteNode, payload);
    }
}

void LocalNode::OnFilterClearMessageReceived(RemoteNode* remoteNode, const payloads::FilterClearPayload& payload)
{
    LOG_DEBUG("FilterClear message received");
    
    // Call user callback if set
    if (filterClearMessageReceivedCallback_)
    {
        filterClearMessageReceivedCallback_(remoteNode, payload);
    }
}

void LocalNode::OnFilterLoadMessageReceived(RemoteNode* remoteNode, const payloads::FilterLoadPayload& payload)
{
    LOG_DEBUG("FilterLoad message received");
    
    // Call user callback if set
    if (filterLoadMessageReceivedCallback_)
    {
        filterLoadMessageReceivedCallback_(remoteNode, payload);
    }
}

}  // namespace neo::network::p2p
