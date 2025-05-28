#include <neo/plugins/rpc_plugin.h>
#include <iostream>

namespace neo::plugins
{
    RPCPlugin::RPCPlugin()
        : PluginBase("RPC", "Adds custom RPC methods", "1.0", "Neo C++ Team")
    {
    }

    bool RPCPlugin::OnInitialize(const std::unordered_map<std::string, std::string>& settings)
    {
        // Register methods
        RegisterMethod("ping", [](const std::vector<std::string>& params) {
            return "pong";
        });
        
        RegisterMethod("echo", [](const std::vector<std::string>& params) {
            if (params.empty())
                return nlohmann::json("echo");
            
            return nlohmann::json(params[0]);
        });
        
        RegisterMethod("time", [](const std::vector<std::string>& params) {
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            return nlohmann::json(timestamp);
        });
        
        return true;
    }

    bool RPCPlugin::OnStart()
    {
        // Register RPC request callback
        auto rpcServer = GetRPCServer();
        if (!rpcServer)
            return false;
        
        // TODO: Register RPC request callback
        
        return true;
    }

    bool RPCPlugin::OnStop()
    {
        // Unregister RPC request callback
        auto rpcServer = GetRPCServer();
        if (!rpcServer)
            return false;
        
        // TODO: Unregister RPC request callback
        
        return true;
    }

    void RPCPlugin::RegisterMethod(const std::string& name, std::function<nlohmann::json(const std::vector<std::string>&)> method)
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
}
