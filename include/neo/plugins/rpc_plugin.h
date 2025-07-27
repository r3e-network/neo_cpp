#pragma once

#include <functional>
#include <memory>
#include <neo/node/rpc_server.h>
#include <neo/plugins/plugin_base.h>
#include <string>
#include <unordered_map>

namespace neo::plugins
{
/**
 * @brief Represents an RPC plugin.
 */
class RPCPlugin : public PluginBase
{
  public:
    /**
     * @brief Constructs an RPCPlugin.
     */
    RPCPlugin();

    /**
     * @brief Destructor.
     */
    virtual ~RPCPlugin() = default;

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

    /**
     * @brief Registers an RPC method.
     * @param name The name.
     * @param method The method.
     */
    void RegisterMethod(const std::string& name, std::function<nlohmann::json(const std::vector<std::string>&)> method);

  private:
    std::unordered_map<std::string, std::function<nlohmann::json(const std::vector<std::string>&)>> methods_;
    std::vector<int32_t> callbackIds_;

    /**
     * @brief Handles an RPC request.
     * @param request The request.
     * @return The response.
     */
    node::RPCResponse OnRequest(const node::RPCRequest& request);
};

/**
 * @brief Represents an RPC plugin factory.
 */
class RPCPluginFactory : public PluginFactoryBase<RPCPlugin>
{
};
}  // namespace neo::plugins
