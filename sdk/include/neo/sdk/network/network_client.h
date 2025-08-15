#pragma once

/**
 * @file network_client.h
 * @brief P2P network client for Neo blockchain
 * @author Neo C++ Team
 * @date 2025
 */

#include <neo/sdk/core/types.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace neo::sdk::network {

/**
 * @brief Network configuration
 */
struct NetworkConfig {
    uint32_t magic = 0x4F454E;  // MainNet by default
    std::vector<std::string> seedList;
    uint16_t port = 10333;
    uint32_t maxConnections = 50;
    uint32_t minConnections = 10;
    std::chrono::seconds connectionTimeout{10};
    std::chrono::seconds pingInterval{30};
};

/**
 * @brief Peer information
 */
struct PeerInfo {
    std::string address;
    uint16_t port;
    uint32_t version;
    std::string userAgent;
    uint32_t startHeight;
    uint32_t lastBlockIndex;
    std::chrono::system_clock::time_point connectedTime;
    bool isOutbound;
};

/**
 * @brief Network message types
 */
enum class MessageType {
    Version,
    Verack,
    GetAddr,
    Addr,
    GetHeaders,
    Headers,
    GetBlocks,
    Block,
    Inv,
    GetData,
    Transaction,
    Mempool,
    Ping,
    Pong,
    Alert,
    Consensus
};

/**
 * @brief Network event callbacks
 */
struct NetworkCallbacks {
    std::function<void(const PeerInfo&)> onPeerConnected;
    std::function<void(const PeerInfo&)> onPeerDisconnected;
    std::function<void(const core::Block&)> onBlockReceived;
    std::function<void(const core::Transaction&)> onTransactionReceived;
    std::function<void(const std::string&)> onError;
};

/**
 * @brief P2P network client for Neo blockchain
 */
class NetworkClient {
public:
    /**
     * @brief Constructor with network configuration
     * @param config Network configuration
     */
    explicit NetworkClient(const NetworkConfig& config = {});

    /**
     * @brief Destructor
     */
    ~NetworkClient();

    /**
     * @brief Connect to the network
     * @return true if connection successful
     */
    bool Connect();

    /**
     * @brief Disconnect from the network
     */
    void Disconnect();

    /**
     * @brief Check if connected to network
     * @return true if connected
     */
    bool IsConnected() const;

    /**
     * @brief Get list of connected peers
     * @return Vector of peer information
     */
    std::vector<PeerInfo> GetPeers() const;

    /**
     * @brief Get peer count
     * @return Number of connected peers
     */
    size_t GetPeerCount() const;

    /**
     * @brief Broadcast transaction to network
     * @param transaction Transaction to broadcast
     * @return true if broadcast successful
     */
    bool BroadcastTransaction(const core::Transaction& transaction);

    /**
     * @brief Request block from network
     * @param hash Block hash
     * @param callback Callback when block received
     */
    void RequestBlock(
        const core::UInt256& hash,
        std::function<void(const core::Block&)> callback
    );

    /**
     * @brief Request blocks from network
     * @param start Start block index
     * @param count Number of blocks
     * @param callback Callback for each block
     */
    void RequestBlocks(
        uint32_t start,
        uint32_t count,
        std::function<void(const core::Block&)> callback
    );

    /**
     * @brief Subscribe to network events
     * @param callbacks Event callbacks
     */
    void Subscribe(const NetworkCallbacks& callbacks);

    /**
     * @brief Send custom message to peer
     * @param peer Peer address
     * @param type Message type
     * @param payload Message payload
     * @return true if sent successfully
     */
    bool SendMessage(
        const std::string& peer,
        MessageType type,
        const std::vector<uint8_t>& payload
    );

    /**
     * @brief Get network statistics
     */
    struct NetworkStats {
        size_t totalPeers;
        size_t inboundPeers;
        size_t outboundPeers;
        uint64_t bytesReceived;
        uint64_t bytesSent;
        uint64_t messagesReceived;
        uint64_t messagesSent;
        std::chrono::system_clock::time_point startTime;
    };
    
    /**
     * @brief Get network statistics
     * @return Network statistics
     */
    NetworkStats GetStats() const;

    /**
     * @brief Create MainNet client
     */
    static std::unique_ptr<NetworkClient> MainNet();

    /**
     * @brief Create TestNet client
     */
    static std::unique_ptr<NetworkClient> TestNet();

    /**
     * @brief Create PrivateNet client
     * @param seedNodes Seed node addresses
     */
    static std::unique_ptr<NetworkClient> PrivateNet(
        const std::vector<std::string>& seedNodes
    );

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace neo::sdk::network