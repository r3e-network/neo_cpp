#pragma once

#include <functional>
#include <memory>
#include <neo/plugins/plugin_base.h>
#include <neo/rpc/rpc_server.h>
#include <string>
#include <unordered_map>

namespace neo::plugins
{
/**
 * @brief Represents an RPC server plugin.
 */
class RpcServerPlugin : public PluginBase
{
  public:
    /**
     * @brief Constructs an RpcServerPlugin.
     */
    RpcServerPlugin();

    /**
     * @brief Registers an RPC method.
     * @param method The method name.
     * @param handler The method handler.
     */
    void RegisterMethod(const std::string& method, std::function<nlohmann::json(const nlohmann::json&)> handler);

  protected:
    /**
     * @brief Initializes the plugin.
     * @param settings The settings.
     * @return True if the plugin was initialized, false otherwise.
     */
    bool OnInitialize(const std::unordered_map<std::string, std::string>& settings) override;

    /**
     * @brief Starts the plugin.
     * @return True if the plugin was started, false otherwise.
     */
    bool OnStart() override;

    /**
     * @brief Stops the plugin.
     * @return True if the plugin was stopped, false otherwise.
     */
    bool OnStop() override;

  private:
    std::shared_ptr<rpc::RpcServer> rpcServer_;
    uint16_t port_;
    bool enableCors_;
    bool enableAuth_;
    std::string username_;
    std::string password_;
};

/**
 * @brief Represents an RPC server plugin factory.
 */
class RpcServerPluginFactory : public PluginFactoryBase<RpcServerPlugin>
{
};
}  // namespace neo::plugins
