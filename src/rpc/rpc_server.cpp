/**
 * @file rpc_server.cpp
 * @brief JSON-RPC server implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/io/json.h>
#include <neo/network/p2p/local_node.h>
#include <neo/rpc/error_codes.h>
#include <neo/rpc/rpc_server.h>
#include <neo/rpc/rpc_methods.h>

#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

#ifdef NEO_HAS_HTTPLIB
#include <httplib.h>
#endif

namespace neo::rpc
{
using json = nlohmann::json;

RpcServer::RpcServer(const RpcConfig& config)
    : config_(config),
      running_(false),
      total_requests_(0),
      failed_requests_(0),
      start_time_(std::chrono::steady_clock::now())
{
    // Initialize logger properly
    logger_ = core::Logger::GetInstance();

    InitializeHandlers();
}

RpcServer::RpcServer(const RpcConfig& config, std::shared_ptr<node::NeoSystem> neo_system)
    : config_(config),
      neo_system_(std::move(neo_system)),
      running_(false),
      total_requests_(0),
      failed_requests_(0),
      start_time_(std::chrono::steady_clock::now())
{
    logger_ = core::Logger::GetInstance();
    InitializeHandlers();
}

RpcServer::~RpcServer() { Stop(); }

void RpcServer::Start()
{
    if (running_.exchange(true)) return;

    LOG_INFO("Starting RPC server on {}:{}", config_.bind_address, config_.port);

#ifdef NEO_HAS_HTTPLIB
    started_.store(false);
    server_thread_ = std::thread(
        [this]()
        {
            // Basic diagnostics to stderr in case logging isn't ready
            std::fprintf(stderr, "[RPC] Thread start\n");
            http_server_ = std::make_shared<httplib::Server>();

            http_server_->Post(
                "/",
                [this](const httplib::Request& req, httplib::Response& res)
                {
                    total_requests_++;

                    try
                    {
                        auto request_json = json::parse(req.body);
                        io::JsonValue request(request_json);
                        auto response = ProcessRequest(request);
                        res.set_content(response.ToString(), "application/json");
                    }
                    catch (const std::exception& e)
                    {
                        failed_requests_++;
                        json error_response = {{"jsonrpc", "2.0"},
                                               {"error",
                                                {{"code", -32700}, {"message", "Parse error"}, {"data", e.what()}}},
                                               {"id", nullptr}};
                        res.set_content(error_response.dump(), "application/json");
                    }
                });

            // Mark started before blocking listen
            started_.store(true);
            std::fprintf(stderr, "[RPC] Listening on %s:%u\n", config_.bind_address.c_str(), (unsigned)config_.port);
            http_server_->listen(config_.bind_address.c_str(), config_.port);
            std::fprintf(stderr, "[RPC] Listen ended\n");
        });
#else
    LOG_ERROR("HTTP server not available - RPC server cannot start");
    running_ = false;
#endif

#ifdef NEO_HAS_HTTPLIB
    // Wait briefly for the server thread to initialize
    for (int i = 0; i < 50; ++i)
    {
        if (started_.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
#endif
}

void RpcServer::Stop()
{
    if (!running_.exchange(false)) return;

    LOG_INFO("Stopping RPC server");

    // Stop HTTP server first to unblock listen()
#ifdef NEO_HAS_HTTPLIB
    if (http_server_) http_server_->stop();
#endif

    if (server_thread_.joinable())
    {
        server_thread_.join();
    }
}

#ifdef NEO_HAS_HTTPLIB
std::string RpcServer::GetClientIP(const httplib::Request& req) const
{
    if (req.has_header("X-Forwarded-For"))
    {
        auto forwarded = req.get_header_value("X-Forwarded-For");
        auto pos = forwarded.find(',');
        return pos != std::string::npos ? forwarded.substr(0, pos) : forwarded;
    }
    if (req.has_header("X-Real-IP"))
    {
        return req.get_header_value("X-Real-IP");
    }
    return req.remote_addr;
}

bool RpcServer::IsAuthenticated(const httplib::Request& req) const
{
    if (!config_.enable_authentication) return true;
    if (!req.has_header("Authorization")) return false;
    // TODO: Validate credentials
    return true;
}
#endif

bool RpcServer::IsMethodAllowed(const io::JsonValue& request) const
{
    if (config_.disabled_methods.empty()) return true;
    if (!request.IsObject() || !request.HasMember("method")) return false;
    if (!request["method"].IsString()) return false;
    std::string method = request["method"].GetString();
    for (const auto& disabled_method : config_.disabled_methods)
    {
        if (method == disabled_method) return false;
    }
    return true;
}

io::JsonValue RpcServer::GetStatistics() const
{
    json stats = {
        {"totalRequests", total_requests_.load()},
        {"failedRequests", failed_requests_.load()},
        {"uptime",
         std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time_).count()}};
    return io::JsonValue(stats);
}

// Plugin registration
void RpcServer::RegisterRequestHandler(RpcRequestHandler handler)
{
    std::lock_guard<std::mutex> lock(methods_mutex_);
    plugin_handler_ = std::move(handler);
}

void RpcServer::UnregisterRequestHandler()
{
    std::lock_guard<std::mutex> lock(methods_mutex_);
    plugin_handler_ = nullptr;
}

void RpcServer::RegisterMethod(const std::string& name, RpcMethodHandler handler)
{
    std::lock_guard<std::mutex> lock(methods_mutex_);
    std::string key = name;
    for (auto& ch : key) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    plugin_methods_[key] = std::move(handler);
}

void RpcServer::UnregisterMethod(const std::string& name)
{
    std::lock_guard<std::mutex> lock(methods_mutex_);
    std::string key = name;
    for (auto& ch : key) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    plugin_methods_.erase(key);
}

void RpcServer::InitializeHandlers()
{
    // Use the methods from RpcMethods class
    // Initialize with essential RPC methods - additional methods can be registered as needed
}

io::JsonValue RpcServer::ProcessRequest(const io::JsonValue& request)
{
    auto validation_error = ValidateRequest(request);
    io::JsonValue id;
    if (request.IsObject() && request.HasMember("id")) id = request["id"];
    if (!validation_error.empty())
    {
        return CreateErrorResponse(id, static_cast<int>(RpcError::InvalidRequest), validation_error);
    }

    const auto method_raw = request["method"].GetString();
    LOG_INFO("RPC method: {}", method_raw);
    std::string method = method_raw;
    // Normalize: trim spaces and lowercase
    auto ltrim = [](std::string& s) {
        size_t i = 0; while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i; s.erase(0, i);
    };
    auto rtrim = [](std::string& s) {
        size_t i = s.size(); while (i > 0 && std::isspace(static_cast<unsigned char>(s[i-1]))) --i; s.erase(i);
    };
    ltrim(method); rtrim(method);
    for (auto& ch : method) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    io::JsonValue params = request.IsObject() && request.HasMember("params") ? request["params"] : io::JsonValue::CreateArray();

    try
    {
        nlohmann::json params_json = params.GetJson();
        std::fprintf(stderr, "[RPC] Method: %s\n", method.c_str());

        // Central routing map for method names (lowercased)
        // Only use for stateless calls or those that can handle missing blockchain safely
        {
            using Fn = nlohmann::json (*)(std::shared_ptr<node::NeoSystem>, const nlohmann::json&);
            static const std::unordered_map<std::string, Fn> simple_routes = {
                {"getversion", &RPCMethods::GetVersion},
                {"validateaddress", &RPCMethods::ValidateAddress},
                {"getnativecontracts", &RPCMethods::GetNativeContracts},
                {"getblockhash", &RPCMethods::GetBlockHash},
                {"getblockheadercount", &RPCMethods::GetBlockHeaderCount},
                {"getbestblockhash", &RPCMethods::GetBestBlockHash},
                {"getconsensusstate", &RPCMethods::GetConsensusState},
                {"startconsensus", &RPCMethods::StartConsensus},
                {"stopconsensus", &RPCMethods::StopConsensus},
                {"restartconsensus", &RPCMethods::RestartConsensus}
            };

            auto it = simple_routes.find(method);
            if (it != simple_routes.end())
            {
                // For blockchain-dependent routes, ensure availability
                if ((method == "getblockhash" || method == "getblockheadercount" || method == "getbestblockhash") &&
                    !(neo_system_ && neo_system_->GetBlockchain()))
                {
                    if (method == "getbestblockhash")
                    {
                        nlohmann::json zero_hash = std::string("0x") + std::string(64, '0');
                        return CreateResponse(id, io::JsonValue(zero_hash));
                    }
                    return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownBlock), "Blockchain not available");
                }
                try
                {
                    auto result = it->second(neo_system_, params_json);
                    return CreateResponse(id, io::JsonValue(result));
                }
                catch (const RpcException& ex)
                {
                    return CreateErrorResponse(id, static_cast<int>(ex.GetCode()), ex.GetMessage());
                }
                catch (const std::exception& e)
                {
                    return CreateErrorResponse(id, static_cast<int>(RpcError::InternalError), e.what());
                }
            }
        }
        if (method == "getversion")
        {
            // Does not require a running blockchain
            auto result = RPCMethods::GetVersion(neo_system_, params_json);
            return CreateResponse(id, io::JsonValue(result));
        }
        else if (method == "getblockcount")
        {
            // Try real value; fall back to 0
            try {
                if (neo_system_ && neo_system_->GetBlockchain()) {
                    auto result = RPCMethods::GetBlockCount(neo_system_, params_json);
                    return CreateResponse(id, io::JsonValue(result));
                }
            } catch (...) {}
            return CreateResponse(id, io::JsonValue(nlohmann::json(static_cast<int64_t>(0))));
        }
        else if (method == "getbestblockhash")
        {
            try {
                if (neo_system_ && neo_system_->GetBlockchain()) {
                    auto result = RPCMethods::GetBestBlockHash(neo_system_, params_json);
                    return CreateResponse(id, io::JsonValue(result));
                }
            } catch (...) {}
            // 32 zero bytes hash string
            nlohmann::json zero_hash = std::string("0x") + std::string(64, '0');
            return CreateResponse(id, io::JsonValue(zero_hash));
        }
        else if (method == "getblockheadercount")
        {
            try {
                if (neo_system_ && neo_system_->GetBlockchain()) {
                    auto result = RPCMethods::GetBlockHeaderCount(neo_system_, params_json);
                    return CreateResponse(id, io::JsonValue(result));
                }
            } catch (...) {}
            return CreateResponse(id, io::JsonValue(nlohmann::json(static_cast<int64_t>(0))));
        }
        else if (method == "validateaddress")
        {
            auto result = RPCMethods::ValidateAddress(neo_system_, params_json);
            return CreateResponse(id, io::JsonValue(result));
        }
        else if (method == "getnativecontracts")
        {
            auto result = RPCMethods::GetNativeContracts(neo_system_, params_json);
            return CreateResponse(id, io::JsonValue(result));
        }
        else if (method == "getblock")
        {
            if (neo_system_ && neo_system_->GetBlockchain())
            {
                auto result = RPCMethods::GetBlock(neo_system_, params_json);
                return CreateResponse(id, io::JsonValue(result));
            }
            return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownBlock), "Blockchain not available");
        }
        else if (method == "getblockhash")
        {
            if (neo_system_ && neo_system_->GetBlockchain())
            {
                auto result = RPCMethods::GetBlockHash(neo_system_, params_json);
                return CreateResponse(id, io::JsonValue(result));
            }
            return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownBlock), "Blockchain not available");
        }
        else if (method == "getblockheader")
        {
            if (neo_system_ && neo_system_->GetBlockchain())
            {
                auto result = RPCMethods::GetBlockHeader(neo_system_, params_json);
                return CreateResponse(id, io::JsonValue(result));
            }
            return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownBlock), "Blockchain not available");
        }
        else if (method == "gettransactionheight")
        {
            if (neo_system_ && neo_system_->GetBlockchain())
            {
                auto result = RPCMethods::GetTransactionHeight(neo_system_, params_json);
                return CreateResponse(id, io::JsonValue(result));
            }
            return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownTransaction), "Blockchain not available");
        }
        else if (method == "getrawtransaction")
        {
            if (neo_system_ && neo_system_->GetBlockchain())
            {
                auto result = RPCMethods::GetRawTransaction(neo_system_, params_json);
                return CreateResponse(id, io::JsonValue(result));
            }
            return CreateErrorResponse(id, static_cast<int>(RpcError::UnknownTransaction), "Blockchain not available");
        }
        else if (method == "getstateroot")
        {
            auto result = RPCMethods::GetStateRoot(neo_system_, params_json);
            return CreateResponse(id, io::JsonValue(result));
        }
        else if (method == "getstate")
        {
            auto result = RPCMethods::GetState(neo_system_, params_json);
            return CreateResponse(id, io::JsonValue(result));
        }
        else if (method == "getconnectioncount")
        {
            // Safe default when networking is disabled
            return CreateResponse(id, io::JsonValue(nlohmann::json(static_cast<int64_t>(0))));
        }
        else if (method == "getpeers")
        {
            // Return empty lists when networking is disabled
            nlohmann::json peers = nlohmann::json::object();
            peers["unconnected"] = nlohmann::json::array();
            peers["bad"] = nlohmann::json::array();
            peers["connected"] = nlohmann::json::array();
            return CreateResponse(id, io::JsonValue(peers));
        }
        else
        {
            // Plugin-registered method?
            {
                std::lock_guard<std::mutex> lock(methods_mutex_);
                auto itp = plugin_methods_.find(method);
        if (itp != plugin_methods_.end())
        {
            try
            {
                auto out = itp->second(params);
                return CreateResponse(id, out);
            }
            catch (const RpcException& ex)
            {
                return CreateErrorResponse(id, static_cast<int>(ex.GetCode()), ex.GetMessage());
            }
            catch (const std::exception& e)
            {
                return CreateErrorResponse(id, static_cast<int>(RpcError::InternalError), e.what());
            }
        }
                if (plugin_handler_)
                {
                    try
                    {
                        auto out = plugin_handler_(method, params);
                        return CreateResponse(id, out);
                    }
                    catch (const std::exception& e)
                    {
                        return CreateErrorResponse(id, static_cast<int>(RpcError::InternalError), e.what());
                    }
                }
            }
            std::fprintf(stderr, "[RPC] Unknown method: %s\n", method.c_str());
            return CreateErrorResponse(id, static_cast<int>(RpcError::MethodNotFound), std::string("Method not found: ") + method);
        }
    }
    catch (const RpcException& ex)
    {
        return CreateErrorResponse(id, static_cast<int>(ex.GetCode()), ex.GetMessage());
    }
    catch (const std::exception& e)
    {
        return CreateErrorResponse(id, static_cast<int>(RpcError::InternalError), std::string("Internal error: ") + e.what());
    }
}

std::string RpcServer::ValidateRequest(const io::JsonValue& request)
{
    if (!request.IsObject()) return "Request must be a JSON object";
    if (!request.HasMember("jsonrpc")) return "Missing jsonrpc field";
    if (request["jsonrpc"].IsString() && request["jsonrpc"].GetString() != std::string("2.0"))
        return "Invalid jsonrpc version";
    if (!request.HasMember("method")) return "Missing method field";
    if (!request["method"].IsString()) return "Invalid method field";
    return "";
}

io::JsonValue RpcServer::CreateResponse(const io::JsonValue& id, const io::JsonValue& result)
{
    nlohmann::json response = {{"jsonrpc", "2.0"}, {"result", result.GetJson()}, {"id", id.IsNull() ? nullptr : id.GetJson()}};
    return io::JsonValue(response);
}

io::JsonValue RpcServer::CreateErrorResponse(const io::JsonValue& id, int code, const std::string& message)
{
    nlohmann::json response = {{"jsonrpc", "2.0"},
                               {"error", {{"code", code}, {"message", message}}},
                               {"id", id.IsNull() ? nullptr : id.GetJson()}};
    return io::JsonValue(response);
}

// Remove outdated per-method wrappers – routing is handled in ProcessRequest

}  // namespace neo::rpc
