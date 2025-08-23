#pragma once

#include <memory>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

#include "neo/json/jtoken.h"
#include "neo/ledger/block.h"
#include "neo/network/p2p/payloads/neo3_transaction.h"

namespace neo::rpc {

/**
 * @brief WebSocket connection identifier
 */
using ConnectionId = std::string;

/**
 * @brief WebSocket notification types
 */
enum class NotificationType {
    BlockAdded,
    TransactionAdded,
    ExecutionNotification,
    ApplicationLog
};

/**
 * @brief WebSocket notification message
 */
struct NotificationMessage {
    NotificationType type;
    json::JToken data;
    std::string subscription_id;
    
    NotificationMessage(NotificationType t, const json::JToken& d, const std::string& id = "")
        : type(t), data(d), subscription_id(id) {}
};

/**
 * @brief WebSocket subscription filter
 */
struct SubscriptionFilter {
    std::string method;
    json::JToken params;
    bool active = true;
    
    SubscriptionFilter(const std::string& m, const json::JToken& p)
        : method(m), params(p) {}
};

/**
 * @brief WebSocket server for real-time notifications
 * 
 * Provides WebSocket endpoints for subscribing to blockchain events:
 * - New blocks
 * - New transactions
 * - Contract execution notifications
 * - Application logs
 */
class WebSocketServer {
public:
    /**
     * @brief Constructor
     * @param port WebSocket server port
     */
    explicit WebSocketServer(uint16_t port = 10334);
    
    /**
     * @brief Destructor
     */
    ~WebSocketServer();
    
    /**
     * @brief Start the WebSocket server
     * @return true if started successfully
     */
    bool Start();
    
    /**
     * @brief Stop the WebSocket server
     */
    void Stop();
    
    /**
     * @brief Check if server is running
     * @return true if running
     */
    bool IsRunning() const { return running_; }
    
    /**
     * @brief Subscribe to notifications
     * @param connection_id Client connection ID
     * @param method Subscription method
     * @param params Subscription parameters
     * @return Subscription ID
     */
    std::string Subscribe(const ConnectionId& connection_id, 
                         const std::string& method, 
                         const json::JToken& params);
    
    /**
     * @brief Unsubscribe from notifications
     * @param connection_id Client connection ID
     * @param subscription_id Subscription ID
     * @return true if unsubscribed successfully
     */
    bool Unsubscribe(const ConnectionId& connection_id, const std::string& subscription_id);
    
    /**
     * @brief Send notification to subscribed clients
     * @param notification Notification message
     */
    void SendNotification(const NotificationMessage& notification);
    
    /**
     * @brief Notify about new block
     * @param block New block
     */
    void NotifyNewBlock(std::shared_ptr<ledger::Block> block);
    
    /**
     * @brief Notify about new transaction
     * @param transaction New transaction
     */
    void NotifyNewTransaction(std::shared_ptr<network::p2p::payloads::Neo3Transaction> transaction);
    
    /**
     * @brief Notify about contract execution
     * @param tx_hash Transaction hash
     * @param execution_data Execution data
     */
    void NotifyExecution(const io::UInt256& tx_hash, const json::JToken& execution_data);
    
    /**
     * @brief Get connection count
     * @return Number of active connections
     */
    size_t GetConnectionCount() const;
    
    /**
     * @brief Get subscription count
     * @return Number of active subscriptions
     */
    size_t GetSubscriptionCount() const;

private:
    /**
     * @brief WebSocket connection handler
     * @param connection_id Connection identifier
     */
    void HandleConnection(const ConnectionId& connection_id);
    
    /**
     * @brief Process WebSocket message
     * @param connection_id Connection identifier
     * @param message Message content
     */
    void ProcessMessage(const ConnectionId& connection_id, const std::string& message);
    
    /**
     * @brief Send message to connection
     * @param connection_id Connection identifier
     * @param message Message to send
     */
    void SendMessage(const ConnectionId& connection_id, const std::string& message);
    
    /**
     * @brief Generate unique subscription ID
     * @return Subscription ID
     */
    std::string GenerateSubscriptionId();
    
    /**
     * @brief Check if notification matches filter
     * @param notification Notification message
     * @param filter Subscription filter
     * @return true if matches
     */
    bool MatchesFilter(const NotificationMessage& notification, const SubscriptionFilter& filter);

private:
    uint16_t port_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    
    // Connection management
    mutable std::mutex connections_mutex_;
    std::unordered_set<ConnectionId> active_connections_;
    
    // Subscription management
    mutable std::mutex subscriptions_mutex_;
    std::unordered_map<ConnectionId, std::unordered_map<std::string, SubscriptionFilter>> subscriptions_;
    
    // Message queue for notifications
    mutable std::mutex notifications_mutex_;
    std::thread notification_thread_;
    std::atomic<bool> processing_notifications_{false};
    
    // Statistics
    std::atomic<uint64_t> messages_sent_{0};
    std::atomic<uint64_t> notifications_processed_{0};
};

} // namespace neo::rpc