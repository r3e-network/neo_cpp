/**
 * @file rpc_server.h
 * @brief JSON-RPC server implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/core/logging.h>
#include <neo/io/json.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <queue>

// Forward declarations
namespace neo::ledger { class Blockchain; }
namespace neo::network::p2p { class LocalNode; }
namespace httplib { class Request; }

namespace neo::rpc
{
/**
 * @brief RPC server configuration
 */
struct RpcConfig
{
    std::string bind_address{"127.0.0.1"};
    uint16_t port{10332};
    uint32_t max_concurrent_requests{100};
    uint32_t max_request_size{10 * 1024 * 1024};  // 10MB
    uint32_t request_timeout_seconds{30};
    bool enable_cors{true};
    std::vector<std::string> allowed_origins{"*"};
    bool enable_authentication{false};
    std::string username;
    std::string password;
    std::vector<std::string> disabled_methods;
    
    // Rate limiting configuration
    bool enable_rate_limiting{true};
    uint32_t max_requests_per_second{100};
};

/**
 * @brief RPC method handler function type
 */
using RpcMethodHandler = std::function<io::JsonValue(const io::JsonValue& params)>;

/**
 * @brief JSON-RPC 2.0 server implementation for Neo
 */
class RpcServer
{
   private:
    class RateLimiter;
    
   private:
    RpcConfig config_;
    std::shared_ptr<ledger::Blockchain> blockchain_;
    std::shared_ptr<network::p2p::LocalNode> local_node_;
    std::unique_ptr<RateLimiter> rate_limiter_;
    std::shared_ptr<core::Logger> logger_;
    std::thread server_thread_;
    
    // Statistics
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> failed_requests_{0};
    std::chrono::steady_clock::time_point start_time_;

   public:
    /**
     * @brief Construct RPC server
     * @param config Server configuration
     */
    explicit RpcServer(const RpcConfig& config);
    
    /**
     * @brief Construct RPC server with dependencies
     * @param config Server configuration
     * @param blockchain Blockchain instance
     * @param local_node Local node instance
     */
    RpcServer(const RpcConfig& config,
              std::shared_ptr<ledger::Blockchain> blockchain,
              std::shared_ptr<network::p2p::LocalNode> local_node);

    ~RpcServer();
    
    // Disable copy and move to avoid issues with incomplete type
    RpcServer(const RpcServer&) = delete;
    RpcServer& operator=(const RpcServer&) = delete;
    RpcServer(RpcServer&&) = delete;
    RpcServer& operator=(RpcServer&&) = delete;

    /**
     * @brief Start the RPC server
     */
    void Start();

    /**
     * @brief Stop the RPC server
     */
    void Stop();

    /**
     * @brief Check if server is running
     */
    bool IsRunning() const;

    /**
     * @brief Get server statistics
     */
    io::JsonValue GetStatistics() const;

   private:
    /**
     * @brief Initialize all RPC method handlers
     */
    void InitializeHandlers();
    
    /**
     * @brief Process RPC request
     */
    io::JsonValue ProcessRequest(const io::JsonValue& request);
    
    /**
     * @brief Get client IP address from request
     */
    std::string GetClientIP(const httplib::Request& req) const;
    
    /**
     * @brief Check if request is authenticated
     */
    bool IsAuthenticated(const httplib::Request& req) const;
    
    /**
     * @brief Check if RPC method is allowed
     */
    bool IsMethodAllowed(const io::JsonValue& request) const;
};

/**
 * @brief JSON-RPC error codes
 */
enum class RpcError : int
{
    // Standard JSON-RPC 2.0 errors
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,

    // Custom Neo errors
    InvalidBlockIndex = -100,
    InvalidBlockHash = -101,
    InvalidTransactionHash = -102,
    InvalidContractHash = -103,
    UnknownBlock = -104,
    UnknownTransaction = -105,
    UnknownContract = -106,
    InsufficientFunds = -107,
    InvalidSignature = -108,
    InvalidScript = -109,
    InvalidAttribute = -110,
    InvalidWitness = -111,
    PolicyFailed = -112,
    Unknown = -113
};
}  // namespace neo::rpc