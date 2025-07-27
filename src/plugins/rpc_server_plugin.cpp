#include <iostream>
#include <neo/plugins/rpc_server_plugin.h>
#include <neo/rpc/rpc_server.h>

namespace neo::plugins
{
RpcServerPlugin::RpcServerPlugin()
    : PluginBase("RpcServer", "Provides RPC server functionality", "1.0", "Neo C++ Team"), port_(10332),
      enableCors_(false), enableAuth_(false)
{
}

bool RpcServerPlugin::OnInitialize(const std::unordered_map<std::string, std::string>& settings)
{
    // Parse settings
    for (const auto& [key, value] : settings)
    {
        if (key == "Port")
        {
            port_ = std::stoi(value);
        }
        else if (key == "EnableCors")
        {
            enableCors_ = (value == "true" || value == "1");
        }
        else if (key == "EnableAuth")
        {
            enableAuth_ = (value == "true" || value == "1");
        }
        else if (key == "Username")
        {
            username_ = value;
        }
        else if (key == "Password")
        {
            password_ = value;
        }
    }

    // Create RPC server
    rpcServer_ =
        std::make_shared<rpc::RPCServer>(GetNeoSystem(), port_, enableCors_, enableAuth_, username_, password_);

    // Register methods
    RegisterMethod("ping", [](const nlohmann::json& params) { return "pong"; });

    RegisterMethod("echo",
                   [](const nlohmann::json& params)
                   {
                       if (params.empty())
                           return nlohmann::json("echo");

                       return params[0];
                   });

    RegisterMethod("time",
                   [](const nlohmann::json& params)
                   {
                       auto now = std::chrono::system_clock::now();
                       auto timestamp =
                           std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                       return nlohmann::json(timestamp);
                   });

    return true;
}

bool RpcServerPlugin::OnStart()
{
    if (!rpcServer_)
        return false;

    rpcServer_->Start();
    return true;
}

bool RpcServerPlugin::OnStop()
{
    if (!rpcServer_)
        return false;

    rpcServer_->Stop();
    return true;
}

void RpcServerPlugin::RegisterMethod(const std::string& method,
                                     std::function<nlohmann::json(const nlohmann::json&)> handler)
{
    if (!rpcServer_)
        return;

    rpcServer_->RegisterMethod(method, handler);
}
}  // namespace neo::plugins
