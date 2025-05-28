#include <neo/rpc/rpc_server.h>
#include <neo/rpc/rpc_methods.h>
#include <neo/io/json.h>
#include <neo/cryptography/base64.h>
#include <httplib.h>
#include <sstream>
#include <iostream>

namespace neo::rpc
{
    RPCServer::RPCServer(std::shared_ptr<node::NeoSystem> neoSystem, uint16_t port, bool enableCors, bool enableAuth, const std::string& username, const std::string& password)
        : neoSystem_(neoSystem), port_(port), enableCors_(enableCors), enableAuth_(enableAuth), username_(username), password_(password), running_(false)
    {
        InitializeMethods();
    }

    RPCServer::~RPCServer()
    {
        Stop();
    }

    void RPCServer::Start()
    {
        if (running_)
            return;

        running_ = true;
        serverThread_ = std::thread(&RPCServer::RunServer, this);
    }

    void RPCServer::Stop()
    {
        if (!running_)
            return;

        running_ = false;
        condition_.notify_all();

        if (serverThread_.joinable())
            serverThread_.join();
    }

    bool RPCServer::IsRunning() const
    {
        return running_;
    }

    uint16_t RPCServer::GetPort() const
    {
        return port_;
    }

    std::shared_ptr<node::NeoSystem> RPCServer::GetNeoSystem() const
    {
        return neoSystem_;
    }

    void RPCServer::RegisterMethod(const std::string& method, std::function<nlohmann::json(const nlohmann::json&)> handler)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        methods_[method] = handler;
    }

    void RPCServer::UnregisterMethod(const std::string& method)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        methods_.erase(method);
    }

    void RPCServer::RunServer()
    {
        httplib::Server server;

        // Set up CORS
        if (enableCors_)
        {
            server.set_default_headers({
                {"Access-Control-Allow-Origin", "*"},
                {"Access-Control-Allow-Methods", "POST, GET, OPTIONS"},
                {"Access-Control-Allow-Headers", "Content-Type, Authorization"}
            });

            server.Options(".*", [](const httplib::Request& req, httplib::Response& res) {
                res.status = 200;
            });
        }

        // Set up authentication
        if (enableAuth_)
        {
            server.set_auth_handler([this](const httplib::Request& req, httplib::Response& res) {
                auto authHeader = req.get_header_value("Authorization");
                if (authHeader.empty())
                    return false;

                if (authHeader.substr(0, 6) != "Basic ")
                    return false;

                auto credentials = cryptography::Base64::Decode(authHeader.substr(6));
                std::string credentialsStr(reinterpret_cast<const char*>(credentials.Data()), credentials.Size());

                auto pos = credentialsStr.find(':');
                if (pos == std::string::npos)
                    return false;

                auto username = credentialsStr.substr(0, pos);
                auto password = credentialsStr.substr(pos + 1);

                return username == username_ && password == password_;
            });
        }

        // Set up RPC endpoint
        server.Post("/", [this](const httplib::Request& req, httplib::Response& res) {
            res.set_content(HandleRequest(req.body), "application/json");
        });

        // Start server
        server.listen("0.0.0.0", port_);
    }

    std::string RPCServer::HandleRequest(const std::string& request)
    {
        try
        {
            // Parse request
            auto json = nlohmann::json::parse(request);

            // Handle request
            auto response = HandleRPCRequest(json);

            // Return response
            return response.dump();
        }
        catch (const std::exception& ex)
        {
            // Create error response
            auto response = CreateErrorResponse(nullptr, -32700, "Parse error");

            // Return response
            return response.dump();
        }
    }

    nlohmann::json RPCServer::HandleRPCRequest(const nlohmann::json& request)
    {
        // Check if request is an array
        if (request.is_array())
        {
            // Handle batch request
            nlohmann::json response = nlohmann::json::array();

            for (const auto& item : request)
            {
                response.push_back(HandleRPCRequest(item));
            }

            return response;
        }

        // Check if request is an object
        if (!request.is_object())
            return CreateErrorResponse(nullptr, -32600, "Invalid Request");

        // Check if request has required fields
        if (!request.contains("jsonrpc") || !request.contains("method") || !request.contains("id"))
            return CreateErrorResponse(request.value("id", nullptr), -32600, "Invalid Request");

        // Check if jsonrpc version is 2.0
        if (request["jsonrpc"] != "2.0")
            return CreateErrorResponse(request["id"], -32600, "Invalid Request");

        // Get method
        auto method = request["method"].get<std::string>();

        // Get params
        auto params = request.value("params", nlohmann::json::array());

        // Find method handler
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = methods_.find(method);
        if (it == methods_.end())
            return CreateErrorResponse(request["id"], -32601, "Method not found");

        try
        {
            // Call method handler
            auto result = it->second(params);

            // Create success response
            return CreateSuccessResponse(request["id"], result);
        }
        catch (const std::exception& ex)
        {
            // Create error response
            return CreateErrorResponse(request["id"], -32603, ex.what());
        }
    }

    nlohmann::json RPCServer::CreateErrorResponse(const nlohmann::json& id, int32_t code, const std::string& message)
    {
        nlohmann::json response;
        response["jsonrpc"] = "2.0";
        response["id"] = id;
        response["error"]["code"] = code;
        response["error"]["message"] = message;
        return response;
    }

    nlohmann::json RPCServer::CreateSuccessResponse(const nlohmann::json& id, const nlohmann::json& result)
    {
        nlohmann::json response;
        response["jsonrpc"] = "2.0";
        response["id"] = id;
        response["result"] = result;
        return response;
    }

    void RPCServer::InitializeMethods()
    {
        // Register methods
        RegisterMethod("getversion", [this](const nlohmann::json& params) { return RPCMethods::GetVersion(neoSystem_, params); });
        RegisterMethod("getblockcount", [this](const nlohmann::json& params) { return RPCMethods::GetBlockCount(neoSystem_, params); });
        RegisterMethod("getblock", [this](const nlohmann::json& params) { return RPCMethods::GetBlock(neoSystem_, params); });
        RegisterMethod("getblockhash", [this](const nlohmann::json& params) { return RPCMethods::GetBlockHash(neoSystem_, params); });
        RegisterMethod("getblockheader", [this](const nlohmann::json& params) { return RPCMethods::GetBlockHeader(neoSystem_, params); });
        RegisterMethod("getrawmempool", [this](const nlohmann::json& params) { return RPCMethods::GetRawMemPool(neoSystem_, params); });
        RegisterMethod("getrawtransaction", [this](const nlohmann::json& params) { return RPCMethods::GetRawTransaction(neoSystem_, params); });
        RegisterMethod("gettransactionheight", [this](const nlohmann::json& params) { return RPCMethods::GetTransactionHeight(neoSystem_, params); });
        RegisterMethod("sendrawtransaction", [this](const nlohmann::json& params) { return RPCMethods::SendRawTransaction(neoSystem_, params); });
        RegisterMethod("invokefunction", [this](const nlohmann::json& params) { return RPCMethods::InvokeFunction(neoSystem_, params); });
        RegisterMethod("invokescript", [this](const nlohmann::json& params) { return RPCMethods::InvokeScript(neoSystem_, params); });
        RegisterMethod("getcontractstate", [this](const nlohmann::json& params) { return RPCMethods::GetContractState(neoSystem_, params); });
        RegisterMethod("getunclaimedgas", [this](const nlohmann::json& params) { return RPCMethods::GetUnclaimedGas(neoSystem_, params); });
        RegisterMethod("getconnectioncount", [this](const nlohmann::json& params) { return RPCMethods::GetConnectionCount(neoSystem_, params); });
        RegisterMethod("getpeers", [this](const nlohmann::json& params) { return RPCMethods::GetPeers(neoSystem_, params); });
        RegisterMethod("getcommittee", [this](const nlohmann::json& params) { return RPCMethods::GetCommittee(neoSystem_, params); });
        RegisterMethod("getvalidators", [this](const nlohmann::json& params) { return RPCMethods::GetValidators(neoSystem_, params); });
        RegisterMethod("getnextblockvalidators", [this](const nlohmann::json& params) { return RPCMethods::GetNextBlockValidators(neoSystem_, params); });
    }
}
