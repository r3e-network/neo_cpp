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
namespace httplib {
class Request;
class Server;
}
namespace neo { class NeoSystem; }

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
using RpcRequestHandler = std::function<io::JsonValue(const std::string& method,
                                                     const io::JsonValue& params)>;

/**
 * @brief JSON-RPC 2.0 server implementation for Neo
 */
class RpcServer
{
   private:
    RpcConfig config_;
    std::shared_ptr<ledger::Blockchain> blockchain_;
    std::shared_ptr<network::p2p::LocalNode> local_node_;
    std::shared_ptr<NeoSystem> neo_system_;
    std::shared_ptr<core::Logger> logger_;
    std::thread server_thread_;
   std::shared_ptr<httplib::Server> http_server_;
   std::atomic<bool> started_{false};
    mutable std::mutex methods_mutex_;
    std::unordered_map<std::string, RpcMethodHandler> plugin_methods_;
    RpcRequestHandler plugin_handler_{};
    
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
     * @brief Construct RPC server with NeoSystem
     */
    RpcServer(const RpcConfig& config, std::shared_ptr<NeoSystem> neo_system);
    
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

    // Plugin integration
    void RegisterRequestHandler(RpcRequestHandler handler);
    void UnregisterRequestHandler();
    void RegisterMethod(const std::string& name, RpcMethodHandler handler);
    void UnregisterMethod(const std::string& name);

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
     * @brief Get client IP address from request (enabled when HTTP server is available)
     */
    std::string GetClientIP(const httplib::Request& req) const;
    
    /**
     * @brief Check if request is authenticated (enabled when HTTP server is available)
     */
    bool IsAuthenticated(const httplib::Request& req) const;
    
    /**
     * @brief Check if RPC method is allowed
     */
    bool IsMethodAllowed(const io::JsonValue& request) const;

    /**
     * @brief Validate a JSON-RPC 2.0 request
     * @return Empty string if valid, or error message
     */
    std::string ValidateRequest(const io::JsonValue& request);

    /**
     * @brief Create a JSON-RPC success response
     */
    io::JsonValue CreateResponse(const io::JsonValue& id, const io::JsonValue& result);

    /**
     * @brief Create a JSON-RPC error response
     */
    io::JsonValue CreateErrorResponse(const io::JsonValue& id, int code, const std::string& message);
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
