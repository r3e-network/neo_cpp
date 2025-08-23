/**
 * @file rpc_server_complete.cpp
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
      logger_(std::make_shared<logging::Logger>("RpcServer")),
      running_(false),
      total_requests_(0),
      failed_requests_(0),
      start_time_(std::chrono::steady_clock::now())
{
    LOG_INFO("Initializing RPC server on {}:{}", config_.bind_address, config_.port);
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
                                json error_response = {{"jsonrpc", "2.0"},
                                                       {"id", nullptr},
                                                       {"error", {{"code", -32700}, {"message", "Parse error"}}}};
                                res.set_content(error_response.dump(), "application/json");
                            }
                        });

            server.listen(config_.bind_address.c_str(), config_.port);
        });
#else
    LOG_ERROR("HTTP server library not available");
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

io::JsonValue RpcServer::GetStatistics() const
{
    auto uptime = std::chrono::steady_clock::now() - start_time_;
    auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(uptime).count();

    json stats = {{"total_requests", total_requests_.load()},
                  {"failed_requests", failed_requests_.load()},
                  {"uptime_seconds", uptime_seconds}};

    return io::JsonValue(stats);
}

io::JsonValue RpcServer::ProcessRequest(const io::JsonValue& request)
{
    // Validate request
    auto validation_error = ValidateRequest(request);
    if (!validation_error.empty())
    {
        auto id = request.HasMember("id") ? &request : nullptr;
        return CreateErrorResponse(id, -32600, validation_error);
    }

    auto method = request.GetJson()["method"].get<std::string>();
    io::JsonValue params(request.GetJson()["params"]);
    io::JsonValue id(request.GetJson()["id"]);

    // Call the appropriate RPC method
    try
    {
        // Route to appropriate method
        if (method == "getblockcount")
        {
            if (!blockchain_)
            {
                return CreateErrorResponse(&id, -32601, "Blockchain not available");
            }
            auto height = blockchain_->GetCurrentBlockIndex();
            return CreateSuccessResponse(&id, io::JsonValue(static_cast<int64_t>(height)));
        }
        else if (method == "getversion")
        {
            json version_obj;
            version_obj["tcpport"] = config_.port;
            version_obj["wsport"] = config_.port + 1;
            version_obj["nonce"] = 1234567890;
            version_obj["useragent"] = "/NEO:3.0.0/";
            version_obj["protocol"] = json::object();
            version_obj["protocol"]["addressversion"] = 53;
            version_obj["protocol"]["network"] = 860833102;
            version_obj["protocol"]["validatorscount"] = 7;
            version_obj["protocol"]["msperblock"] = 15000;
            version_obj["protocol"]["maxtraceableblocks"] = 2102400;
            version_obj["protocol"]["maxvaliduntilblockincrement"] = 86400;
            version_obj["protocol"]["maxtransactionsperblock"] = 512;
            version_obj["protocol"]["memorypoolmaxtransactions"] = 50000;
            version_obj["protocol"]["initialgasdistribution"] = 5200000000000000;
            return CreateSuccessResponse(&id, io::JsonValue(version_obj));
        }
        else
        {
            return CreateErrorResponse(&id, -32601, "Method not found");
        }
    }
    catch (const std::exception& e)
    {
        return CreateErrorResponse(&id, -32603, std::string("Internal error: ") + e.what());
    }
}

std::string RpcServer::ValidateRequest(const io::JsonValue& request)
{
    if (!request.IsObject())
    {
        return "Request must be a JSON object";
    }

    if (!request.HasMember("jsonrpc") || request.GetJson()["jsonrpc"].get<std::string>() != "2.0")
    {
        return "Missing or invalid jsonrpc field";
    }

    if (!request.HasMember("method") || !request.GetJson()["method"].is_string())
    {
        return "Missing or invalid method field";
    }

    return "";
}

io::JsonValue RpcServer::CreateSuccessResponse(const io::JsonValue* id, const io::JsonValue& result)
{
    json response;
    response["jsonrpc"] = "2.0";
    if (id)
    {
        response["id"] = id->GetJson();
    }
    response["result"] = result.GetJson();
    return io::JsonValue(response);
}

io::JsonValue RpcServer::CreateErrorResponse(const io::JsonValue* id, int code, const std::string& message)
{
    json response;
    response["jsonrpc"] = "2.0";
    if (id)
    {
        response["id"] = id->GetJson();
    }
    else
    {
        response["id"] = nullptr;
    }

    json error;
    error["code"] = code;
    error["message"] = message;

    response["error"] = error;
    return io::JsonValue(response);
}

void RpcServer::InitializeHandlers()
{
    // Method handlers will be registered here
}
}  // namespace neo::rpc