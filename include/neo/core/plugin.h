#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace neo
{
namespace core
{
/**
 * @brief Base class for Neo plugins
 *
 * Plugins extend the functionality of the Neo node by providing
 * additional services, monitoring, or protocol extensions.
 */
class Plugin
{
   public:
    /**
     * @brief Plugin configuration
     */
    struct Config
    {
        std::string name;
        std::string version;
        std::string description;
        std::string author;
        bool enabled = true;
    };

    /**
     * @brief Construct a new Plugin
     * @param config Plugin configuration
     */
    explicit Plugin(const Config& config) : config_(config) {}

    /**
     * @brief Virtual destructor
     */
    virtual ~Plugin() = default;

    /**
     * @brief Get plugin name
     * @return Plugin name
     */
    const std::string& GetName() const { return config_.name; }

    /**
     * @brief Get plugin version
     * @return Plugin version
     */
    const std::string& GetVersion() const { return config_.version; }

    /**
     * @brief Get plugin description
     * @return Plugin description
     */
    const std::string& GetDescription() const { return config_.description; }

    /**
     * @brief Get plugin author
     * @return Plugin author
     */
    const std::string& GetAuthor() const { return config_.author; }

    /**
     * @brief Check if plugin is enabled
     * @return true if enabled, false otherwise
     */
    bool IsEnabled() const { return config_.enabled; }

    /**
     * @brief Initialize the plugin
     * @return true if initialization successful, false otherwise
     */
    virtual bool Initialize() = 0;

    /**
     * @brief Start the plugin
     * @return true if started successfully, false otherwise
     */
    virtual bool Start() = 0;

    /**
     * @brief Stop the plugin
     */
    virtual void Stop() = 0;

    /**
     * @brief Handle configuration changes
     * @param newConfig New configuration
     */
    virtual void OnConfigChanged(const Config& newConfig) { config_ = newConfig; }

   protected:
    Config config_;
};

/**
 * @brief Plugin manager handles loading and lifecycle of plugins
 */
class PluginManager
{
   public:
    /**
     * @brief Get singleton instance
     * @return Plugin manager instance
     */
    static PluginManager& GetInstance()
    {
        static PluginManager instance;
        return instance;
    }

    /**
     * @brief Register a plugin
     * @param plugin Plugin to register
     * @return true if registered successfully, false otherwise
     */
    bool RegisterPlugin(std::shared_ptr<Plugin> plugin);

    /**
     * @brief Unregister a plugin
     * @param name Plugin name
     * @return true if unregistered successfully, false otherwise
     */
    bool UnregisterPlugin(const std::string& name);

    /**
     * @brief Get a plugin by name
     * @param name Plugin name
     * @return Plugin pointer or nullptr if not found
     */
    std::shared_ptr<Plugin> GetPlugin(const std::string& name) const;

    /**
     * @brief Get all registered plugins
     * @return Vector of all plugins
     */
    std::vector<std::shared_ptr<Plugin>> GetAllPlugins() const;

    /**
     * @brief Initialize all plugins
     * @return true if all plugins initialized successfully, false otherwise
     */
    bool InitializeAll();

    /**
     * @brief Start all enabled plugins
     * @return true if all enabled plugins started successfully, false otherwise
     */
    bool StartAll();

    /**
     * @brief Stop all running plugins
     */
    void StopAll();

    /**
     * @brief Load plugins from directory
     * @param directory Plugin directory path
     * @return Number of plugins loaded
     */
    size_t LoadPluginsFromDirectory(const std::string& directory);

   private:
    PluginManager() = default;
    ~PluginManager() = default;
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    std::vector<std::shared_ptr<Plugin>> plugins_;
    mutable std::mutex mutex_;
};

/**
 * @brief Macro to export plugin factory function
 *
 * Each plugin shared library should use this macro to export
 * a factory function that creates the plugin instance.
 */
#define NEO_EXPORT_PLUGIN(PluginClass)                                                                \
    extern "C"                                                                                        \
    {                                                                                                 \
        std::shared_ptr<neo::core::Plugin> CreatePlugin() { return std::make_shared<PluginClass>(); } \
    }

}  // namespace core
}  // namespace neo