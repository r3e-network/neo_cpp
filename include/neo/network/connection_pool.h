/**
 * @file connection_pool.h
 * @brief Connection pooling for network layer
 */

#pragma once

#include <neo/io/uint256.h>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <functional>
#include <thread>
#include <boost/asio/io_context.hpp>

namespace neo {
namespace network {

// Forward declarations
class TcpConnection;

/**
 * @brief Connection pool for efficient network resource management
 */
class ConnectionPool {
public:
    struct Config {
        size_t min_connections;         // Minimum connections to maintain
        size_t max_connections;        // Maximum connections allowed
        size_t max_idle_connections;   // Maximum idle connections to keep
        std::chrono::seconds idle_timeout;  // 5 minutes idle timeout
        std::chrono::seconds connection_timeout; // Connection establishment timeout
        bool enable_keep_alive;      // Enable TCP keep-alive
        std::chrono::seconds keep_alive_interval; // Keep-alive interval
        
        Config()
            : min_connections(5)
            , max_connections(50)
            , max_idle_connections(20)
            , idle_timeout(300)
            , connection_timeout(30)
            , enable_keep_alive(true)
            , keep_alive_interval(60) {}
    };

    /**
     * @brief Connection statistics
     */
    struct Stats {
        size_t total_connections = 0;
        size_t active_connections = 0;
        size_t idle_connections = 0;
        size_t failed_connections = 0;
        size_t reused_connections = 0;
        uint64_t total_bytes_sent = 0;
        uint64_t total_bytes_received = 0;
    };

private:
    /**
     * @brief Pooled connection wrapper
     */
    struct PooledConnection {
        std::shared_ptr<TcpConnection> connection;
        std::chrono::steady_clock::time_point last_used;
        std::string host;
        uint16_t port;
        bool in_use;
        
        PooledConnection(std::shared_ptr<TcpConnection> conn, 
                        const std::string& h, 
                        uint16_t p)
            : connection(conn)
            , last_used(std::chrono::steady_clock::now())
            , host(h)
            , port(p)
            , in_use(false) {}
    };

    Config config_;
    
    // Internal atomic statistics
    struct AtomicStats {
        std::atomic<size_t> total_connections{0};
        std::atomic<size_t> active_connections{0};
        std::atomic<size_t> idle_connections{0};
        std::atomic<size_t> failed_connections{0};
        std::atomic<size_t> reused_connections{0};
        std::atomic<uint64_t> total_bytes_sent{0};
        std::atomic<uint64_t> total_bytes_received{0};
    };
    AtomicStats stats_;
    
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    
    // Connection pools organized by endpoint
    std::unordered_map<std::string, std::queue<std::shared_ptr<PooledConnection>>> idle_pools_;
    std::unordered_map<std::string, std::vector<std::shared_ptr<PooledConnection>>> active_pools_;
    
    // Connection factory
    std::function<std::shared_ptr<TcpConnection>(const std::string&, uint16_t)> connection_factory_;
    
    // Background cleanup thread
    std::thread cleanup_thread_;
    std::atomic<bool> running_{false};
    
    // Boost ASIO context (optional, for default factory)
    std::unique_ptr<boost::asio::io_context> io_context_;

public:
    explicit ConnectionPool(const Config& config = Config());
    ~ConnectionPool();

    /**
     * @brief Set the connection factory function
     */
    void SetConnectionFactory(
        std::function<std::shared_ptr<TcpConnection>(const std::string&, uint16_t)> factory) {
        connection_factory_ = factory;
    }

    /**
     * @brief Get a connection from the pool
     * @param host Target host
     * @param port Target port
     * @return Connection handle (nullptr if failed)
     */
    std::shared_ptr<TcpConnection> GetConnection(const std::string& host, uint16_t port);

    /**
     * @brief Return a connection to the pool
     * @param connection Connection to return
     */
    void ReturnConnection(std::shared_ptr<TcpConnection> connection);

    /**
     * @brief Close all connections to a specific endpoint
     * @param host Target host
     * @param port Target port
     */
    void CloseEndpoint(const std::string& host, uint16_t port);

    /**
     * @brief Get current pool statistics
     */
    Stats GetStats() const { 
        Stats result;
        result.total_connections = stats_.total_connections.load();
        result.active_connections = stats_.active_connections.load();
        result.idle_connections = stats_.idle_connections.load();
        result.failed_connections = stats_.failed_connections.load();
        result.reused_connections = stats_.reused_connections.load();
        result.total_bytes_sent = stats_.total_bytes_sent.load();
        result.total_bytes_received = stats_.total_bytes_received.load();
        return result;
    }

    /**
     * @brief Start the connection pool
     */
    void Start();

    /**
     * @brief Stop the connection pool
     */
    void Stop();

    /**
     * @brief Perform health check on all connections
     */
    void HealthCheck();

private:
    /**
     * @brief Create endpoint key from host and port
     */
    std::string MakeEndpointKey(const std::string& host, uint16_t port) const {
        return host + ":" + std::to_string(port);
    }

    /**
     * @brief Create a new connection
     */
    std::shared_ptr<TcpConnection> CreateConnection(const std::string& host, uint16_t port);

    /**
     * @brief Cleanup idle connections
     */
    void CleanupIdleConnections();

    /**
     * @brief Background cleanup task
     */
    void CleanupTask();

    /**
     * @brief Validate a connection is still alive
     */
    bool IsConnectionAlive(const std::shared_ptr<TcpConnection>& connection);

    /**
     * @brief Remove expired connections from pool
     */
    void RemoveExpiredConnections();
};

/**
 * @brief RAII connection handle for automatic return to pool
 */
class PooledConnectionHandle {
private:
    std::shared_ptr<TcpConnection> connection_;
    ConnectionPool* pool_;
    bool returned_{false};

public:
    PooledConnectionHandle(std::shared_ptr<TcpConnection> conn, ConnectionPool* pool)
        : connection_(conn), pool_(pool) {}

    ~PooledConnectionHandle() {
        if (!returned_ && connection_ && pool_) {
            pool_->ReturnConnection(connection_);
        }
    }

    // Disable copy
    PooledConnectionHandle(const PooledConnectionHandle&) = delete;
    PooledConnectionHandle& operator=(const PooledConnectionHandle&) = delete;

    // Enable move
    PooledConnectionHandle(PooledConnectionHandle&& other) noexcept
        : connection_(std::move(other.connection_))
        , pool_(other.pool_)
        , returned_(other.returned_) {
        other.returned_ = true;
    }

    PooledConnectionHandle& operator=(PooledConnectionHandle&& other) noexcept {
        if (this != &other) {
            if (!returned_ && connection_ && pool_) {
                pool_->ReturnConnection(connection_);
            }
            connection_ = std::move(other.connection_);
            pool_ = other.pool_;
            returned_ = other.returned_;
            other.returned_ = true;
        }
        return *this;
    }

    /**
     * @brief Get the underlying connection
     */
    std::shared_ptr<TcpConnection> Get() const { return connection_; }

    /**
     * @brief Access the connection
     */
    TcpConnection* operator->() const { return connection_.get(); }

    /**
     * @brief Check if connection is valid
     */
    explicit operator bool() const { return connection_ != nullptr; }

    /**
     * @brief Manually return connection to pool
     */
    void Return() {
        if (!returned_ && connection_ && pool_) {
            pool_->ReturnConnection(connection_);
            returned_ = true;
            connection_.reset();
        }
    }
};

} // namespace network
} // namespace neo