#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <neo/io/byte_vector.h>
#include <neo/io/uint256.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/message.h>
#include <neo/network/p2p/local_node.h>
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
    return std::make_shared<payloads::VersionPayload>(
        payloads::VersionPayload::Create(0x4F454E, nonce_, userAgent_, capabilities_));
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

}  // namespace neo::network::p2p
