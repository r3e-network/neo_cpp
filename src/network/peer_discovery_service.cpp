#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/logging/logger.h>
#include <neo/network/message.h>
#include <neo/network/network_address.h>
#include <neo/network/p2p/message_command.h>
#include <neo/network/p2p_peer.h>
#include <neo/network/p2p_server.h>
#include <neo/network/payloads/addr_payload.h>
#include <neo/network/payloads/get_addr_payload.h>
#include <neo/network/peer_discovery_service.h>

#include <algorithm>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <chrono>
#include <fstream>
#include <random>
#include <sstream>

namespace neo::network
{

using namespace std::chrono_literals;

static constexpr auto DISCOVERY_INTERVAL = 5min;
static constexpr auto PEER_EXPIRATION = 7 * 24h;  // 7 days
static constexpr auto RECONNECT_DELAY = 5min;
static constexpr auto MAX_FAILED_ATTEMPTS = 5;
static constexpr auto MAX_ADDRS_TO_SEND = 1000;

PeerDiscoveryService::PeerDiscoveryService(boost::asio::io_context& ioContext, std::shared_ptr<P2PServer> p2pServer,
                                           size_t maxPeers)
    : ioContext_(ioContext), p2pServer_(std::move(p2pServer)), discoveryTimer_(ioContext), maxPeers_(maxPeers)
{
    if (!p2pServer_)
    {
        throw std::invalid_argument("P2PServer cannot be null");
    }
}

void PeerDiscoveryService::Start()
{
    if (running_) return;

    running_ = true;
    LoadKnownPeers();
    ScheduleNextDiscovery();

    NEO_LOG(INFO, "P2P") << "Peer discovery service started";
}

void PeerDiscoveryService::Stop()
{
    if (!running_) return;

    running_ = false;
    boost::system::error_code ec;
    discoveryTimer_.cancel(ec);

    if (ec)
    {
        NEO_LOG(ERROR, "P2P") << "Error stopping peer discovery: " << ec.message();
    }

    SaveKnownPeers();
    NEO_LOG(INFO, "P2P") << "Peer discovery service stopped";
}

void PeerDiscoveryService::AddSeedNodes(const std::vector<NetworkAddress>& seedNodes)
{
    if (seedNodes.empty()) return;

    std::lock_guard<std::mutex> lock(mutex_);

    // Add to seed nodes if not already present
    for (const auto& seed : seedNodes)
    {
        std::string endpoint = seed.GetAddress() + ":" + std::to_string(seed.GetPort());

        // Check if already in seed nodes
        if (std::find_if(seedNodes_.begin(), seedNodes_.end(), [&seed](const NetworkAddress& addr)
                         { return addr.GetAddress() == seed.GetAddress() && addr.GetPort() == seed.GetPort(); }) ==
            seedNodes_.end())
        {
            seedNodes_.push_back(seed);
        }

        // Add to known peers if not already present
        if (knownPeers_.find(endpoint) == knownPeers_.end())
        {
            knownPeers_.emplace(endpoint, PeerInfo{seed, std::chrono::system_clock::now(),
                                                   std::chrono::system_clock::time_point{}, 0, false});
            NEO_LOG(DEBUG, "P2P") << "Added seed node to known peers: " << endpoint;
        }
    }
}

void PeerDiscoveryService::AddKnownPeer(const NetworkAddress& address)
{
    if (address.GetAddress().empty() || address.GetPort() == 0)
    {
        NEO_LOG(WARNING, "P2P") << "Attempted to add invalid peer address";
        return;
    }

    std::string endpoint = address.GetAddress() + ":" + std::to_string(address.GetPort());

    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    auto it = knownPeers_.find(endpoint);

    if (it == knownPeers_.end())
    {
        // Add new peer
        knownPeers_.emplace(endpoint, PeerInfo{address, now, std::chrono::system_clock::time_point{}, 0, false});
        NEO_LOG(DEBUG, "P2P") << "Added new peer to known peers: " << endpoint;
    }
    else
    {
        // Update existing peer
        it->second.address = address;
        it->second.lastSeen = now;

        // Reset failed attempts if this is a new sighting
        if (it->second.failedAttempts > 0)
        {
            NEO_LOG(DEBUG, "P2P") << "Resetting failed attempts for peer: " << endpoint;
            it->second.failedAttempts = 0;
        }
    }

    // Save known peers periodically
    static size_t saveCounter = 0;
    if (++saveCounter % 10 == 0)
    {
        SaveKnownPeers();
    }
}

void PeerDiscoveryService::AddKnownPeers(const std::vector<NetworkAddress>& addresses)
{
    if (addresses.empty()) return;

    size_t added = 0;
    for (const auto& addr : addresses)
    {
        std::string endpoint = addr.GetAddress() + ":" + std::to_string(addr.GetPort());

        // Skip invalid addresses
        if (addr.GetAddress().empty() || addr.GetPort() == 0)
        {
            continue;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();
        auto it = knownPeers_.find(endpoint);

        if (it == knownPeers_.end())
        {
            knownPeers_.emplace(endpoint, PeerInfo{addr, now, std::chrono::system_clock::time_point{}, 0, false});
            added++;
        }
        else
        {
            // Update last seen time
            it->second.lastSeen = now;
        }
    }

    if (added > 0)
    {
        NEO_LOG(DEBUG, "P2P") << "Added " << added << " new peers from address message";
        SaveKnownPeers();
    }
}

std::vector<NetworkAddress> PeerDiscoveryService::GetKnownPeers() const
{
    std::vector<NetworkAddress> result;
    std::lock_guard<std::mutex> lock(mutex_);

    result.reserve(knownPeers_.size());
    for (const auto& [endpoint, info] : knownPeers_)
    {
        result.push_back(info.address);
    }

    return result;
}

std::vector<std::string> PeerDiscoveryService::GetConnectedPeers() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return {connectedPeers_.begin(), connectedPeers_.end()};
}

void PeerDiscoveryService::DiscoverPeers()
{
    if (!running_ || !p2pServer_)
    {
        NEO_LOG(ERROR, "P2P") << "Peer discovery service not running or P2P server not available";
        return;
    }

    NEO_LOG(DEBUG, "P2P") << "Starting peer discovery...";

    try
    {
        // Request addresses from connected peers
        auto connectedPeers = p2pServer_->GetConnectedPeers();

        if (connectedPeers.empty())
        {
            NEO_LOG(INFO, "P2P") << "No connected peers, attempting to connect to seed nodes";
            AttemptConnections();
            return;
        }

        // Request addresses from random connected peers
        std::vector<std::shared_ptr<P2PPeer>> peers(connectedPeers.begin(), connectedPeers.end());
        std::shuffle(peers.begin(), peers.end(), rng_);

        size_t requests = std::min<size_t>(3, peers.size());
        size_t successCount = 0;

        for (size_t i = 0; i < requests; ++i)
        {
            try
            {
                auto peer = peers[i];
                if (!peer || !peer->IsConnected()) continue;

                // Create and send getaddr message
                auto getAddrPayload = std::make_shared<GetAddrPayload>();
                Message getAddrMsg(p2p::MessageCommand::GetAddr, getAddrPayload);
                peer->Send(getAddrMsg);

                NEO_LOG(DEBUG, "P2P") << "Requested addresses from peer: "
                                      << peer->GetConnection()->GetRemoteEndpoint().ToString();
                successCount++;
            }
            catch (const std::exception& ex)
            {
                NEO_LOG(ERROR, "P2P") << "Error requesting addresses from peer: " << ex.what();
            }
        }

        if (successCount == 0)
        {
            NEO_LOG(WARNING, "P2P") << "Failed to request addresses from any peers";
        }

        // Clean up old peers
        CleanupOldPeers();

        // Attempt to connect to more peers if needed
        AttemptConnections();
    }
    catch (const std::exception& ex)
    {
        NEO_LOG(ERROR, "P2P") << "Error in DiscoverPeers: " << ex.what();
    }

    // Schedule next discovery
    ScheduleNextDiscovery();
}

void PeerDiscoveryService::AttemptConnections()
{
    if (!running_ || !p2pServer_)
    {
        NEO_LOG(ERROR, "P2P") << "Peer discovery service not running or P2P server not available";
        return;
    }

    // Get current number of connected peers
    size_t connectedCount;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connectedCount = connectedPeers_.size();
    }

    if (connectedCount >= maxPeers_)
    {
        NEO_LOG(DEBUG, "P2P") << "Already connected to " << connectedCount << " peers, not connecting to more";
        return;
    }

    // Calculate how many more peers we need
    size_t needed = maxPeers_ - connectedCount;
    NEO_LOG(DEBUG, "P2P") << "Attempting to connect to " << needed << " more peers";

    // Get candidate peers to connect to
    std::vector<std::pair<std::string, NetworkAddress>> candidates;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();

        // First, try seed nodes if we don't have enough connections
        if (connectedCount == 0 && !seedNodes_.empty())
        {
            for (const auto& seed : seedNodes_)
            {
                std::string endpoint = seed.GetAddress() + ":" + std::to_string(seed.GetPort());
                if (connectedPeers_.find(endpoint) == connectedPeers_.end())
                {
                    candidates.emplace_back(endpoint, seed);
                }
            }
        }

        // Then try known peers
        if (candidates.empty())
        {
            for (const auto& [endpoint, info] : knownPeers_)
            {
                // Skip if already connected
                if (connectedPeers_.find(endpoint) != connectedPeers_.end()) continue;

                // Skip if we recently tried and failed
                if (info.failedAttempts > 0)
                {
                    auto backoff = RECONNECT_DELAY * (1 << std::min<size_t>(info.failedAttempts, 8));
                    if (now - info.lastAttempt < backoff) continue;
                }

                candidates.emplace_back(endpoint, info.address);
            }
        }

        // Sort by last seen (newest first) and then by failed attempts (fewest first)
        std::sort(candidates.begin(), candidates.end(),
                  [](const auto& a, const auto& b)
                  {
                      // Prefer peers we've seen more recently
                      if (a.second.GetTimestamp() != b.second.GetTimestamp())
                          return a.second.GetTimestamp() > b.second.GetTimestamp();
                      // Then prefer peers with fewer failed attempts
                      return a.second.GetServices() < b.second.GetServices();
                  });

        // Limit the number of candidates to process
        candidates.resize(std::min<size_t>(needed * 2, candidates.size()));
    }

    if (candidates.empty())
    {
        NEO_LOG(WARNING, "P2P") << "No suitable peers found to connect to";
        return;
    }

    NEO_LOG(DEBUG, "P2P") << "Found " << candidates.size() << " candidate peers to connect to";
    // Shuffle candidates
    std::shuffle(candidates.begin(), candidates.end(), rng_);

    // Try to connect to candidates
    size_t connectionsToMake = std::min(candidates.size(), maxPeers_ - connectedPeers_.size());

    for (size_t i = 0; i < connectionsToMake; ++i)
    {
        const auto& [endpoint, address] = candidates[i];

        // Update last attempt time
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = knownPeers_.find(endpoint);
            if (it != knownPeers_.end())
            {
                it->second.lastAttempt = now;
            }
        }

        // Try to connect
        NEO_LOG(DEBUG, "P2P") << "Attempting to connect to peer: " << endpoint;

        try
        {
            // Create endpoint
            auto address = boost::asio::ip::make_address(address.GetAddress());
            auto port = address.GetPort();
            IPEndPoint peerEndpoint(address, port);

            // Connect to peer
            auto peer = p2pServer_->ConnectToPeer(peerEndpoint);

            // If we get here, connection was successful
            OnPeerConnected(endpoint);
        }
        catch (const std::exception& ex)
        {
            NEO_LOG(WARNING, "P2P") << "Failed to connect to peer " << endpoint << ": " << ex.what();

            // Update failed attempts
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = knownPeers_.find(endpoint);
            if (it != knownPeers_.end())
            {
                it->second.failedAttempts++;
            }
        }
    }
}

void PeerDiscoveryService::OnPeerConnected(const std::string& endpoint)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = knownPeers_.find(endpoint);
    if (it != knownPeers_.end())
    {
        it->second.connected = true;
        it->second.lastSeen = std::chrono::system_clock::now();
        it->second.failedAttempts = 0;
    }
    connectedPeers_.insert(endpoint);
    NEO_LOG(INFO, "P2P") << "Connected to peer: " << endpoint;
}

void PeerDiscoveryService::OnPeerDisconnected(const std::string& endpoint)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = knownPeers_.find(endpoint);
    if (it != knownPeers_.end())
    {
        it->second.connected = false;
    }
    connectedPeers_.erase(endpoint);
    NEO_LOG(INFO, "P2P") << "Disconnected from peer: " << endpoint;
}

std::string PeerDiscoveryService::GetAddressString(const NetworkAddress& address)
{
    return address.GetAddress() + ":" + std::to_string(address.GetPort());
}

std::string PeerDiscoveryService::GetEndpointString(const IPEndPoint& endpoint)
{
    return endpoint.GetAddress().ToString() + ":" + std::to_string(endpoint.GetPort());
}

void PeerDiscoveryService::HandleGetAddrMessage(const std::shared_ptr<P2PPeer>& peer,
                                                const std::shared_ptr<GetAddrPayload>& /*payload*/)
{
    if (!peer || !p2pServer_)
    {
        NEO_LOG(WARNING, "P2P") << "Invalid peer or P2P server in HandleGetAddrMessage";
        return;
    }

    NEO_LOG(DEBUG, "P2P") << "Processing GetAddr request from peer: " << peer->GetUserAgent();

    std::vector<NetworkAddress> addresses;

    // Get connected peers (excluding the requesting peer)
    auto connectedPeers = p2pServer_->GetConnectedPeers();
    for (const auto& connectedPeer : connectedPeers)
    {
        if (connectedPeer && connectedPeer != peer && connectedPeer->IsConnected())
        {
            auto endpoint = connectedPeer->GetConnection()->GetRemoteEndpoint();
            addresses.emplace_back(
                std::chrono::duration_cast<std::chrono::seconds>(connectedPeer->GetLastSeen().time_since_epoch())
                    .count(),
                connectedPeer->GetServices(), endpoint.GetAddress().ToString(), endpoint.GetPort());
        }
    }

    // Add known peers (up to a limit)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t count = 0;

        for (const auto& [endpoint, peerInfo] : knownPeers_)
        {
            if (count >= MAX_ADDRS_TO_SEND) break;
            if (!peerInfo.connected)  // Don't include already connected peers (they're already included above)
            {
                addresses.push_back(peerInfo.address);
                count++;
            }
        }
    }

    // If we have no addresses to send, don't send an empty Addr message
    if (addresses.empty())
    {
        NEO_LOG(DEBUG, "P2P") << "No addresses to send in response to GetAddr";
        return;
    }

    // Limit the number of addresses to send
    if (addresses.size() > MAX_ADDRS_TO_SEND)
    {
        // Randomly shuffle and trim to stay under the limit
        std::shuffle(addresses.begin(), addresses.end(), rng_);
        addresses.resize(MAX_ADDRS_TO_SEND);
    }

    // Create and send the Addr message
    try
    {
        auto addrPayload = std::make_shared<AddrPayload>(addresses);
        Message addrMessage(MessageCommand::Addr, addrPayload);
        peer->Send(addrMessage);

        NEO_LOG(DEBUG, "P2P") << "Sent " << addresses.size() << " addresses to peer: " << peer->GetUserAgent();
    }
    catch (const std::exception& ex)
    {
        NEO_LOG(ERROR, "P2P") << "Failed to send Addr message to peer " << peer->GetUserAgent() << ": " << ex.what();
        peer->Disconnect();
    }
}

void PeerDiscoveryService::HandleAddrMessage(const std::shared_ptr<P2PPeer>& peer,
                                             const std::shared_ptr<AddrPayload>& payload)
{
    if (!peer || !payload || !p2pServer_)
    {
        NEO_LOG(WARNING, "P2P") << "Invalid peer, payload, or P2P server in HandleAddrMessage";
        return;
    }

    const auto& addresses = payload->GetAddresses();
    if (addresses.empty())
    {
        NEO_LOG(DEBUG, "P2P") << "Received empty Addr message from peer: " << peer->GetUserAgent();
        return;
    }

    NEO_LOG(DEBUG, "P2P") << "Processing Addr message with " << addresses.size()
                          << " addresses from peer: " << peer->GetUserAgent();

    // Process the received addresses
    std::vector<NetworkAddress> validAddresses;
    for (const auto& addr : addresses)
    {
        // Basic validation
        if (addr.GetAddress().empty() || addr.GetPort() == 0)
        {
            NEO_LOG(DEBUG, "P2P") << "Skipping invalid address from peer: " << peer->GetUserAgent();
            continue;
        }

        // Skip our own address
        auto endpoint = p2pServer_->GetEndpoint();
        if (addr.GetAddress() == endpoint.GetAddress().ToString() && addr.GetPort() == endpoint.GetPort())
        {
            NEO_LOG(DEBUG, "P2P") << "Skipping our own address: " << addr.GetAddress() << ":" << addr.GetPort();
            continue;
        }

        // Only consider nodes with the services we support
        if ((addr.GetServices() & p2pServer_->GetServices()) != 0)
        {
            validAddresses.push_back(addr);
            NEO_LOG(DEBUG, "P2P") << "Discovered peer: " << addr.GetAddress() << ":" << addr.GetPort()
                                  << " (services: 0x" << std::hex << addr.GetServices() << std::dec << ")";
        }
    }

    // Add valid addresses to our known peers
    if (!validAddresses.empty())
    {
        AddKnownPeers(validAddresses);
        NEO_LOG(INFO, "P2P") << "Added " << validAddresses.size() << " new peers from Addr message";
    }
    else
    {
        NEO_LOG(DEBUG, "P2P") << "No valid addresses found in Addr message from peer: " << peer->GetUserAgent();
    }
}

void PeerDiscoveryService::CleanupOldPeers()
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    size_t removed = 0;

    for (auto it = knownPeers_.begin(); it != knownPeers_.end();)
    {
        const auto& peerInfo = it->second;
        bool shouldRemove = false;

        // Remove peers we haven't seen in a while and aren't connected to
        if (!peerInfo.connected && now - peerInfo.lastSeen > PEER_EXPIRATION &&
            now - peerInfo.lastAttempt > RECONNECT_DELAY * (1 << std::min<size_t>(peerInfo.failedAttempts, 8)))
        {
            shouldRemove = true;
        }
        // Remove peers with too many failed attempts
        else if (peerInfo.failedAttempts > MAX_FAILED_ATTEMPTS)
        {
            shouldRemove = true;
        }

        if (shouldRemove)
        {
            NEO_LOG(DEBUG, "P2P") << "Removing stale peer: " << it->first << " (last seen: "
                                  << std::chrono::duration_cast<std::chrono::hours>(now - peerInfo.lastSeen).count()
                                  << "h ago, "
                                  << "failed attempts: " << peerInfo.failedAttempts << ")";
            it = knownPeers_.erase(it);
            removed++;
        }
        else
        {
            ++it;
        }
    }

    if (removed > 0)
    {
        NEO_LOG(INFO, "P2P") << "Removed " << removed << " stale peers";
        SaveKnownPeers();
    }
}

void PeerDiscoveryService::ScheduleNextDiscovery()
{
    if (!running_) return;

    discoveryTimer_.expires_after(DISCOVERY_INTERVAL);
    discoveryTimer_.async_wait(
        [self = shared_from_this()](const boost::system::error_code& ec)
        {
            if (!ec)
            {
                self->DiscoverPeers();
            }
            else if (ec != boost::asio::error::operation_aborted)
            {
                NEO_LOG(ERROR, "P2P") << "Error in discovery timer: " << ec.message();
            }
        });
}

void PeerDiscoveryService::SaveKnownPeers()
{
    try
    {
        std::string filename = "peers.dat";
        std::ofstream file(filename, std::ios::binary);

        if (!file)
        {
            NEO_LOG(ERROR, "P2P") << "Failed to open " << filename << " for writing";
            return;
        }

        io::BinaryWriter writer(file);
        std::lock_guard<std::mutex> lock(mutex_);

        // Write number of peers
        writer.WriteVarInt(knownPeers_.size());

        // Write each peer
        for (const auto& [endpoint, info] : knownPeers_)
        {
            info.address.Serialize(writer);
            writer.Write(info.lastSeen.time_since_epoch().count());
            writer.Write(info.failedAttempts);
        }

        NEO_LOG(INFO, "P2P") << "Saved " << knownPeers_.size() << " known peers to disk";
    }
    catch (const std::exception& ex)
    {
        NEO_LOG(ERROR, "P2P") << "Failed to save known peers: " << ex.what();
    }
}

void PeerDiscoveryService::LoadKnownPeers()
{
    try
    {
        std::string filename = "peers.dat";
        std::ifstream file(filename, std::ios::binary);

        if (!file)
        {
            NEO_LOG(INFO, "P2P") << "No existing peer database found";
            return;
        }

        io::BinaryReader reader(file);

        // Read number of peers
        size_t count = reader.ReadVarInt<size_t>();

        // Read each peer
        for (size_t i = 0; i < count; ++i)
        {
            NetworkAddress addr;
            addr.Deserialize(reader);

            std::chrono::system_clock::time_point lastSeen{std::chrono::system_clock::duration{reader.Read<int64_t>()}};

            uint32_t failedAttempts = reader.Read<uint32_t>();

            std::string endpoint = addr.GetAddress() + ":" + std::to_string(addr.GetPort());
            knownPeers_.emplace(endpoint, PeerInfo{std::move(addr), lastSeen, {}, failedAttempts, false});
        }

        NEO_LOG(INFO, "P2P") << "Loaded " << count << " known peers from disk";
    }
    catch (const std::exception& ex)
    {
        NEO_LOG(ERROR, "P2P") << "Failed to load known peers: " << ex.what();
    }
}

}  // namespace neo::network
