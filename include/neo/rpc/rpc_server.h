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

// Forward declarations
namespace neo::ledger { class Blockchain; }
namespace neo::network::p2p { class LocalNode; }

namespace neo::rpc
{
/**
 * @brief RPC server configuration
 */
struct RpcConfig
{
    std::string bind_address{"127.0.0.1"};
    uint16_t port{10332};
    size_t max_concurrent_requests{100};
    size_t max_request_size{10 * 1024 * 1024};  // 10MB
    std::chrono::seconds request_timeout{30};
    bool enable_cors{true};
    std::vector<std::string> allowed_origins{"*"};
    bool enable_authentication{false};
    std::string username;
    std::string password;
    
    // Rate limiting configuration
    bool enable_rate_limiting{true};
    size_t rate_limit_requests_per_second{10};
    size_t rate_limit_burst_size{20};
    size_t rate_limit_requests_per_minute{100};
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
    RpcConfig config_;
    std::shared_ptr<ledger::Blockchain> blockchain_;
    std::shared_ptr<network::p2p::LocalNode> local_node_;
    
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