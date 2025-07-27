#include <neo/core/logging.h>
#include <neo/network/p2p/local_node.h>

namespace neo::network::p2p
{
LocalNode::LocalNode(std::shared_ptr<NeoSystem> system)
    : system_(system), running_(false), nonce_(GenerateNonce()), user_agent_("NEO:3.0.0")
{
    LOG_INFO("LocalNode initialized");
}

LocalNode::~LocalNode()
{
    Stop();
}

void LocalNode::Start(const ChannelsConfig& config)
{
    if (running_.exchange(true))
        return;

    LOG_INFO("Starting LocalNode with config - TCP: {}, WS: {}", config.tcp_port, config.ws_port);

    tcp_port_ = config.tcp_port;
    ws_port_ = config.ws_port;

    // Start network threads
    network_thread_ = std::thread(&LocalNode::NetworkThread, this);

    LOG_INFO("LocalNode started");
}

void LocalNode::Stop()
{
    if (!running_.exchange(false))
        return;

    LOG_INFO("Stopping LocalNode");

    // Stop all connections
    {
        std::lock_guard<std::shared_mutex> lock(peers_mutex_);
        connected_peers_.clear();
    }

    // Wait for network thread
    if (network_thread_.joinable())
    {
        network_thread_.join();
    }

    LOG_INFO("LocalNode stopped");
}

bool LocalNode::AddPeer(std::shared_ptr<RemoteNode> peer)
{
    if (!peer)
        return false;

    std::lock_guard<std::shared_mutex> lock(peers_mutex_);

    auto endpoint = peer->GetEndpoint();
    if (connected_peers_.find(endpoint) != connected_peers_.end())
    {
        return false;  // Already connected
    }

    connected_peers_[endpoint] = peer;
    LOG_INFO("Added peer: {}", endpoint);

    return true;
}

void LocalNode::RemovePeer(const std::string& endpoint)
{
    std::lock_guard<std::shared_mutex> lock(peers_mutex_);

    auto it = connected_peers_.find(endpoint);
    if (it != connected_peers_.end())
    {
        connected_peers_.erase(it);
        LOG_INFO("Removed peer: {}", endpoint);
    }
}

std::shared_ptr<RemoteNode> LocalNode::GetPeer(const std::string& endpoint) const
{
    std::shared_lock<std::shared_mutex> lock(peers_mutex_);

    auto it = connected_peers_.find(endpoint);
    if (it != connected_peers_.end())
    {
        return it->second;
    }

    return nullptr;
}

std::vector<std::shared_ptr<RemoteNode>> LocalNode::GetPeers() const
{
    std::shared_lock<std::shared_mutex> lock(peers_mutex_);

    std::vector<std::shared_ptr<RemoteNode>> peers;
    for (const auto& [endpoint, peer] : connected_peers_)
    {
        peers.push_back(peer);
    }

    return peers;
}

size_t LocalNode::GetConnectedPeersCount() const
{
    std::shared_lock<std::shared_mutex> lock(peers_mutex_);
    return connected_peers_.size();
}

void LocalNode::Broadcast(std::shared_ptr<payloads::IInventory> inventory)
{
    if (!inventory)
        return;

    auto peers = GetPeers();
    for (const auto& peer : peers)
    {
        try
        {
            peer->SendInventory(inventory);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to broadcast to peer {}: {}", peer->GetEndpoint(), e.what());
        }
    }
}

void LocalNode::SendTo(const std::string& endpoint, std::shared_ptr<payloads::IInventory> inventory)
{
    if (!inventory)
        return;

    auto peer = GetPeer(endpoint);
    if (peer)
    {
        try
        {
            peer->SendInventory(inventory);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to send to peer {}: {}", endpoint, e.what());
        }
    }
}

uint32_t LocalNode::GenerateNonce()
{
    // Generate random nonce
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis;
    return dis(gen);
}

void LocalNode::NetworkThread()
{
    LOG_INFO("LocalNode network thread started");

    while (running_)
    {
        // Network processing logic would go here
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_INFO("LocalNode network thread stopped");
}
}  // namespace neo::network::p2p