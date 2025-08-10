#pragma once

#include <neo/node/neo_system.h>
#include <neo/plugins/plugin.h>
#include <neo/rpc/rpc_server.h>

#include <atomic>
#include <memory>
#include <string>

namespace neo::plugins
{
/**
 * @brief Represents a base plugin.
 */
class PluginBase : public Plugin
{
   public:
    /**
     * @brief Constructs a PluginBase.
     * @param name The name.
     * @param description The description.
     * @param version The version.
     * @param author The author.
     */
    PluginBase(const std::string& name, const std::string& description, const std::string& version,
               const std::string& author);

    /**
     * @brief Destructor.
     */
    virtual ~PluginBase() = default;

    /**
     * @brief Gets the name.
     * @return The name.
     */
    std::string GetName() const override;

    /**
     * @brief Gets the description.
     * @return The description.
     */
    std::string GetDescription() const override;

    /**
     * @brief Gets the version.
     * @return The version.
     */
    std::string GetVersion() const override;

    /**
     * @brief Gets the author.
     * @return The author.
     */
    std::string GetAuthor() const override;

    /**
     * @brief Initializes the plugin.
     * @param neoSystem The Neo system.
     * @param settings The settings.
     * @return True if the plugin was initialized, false otherwise.
     */
    bool Initialize(std::shared_ptr<node::NeoSystem> neoSystem,
                    const std::unordered_map<std::string, std::string>& settings) override;

    /**
     * @brief Starts the plugin.
     * @return True if the plugin was started, false otherwise.
     */
    bool Start() override;

    /**
     * @brief Stops the plugin.
     * @return True if the plugin was stopped, false otherwise.
     */
    bool Stop() override;

    /**
     * @brief Checks if the plugin is running.
     * @return True if the plugin is running, false otherwise.
     */
    bool IsRunning() const override;

   protected:
    /**
     * @brief Initializes the plugin.
     * @param settings The settings.
     * @return True if the plugin was initialized, false otherwise.
     */
    virtual bool OnInitialize(const std::unordered_map<std::string, std::string>& settings);

    /**
     * @brief Starts the plugin.
     * @return True if the plugin was started, false otherwise.
     */
    virtual bool OnStart();

    /**
     * @brief Stops the plugin.
     * @return True if the plugin was stopped, false otherwise.
     */
    virtual bool OnStop();

    /**
     * @brief Gets the Neo system.
     * @return The Neo system.
     */
    std::shared_ptr<node::NeoSystem> GetNeoSystem() const;

    /**
     * @brief Gets the RPC server.
     * @return The RPC server.
     */
    std::shared_ptr<rpc::RpcServer> GetRPCServer() const;

   private:
    std::string name_;
    std::string description_;
    std::string version_;
    std::string author_;
    std::shared_ptr<node::NeoSystem> neoSystem_;
    std::shared_ptr<rpc::RpcServer> rpcServer_;
    std::atomic<bool> running_;
};

/**
 * @brief Represents a base plugin factory.
 * @tparam T The plugin type.
 */
template <typename T>
class PluginFactoryBase : public PluginFactory
{
   public:
    /**
     * @brief Creates a plugin.
     * @return The plugin.
     */
    std::shared_ptr<Plugin> CreatePlugin() override { return std::shared_ptr<Plugin>(new T()); }
};
}  // namespace neo::plugins
