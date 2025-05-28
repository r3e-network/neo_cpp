#pragma once

#include <neo/node/neo_system.h>
#include <neo/rpc/rpc_server.h>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>

namespace neo::plugins
{
    /**
     * @brief Represents a plugin.
     */
    class Plugin
    {
    public:
        /**
         * @brief Destructor.
         */
        virtual ~Plugin() = default;

        /**
         * @brief Gets the name.
         * @return The name.
         */
        virtual std::string GetName() const = 0;

        /**
         * @brief Gets the description.
         * @return The description.
         */
        virtual std::string GetDescription() const = 0;

        /**
         * @brief Gets the version.
         * @return The version.
         */
        virtual std::string GetVersion() const = 0;

        /**
         * @brief Gets the author.
         * @return The author.
         */
        virtual std::string GetAuthor() const = 0;

        /**
         * @brief Initializes the plugin.
         * @param neoSystem The Neo system.
         * @param settings The settings.
         * @return True if the plugin was initialized, false otherwise.
         */
        virtual bool Initialize(std::shared_ptr<node::NeoSystem> neoSystem, const std::unordered_map<std::string, std::string>& settings) = 0;

        /**
         * @brief Starts the plugin.
         * @return True if the plugin was started, false otherwise.
         */
        virtual bool Start() = 0;

        /**
         * @brief Stops the plugin.
         * @return True if the plugin was stopped, false otherwise.
         */
        virtual bool Stop() = 0;

        /**
         * @brief Checks if the plugin is running.
         * @return True if the plugin is running, false otherwise.
         */
        virtual bool IsRunning() const = 0;
    };

    /**
     * @brief Represents a plugin factory.
     */
    class PluginFactory
    {
    public:
        /**
         * @brief Destructor.
         */
        virtual ~PluginFactory() = default;

        /**
         * @brief Creates a plugin.
         * @return The plugin.
         */
        virtual std::shared_ptr<Plugin> CreatePlugin() = 0;
    };

    /**
     * @brief Represents a plugin manager.
     */
    class PluginManager
    {
    public:
        /**
         * @brief Gets the instance.
         * @return The instance.
         */
        static PluginManager& GetInstance();

        /**
         * @brief Registers a plugin factory.
         * @param factory The factory.
         */
        void RegisterPluginFactory(std::shared_ptr<PluginFactory> factory);

        /**
         * @brief Gets the plugin factories.
         * @return The plugin factories.
         */
        const std::vector<std::shared_ptr<PluginFactory>>& GetPluginFactories() const;

        /**
         * @brief Gets the plugins.
         * @return The plugins.
         */
        const std::vector<std::shared_ptr<Plugin>>& GetPlugins() const;

        /**
         * @brief Gets a plugin by name.
         * @param name The name.
         * @return The plugin, or nullptr if not found.
         */
        std::shared_ptr<Plugin> GetPlugin(const std::string& name) const;

        /**
         * @brief Loads plugins.
         * @param neoSystem The Neo system.
         * @param settings The settings.
         * @return True if the plugins were loaded, false otherwise.
         */
        bool LoadPlugins(std::shared_ptr<node::NeoSystem> neoSystem, const std::unordered_map<std::string, std::string>& settings);

        /**
         * @brief Starts plugins.
         * @return True if the plugins were started, false otherwise.
         */
        bool StartPlugins();

        /**
         * @brief Stops plugins.
         * @return True if the plugins were stopped, false otherwise.
         */
        bool StopPlugins();

    private:
        PluginManager();
        std::vector<std::shared_ptr<PluginFactory>> factories_;
        std::vector<std::shared_ptr<Plugin>> plugins_;
    };
}
