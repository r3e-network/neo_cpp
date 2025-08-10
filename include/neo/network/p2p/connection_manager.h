#pragma once

#include <neo/core/logging.h>
#include <neo/network/p2p/message_handler.h>
#include <neo/network/p2p/peer_manager.h>
#include <neo/network/p2p/tcp_server.h>

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <thread>

namespace neo::network::p2p
{
/**
 * @brief Connection statistics
 */
struct ConnectionStats
{
    std::atomic<uint64_t> total_connections{0};
    std::atomic<uint64_t> active_connections{0};
    std::atomic<uint64_t> failed_connections{0};
    std::atomic<uint64_t> bytes_sent{0};
    std::atomic<uint64_t> bytes_received{0};
    std::atomic<uint64_t> messages_sent{0};
    std::atomic<uint64_t> messages_received{0};
};

/**
 * @brief Connection manager for P2P networking
 */
class ConnectionManager
{
   public:
    struct Config
    {
        std::string bind_address{"0.0.0.0"};
        uint16_t port{10333};
        size_t max_connections{100};
        size_t io_threads{4};
        std::chrono::seconds connection_timeout{30};
        std::chrono::seconds handshake_timeout{10};
        bool enable_tls{false};
        std::string tls_cert_file;
        std::string tls_key_file;
        std::string tls_ca_file;
        size_t send_buffer_size{64 * 1024};
        size_t receive_buffer_size{64 * 1024};
    };

   private:
    Config config_;
    std::shared_ptr<core::Logger> logger_;
    std::shared_ptr<PeerManager> peer_manager_;
    std::shared_ptr<MessageHandler> message_handler_;

    // ASIO components
    asio::io_context io_context_;
    std::unique_ptr<asio::ip::tcp::acceptor> acceptor_;
    std::unique_ptr<asio::ssl::context> ssl_context_;
    std::vector<std::thread> io_threads_;
    std::unique_ptr<asio::steady_timer> connect_timer_;
    std::unique_ptr<asio::steady_timer> cleanup_timer_;

    // Connection state
    std::atomic<bool> running_{false};
    ConnectionStats stats_;

    // Outbound connection queue
    std::queue<NetworkAddress> connection_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread connector_thread_;

   public:
    ConnectionManager(const Config& config, std::shared_ptr<PeerManager> peer_manager,
                      std::shared_ptr<MessageHandler> message_handler);
    ~ConnectionManager();

    /**
     * @brief Start the connection manager
     */
    void Start();

    /**
     * @brief Stop the connection manager
     */
    void Stop();

    /**
     * @brief Connect to a specific address
     */
    void ConnectTo(const NetworkAddress& address);

    /**
     * @brief Disconnect a peer
     */
    void Disconnect(const io::UInt256& peer_id);

    /**
     * @brief Send message to peer
     */
    void SendMessage(const io::UInt256& peer_id, const Message& message);

    /**
     * @brief Broadcast message to multiple peers
     */
    void BroadcastMessage(const Message& message, const std::vector<io::UInt256>& peer_ids = {});

    /**
     * @brief Get connection statistics
     */
    ConnectionStats GetStatistics() const { return stats_; }

    /**
     * @brief Check if running
     */
    bool IsRunning() const { return running_; }

   private:
    /**
     * @brief Initialize SSL context
     */
    void InitializeSSL();

    /**
     * @brief Start accepting connections
     */
    void StartAccept();

    /**
     * @brief Handle accepted connection
     */
    void HandleAccept(std::shared_ptr<asio::ip::tcp::socket> socket, const asio::error_code& error);

    /**
     * @brief Start outbound connector thread
     */
    void StartConnector();

    /**
     * @brief Connector thread main loop
     */
    void ConnectorLoop();

    /**
     * @brief Create outbound connection
     */
    void CreateOutboundConnection(const NetworkAddress& address);

    /**
     * @brief Handle connection result
     */
    void HandleConnect(std::shared_ptr<RemoteNode> peer, const asio::error_code& error);

    /**
     * @brief Start periodic timers
     */
    void StartTimers();

    /**
     * @brief Periodic connection check
     */
    void OnConnectTimer();

    /**
     * @brief Periodic cleanup
     */
    void OnCleanupTimer();

    /**
     * @brief Create and setup a new peer connection
     */
    std::shared_ptr<RemoteNode> CreatePeer(std::shared_ptr<asio::ip::tcp::socket> socket, bool is_outbound);

    /**
     * @brief Setup peer callbacks
     */
    void SetupPeerCallbacks(std::shared_ptr<RemoteNode> peer);

    /**
     * @brief Handle peer disconnection
     */
    void OnPeerDisconnected(const io::UInt256& peer_id);

    /**
     * @brief Handle received message
     */
    void OnMessageReceived(const io::UInt256& peer_id, const Message& message);

    /**
     * @brief Update connection statistics
     */
    void UpdateStats(uint64_t bytes_sent, uint64_t bytes_received, bool message_sent);
};

/**
 * @brief Message handler for processing P2P messages
 */
class MessageHandler
{
   public:
    using MessageCallback = std::function<void(const io::UInt256& peer_id, const Message& message)>;

   private:
    std::shared_ptr<core::Logger> logger_;
    std::unordered_map<MessageType, MessageCallback> handlers_;
    std::mutex mutex_;

    // Message processing queue
    struct QueuedMessage
    {
        io::UInt256 peer_id;
        std::unique_ptr<Message> message;
        std::chrono::steady_clock::time_point received_time;
    };

    std::queue<QueuedMessage> message_queue_;
    std::condition_variable queue_cv_;
    std::thread processor_thread_;
    std::atomic<bool> running_{false};

   public:
    MessageHandler();
    ~MessageHandler();

    /**
     * @brief Register message handler
     */
    void RegisterHandler(MessageType type, MessageCallback callback);

    /**
     * @brief Process incoming message
     */
    void ProcessMessage(const io::UInt256& peer_id, std::unique_ptr<Message> message);

    /**
     * @brief Start message processing
     */
    void Start();

    /**
     * @brief Stop message processing
     */
    void Stop();

   private:
    /**
     * @brief Message processor thread
     */
    void ProcessorLoop();

    /**
     * @brief Handle specific message
     */
    void HandleMessage(const io::UInt256& peer_id, const Message& message);

    /**
     * @brief Default handlers for core messages
     */
    void RegisterDefaultHandlers();
};
}  // namespace neo::network::p2p