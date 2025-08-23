#include "neo/rpc/websocket_server.h"
#include "neo/core/common_logging.h"

#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

namespace neo::rpc {

WebSocketServer::WebSocketServer(uint16_t port) : port_(port) {
    // Initialize logging
    core::CommonLogging::Info("WebSocket server initialized on port " + std::to_string(port));
}

WebSocketServer::~WebSocketServer() {
    Stop();
}

bool WebSocketServer::Start() {
    if (running_.load()) {
        core::CommonLogging::Warning("WebSocket server is already running");
        return false;
    }
    
    try {
        running_.store(true);
        processing_notifications_.store(true);
        
        // Start server thread
        server_thread_ = std::thread([this]() {
            core::CommonLogging::Info("WebSocket server listening on port " + std::to_string(port_));
            
            // Main server loop
            while (running_.load()) {
                try {
                    // Accept connections and handle messages
                    // In a real implementation, this would use a WebSocket library like libwebsockets
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                } catch (const std::exception& e) {
                    core::CommonLogging::Error("WebSocket server error: " + std::string(e.what()));
                }
            }
            
            core::CommonLogging::Info("WebSocket server stopped");
        });
        
        // Start notification processing thread
        notification_thread_ = std::thread([this]() {
            while (processing_notifications_.load()) {
                try {
                    // Process notification queue
                    // In a real implementation, this would process queued notifications
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                } catch (const std::exception& e) {
                    core::CommonLogging::Error("Notification processing error: " + std::string(e.what()));
                }
            }
        });
        
        core::CommonLogging::Info("WebSocket server started successfully");
        return true;
        
    } catch (const std::exception& e) {
        core::CommonLogging::Error("Failed to start WebSocket server: " + std::string(e.what()));
        running_.store(false);
        return false;
    }
}

void WebSocketServer::Stop() {
    if (!running_.load()) {
        return;
    }
    
    core::CommonLogging::Info("Stopping WebSocket server...");
    
    // Stop server
    running_.store(false);
    processing_notifications_.store(false);
    
    // Wait for threads to finish
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    if (notification_thread_.joinable()) {
        notification_thread_.join();
    }
    
    // Clear connections and subscriptions
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        active_connections_.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        subscriptions_.clear();
    }
    
    core::CommonLogging::Info("WebSocket server stopped");
}

std::string WebSocketServer::Subscribe(const ConnectionId& connection_id, 
                                     const std::string& method, 
                                     const json::JToken& params) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    // Generate unique subscription ID
    std::string subscription_id = GenerateSubscriptionId();
    
    // Add subscription
    subscriptions_[connection_id][subscription_id] = SubscriptionFilter(method, params);
    
    core::CommonLogging::Info("New subscription: " + subscription_id + " for method: " + method);
    
    return subscription_id;
}

bool WebSocketServer::Unsubscribe(const ConnectionId& connection_id, const std::string& subscription_id) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    auto conn_it = subscriptions_.find(connection_id);
    if (conn_it != subscriptions_.end()) {
        auto sub_it = conn_it->second.find(subscription_id);
        if (sub_it != conn_it->second.end()) {
            conn_it->second.erase(sub_it);
            
            // Remove connection if no more subscriptions
            if (conn_it->second.empty()) {
                subscriptions_.erase(conn_it);
            }
            
            core::CommonLogging::Info("Unsubscribed: " + subscription_id);
            return true;
        }
    }
    
    return false;
}

void WebSocketServer::SendNotification(const NotificationMessage& notification) {
    if (!running_.load()) {
        return;
    }
    
    std::lock_guard<std::mutex> sub_lock(subscriptions_mutex_);
    
    // Send to all matching subscriptions
    for (const auto& [connection_id, connection_subscriptions] : subscriptions_) {
        for (const auto& [subscription_id, filter] : connection_subscriptions) {
            if (filter.active && MatchesFilter(notification, filter)) {
                try {
                    // Create notification message
                    json::JToken message = json::JToken::CreateObject();
                    message.AddMember("jsonrpc", "2.0");
                    message.AddMember("method", "subscription");
                    
                    json::JToken params = json::JToken::CreateObject();
                    params.AddMember("subscription", subscription_id);
                    params.AddMember("result", notification.data);
                    message.AddMember("params", params);
                    
                    // Send message
                    SendMessage(connection_id, message.ToString());
                    messages_sent_.fetch_add(1);
                    
                } catch (const std::exception& e) {
                    core::CommonLogging::Error("Failed to send notification: " + std::string(e.what()));
                }
            }
        }
    }
    
    notifications_processed_.fetch_add(1);
}

void WebSocketServer::NotifyNewBlock(std::shared_ptr<ledger::Block> block) {
    if (!block) return;
    
    try {
        json::JToken block_json = block->ToJson();
        NotificationMessage notification(NotificationType::BlockAdded, block_json);
        SendNotification(notification);
        
        core::CommonLogging::Debug("Notified new block: " + block->GetHash().ToString());
        
    } catch (const std::exception& e) {
        core::CommonLogging::Error("Failed to notify new block: " + std::string(e.what()));
    }
}

void WebSocketServer::NotifyNewTransaction(std::shared_ptr<network::p2p::payloads::Neo3Transaction> transaction) {
    if (!transaction) return;
    
    try {
        json::JToken tx_json = transaction->ToJson();
        NotificationMessage notification(NotificationType::TransactionAdded, tx_json);
        SendNotification(notification);
        
        core::CommonLogging::Debug("Notified new transaction: " + transaction->GetHash().ToString());
        
    } catch (const std::exception& e) {
        core::CommonLogging::Error("Failed to notify new transaction: " + std::string(e.what()));
    }
}

void WebSocketServer::NotifyExecution(const io::UInt256& tx_hash, const json::JToken& execution_data) {
    try {
        json::JToken notification_data = json::JToken::CreateObject();
        notification_data.AddMember("txhash", tx_hash.ToString());
        notification_data.AddMember("execution", execution_data);
        
        NotificationMessage notification(NotificationType::ExecutionNotification, notification_data);
        SendNotification(notification);
        
        core::CommonLogging::Debug("Notified execution for tx: " + tx_hash.ToString());
        
    } catch (const std::exception& e) {
        core::CommonLogging::Error("Failed to notify execution: " + std::string(e.what()));
    }
}

size_t WebSocketServer::GetConnectionCount() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return active_connections_.size();
}

size_t WebSocketServer::GetSubscriptionCount() const {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    size_t count = 0;
    for (const auto& [connection_id, connection_subscriptions] : subscriptions_) {
        count += connection_subscriptions.size();
    }
    return count;
}

void WebSocketServer::HandleConnection(const ConnectionId& connection_id) {
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        active_connections_.insert(connection_id);
    }
    
    core::CommonLogging::Info("New WebSocket connection: " + connection_id);
    
    // Connection handling would be implemented here with a WebSocket library
}

void WebSocketServer::ProcessMessage(const ConnectionId& connection_id, const std::string& message) {
    try {
        // Parse JSON-RPC message
        json::JToken request = json::JToken::Parse(message);
        
        if (request.HasMember("method")) {
            std::string method = request.GetMember("method").GetString();
            
            if (method == "subscribe") {
                // Handle subscription request
                if (request.HasMember("params")) {
                    json::JToken params = request.GetMember("params");
                    std::string sub_method = params.GetArray()[0].GetString();
                    json::JToken sub_params = params.GetArray().size() > 1 ? 
                        params.GetArray()[1] : json::JToken::CreateNull();
                    
                    std::string subscription_id = Subscribe(connection_id, sub_method, sub_params);
                    
                    // Send subscription response
                    json::JToken response = json::JToken::CreateObject();
                    response.AddMember("jsonrpc", "2.0");
                    if (request.HasMember("id")) {
                        response.AddMember("id", request.GetMember("id"));
                    }
                    response.AddMember("result", subscription_id);
                    
                    SendMessage(connection_id, response.ToString());
                }
            } else if (method == "unsubscribe") {
                // Handle unsubscription request
                if (request.HasMember("params")) {
                    json::JToken params = request.GetMember("params");
                    std::string subscription_id = params.GetArray()[0].GetString();
                    
                    bool success = Unsubscribe(connection_id, subscription_id);
                    
                    // Send unsubscription response
                    json::JToken response = json::JToken::CreateObject();
                    response.AddMember("jsonrpc", "2.0");
                    if (request.HasMember("id")) {
                        response.AddMember("id", request.GetMember("id"));
                    }
                    response.AddMember("result", success);
                    
                    SendMessage(connection_id, response.ToString());
                }
            }
        }
        
    } catch (const std::exception& e) {
        core::CommonLogging::Error("Failed to process WebSocket message: " + std::string(e.what()));
        
        // Send error response
        json::JToken error_response = json::JToken::CreateObject();
        error_response.AddMember("jsonrpc", "2.0");
        error_response.AddMember("id", json::JToken::CreateNull());
        
        json::JToken error = json::JToken::CreateObject();
        error.AddMember("code", -32700);
        error.AddMember("message", "Parse error");
        error_response.AddMember("error", error);
        
        SendMessage(connection_id, error_response.ToString());
    }
}

void WebSocketServer::SendMessage(const ConnectionId& connection_id, const std::string& message) {
    // In a real implementation, this would send the message over the WebSocket connection
    core::CommonLogging::Debug("Sending WebSocket message to " + connection_id + ": " + message);
}

std::string WebSocketServer::GenerateSubscriptionId() {
    // Generate unique subscription ID using timestamp and random number
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    std::stringstream ss;
    ss << std::hex << timestamp << "_" << dis(gen);
    return ss.str();
}

bool WebSocketServer::MatchesFilter(const NotificationMessage& notification, const SubscriptionFilter& filter) {
    // Match notification type to subscription method
    switch (notification.type) {
        case NotificationType::BlockAdded:
            return filter.method == "block_added" || filter.method == "new_block";
        case NotificationType::TransactionAdded:
            return filter.method == "transaction_added" || filter.method == "new_transaction";
        case NotificationType::ExecutionNotification:
            return filter.method == "execution" || filter.method == "contract_execution";
        case NotificationType::ApplicationLog:
            return filter.method == "application_log" || filter.method == "app_log";
        default:
            return false;
    }
}

} // namespace neo::rpc