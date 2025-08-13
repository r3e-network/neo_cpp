/**
 * @file rpc_plugin.cpp
 * @brief Rpc Plugin
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/plugins/rpc_plugin.h>

#include <iostream>

namespace neo::plugins
{
RPCPlugin::RPCPlugin() : PluginBase("RPC", "Adds custom RPC methods", "1.0", "Neo C++ Team") {}

bool RPCPlugin::OnInitialize(const std::unordered_map<std::string, std::string>& settings)
{
    // Register methods
    RegisterMethod("ping", [](const std::vector<std::string>& params) { return "pong"; });

    RegisterMethod("echo",
                   [](const std::vector<std::string>& params)
                   {
                       if (params.empty()) return nlohmann::json("echo");

                       return nlohmann::json(params[0]);
                   });

    RegisterMethod("time",
                   [](const std::vector<std::string>& params)
                   {
                       auto now = std::chrono::system_clock::now();
                       auto timestamp =
                           std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                       return nlohmann::json(timestamp);
                   });

    return true;
}

bool RPCPlugin::OnStart()
{
    // Register RPC request callback matching typical plugin architecture
    try
    {
        if (rpcServer_)
        {
            // Register this plugin as a request handler
            rpcServer_->RegisterRequestHandler(
                [this](const std::string& method, const json::JsonValue& params) -> json::JsonValue
                { return HandleRpcRequest(method, params); });

            // Register specific methods this plugin handles
            RegisterRpcMethods();

            std::cout << "RPC plugin callbacks registered successfully" << std::endl;
        }
        else
        {
            std::cerr << "RPC server not available for callback registration" << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to register RPC callbacks: " << e.what() << std::endl;
    }

    return true;
}

bool RPCPlugin::OnStop()
{
    // Unregister RPC request callback for proper cleanup
    try
    {
        if (rpcServer_)
        {
            // Unregister this plugin's request handler
            rpcServer_->UnregisterRequestHandler();

            // Unregister specific methods this plugin handled
            UnregisterRpcMethods();

            std::cout << "RPC plugin callbacks unregistered successfully" << std::endl;
        }
        else
        {
            std::cerr << "RPC server not available for callback unregistration" << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to unregister RPC callbacks: " << e.what() << std::endl;
    }

    return true;
}

void RPCPlugin::RegisterMethod(const std::string& name,
                               std::function<nlohmann::json(const std::vector<std::string>&)> method)
{
    methods_[name] = method;
}

node::RPCResponse RPCPlugin::OnRequest(const node::RPCRequest& request)
{
    node::RPCResponse response;
    response.SetId(request.GetId());

    try
    {
        auto method = methods_.find(request.GetMethod());
        if (method == methods_.end())
        {
            nlohmann::json error;
            error["code"] = -32601;
            error["message"] = "Method not found";
            response.SetError(error);
            return response;
        }

        nlohmann::json result = method->second(request.GetParams());
        response.SetResult(result);
    }
    catch (const std::exception& ex)
    {
        nlohmann::json error;
        error["code"] = -32000;
        error["message"] = ex.what();
        response.SetError(error);
    }

    return response;
}
}  // namespace neo::plugins
