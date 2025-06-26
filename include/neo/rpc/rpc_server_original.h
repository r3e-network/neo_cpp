#pragma once

#include <neo/core/logging.h>
#include <neo/json/jobject.h>
#include <neo/persistence/data_cache.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <thread>
#include <atomic>

// Forward declarations for optional network dependency
namespace neo::network::p2p {
    class LocalNode;
}

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
        size_t max_request_size{10 * 1024 * 1024}; // 10MB
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
    using RpcMethodHandler = std::function<json::JObject(const json::JArray& params)>;

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
        void SetBlockchain(std::shared_ptr<persistence::DataCache> blockchain)
        {
            blockchain_ = blockchain;
        }
        
        /**
         * @brief Set local node for P2P information
         */
        void SetLocalNode(std::shared_ptr<network::p2p::LocalNode> node)
        {
            local_node_ = node;
        }
        
        /**
         * @brief Get server statistics
         */
        json::JObject GetStatistics() const;
        
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
        json::JObject ProcessRequest(const json::JObject& request);
        
        /**
         * @brief Validate JSON-RPC request format
         * @param request The request to validate
         * @return Error message if invalid, empty string if valid
         */
        std::string ValidateRequest(const json::JObject& request);
        
        /**
         * @brief Create JSON-RPC error response
         * @param id Request ID (can be null)
         * @param code Error code
         * @param message Error message
         * @return Error response object
         */
        json::JObject CreateErrorResponse(const json::JToken* id, int code, const std::string& message);
        
        /**
         * @brief Create JSON-RPC success response
         * @param id Request ID
         * @param result Result object
         * @return Success response object
         */
        json::JObject CreateSuccessResponse(const json::JToken* id, const json::JToken& result);
        
        // RPC Method Implementations
        
        /**
         * @brief Get block by index or hash
         */
        json::JObject GetBlock(const json::JArray& params);
        
        /**
         * @brief Get block count
         */
        json::JObject GetBlockCount(const json::JArray& params);
        
        /**
         * @brief Get block hash by index
         */
        json::JObject GetBlockHash(const json::JArray& params);
        
        /**
         * @brief Get block header by index or hash
         */
        json::JObject GetBlockHeader(const json::JArray& params);
        
        /**
         * @brief Get transaction by hash
         */
        json::JObject GetTransaction(const json::JArray& params);
        
        /**
         * @brief Get contract state by hash or id
         */
        json::JObject GetContractState(const json::JArray& params);
        
        /**
         * @brief Get storage value
         */
        json::JObject GetStorage(const json::JArray& params);
        
        /**
         * @brief Get transaction height
         */
        json::JObject GetTransactionHeight(const json::JArray& params);
        
        /**
         * @brief Get next block validators
         */
        json::JObject GetNextBlockValidators(const json::JArray& params);
        
        /**
         * @brief Get committee members
         */
        json::JObject GetCommittee(const json::JArray& params);
        
        /**
         * @brief Invoke contract method (read-only)
         */
        json::JObject InvokeFunction(const json::JArray& params);
        
        /**
         * @brief Invoke script (read-only)
         */
        json::JObject InvokeScript(const json::JArray& params);
        
        /**
         * @brief Get unclaimed GAS
         */
        json::JObject GetUnclaimedGas(const json::JArray& params);
        
        /**
         * @brief List plugins
         */
        json::JObject ListPlugins(const json::JArray& params);
        
        /**
         * @brief Send raw transaction
         */
        json::JObject SendRawTransaction(const json::JArray& params);
        
        /**
         * @brief Submit new block
         */
        json::JObject SubmitBlock(const json::JArray& params);
        
        /**
         * @brief Get connection count
         */
        json::JObject GetConnectionCount(const json::JArray& params);
        
        /**
         * @brief Get connected peers
         */
        json::JObject GetPeers(const json::JArray& params);
        
        /**
         * @brief Get node version
         */
        json::JObject GetVersion(const json::JArray& params);
        
        /**
         * @brief Validate address
         */
        json::JObject ValidateAddress(const json::JArray& params);
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
}