#pragma once

#include <neo/core/logging.h>
#include <neo/network/p2p/remote_node.h>
#include <neo/network/p2p/network_address.h>
#include <neo/io/uint256.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <chrono>
#include <random>

namespace neo::network::p2p
{
    /**
     * @brief Peer statistics
     */
    struct PeerStats
    {
        std::chrono::steady_clock::time_point connected_time;
        std::chrono::steady_clock::time_point last_seen;
        uint64_t bytes_sent{0};
        uint64_t bytes_received{0};
        uint64_t messages_sent{0};
        uint64_t messages_received{0};
        uint32_t misbehavior_score{0};
        double latency_ms{0.0};
        uint32_t failed_attempts{0};
    };

    /**
     * @brief Peer quality score calculation
     */
    enum class PeerQuality
    {
        Excellent,
        Good,
        Fair,
        Poor,
        Banned
    };

    /**
     * @brief Production-ready peer manager for P2P networking
     */
    class PeerManager
    {
    public:
        struct Config
        {
            size_t max_peers{50};
            size_t max_peers_per_ip{3};
            size_t target_outbound_peers{8};
            size_t max_banned_peers{1000};
            std::chrono::seconds peer_timeout{30};
            std::chrono::seconds retry_interval{60};
            std::chrono::seconds ban_duration{3600};
            uint32_t max_misbehavior_score{100};
            bool enable_peer_discovery{true};
            std::vector<NetworkAddress> seed_nodes;
        };

    private:
        Config config_;
        std::shared_ptr<core::Logger> logger_;
        mutable std::mutex mutex_;
        
        // Active peers
        std::unordered_map<io::UInt256, std::shared_ptr<RemoteNode>> peers_;
        std::unordered_map<std::string, std::vector<io::UInt256>> peers_by_ip_;
        
        // Peer statistics
        std::unordered_map<io::UInt256, PeerStats> peer_stats_;
        
        // Known addresses
        std::vector<NetworkAddress> known_addresses_;
        std::unordered_set<NetworkAddress> tried_addresses_;
        
        // Banned peers
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> banned_ips_;
        std::unordered_set<io::UInt256> banned_peers_;
        
        // Random generator for peer selection
        std::mt19937 rng_{std::random_device{}()};
        
    public:
        explicit PeerManager(const Config& config);
        
        /**
         * @brief Add a new peer
         * @return true if peer was added successfully
         */
        bool AddPeer(std::shared_ptr<RemoteNode> peer);
        
        /**
         * @brief Remove a peer
         */
        void RemovePeer(const io::UInt256& peer_id);
        
        /**
         * @brief Get peer by ID
         */
        std::shared_ptr<RemoteNode> GetPeer(const io::UInt256& peer_id) const;
        
        /**
         * @brief Get all connected peers
         */
        std::vector<std::shared_ptr<RemoteNode>> GetConnectedPeers() const;
        
        /**
         * @brief Get peers for broadcasting
         * @param count Maximum number of peers
         * @return Selected peers for broadcast
         */
        std::vector<std::shared_ptr<RemoteNode>> GetBroadcastPeers(size_t count) const;
        
        /**
         * @brief Get peer count
         */
        size_t GetPeerCount() const;
        
        /**
         * @brief Get outbound peer count
         */
        size_t GetOutboundPeerCount() const;
        
        /**
         * @brief Check if we need more peers
         */
        bool NeedMorePeers() const;
        
        /**
         * @brief Get addresses for new connections
         * @param count Number of addresses to get
         * @return Addresses to try connecting to
         */
        std::vector<NetworkAddress> GetAddressesToConnect(size_t count);
        
        /**
         * @brief Add known addresses from peer discovery
         */
        void AddKnownAddresses(const std::vector<NetworkAddress>& addresses);
        
        /**
         * @brief Mark address as tried
         */
        void MarkAddressTried(const NetworkAddress& address);
        
        /**
         * @brief Update peer statistics
         */
        void UpdatePeerStats(const io::UInt256& peer_id, 
                           uint64_t bytes_sent, 
                           uint64_t bytes_received,
                           double latency_ms);
        
        /**
         * @brief Report peer misbehavior
         * @param peer_id Peer ID
         * @param score Misbehavior score to add
         * @param reason Reason for misbehavior
         */
        void ReportMisbehavior(const io::UInt256& peer_id, uint32_t score, const std::string& reason);
        
        /**
         * @brief Ban an IP address
         */
        void BanIP(const std::string& ip_address, std::chrono::seconds duration = std::chrono::seconds(0));
        
        /**
         * @brief Check if IP is banned
         */
        bool IsIPBanned(const std::string& ip_address) const;
        
        /**
         * @brief Unban an IP address
         */
        void UnbanIP(const std::string& ip_address);
        
        /**
         * @brief Clean up disconnected peers and expired bans
         */
        void Cleanup();
        
        /**
         * @brief Get peer quality score
         */
        PeerQuality GetPeerQuality(const io::UInt256& peer_id) const;
        
        /**
         * @brief Export peer statistics
         */
        json::JObject ExportStatistics() const;
        
        /**
         * @brief Save peer addresses to file
         */
        bool SavePeerAddresses(const std::string& filepath) const;
        
        /**
         * @brief Load peer addresses from file
         */
        bool LoadPeerAddresses(const std::string& filepath);
        
    private:
        /**
         * @brief Check if we can accept more peers from this IP
         */
        bool CanAcceptFromIP(const std::string& ip_address) const;
        
        /**
         * @brief Select random addresses from known pool
         */
        std::vector<NetworkAddress> SelectRandomAddresses(size_t count);
        
        /**
         * @brief Score an address for connection priority
         */
        double ScoreAddress(const NetworkAddress& address) const;
        
        /**
         * @brief Update last seen time for peer
         */
        void UpdateLastSeen(const io::UInt256& peer_id);
        
        /**
         * @brief Check and enforce peer limits
         */
        void EnforcePeerLimits();
        
        /**
         * @brief Calculate peer score based on statistics
         */
        double CalculatePeerScore(const PeerStats& stats) const;
    };

    /**
     * @brief Peer discovery service
     */
    class PeerDiscovery
    {
    private:
        std::shared_ptr<PeerManager> peer_manager_;
        std::shared_ptr<core::Logger> logger_;
        std::thread discovery_thread_;
        std::atomic<bool> running_{false};
        std::chrono::seconds discovery_interval_{300}; // 5 minutes
        
    public:
        PeerDiscovery(std::shared_ptr<PeerManager> peer_manager);
        ~PeerDiscovery();
        
        /**
         * @brief Start peer discovery
         */
        void Start();
        
        /**
         * @brief Stop peer discovery
         */
        void Stop();
        
        /**
         * @brief Request addresses from peers
         */
        void RequestAddresses();
        
        /**
         * @brief Process received addresses
         */
        void ProcessReceivedAddresses(const std::vector<NetworkAddress>& addresses);
        
    private:
        /**
         * @brief Discovery thread main loop
         */
        void DiscoveryLoop();
        
        /**
         * @brief Query DNS seeds
         */
        std::vector<NetworkAddress> QueryDNSSeeds();
        
        /**
         * @brief Filter and validate addresses
         */
        std::vector<NetworkAddress> FilterAddresses(const std::vector<NetworkAddress>& addresses);
    };
}