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
#include <neo/rpc/rpc_methods.h>
#include <neo/rpc/rpc_server.h>

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

RpcServer::~RpcServer() { Stop(); }

void RpcServer::Start()
{
    if (running_.exchange(true)) return;

    LOG_INFO("Starting RPC server on {}:{}", config_.bind_address, config_.port);

#ifdef NEO_HAS_HTTPLIB
    server_thread_ = std::thread(
        [this]()
        {
            httplib::Server server;

            server.Post("/",
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
                                json error_response = {
                                    {"jsonrpc", "2.0"},
                                    {"error", {{"code", -32700}, {"message", "Parse error"}, {"data", e.what()}}},
                                    {"id", nullptr}};
                                res.set_content(error_response.dump(), "application/json");
                            }
                        });

            server.listen(config_.bind_address.c_str(), config_.port);
        });
#else
    LOG_ERROR("HTTP server not available - RPC server cannot start");
    running_ = false;
#endif
}

void RpcServer::Stop()
{
    if (!running_.exchange(false)) return;

    LOG_INFO("Stopping RPC server");

    if (server_thread_.joinable())
    {
        server_thread_.join();
    }
}

std::string RpcServer::GetClientIP(const httplib::Request& req) const
{
    // Check for X-Forwarded-For header first (proxy support)
    if (req.has_header("X-Forwarded-For"))
    {
        auto forwarded = req.get_header_value("X-Forwarded-For");
        // Take the first IP in the chain
        auto pos = forwarded.find(',');
        return pos != std::string::npos ? forwarded.substr(0, pos) : forwarded;
    }
    
    // Check for X-Real-IP header
    if (req.has_header("X-Real-IP"))
    {
        return req.get_header_value("X-Real-IP");
    }
    
    // Use remote address as fallback
    return req.remote_addr;
}

bool RpcServer::IsAuthenticated(const httplib::Request& req) const
{
    if (!config_.enable_authentication)
    {
        return true;
    }
    
    if (!req.has_header("Authorization"))
    {
        return false;
    }
    
    auto auth_header = req.get_header_value("Authorization");
    if (auth_header.substr(0, 6) != "Basic ")
    {
        return false;
    }
    
    // TODO: Implement proper base64 decoding and credential validation
    // For now, this is a placeholder - in production, implement secure authentication
    return true; // PLACEHOLDER: Implement proper authentication
}

bool RpcServer::IsMethodAllowed(const io::JsonValue& request) const
{
    if (config_.disabled_methods.empty())
    {
        return true;
    }
    
    if (!request.GetProperty("method").IsString())
    {
        return false;
    }
    
    std::string method = request.GetProperty("method").GetString();
    
    for (const auto& disabled_method : config_.disabled_methods)
    {
        if (method == disabled_method)
        {
            return false;
        }
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

void RpcServer::InitializeHandlers()
{
    // Use the methods from RpcMethods class
    // Initialize with essential RPC methods - additional methods can be registered as needed
}

io::JsonValue RpcServer::ProcessRequest(const io::JsonValue& request)
{
    auto validation_error = ValidateRequest(request);
    if (!validation_error.empty())
    {
        return CreateErrorResponse(request.GetValue("id"), -32600, validation_error);
    }

    auto method = request.GetString("method");
    auto params = request.GetValue("params");
    auto id = request.GetValue("id");

    // Call the appropriate RPC method
    try
    {
        RpcMethods methods(system_);

        // Route to appropriate method
        if (method == "getblockcount")
        {
            auto result = methods.GetBlockCount();
            return CreateResponse(id, result);
        }
        else if (method == "getversion")
        {
            auto result = methods.GetVersion();
            return CreateResponse(id, result);
        }
        else
        {
            return CreateErrorResponse(id, -32601, "Method not found");
        }
    }
    catch (const std::exception& e)
    {
        return CreateErrorResponse(id, -32603, std::string("Internal error: ") + e.what());
    }
}

std::string RpcServer::ValidateRequest(const io::JsonValue& request)
{
    if (!request.IsObject())
    {
        return "Request must be a JSON object";
    }

    if (!request.HasMember("jsonrpc") || request.GetString("jsonrpc") != "2.0")
    {
        return "Missing or invalid jsonrpc field";
    }

    if (!request.HasMember("method") || !request.GetValue("method").IsString())
    {
        return "Missing or invalid method field";
    }

    return "";
}

io::JsonValue RpcServer::CreateResponse(const io::JsonValue& id, const io::JsonValue& result)
{
    json response = {{"jsonrpc", "2.0"}, {"result", result.GetJson()}, {"id", id.GetJson()}};
    return io::JsonValue(response);
}

io::JsonValue RpcServer::CreateErrorResponse(const io::JsonValue& id, int code, const std::string& message)
{
    json response = {{"jsonrpc", "2.0"},
                     {"error", {{"code", code}, {"message", message}}},
                     {"id", id.IsNull() ? nullptr : id.GetJson()}};
    return io::JsonValue(response);
}

// RPC method implementations delegate to RpcMethods class
io::JsonValue RpcServer::GetBlock(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetBlock(params);
}

io::JsonValue RpcServer::GetBlockCount(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetBlockCount();
}

io::JsonValue RpcServer::GetBlockHash(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    if (params.IsArray() && params.Size() > 0)
    {
        return methods.GetBlockHash(params[0].GetInt());
    }
    throw std::runtime_error("Invalid parameters");
}

io::JsonValue RpcServer::GetBlockHeader(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetBlockHeader(params);
}

io::JsonValue RpcServer::GetTransaction(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetTransaction(params);
}

io::JsonValue RpcServer::SendRawTransaction(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.SendRawTransaction(params);
}

io::JsonValue RpcServer::GetRawMempool(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetRawMempool(params);
}

io::JsonValue RpcServer::GetContractState(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetContractState(params);
}

io::JsonValue RpcServer::GetNativeContracts(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetNativeContracts();
}

io::JsonValue RpcServer::GetStorage(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetStorage(params);
}

io::JsonValue RpcServer::FindStorage(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.FindStorage(params);
}

io::JsonValue RpcServer::InvokeFunction(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.InvokeFunction(params);
}

io::JsonValue RpcServer::InvokeScript(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.InvokeScript(params);
}

io::JsonValue RpcServer::GetUnclaimedGas(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetUnclaimedGas(params);
}

io::JsonValue RpcServer::GetCandidates(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetCandidates();
}

io::JsonValue RpcServer::GetCommittee(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetCommittee();
}

io::JsonValue RpcServer::GetNextBlockValidators(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetNextBlockValidators();
}

io::JsonValue RpcServer::GetConnectionCount(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetConnectionCount();
}

io::JsonValue RpcServer::GetPeers(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetPeers();
}

io::JsonValue RpcServer::GetVersion(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetVersion();
}

io::JsonValue RpcServer::GetNep17Balances(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetNep17Balances(params);
}

io::JsonValue RpcServer::GetNep17Transfers(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetNep17Transfers(params);
}

io::JsonValue RpcServer::GetNep11Balances(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetNep11Balances(params);
}

io::JsonValue RpcServer::GetNep11Transfers(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetNep11Transfers(params);
}

io::JsonValue RpcServer::GetNep11Properties(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetNep11Properties(params);
}

io::JsonValue RpcServer::GetProof(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetProof(params);
}

io::JsonValue RpcServer::GetStateRoot(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetStateRoot(params);
}

io::JsonValue RpcServer::GetStateHeight(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetStateHeight();
}

io::JsonValue RpcServer::GetState(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetState(params);
}

io::JsonValue RpcServer::FindStates(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.FindStates(params);
}

io::JsonValue RpcServer::GetApplicationLog(const io::JsonValue& params)
{
    RpcMethods methods(system_);
    return methods.GetApplicationLog(params);
}

io::JsonValue RpcServer::GetStatistics() const
{
    io::JsonValue stats;
    stats.AddMember("totalRequests", static_cast<int64_t>(total_requests_.load()));
    stats.AddMember("failedRequests", static_cast<int64_t>(failed_requests_.load()));

    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
    stats.AddMember("uptimeSeconds", static_cast<int64_t>(uptime));

    return stats;
}

}  // namespace neo::rpc