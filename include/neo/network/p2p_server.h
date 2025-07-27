#pragma once

#include <atomic>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <neo/io/uint256.h>
#include <neo/network/inventory_type.h>
#include <neo/network/message.h>
#include <neo/network/peer_discovery_service.h>
#include <neo/network/tcp_client.h>
#include <neo/network/tcp_server.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace neo::network
{
/**
 * @brief Represents a P2P peer.
 */
class P2PPeer
{
  public:
    /**
     * @brief Constructs a P2PPeer.
     * @param connection The TCP connection.
     */
    explicit P2PPeer(std::shared_ptr<TcpConnection> connection);

    /**
     * @brief Gets the connection.
     * @return The connection.
     */
    std::shared_ptr<TcpConnection> GetConnection() const;

    /**
     * @brief Gets the version.
     * @return The version.
     */
    uint32_t GetVersion() const;

    /**
     * @brief Sets the version.
     * @param version The version.
     */
    void SetVersion(uint32_t version);

    /**
     * @brief Gets the services.
     * @return The services.
     */
    uint64_t GetServices() const;

    /**
     * @brief Sets the services.
     * @param services The services.
     */
    void SetServices(uint64_t services);

    /**
     * @brief Gets the user agent.
     * @return The user agent.
     */
    const std::string& GetUserAgent() const;

    /**
     * @brief Sets the user agent.
     * @param userAgent The user agent.
     */
    void SetUserAgent(const std::string& userAgent);

    /**
     * @brief Gets the start height.
     * @return The start height.
     */
    uint32_t GetStartHeight() const;

    /**
     * @brief Sets the start height.
     * @param startHeight The start height.
     */
    void SetStartHeight(uint32_t startHeight);

    /**
     * @brief Gets whether to relay transactions.
     * @return Whether to relay transactions.
     */
    bool GetRelay() const;

    /**
     * @brief Sets whether to relay transactions.
     * @param relay Whether to relay transactions.
     */
    void SetRelay(bool relay);

    /**
     * @brief Gets the last seen time.
     * @return The last seen time.
     */
    std::chrono::system_clock::time_point GetLastSeen() const;

    /**
     * @brief Updates the last seen time.
     */
    void UpdateLastSeen();

    /**
     * @brief Gets whether the peer is connected.
     * @return Whether the peer is connected.
     */
    bool IsConnected() const;

    /**
     * @brief Disconnects the peer.
     */
    void Disconnect();

    /**
     * @brief Sends a message to the peer.
     * @param message The message.
     */
    void Send(const Message& message);

  private:
    std::shared_ptr<TcpConnection> connection_;
    uint32_t version_;
    uint64_t services_;
    std::string userAgent_;
    uint32_t startHeight_;
    bool relay_;
    std::chrono::system_clock::time_point lastSeen_;
};

/**
 * @brief Represents a P2P server.
 */
class P2PServer : public std::enable_shared_from_this<P2PServer>
{
  public:
    /**
     * @brief Constructs a P2PServer.
     * @param ioContext The IO context.
     * @param endpoint The endpoint to listen on.
     * @param userAgent The user agent.
     * @param startHeight The start height.
     */
    P2PServer(boost::asio::io_context& ioContext, const IPEndPoint& endpoint, const std::string& userAgent,
              uint32_t startHeight);

    /**
     * @brief Gets the peer discovery service.
     * @return The peer discovery service.
     */
    std::shared_ptr<PeerDiscoveryService> GetPeerDiscovery() const;

    /**
     * @brief Destructor.
     */
    ~P2PServer();

    /**
     * @brief Starts the server.
     */
    void Start();

    /**
     * @brief Stops the server.
     */
    void Stop();

    /**
     * @brief Gets the endpoint.
     * @return The endpoint.
     */
    const IPEndPoint& GetEndpoint() const;

    /**
     * @brief Gets the user agent.
     * @return The user agent.
     */
    const std::string& GetUserAgent() const;

    /**
     * @brief Gets the start height.
     * @return The start height.
     */
    uint32_t GetStartHeight() const;

    /**
     * @brief Sets the start height.
     * @param startHeight The start height.
     */
    void SetStartHeight(uint32_t startHeight);

    /**
     * @brief Connects to a peer.
     * @param endpoint The endpoint to connect to.
     * @return The peer.
     */
    std::shared_ptr<P2PPeer> ConnectToPeer(const IPEndPoint& endpoint);

    /**
     * @brief Gets the connected peers.
     * @return The connected peers.
     */
    std::vector<std::shared_ptr<P2PPeer>> GetConnectedPeers() const;

    /**
     * @brief Gets the number of connected peers.
     * @return The number of connected peers.
     */
    size_t GetConnectedPeersCount() const;

    /**
     * @brief Gets the port number.
     * @return The port number.
     */
    uint16_t GetPort() const;

    /**
     * @brief Gets a random nonce.
     * @return A random nonce.
     */
    uint32_t GetNonce() const;

    /**
     * @brief Broadcasts a message to all connected peers.
     * @param message The message.
     */
    void Broadcast(const Message& message);

    /**
     * @brief Sets the inventory received callback.
     * @param callback The callback.
     */
    void SetInventoryReceivedCallback(
        std::function<void(std::shared_ptr<P2PPeer>, InventoryType, const std::vector<io::UInt256>&)> callback);

  private:
    IPEndPoint endpoint_;
    std::string userAgent_;
    std::atomic<uint32_t> startHeight_;
    std::unique_ptr<TcpServer> server_;
    std::unique_ptr<TcpClient> client_;
    std::shared_ptr<PeerDiscoveryService> peerDiscovery_;
    mutable std::mutex peersMutex_;
    std::unordered_map<std::string, std::shared_ptr<P2PPeer>> peers_;
    std::function<void(std::shared_ptr<P2PPeer>, InventoryType, const std::vector<io::UInt256>&)>
        inventoryReceivedCallback_;
    boost::asio::io_context& ioContext_;

    void HandleConnectionAccepted(std::shared_ptr<TcpConnection> connection);
    void HandleMessageReceived(std::shared_ptr<P2PPeer> peer, const Message& message);
    void HandleVersionMessage(std::shared_ptr<P2PPeer> peer, const Message& message);
    void HandleVerackMessage(std::shared_ptr<P2PPeer> peer, const Message& message);
    void HandleInventoryMessage(std::shared_ptr<P2PPeer> peer, const Message& message);
    void HandleGetDataMessage(std::shared_ptr<P2PPeer> peer, const Message& message);
    void HandlePingMessage(std::shared_ptr<P2PPeer> peer, const Message& message);
    void HandlePongMessage(std::shared_ptr<P2PPeer> peer, const Message& message);
    /**
     * @brief Gets the peer discovery service.
     * @return The peer discovery service.
     */
    std::shared_ptr<PeerDiscoveryService> GetPeerDiscoveryService() const;

  private:
    void HandleGetAddrMessage(std::shared_ptr<P2PPeer> peer, const Message& message);
    void HandleAddrMessage(std::shared_ptr<P2PPeer> peer, const Message& message);
    void RequestAddresses(std::shared_ptr<P2PPeer> peer);
    void SendVersionMessage(std::shared_ptr<P2PPeer> peer);
};
}  // namespace neo::network
