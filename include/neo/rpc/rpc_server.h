#pragma once

#include <neo/core/logging.h>
#include <neo/io/json.h>
#include <neo/network/p2p/local_node.h>
#include <neo/persistence/data_cache.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

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
    std::shared_ptr<core::Logger> logger_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;

    // Method handlers
    std::unordered_map<std::string, RpcMethodHandler> method_handlers_;

    // Dependencies
    std::shared_ptr<persistence::DataCache> blockchain_;
    std::shared_ptr<network::p2p::LocalNode> local_node_;

    // Statistics
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> failed_requests_{0};
    std::chrono::steady_clock::time_point start_time_;

   public:
    /**
     * @brief Construct RPC server
     * @param config Server configuration
     */
    explicit RpcServer(const RpcConfig& config);

    ~RpcServer();

    /**
     * @brief Start the RPC server
     */
    void Start();

    /**
     * @brief Stop the RPC server
     */
    void Stop();

    /**
     * @brief Set blockchain data cache
     */
    void SetBlockchain(std::shared_ptr<persistence::DataCache> blockchain) { blockchain_ = blockchain; }

    /**
     * @brief Set local node for P2P information
     */
    void SetLocalNode(std::shared_ptr<network::p2p::LocalNode> node) { local_node_ = node; }

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
     * @brief Server main loop
     */
    void ServerLoop();

    /**
     * @brief Process a single JSON-RPC request
     * @param request The JSON-RPC request
     * @return The JSON-RPC response
     */
    io::JsonValue ProcessRequest(const io::JsonValue& request);

    /**
     * @brief Validate JSON-RPC request format
     * @param request The request to validate
     * @return Error message if invalid, empty string if valid
     */
    std::string ValidateRequest(const io::JsonValue& request);

    /**
     * @brief Create JSON-RPC error response
     * @param id Request ID (can be null)
     * @param code Error code
     * @param message Error message
     * @return Error response object
     */
    io::JsonValue CreateErrorResponse(const io::JsonValue* id, int code, const std::string& message);

    /**
     * @brief Create JSON-RPC success response
     * @param id Request ID
     * @param result Result object
     * @return Success response object
     */
    io::JsonValue CreateSuccessResponse(const io::JsonValue* id, const io::JsonValue& result);

    // RPC Method Implementations

    /**
     * @brief Get block by index or hash
     */
    io::JsonValue GetBlock(const io::JsonValue& params);

    /**
     * @brief Get block count
     */
    io::JsonValue GetBlockCount(const io::JsonValue& params);

    /**
     * @brief Get block hash by index
     */
    io::JsonValue GetBlockHash(const io::JsonValue& params);

    /**
     * @brief Get block header by index or hash
     */
    io::JsonValue GetBlockHeader(const io::JsonValue& params);

    /**
     * @brief Get transaction by hash
     */
    io::JsonValue GetTransaction(const io::JsonValue& params);

    /**
     * @brief Get contract state by hash or id
     */
    io::JsonValue GetContractState(const io::JsonValue& params);

    /**
     * @brief Get storage value
     */
    io::JsonValue GetStorage(const io::JsonValue& params);

    /**
     * @brief Get transaction height
     */
    io::JsonValue GetTransactionHeight(const io::JsonValue& params);

    /**
     * @brief Get next block validators
     */
    io::JsonValue GetNextBlockValidators(const io::JsonValue& params);

    /**
     * @brief Get committee members
     */
    io::JsonValue GetCommittee(const io::JsonValue& params);

    /**
     * @brief Invoke contract method (read-only)
     */
    io::JsonValue InvokeFunction(const io::JsonValue& params);

    /**
     * @brief Invoke script (read-only)
     */
    io::JsonValue InvokeScript(const io::JsonValue& params);

    /**
     * @brief Get unclaimed GAS
     */
    io::JsonValue GetUnclaimedGas(const io::JsonValue& params);

    /**
     * @brief List plugins
     */
    io::JsonValue ListPlugins(const io::JsonValue& params);

    /**
     * @brief Send raw transaction
     */
    io::JsonValue SendRawTransaction(const io::JsonValue& params);

    /**
     * @brief Submit new block
     */
    io::JsonValue SubmitBlock(const io::JsonValue& params);

    /**
     * @brief Get connection count
     */
    io::JsonValue GetConnectionCount(const io::JsonValue& params);

    /**
     * @brief Get connected peers
     */
    io::JsonValue GetPeers(const io::JsonValue& params);

    /**
     * @brief Get node version
     */
    io::JsonValue GetVersion(const io::JsonValue& params);

    /**
     * @brief Validate address
     */
    io::JsonValue ValidateAddress(const io::JsonValue& params);
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