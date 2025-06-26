#include <neo/rpc/rpc_server_simple.h>
#include <neo/json/jstring.h>
#include <neo/json/jnumber.h>
#include <neo/json/jarray.h>
#include <neo/json/jboolean.h>
// Note: Using nullptr for null values since JNull class doesn't exist
#include <sstream>

#ifdef HAS_HTTPLIB
#include <httplib.h>
#endif

namespace neo::rpc
{
    RpcServer::RpcServer(const RpcConfig& config)
        : config_(config),
          logger_(core::Logger::GetInstance())
    {
        InitializeHandlers();
    }

    RpcServer::~RpcServer()
    {
        Stop();
    }

    void RpcServer::Start()
    {
        if (running_.exchange(true))
        {
            return; // Already running
        }

#ifdef HAS_HTTPLIB
        LOG_INFO("Starting RPC server on {}:{}", config_.bind_address, config_.port);
        server_thread_ = std::thread(&RpcServer::ServerLoop, this);
#else
        LOG_WARNING("RPC server disabled - httplib not available");
        running_ = false;
#endif
    }

    void RpcServer::Stop()
    {
        if (!running_.exchange(false))
        {
            return; // Already stopped
        }

        LOG_INFO("Stopping RPC server");

        if (server_thread_.joinable())
        {
            server_thread_.join();
        }
    }

    json::JObject RpcServer::GetStatistics() const
    {
        json::JObject stats;
        stats.SetProperty("totalRequests", std::make_shared<json::JNumber>(total_requests_.load()));
        stats.SetProperty("failedRequests", std::make_shared<json::JNumber>(failed_requests_.load()));
        stats.SetProperty("uptime", std::make_shared<json::JNumber>(0)); // TODO: Track uptime
        return stats;
    }

    void RpcServer::InitializeHandlers()
    {
        // Core blockchain methods
        method_handlers_["getblockcount"] = [this](const json::JArray& params) { return GetBlockCount(params); };
        method_handlers_["getversion"] = [this](const json::JArray& params) { return GetVersion(params); };
        method_handlers_["validateaddress"] = [this](const json::JArray& params) { return ValidateAddress(params); };
        
        // Extended Neo RPC methods
        method_handlers_["getpeers"] = [this](const json::JArray& params) { return GetPeers(params); };
        method_handlers_["getconnectioncount"] = [this](const json::JArray& params) { return GetConnectionCount(params); };
        method_handlers_["getnep17balances"] = [this](const json::JArray& params) { return GetNep17Balances(params); };
        method_handlers_["getnep17transfers"] = [this](const json::JArray& params) { return GetNep17Transfers(params); };
        method_handlers_["getstate"] = [this](const json::JArray& params) { return GetState(params); };
        method_handlers_["getstateroot"] = [this](const json::JArray& params) { return GetStateRoot(params); };
        method_handlers_["getblockheader"] = [this](const json::JArray& params) { return GetBlockHeader(params); };
        method_handlers_["gettransactionheight"] = [this](const json::JArray& params) { return GetTransactionHeight(params); };
    }

    void RpcServer::ServerLoop()
    {
#ifdef HAS_HTTPLIB
        httplib::Server server;

        // Configure CORS if enabled
        if (config_.enable_cors)
        {
            server.set_default_headers({
                {"Access-Control-Allow-Origin", "*"},
                {"Access-Control-Allow-Methods", "POST, GET, OPTIONS"},
                {"Access-Control-Allow-Headers", "Content-Type, Authorization"}
            });

            server.Options(".*", [](const httplib::Request&, httplib::Response& res) {
                res.status = 200;
            });
        }

        // Main RPC endpoint
        server.Post("/", [this](const httplib::Request& req, httplib::Response& res) {
            total_requests_++;

            try
            {
                // Parse JSON-RPC request
                auto request_json = json::JToken::Parse(req.body);
                if (!request_json) {
                    throw std::runtime_error("Invalid JSON");
                }
                
                // Extract method and params
                auto method_token = request_json->operator[]("method");
                auto params_token = request_json->operator[]("params");
                auto id_token = request_json->operator[]("id");
                
                if (!method_token) {
                    throw std::runtime_error("Missing method");
                }
                
                std::string method = method_token->AsString();
                json::JArray params;
                if (params_token) {
                    // Convert params to JArray if present
                    // For simplicity, we'll use empty params for now
                }
                
                // Find and execute handler
                json::JObject result;
                auto handler_it = method_handlers_.find(method);
                if (handler_it != method_handlers_.end()) {
                    result = handler_it->second(params);
                } else {
                    // Method not found - return error
                    json::JObject error_response;
                    error_response.SetProperty("jsonrpc", std::make_shared<json::JString>("2.0"));
                    error_response.SetProperty("id", id_token ? id_token : nullptr);
                    
                    json::JObject error;
                    error.SetProperty("code", std::make_shared<json::JNumber>(-32601));
                    error.SetProperty("message", std::make_shared<json::JString>("Method not found"));
                    
                    error_response.SetProperty("error", std::make_shared<json::JObject>(error));
                    
                    res.set_content(error_response.ToString(), "application/json");
                    res.status = 200;
                    return;
                }
                
                // Create successful response
                json::JObject response;
                response.SetProperty("jsonrpc", std::make_shared<json::JString>("2.0"));
                response.SetProperty("id", id_token ? id_token : std::make_shared<json::JNumber>(1));
                response.SetProperty("result", std::make_shared<json::JObject>(result));
                
                res.set_content(response.ToString(), "application/json");
                res.status = 200;
            }
            catch (const std::exception& e)
            {
                failed_requests_++;
                LOG_ERROR("RPC request failed: {}", e.what());
                
                json::JObject error_response;
                error_response.SetProperty("jsonrpc", std::make_shared<json::JString>("2.0"));
                error_response.SetProperty("id", nullptr); // Using nullptr for null values
                
                json::JObject error;
                error.SetProperty("code", std::make_shared<json::JNumber>(-32603));
                error.SetProperty("message", std::make_shared<json::JString>("Internal error"));
                
                error_response.SetProperty("error", std::make_shared<json::JObject>(error));
                
                res.set_content(error_response.ToString(), "application/json");
                res.status = 200;
            }
        });

        // Health check endpoint
        server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("{\"status\":\"healthy\"}", "application/json");
            res.status = 200;
        });

        // Start server
        LOG_INFO("RPC server listening on {}:{}", config_.bind_address, config_.port);
        server.listen(config_.bind_address.c_str(), config_.port);
#else
        LOG_WARNING("RPC server not started - httplib not available");
#endif
    }

    // Simple method implementations
    json::JObject RpcServer::GetBlockCount(const json::JArray& /* params */)
    {
        json::JObject result;
        result.SetProperty("blockcount", std::make_shared<json::JNumber>(0));
        return result;
    }

    json::JObject RpcServer::GetVersion(const json::JArray& /* params */)
    {
        json::JObject result;
        result.SetProperty("tcpport", std::make_shared<json::JNumber>(config_.port));
        result.SetProperty("nonce", std::make_shared<json::JNumber>(0));
        result.SetProperty("useragent", std::make_shared<json::JString>("/Neo:3.6.0/"));
        
        json::JObject protocol;
        protocol.SetProperty("network", std::make_shared<json::JNumber>(0));
        protocol.SetProperty("validatorscount", std::make_shared<json::JNumber>(1));
        protocol.SetProperty("msperblock", std::make_shared<json::JNumber>(15000));
        
        result.SetProperty("protocol", std::make_shared<json::JObject>(protocol));
        return result;
    }

    json::JObject RpcServer::ValidateAddress(const json::JArray& /* params */)
    {
        json::JObject result;
        result.SetProperty("address", std::make_shared<json::JString>("test"));
        result.SetProperty("isvalid", std::make_shared<json::JBoolean>(false));
        return result;
    }

    json::JObject RpcServer::GetPeers(const json::JArray& /* params */)
    {
        json::JObject result;
        
        // Create mock connected peers array
        json::JArray connected;
        json::JObject peer1;
        peer1.SetProperty("address", std::make_shared<json::JString>("127.0.0.1"));
        peer1.SetProperty("port", std::make_shared<json::JNumber>(20333));
        connected.Add(std::make_shared<json::JObject>(peer1));
        
        result.SetProperty("connected", std::make_shared<json::JArray>(connected));
        result.SetProperty("bad", std::make_shared<json::JArray>(json::JArray()));
        result.SetProperty("unconnected", std::make_shared<json::JArray>(json::JArray()));
        
        return result;
    }

    json::JObject RpcServer::GetConnectionCount(const json::JArray& /* params */)
    {
        json::JObject result;
        result.SetProperty("count", std::make_shared<json::JNumber>(1)); // Mock connection count
        return result;
    }

    json::JObject RpcServer::GetNep17Balances(const json::JArray& /* params */)
    {
        json::JObject result;
        result.SetProperty("address", std::make_shared<json::JString>("NcJCwvKWMMLT2WAbdvnCXxFPfYf5IcByDZ"));
        
        // Mock NEO and GAS balances
        json::JArray balances;
        
        json::JObject neoBalance;
        neoBalance.SetProperty("assethash", std::make_shared<json::JString>("0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5"));
        neoBalance.SetProperty("amount", std::make_shared<json::JString>("100"));
        neoBalance.SetProperty("lastupdatedblock", std::make_shared<json::JNumber>(1000));
        balances.Add(std::make_shared<json::JObject>(neoBalance));
        
        json::JObject gasBalance;
        gasBalance.SetProperty("assethash", std::make_shared<json::JString>("0xd2a4cff31913016155e38e474a2c06d08be276cf"));
        gasBalance.SetProperty("amount", std::make_shared<json::JString>("50.12345678"));
        gasBalance.SetProperty("lastupdatedblock", std::make_shared<json::JNumber>(1000));
        balances.Add(std::make_shared<json::JObject>(gasBalance));
        
        result.SetProperty("balance", std::make_shared<json::JArray>(balances));
        return result;
    }

    json::JObject RpcServer::GetNep17Transfers(const json::JArray& /* params */)
    {
        json::JObject result;
        result.SetProperty("sent", std::make_shared<json::JArray>(json::JArray()));
        result.SetProperty("received", std::make_shared<json::JArray>(json::JArray()));
        result.SetProperty("address", std::make_shared<json::JString>("NcJCwvKWMMLT2WAbdvnCXxFPfYf5IcByDZ"));
        return result;
    }

    json::JObject RpcServer::GetState(const json::JArray& /* params */)
    {
        json::JObject result;
        result.SetProperty("script", std::make_shared<json::JString>(""));
        result.SetProperty("state", std::make_shared<json::JString>("HALT"));
        result.SetProperty("gasconsumed", std::make_shared<json::JString>("0"));
        result.SetProperty("stack", std::make_shared<json::JArray>(json::JArray()));
        return result;
    }

    json::JObject RpcServer::GetStateRoot(const json::JArray& /* params */)
    {
        json::JObject result;
        result.SetProperty("version", std::make_shared<json::JNumber>(0));
        result.SetProperty("index", std::make_shared<json::JNumber>(1000));
        result.SetProperty("roothash", std::make_shared<json::JString>("0x0000000000000000000000000000000000000000000000000000000000000000"));
        
        json::JArray witnesses;
        result.SetProperty("witnesses", std::make_shared<json::JArray>(witnesses));
        
        return result;
    }

    json::JObject RpcServer::GetBlockHeader(const json::JArray& /* params */)
    {
        json::JObject result;
        result.SetProperty("hash", std::make_shared<json::JString>("0x0000000000000000000000000000000000000000000000000000000000000000"));
        result.SetProperty("size", std::make_shared<json::JNumber>(401));
        result.SetProperty("version", std::make_shared<json::JNumber>(0));
        result.SetProperty("previousblockhash", std::make_shared<json::JString>("0x0000000000000000000000000000000000000000000000000000000000000000"));
        result.SetProperty("merkleroot", std::make_shared<json::JString>("0x0000000000000000000000000000000000000000000000000000000000000000"));
        result.SetProperty("time", std::make_shared<json::JNumber>(1622548800));
        result.SetProperty("index", std::make_shared<json::JNumber>(1000));
        result.SetProperty("nonce", std::make_shared<json::JString>("0000000000000000"));
        result.SetProperty("speaker", std::make_shared<json::JNumber>(0));
        
        json::JArray witnesses;
        result.SetProperty("witnesses", std::make_shared<json::JArray>(witnesses));
        
        return result;
    }

    json::JObject RpcServer::GetTransactionHeight(const json::JArray& /* params */)
    {
        json::JObject result;
        result.SetProperty("height", std::make_shared<json::JNumber>(1000));
        return result;
    }
}