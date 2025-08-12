#pragma once

#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/logging/logger.h>
#include <neo/network/network_address.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace neo::network
{

class P2PServer;

/**
 * @brief Represents a peer discovery service that manages known peers and connection attempts.
 */
class PeerDiscoveryService : public std::enable_shared_from_this<PeerDiscoveryService>
{
   public:
    /**
     * @brief Constructs a PeerDiscoveryService.
     * @param ioContext The IO context.
     * @param p2pServer The P2P server.
     * @param maxPeers The maximum number of peers to connect to.
     */
    PeerDiscoveryService(boost::asio::io_context& ioContext, std::shared_ptr<P2PServer> p2pServer, size_t maxPeers = 8);

    /**
     * @brief Starts the peer discovery service.
     */
    void Start();

    /**
     * @brief Stops the peer discovery service.
     */
    void Stop();

    /**
     * @brief Adds seed nodes to the peer discovery service.
     * @param seedNodes The seed nodes to add.
     */
    /**
     * @brief Adds seed nodes to the peer discovery service.
     * @param seedNodes The seed nodes to add.
     */
    void AddSeedNodes(const std::vector<NetworkAddress>& seedNodes);

    /**
     * @brief Adds a peer to the known peers list.
     * @param address The network address of the peer.
     */
    void AddKnownPeer(const NetworkAddress& address);

    /**
     * @brief Adds multiple peers to the known peers list.
     * @param addresses The list of network addresses to add.
     */
    void AddKnownPeers(const std::vector<NetworkAddress>& addresses);

    /**
     * @brief Gets the list of known peers.
     * @return The list of known peers.
     */
    std::vector<NetworkAddress> GetKnownPeers() const;

    /**
     * @brief Gets the list of connected peers.
     * @return The list of connected peer endpoints.
     */
    std::vector<std::string> GetConnectedPeers() const;

    /**
     * @brief Called when a peer is connected.
     * @param endpoint The peer endpoint.
     */
    void OnPeerConnected(const std::string& endpoint);

    /**
     * @brief Called when a peer is disconnected.
     * @param endpoint The peer endpoint.
     */
    void OnPeerDisconnected(const std::string& endpoint);

    /**
     * @brief Handles a GetAddr message from a peer.
     * @param peer The peer that sent the message.
     * @param payload The GetAddr payload.
     */
    void HandleGetAddrMessage(const std::shared_ptr<class P2PPeer>& peer,
                              const std::shared_ptr<class GetAddrPayload>& payload);

    /**
     * @brief Handles an Addr message from a peer.
     * @param peer The peer that sent the message.
     * @param payload The Addr payload.
     */
    void HandleAddrMessage(const std::shared_ptr<class P2PPeer>& peer,
                           const std::shared_ptr<class AddrPayload>& payload);

   private:
    struct PeerInfo
    {
        NetworkAddress address;
        std::chrono::system_clock::time_point lastSeen;
        std::chrono::system_clock::time_point lastAttempt;
        uint32_t failedAttempts{0};
        bool connected{false};
    };

    void DiscoverPeers();
    void AttemptConnections();

    /**
     * @brief Removes old and inactive peers from the known peers list.
     */
    void CleanupOldPeers();

    void ScheduleNextDiscovery();
    void SaveKnownPeers();
    void LoadKnownPeers();

    boost::asio::io_context& ioContext_;
    std::shared_ptr<P2PServer> p2pServer_;
    boost::asio::steady_timer discoveryTimer_;
    std::vector<NetworkAddress> seedNodes_;
    std::unordered_map<std::string, PeerInfo> knownPeers_;
    std::unordered_set<std::string> connectedPeers_;
    mutable std::mutex mutex_;
    size_t maxPeers_;
    bool running_{false};
    std::mt19937 rng_{std::random_device{}()};
};

}  // namespace neo::network
