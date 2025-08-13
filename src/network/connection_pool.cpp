/**
 * @file connection_pool.cpp
 * @brief Implementation of connection pooling for network layer
 */

#include <neo/network/connection_pool.h>
#include <neo/network/tcp_connection.h>
#include <neo/logging/console_logger.h>
#include <algorithm>
#include <sstream>

namespace neo {
namespace network {

ConnectionPool::ConnectionPool(const Config& config)
    : config_(config)
    , running_(false) {
}

ConnectionPool::~ConnectionPool() {
    Stop();
}

void ConnectionPool::Start() {
    if (running_.exchange(true)) {
        return; // Already running
    }
    
    // Start background cleanup thread
    cleanup_thread_ = std::thread(&ConnectionPool::CleanupTask, this);
    
    logging::ConsoleLogger::Info("ConnectionPool started with config: "
                         "min=" + std::to_string(config_.min_connections) +
                         ", max=" + std::to_string(config_.max_connections) +
                         ", idle=" + std::to_string(config_.max_idle_connections));
}

void ConnectionPool::Stop() {
    if (!running_.exchange(false)) {
        return; // Already stopped
    }
    
    // Signal cleanup thread to stop
    cv_.notify_all();
    
    // Wait for cleanup thread
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
    
    // Close all connections
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Close idle connections
        for (auto& [endpoint, queue] : idle_pools_) {
            while (!queue.empty()) {
                auto pooled = queue.front();
                queue.pop();
                if (pooled->connection) {
                    pooled->connection->Close();
                }
            }
        }
        idle_pools_.clear();
        
        // Close active connections
        for (auto& [endpoint, connections] : active_pools_) {
            for (auto& pooled : connections) {
                if (pooled->connection) {
                    pooled->connection->Close();
                }
            }
        }
        active_pools_.clear();
    }
    
    logging::ConsoleLogger::Info("ConnectionPool stopped");
}

std::shared_ptr<TcpConnection> ConnectionPool::GetConnection(const std::string& host, uint16_t port) {
    std::string endpoint_key = MakeEndpointKey(host, port);
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check for idle connection
        auto idle_it = idle_pools_.find(endpoint_key);
        if (idle_it != idle_pools_.end() && !idle_it->second.empty()) {
            auto pooled = idle_it->second.front();
            idle_it->second.pop();
            
            // Validate connection is still alive
            if (IsConnectionAlive(pooled->connection)) {
                pooled->in_use = true;
                pooled->last_used = std::chrono::steady_clock::now();
                
                // Move to active pool
                active_pools_[endpoint_key].push_back(pooled);
                
                stats_.reused_connections.fetch_add(1);
                stats_.idle_connections.fetch_sub(1);
                stats_.active_connections.fetch_add(1);
                
                logging::ConsoleLogger::Debug("Reused connection to " + endpoint_key);
                return pooled->connection;
            }
        }
        
        // Check if we can create a new connection
        size_t total_connections = 0;
        for (const auto& [_, connections] : active_pools_) {
            total_connections += connections.size();
        }
        for (const auto& [_, queue] : idle_pools_) {
            total_connections += queue.size();
        }
        
        if (total_connections >= config_.max_connections) {
            logging::ConsoleLogger::Warning("Connection pool limit reached: " + 
                                   std::to_string(total_connections) + "/" + 
                                   std::to_string(config_.max_connections));
            return nullptr;
        }
    }
    
    // Create new connection
    auto connection = CreateConnection(host, port);
    if (!connection) {
        stats_.failed_connections.fetch_add(1);
        return nullptr;
    }
    
    // Add to active pool
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto pooled = std::make_shared<PooledConnection>(connection, host, port);
        pooled->in_use = true;
        active_pools_[endpoint_key].push_back(pooled);
        
        stats_.total_connections.fetch_add(1);
        stats_.active_connections.fetch_add(1);
    }
    
    logging::ConsoleLogger::Debug("Created new connection to " + endpoint_key);
    return connection;
}

void ConnectionPool::ReturnConnection(std::shared_ptr<TcpConnection> connection) {
    if (!connection) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find connection in active pools
    for (auto& [endpoint_key, connections] : active_pools_) {
        auto it = std::find_if(connections.begin(), connections.end(),
            [&connection](const std::shared_ptr<PooledConnection>& pooled) {
                return pooled->connection == connection;
            });
        
        if (it != connections.end()) {
            auto pooled = *it;
            connections.erase(it);
            
            // Check if connection is still alive and pool isn't full
            if (IsConnectionAlive(connection) && 
                idle_pools_[endpoint_key].size() < config_.max_idle_connections) {
                
                pooled->in_use = false;
                pooled->last_used = std::chrono::steady_clock::now();
                idle_pools_[endpoint_key].push(pooled);
                
                stats_.active_connections.fetch_sub(1);
                stats_.idle_connections.fetch_add(1);
                
                logging::ConsoleLogger::Debug("Returned connection to pool: " + endpoint_key);
            } else {
                // Close connection
                connection->Close();
                stats_.active_connections.fetch_sub(1);
                stats_.total_connections.fetch_sub(1);
                
                logging::ConsoleLogger::Debug("Closed connection: " + endpoint_key);
            }
            
            return;
        }
    }
    
    // Connection not found in pool - close it
    connection->Close();
    logging::ConsoleLogger::Warning("Connection not found in pool, closing");
}

void ConnectionPool::CloseEndpoint(const std::string& host, uint16_t port) {
    std::string endpoint_key = MakeEndpointKey(host, port);
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Close idle connections
    auto idle_it = idle_pools_.find(endpoint_key);
    if (idle_it != idle_pools_.end()) {
        while (!idle_it->second.empty()) {
            auto pooled = idle_it->second.front();
            idle_it->second.pop();
            if (pooled->connection) {
                pooled->connection->Close();
            }
            stats_.idle_connections.fetch_sub(1);
            stats_.total_connections.fetch_sub(1);
        }
        idle_pools_.erase(idle_it);
    }
    
    // Close active connections
    auto active_it = active_pools_.find(endpoint_key);
    if (active_it != active_pools_.end()) {
        for (auto& pooled : active_it->second) {
            if (pooled->connection) {
                pooled->connection->Close();
            }
            stats_.active_connections.fetch_sub(1);
            stats_.total_connections.fetch_sub(1);
        }
        active_pools_.erase(active_it);
    }
    
    logging::ConsoleLogger::Info("Closed all connections to " + endpoint_key);
}

void ConnectionPool::HealthCheck() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check idle connections
    for (auto& [endpoint_key, queue] : idle_pools_) {
        std::queue<std::shared_ptr<PooledConnection>> healthy_connections;
        
        while (!queue.empty()) {
            auto pooled = queue.front();
            queue.pop();
            
            if (IsConnectionAlive(pooled->connection)) {
                healthy_connections.push(pooled);
            } else {
                pooled->connection->Close();
                stats_.idle_connections.fetch_sub(1);
                stats_.total_connections.fetch_sub(1);
                logging::ConsoleLogger::Debug("Removed unhealthy connection: " + endpoint_key);
            }
        }
        
        queue = healthy_connections;
    }
    
    // Check active connections
    for (auto& [endpoint_key, connections] : active_pools_) {
        connections.erase(
            std::remove_if(connections.begin(), connections.end(),
                [this, &endpoint_key](const std::shared_ptr<PooledConnection>& pooled) {
                    if (!IsConnectionAlive(pooled->connection)) {
                        pooled->connection->Close();
                        stats_.active_connections.fetch_sub(1);
                        stats_.total_connections.fetch_sub(1);
                        logging::ConsoleLogger::Debug("Removed unhealthy active connection: " + endpoint_key);
                        return true;
                    }
                    return false;
                }),
            connections.end()
        );
    }
}

std::shared_ptr<TcpConnection> ConnectionPool::CreateConnection(const std::string& host, uint16_t port) {
    if (!connection_factory_) {
        // Default factory - for now just log an error as we need boost::asio context
        logging::ConsoleLogger::Error("No connection factory set for ConnectionPool");
        return nullptr;
    }
    
    // Use custom factory
    return connection_factory_(host, port);
}

void ConnectionPool::CleanupIdleConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::steady_clock::now();
    
    for (auto& [endpoint_key, queue] : idle_pools_) {
        std::queue<std::shared_ptr<PooledConnection>> retained_connections;
        
        while (!queue.empty()) {
            auto pooled = queue.front();
            queue.pop();
            
            auto idle_time = std::chrono::duration_cast<std::chrono::seconds>(
                now - pooled->last_used);
            
            if (idle_time < config_.idle_timeout && IsConnectionAlive(pooled->connection)) {
                retained_connections.push(pooled);
            } else {
                pooled->connection->Close();
                stats_.idle_connections.fetch_sub(1);
                stats_.total_connections.fetch_sub(1);
                logging::ConsoleLogger::Debug("Cleaned up idle connection: " + endpoint_key);
            }
        }
        
        queue = retained_connections;
    }
}

void ConnectionPool::CleanupTask() {
    while (running_.load()) {
        // Wait for interval or stop signal
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::seconds(30), [this] {
            return !running_.load();
        });
        lock.unlock();
        
        if (!running_.load()) {
            break;
        }
        
        // Perform cleanup
        CleanupIdleConnections();
        RemoveExpiredConnections();
        
        // Log statistics periodically
        auto stats = GetStats();
        logging::ConsoleLogger::Debug("ConnectionPool stats: total=" + std::to_string(stats.total_connections) +
                             ", active=" + std::to_string(stats.active_connections) +
                             ", idle=" + std::to_string(stats.idle_connections) +
                             ", failed=" + std::to_string(stats.failed_connections) +
                             ", reused=" + std::to_string(stats.reused_connections));
    }
}

bool ConnectionPool::IsConnectionAlive(const std::shared_ptr<TcpConnection>& connection) {
    if (!connection) {
        return false;
    }
    
    // Check if socket is open
    try {
        if (!connection->GetSocket().is_open()) {
            return false;
        }
    } catch (...) {
        return false;
    }
    
    // Could add more sophisticated health checks here
    // e.g., send a ping message and wait for response
    
    return true;
}

void ConnectionPool::RemoveExpiredConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove empty endpoint entries
    for (auto it = idle_pools_.begin(); it != idle_pools_.end();) {
        if (it->second.empty()) {
            it = idle_pools_.erase(it);
        } else {
            ++it;
        }
    }
    
    for (auto it = active_pools_.begin(); it != active_pools_.end();) {
        if (it->second.empty()) {
            it = active_pools_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace network
} // namespace neo