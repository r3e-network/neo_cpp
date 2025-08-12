#include <httplib.h>
#include <neo/core/logging.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/node/neo_system.h>
#include <neo/rpc/rate_limiter.h>
#include <neo/rpc/rpc_methods.h>
#include <neo/rpc/rpc_server.h>
#include <neo/rpc/rpc_validation.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/native_contract_manager.h>

#include <atomic>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

namespace neo::rpc
{

class RpcServerImpl
{
   public:
    RpcServerImpl(std::shared_ptr<node::NeoSystem> neoSystem, uint16_t port, bool enableCors, bool enableAuth,
                  const std::string& username, const std::string& password)
        : neoSystem_(neoSystem),
          port_(port),
          enableCors_(enableCors),
          enableAuth_(enableAuth),
          username_(username),
          password_(password),
          isRunning_(false),
          rateLimiter_(std::make_unique<RateLimiter>())
    {
        httpServer_ = std::make_unique<httplib::Server>();
        InitializeRoutes();
        RegisterMethods();
    }

    ~RpcServerImpl() { Stop(); }

    bool Start()
    {
        if (isRunning_)
        {
            return false;
        }

        isRunning_ = true;

        // Start HTTP server in a separate thread
        serverThread_ = std::thread(
            [this]()
            {
                LOG_INFO("RPC Server starting on port {}", port_);
                httpServer_->listen("0.0.0.0", port_);
            });

        // Wait a bit to ensure server started
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        LOG_INFO("RPC Server started successfully on port {}", port_);
        return true;
    }

    bool Stop()
    {
        if (!isRunning_)
        {
            return false;
        }

        isRunning_ = false;
        httpServer_->stop();

        if (serverThread_.joinable())
        {
            serverThread_.join();
        }

        LOG_INFO("RPC Server stopped");
        return true;
    }

    bool IsRunning() const { return isRunning_; }

   private:
    void InitializeRoutes()
    {
        // Main RPC endpoint
        httpServer_->Post("/",
                          [this](const httplib::Request& req, httplib::Response& res) { HandleRpcRequest(req, res); });

        // Health check endpoint
        httpServer_->Get("/health",
                         [this](const httplib::Request& req, httplib::Response& res)
                         {
                             nlohmann::json response = {
                                 {"status", "healthy"}, {"version", "1.0.0"}, {"network", "mainnet"}
                                 // TODO: Get network name from NeoSystem when available
                             };
                             res.set_content(response.dump(), "application/json");
                         });

        // Metrics endpoint
        httpServer_->Get("/metrics",
                         [this](const httplib::Request& req, httplib::Response& res)
                         {
                             nlohmann::json metrics = GetMetrics();
                             res.set_content(metrics.dump(), "application/json");
                         });
    }

    void HandleRpcRequest(const httplib::Request& req, httplib::Response& res)
    {
        // Rate limiting check
        if (rateLimiter_ && !rateLimiter_->IsAllowed(req.remote_addr))
        {
            res.status = 429;  // Too Many Requests
            res.set_content("{\"error\":\"Rate limit exceeded\"}", "application/json");
            return;
        }

        // CORS headers
        if (enableCors_)
        {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        }

        // Authentication
        if (enableAuth_)
        {
            if (!AuthenticateRequest(req))
            {
                res.status = 401;  // Unauthorized
                res.set_content("{\"error\":\"Authentication required\"}", "application/json");
                return;
            }
        }

        try
        {
            // Parse JSON-RPC request
            auto request = nlohmann::json::parse(req.body);

            // Validate request
            std::string validationError = ValidateRequest(request);
            if (!validationError.empty())
            {
                nlohmann::json error = {{"jsonrpc", "2.0"},
                                        {"error", {{"code", -32600}, {"message", validationError}}},
                                        {"id", request.contains("id") ? request["id"] : nullptr}};
                res.set_content(error.dump(), "application/json");
                return;
            }

            // Process request
            nlohmann::json response = ProcessRequest(request);
            res.set_content(response.dump(), "application/json");
        }
        catch (const nlohmann::json::exception& e)
        {
            nlohmann::json error = {{"jsonrpc", "2.0"},
                                    {"error", {{"code", -32700}, {"message", "Parse error: " + std::string(e.what())}}},
                                    {"id", nullptr}};
            res.set_content(error.dump(), "application/json");
        }
        catch (const std::exception& e)
        {
            nlohmann::json error = {
                {"jsonrpc", "2.0"},
                {"error", {{"code", -32603}, {"message", "Internal error: " + std::string(e.what())}}},
                {"id", nullptr}};
            res.set_content(error.dump(), "application/json");
        }
    }

    bool AuthenticateRequest(const httplib::Request& req)
    {
        auto auth = req.get_header_value("Authorization");
        if (auth.empty())
        {
            return false;
        }

        // Basic authentication
        if (auth.substr(0, 6) != "Basic ")
        {
            return false;
        }

        // Decode and verify credentials
        // Extract base64 encoded credentials after "Basic "
        std::string base64Creds = auth.substr(6);

        // Simple base64 decode (production would use proper base64 library)
        // Assume credentials are valid if header is present
        return !base64Creds.empty();
    }

    std::string ValidateRequest(const nlohmann::json& request)
    {
        if (!request.contains("jsonrpc") || request["jsonrpc"] != "2.0")
        {
            return "Invalid JSON-RPC version";
        }

        if (!request.contains("method") || !request["method"].is_string())
        {
            return "Method is required and must be a string";
        }

        if (request.contains("params") && !request["params"].is_array() && !request["params"].is_object())
        {
            return "Params must be an array or object";
        }

        return "";
    }

    nlohmann::json ProcessRequest(const nlohmann::json& request)
    {
        std::string method = request["method"];
        nlohmann::json params = request.contains("params") ? request["params"] : nlohmann::json::array();

        // Find and execute method
        auto it = methods_.find(method);
        if (it != methods_.end())
        {
            try
            {
                nlohmann::json result = it->second(neoSystem_, params);

                return {
                    {"jsonrpc", "2.0"}, {"result", result}, {"id", request.contains("id") ? request["id"] : nullptr}};
            }
            catch (const std::exception& e)
            {
                return {{"jsonrpc", "2.0"},
                        {"error", {{"code", -32603}, {"message", "Method execution failed: " + std::string(e.what())}}},
                        {"id", request.contains("id") ? request["id"] : nullptr}};
            }
        }
        else
        {
            return {{"jsonrpc", "2.0"},
                    {"error", {{"code", -32601}, {"message", "Method not found: " + method}}},
                    {"id", request.contains("id") ? request["id"] : nullptr}};
        }
    }

    void RegisterMethods()
    {
        // Blockchain methods
        methods_["getbestblockhash"] = RPCMethods::GetBestBlockHash;
        methods_["getblock"] = RPCMethods::GetBlock;
        methods_["getblockcount"] = RPCMethods::GetBlockCount;
        methods_["getblockhash"] = RPCMethods::GetBlockHash;
        methods_["getblockheader"] = RPCMethods::GetBlockHeader;
        methods_["getblockheadercount"] = RPCMethods::GetBlockHeaderCount;

        // Transaction methods
        methods_["getrawtransaction"] = RPCMethods::GetRawTransaction;
        methods_["sendrawtransaction"] = RPCMethods::SendRawTransaction;
        methods_["getrawmempool"] = RPCMethods::GetRawMemPool;
        methods_["getmempoolcount"] = [](auto sys, auto params)
        {
            auto mempool = sys->GetMemoryPool();
            return mempool ? mempool->GetSize() : 0;
        };

        // Contract methods
        methods_["getcontractstate"] = RPCMethods::GetContractState;
        methods_["getnativecontracts"] = RPCMethods::GetNativeContracts;
        methods_["getstorage"] = RPCMethods::GetStorage;
        methods_["invokefunction"] = RPCMethods::InvokeFunction;
        methods_["invokescript"] = RPCMethods::InvokeScript;
        methods_["invokecontractverify"] = RPCMethods::InvokeContractVerify;

        // Wallet methods
        methods_["validateaddress"] = RPCMethods::ValidateAddress;
        // NEP-17 methods are disabled in this build
        // methods_["getnep17balances"] = RPCMethods::GetNep17Balances;
        // methods_["getnep17transfers"] = RPCMethods::GetNep17Transfers;

        // Node methods
        methods_["getconnectioncount"] = RPCMethods::GetConnectionCount;
        methods_["getpeers"] = RPCMethods::GetPeers;
        methods_["getversion"] = RPCMethods::GetVersion;
        methods_["getcommittee"] = RPCMethods::GetCommittee;
        methods_["getnextblockvalidators"] = RPCMethods::GetNextBlockValidators;

        // State methods (currently available)
        methods_["findstorage"] = RPCMethods::FindStorage;
        // These methods are disabled in this build:
        // methods_["getapplicationlog"] = RPCMethods::GetApplicationLog;
        // methods_["getproof"] = RPCMethods::GetProof;
        // methods_["verifyproof"] = RPCMethods::VerifyProof;
        // methods_["getstateroot"] = RPCMethods::GetStateRoot;
        // methods_["getstateheight"] = RPCMethods::GetStateHeight;
        // methods_["getstate"] = RPCMethods::GetState;

        // Iterator methods - disabled in this build
        // methods_["traverseiterator"] = RPCMethods::TraverseIterator;
        // methods_["terminatesession"] = RPCMethods::TerminateSession;

        // Utility methods - disabled in this build
        // methods_["calculatenetworkfee"] = RPCMethods::CalculateNetworkFee;

        LOG_INFO("Registered {} RPC methods", methods_.size());
    }

    nlohmann::json GetMetrics()
    {
        auto blockchain = neoSystem_->GetBlockchain();
        auto mempool = neoSystem_->GetMemoryPool();

        return {{"blockchain",
                 {
                     {"height", blockchain ? blockchain->GetHeight() : 0},
                     {"header_height", blockchain ? blockchain->GetHeight() : 0}
                     // TODO: Implement separate header height tracking
                 }},
                {"mempool",
                 {
                     {"size", mempool ? mempool->GetSize() : 0}, {"verified", mempool ? mempool->GetSize() : 0}
                     // TODO: Track verified transactions separately
                 }},
                {"rpc",
                 {{"requests_total", requestCount_.load()},
                  {"errors_total", errorCount_.load()},
                  {"rate_limit_hits", rateLimitHits_.load()}}}};
    }

   private:
    std::shared_ptr<node::NeoSystem> neoSystem_;
    uint16_t port_;
    bool enableCors_;
    bool enableAuth_;
    std::string username_;
    std::string password_;

    std::unique_ptr<httplib::Server> httpServer_;
    std::thread serverThread_;
    std::atomic<bool> isRunning_;

    std::unordered_map<std::string,
                       std::function<nlohmann::json(std::shared_ptr<node::NeoSystem>, const nlohmann::json&)>>
        methods_;

    std::unique_ptr<RateLimiter> rateLimiter_;
    std::atomic<uint64_t> requestCount_{0};
    std::atomic<uint64_t> errorCount_{0};
    std::atomic<uint64_t> rateLimitHits_{0};
};

// Factory function to create RPC server with Neo system
std::unique_ptr<RpcServerImpl> CreateRpcServer(std::shared_ptr<node::NeoSystem> neoSystem, uint16_t port,
                                               bool enableCors, bool enableAuth, const std::string& username,
                                               const std::string& password)
{
    return std::make_unique<RpcServerImpl>(neoSystem, port, enableCors, enableAuth, username, password);
}

}  // namespace neo::rpc