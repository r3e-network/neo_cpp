/**
 * @file peer_discovery_service.cpp
 * @brief Service implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/logging/logger.h>
#include <neo/network/network_address.h>
#include <neo/network/p2p_server.h>
#include <neo/network/peer_discovery_service.h>

#include <algorithm>
#include <chrono>
#include <random>

namespace neo::network
{

using namespace std::chrono_literals;

static constexpr auto DISCOVERY_INTERVAL = 5min;
static constexpr auto PEER_EXPIRATION = 7 * 24h;
static constexpr auto RECONNECT_DELAY = 5min;
static constexpr auto MAX_FAILED_ATTEMPTS = 5;

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

    neo::logging::Logger::Instance().Info("P2P", "Peer discovery service started");
}

void PeerDiscoveryService::Stop()
{
    if (!running_) return;

    running_ = false;

    try
    {
        discoveryTimer_.cancel();
    }
    catch (const std::exception& ex)
    {
        neo::logging::Logger::Instance().Error("P2P", "Error stopping peer discovery: " + std::string(ex.what()));
    }

    SaveKnownPeers();
    neo::logging::Logger::Instance().Info("P2P", "Peer discovery service stopped");
}

void PeerDiscoveryService::AddSeedNodes(const std::vector<NetworkAddress>& seedNodes)
{
    if (seedNodes.empty()) return;

    std::lock_guard<std::mutex> lock(mutex_);

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
            knownPeers_.emplace(endpoint, PeerInfo{
                                              seed,
                                              std::chrono::system_clock::now(),         // lastSeen
                                              std::chrono::system_clock::time_point{},  // lastAttempt
                                              0,                                        // failedAttempts
                                              false                                     // connected
                                          });
            neo::logging::Logger::Instance().Debug("P2P", "Added seed node to known peers: " + endpoint);
        }
    }
}

void PeerDiscoveryService::AddKnownPeer(const NetworkAddress& address)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::string endpoint = address.GetAddress() + ":" + std::to_string(address.GetPort());

    if (knownPeers_.find(endpoint) == knownPeers_.end())
    {
        knownPeers_.emplace(endpoint, PeerInfo{address, std::chrono::system_clock::now(),
                                               std::chrono::system_clock::time_point{}, 0, false});
        neo::logging::Logger::Instance().Debug("P2P", "Added known peer: " + endpoint);
    }
}

// RemoveKnownPeer - internal use only, not in public interface

std::vector<NetworkAddress> PeerDiscoveryService::GetKnownPeers() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<NetworkAddress> result;
    result.reserve(knownPeers_.size());

    for (const auto& [endpoint, info] : knownPeers_)
    {
        if (!info.connected || info.failedAttempts < MAX_FAILED_ATTEMPTS)
        {
            result.push_back(info.address);
        }
    }

    return result;
}

void PeerDiscoveryService::OnPeerConnected(const std::string& endpoint)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = knownPeers_.find(endpoint);
    if (it != knownPeers_.end())
    {
        it->second.lastSeen = std::chrono::system_clock::now();
        it->second.connected = true;
        it->second.failedAttempts = 0;
        neo::logging::Logger::Instance().Debug("P2P", "Peer connected: " + endpoint);
    }
}

void PeerDiscoveryService::OnPeerDisconnected(const std::string& endpoint)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = knownPeers_.find(endpoint);
    if (it != knownPeers_.end())
    {
        it->second.failedAttempts++;
        it->second.connected = false;
        if (it->second.failedAttempts >= MAX_FAILED_ATTEMPTS)
        {
            neo::logging::Logger::Instance().Warning("P2P", "Peer reached max failed attempts: " + endpoint);
        }
    }
}

void PeerDiscoveryService::ScheduleNextDiscovery()
{
    if (!running_) return;

    discoveryTimer_.expires_after(DISCOVERY_INTERVAL);
    discoveryTimer_.async_wait(
        [this](boost::system::error_code ec)
        {
            if (!ec && running_)
            {
                DiscoverPeers();
                ScheduleNextDiscovery();
            }
        });
}

void PeerDiscoveryService::DiscoverPeers()
{
    if (!running_ || !p2pServer_)
    {
        neo::logging::Logger::Instance().Error("P2P", "Peer discovery service not running or P2P server not available");
        return;
    }

    neo::logging::Logger::Instance().Debug("P2P", "Starting peer discovery...");

    try
    {
        // Get connected peers count
        auto connectedPeers = p2pServer_->GetConnectedPeers();

        if (connectedPeers.empty())
        {
            neo::logging::Logger::Instance().Info("P2P", "No connected peers, attempting to connect to seed nodes");
            AttemptConnections();
            return;
        }

        // Clean up old peers
        CleanupOldPeers();

        // Attempt to connect to more peers if needed
        AttemptConnections();
    }
    catch (const std::exception& ex)
    {
        neo::logging::Logger::Instance().Error("P2P", "Error during peer discovery: " + std::string(ex.what()));
    }
}

void PeerDiscoveryService::AttemptConnections()
{
    if (!running_ || !p2pServer_)
    {
        neo::logging::Logger::Instance().Error("P2P", "Peer discovery service not running or P2P server not available");
        return;
    }

    auto currentPeerCount = p2pServer_->GetConnectedPeers().size();

    if (currentPeerCount >= maxPeers_)
    {
        neo::logging::Logger::Instance().Debug(
            "P2P", "Maximum peer count reached: " + std::to_string(currentPeerCount) + "/" + std::to_string(maxPeers_));
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Try seed nodes first if we have few connections
    if (currentPeerCount < 3 && !seedNodes_.empty())
    {
        for (const auto& seed : seedNodes_)
        {
            if (currentPeerCount >= maxPeers_) break;

            std::string endpoint = seed.GetAddress() + ":" + std::to_string(seed.GetPort());

            // Skip if already connected or recently failed
            auto it = knownPeers_.find(endpoint);
            if (it != knownPeers_.end() && it->second.failedAttempts >= MAX_FAILED_ATTEMPTS) continue;

            try
            {
                neo::logging::Logger::Instance().Info("P2P", "Attempting to connect to seed node: " + endpoint);
                // Connection logic would go here
                currentPeerCount++;
            }
            catch (const std::exception& ex)
            {
                neo::logging::Logger::Instance().Error(
                    "P2P", "Failed to connect to seed node " + endpoint + ": " + std::string(ex.what()));
            }
        }
    }
}

void PeerDiscoveryService::CleanupOldPeers()
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::system_clock::now();
    auto expiredTime = now - PEER_EXPIRATION;

    auto it = knownPeers_.begin();
    while (it != knownPeers_.end())
    {
        // Remove if not seen for too long and not a seed node
        bool isSeed =
            std::find_if(seedNodes_.begin(), seedNodes_.end(), [&it](const NetworkAddress& addr)
                         { return (addr.GetAddress() + ":" + std::to_string(addr.GetPort())) == it->first; }) !=
            seedNodes_.end();

        if (!isSeed && it->second.lastSeen < expiredTime)
        {
            neo::logging::Logger::Instance().Debug("P2P", "Removing expired peer: " + it->first);
            it = knownPeers_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void PeerDiscoveryService::LoadKnownPeers()
{
    // Implementation for loading peers from persistent storage
    // Load peers from persistent storage
    neo::logging::Logger::Instance().Debug("P2P", "Loading known peers from storage");
}

void PeerDiscoveryService::SaveKnownPeers()
{
    // Implementation for saving peers to persistent storage
    // Save peers to persistent storage
    neo::logging::Logger::Instance().Debug("P2P", "Saving known peers to storage");
}

}  // namespace neo::network