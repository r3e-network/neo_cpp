#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>

#include "neo/plugins/plugin_base.h"
#include "neo/core/neo_system.h"
#include "neo/json/jtoken.h"

namespace neo::plugins {

/**
 * @brief Plugin lifecycle states
 */
enum class PluginState {
    Unloaded,
    Loading,
    Loaded,
    Starting,
    Started,
    Stopping,
    Stopped,
    Failed,
    Disabled
};

/**
 * @brief Plugin metadata
 */
struct PluginMetadata {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::vector<std::string> dependencies;
    std::unordered_map<std::string, std::string> configuration;
    PluginState state = PluginState::Unloaded;
    std::string error_message;
    
    PluginMetadata(const std::string& n, const std::string& v = "1.0.0")
        : name(n), version(v) {}
};

/**
 * @brief Plugin event types
 */
enum class PluginEventType {
    SystemStarting,
    SystemStarted,
    SystemStopping,
    SystemStopped,
    BlockAdded,
    TransactionAdded,
    ConsensusStarted,
    ConsensusCompleted,
    PeerConnected,
    PeerDisconnected
};

/**
 * @brief Plugin event data
 */
struct PluginEventData {
    PluginEventType type;
    json::JToken data;
    std::chrono::system_clock::time_point timestamp;
    
    PluginEventData(PluginEventType t, const json::JToken& d)
        : type(t), data(d), timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Enhanced plugin manager with lifecycle management and event system
 * 
 * Provides comprehensive plugin management capabilities:
 * - Dynamic plugin loading and unloading
 * - Dependency resolution
 * - Configuration management
 * - Event broadcasting
 * - Health monitoring
 * - Hot reload support
 */
class PluginManagerEnhanced {
public:
    /**
     * @brief Constructor
     * @param neo_system Neo system instance
     */
    explicit PluginManagerEnhanced(std::shared_ptr<core::NeoSystem> neo_system);
    
    /**
     * @brief Destructor
     */
    ~PluginManagerEnhanced();
    
    /**
     * @brief Initialize plugin manager
     * @return true if initialized successfully
     */
    bool Initialize();
    
    /**
     * @brief Shutdown plugin manager
     */
    void Shutdown();
    
    /**
     * @brief Load plugin from shared library
     * @param plugin_path Path to plugin shared library
     * @param config Plugin configuration
     * @return true if loaded successfully
     */
    bool LoadPlugin(const std::string& plugin_path, const json::JToken& config = json::JToken());
    
    /**
     * @brief Unload plugin
     * @param plugin_name Plugin name
     * @return true if unloaded successfully
     */
    bool UnloadPlugin(const std::string& plugin_name);
    
    /**
     * @brief Start plugin
     * @param plugin_name Plugin name
     * @return true if started successfully
     */
    bool StartPlugin(const std::string& plugin_name);
    
    /**
     * @brief Stop plugin
     * @param plugin_name Plugin name
     * @return true if stopped successfully
     */
    bool StopPlugin(const std::string& plugin_name);
    
    /**
     * @brief Get plugin by name
     * @param plugin_name Plugin name
     * @return Plugin instance or nullptr
     */
    std::shared_ptr<PluginBase> GetPlugin(const std::string& plugin_name) const;
    
    /**
     * @brief Get all loaded plugins
     * @return Vector of plugin instances
     */
    std::vector<std::shared_ptr<PluginBase>> GetAllPlugins() const;
    
    /**
     * @brief Get plugin metadata
     * @param plugin_name Plugin name
     * @return Plugin metadata
     */
    PluginMetadata GetPluginMetadata(const std::string& plugin_name) const;
    
    /**
     * @brief Get all plugin metadata
     * @return Vector of plugin metadata
     */
    std::vector<PluginMetadata> GetAllPluginMetadata() const;
    
    /**
     * @brief Check if plugin is loaded
     * @param plugin_name Plugin name
     * @return true if loaded
     */
    bool IsPluginLoaded(const std::string& plugin_name) const;
    
    /**
     * @brief Check if plugin is started
     * @param plugin_name Plugin name
     * @return true if started
     */
    bool IsPluginStarted(const std::string& plugin_name) const;
    
    /**
     * @brief Broadcast event to all plugins
     * @param event_data Event data
     */
    void BroadcastEvent(const PluginEventData& event_data);
    
    /**
     * @brief Subscribe plugin to specific event types
     * @param plugin_name Plugin name
     * @param event_types Vector of event types
     */
    void SubscribeToEvents(const std::string& plugin_name, 
                          const std::vector<PluginEventType>& event_types);
    
    /**
     * @brief Unsubscribe plugin from event types
     * @param plugin_name Plugin name
     * @param event_types Vector of event types
     */
    void UnsubscribeFromEvents(const std::string& plugin_name,
                              const std::vector<PluginEventType>& event_types);
    
    /**
     * @brief Enable hot reload for plugins
     * @param enabled Enable/disable hot reload
     */
    void SetHotReloadEnabled(bool enabled);
    
    /**
     * @brief Check for plugin updates and reload if necessary
     */
    void CheckAndReloadPlugins();
    
    /**
     * @brief Get plugin health status
     * @param plugin_name Plugin name
     * @return Health status (true = healthy)
     */
    bool GetPluginHealth(const std::string& plugin_name) const;
    
    /**
     * @brief Get plugin performance metrics
     * @param plugin_name Plugin name
     * @return Performance metrics as JSON
     */
    json::JToken GetPluginMetrics(const std::string& plugin_name) const;
    
    /**
     * @brief Set plugin configuration
     * @param plugin_name Plugin name
     * @param config Configuration data
     * @return true if configuration applied successfully
     */
    bool SetPluginConfiguration(const std::string& plugin_name, const json::JToken& config);
    
    /**
     * @brief Get plugin configuration
     * @param plugin_name Plugin name
     * @return Plugin configuration
     */
    json::JToken GetPluginConfiguration(const std::string& plugin_name) const;
    
    /**
     * @brief Enable or disable plugin
     * @param plugin_name Plugin name
     * @param enabled Enable/disable plugin
     * @return true if state changed successfully
     */
    bool SetPluginEnabled(const std::string& plugin_name, bool enabled);

private:
    /**
     * @brief Load plugin metadata from file
     * @param plugin_path Plugin path
     * @return Plugin metadata
     */
    PluginMetadata LoadPluginMetadata(const std::string& plugin_path);
    
    /**
     * @brief Resolve plugin dependencies
     * @param plugin_name Plugin name
     * @return true if dependencies resolved
     */
    bool ResolveDependencies(const std::string& plugin_name);
    
    /**
     * @brief Update plugin state
     * @param plugin_name Plugin name
     * @param state New state
     * @param error_message Optional error message
     */
    void UpdatePluginState(const std::string& plugin_name, 
                          PluginState state, 
                          const std::string& error_message = "");
    
    /**
     * @brief Monitor plugin health
     */
    void MonitorPluginHealth();
    
    /**
     * @brief Handle plugin failure
     * @param plugin_name Plugin name
     * @param error Error description
     */
    void HandlePluginFailure(const std::string& plugin_name, const std::string& error);
    
    /**
     * @brief Get event type name
     * @param type Event type
     * @return Event type name
     */
    std::string GetEventTypeName(PluginEventType type) const;

private:
    std::shared_ptr<core::NeoSystem> neo_system_;
    
    // Plugin management
    mutable std::mutex plugins_mutex_;
    std::unordered_map<std::string, std::shared_ptr<PluginBase>> loaded_plugins_;
    std::unordered_map<std::string, PluginMetadata> plugin_metadata_;
    std::unordered_map<std::string, void*> plugin_handles_;  // Shared library handles
    
    // Event system
    mutable std::mutex events_mutex_;
    std::unordered_map<std::string, std::vector<PluginEventType>> plugin_subscriptions_;
    
    // Configuration
    mutable std::mutex config_mutex_;
    std::unordered_map<std::string, json::JToken> plugin_configurations_;
    
    // Hot reload
    std::atomic<bool> hot_reload_enabled_{false};
    std::unordered_map<std::string, std::filesystem::file_time_type> plugin_file_times_;
    
    // Health monitoring
    std::atomic<bool> health_monitoring_enabled_{true};
    std::thread health_monitor_thread_;
    
    // State management
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutting_down_{false};
};

/**
 * @brief Plugin factory function type
 */
using PluginFactoryFunction = std::function<std::shared_ptr<PluginBase>()>;

/**
 * @brief Plugin registry for built-in plugins
 */
class PluginRegistry {
public:
    /**
     * @brief Register plugin factory
     * @param name Plugin name
     * @param factory Factory function
     */
    static void RegisterPlugin(const std::string& name, PluginFactoryFunction factory);
    
    /**
     * @brief Create plugin instance
     * @param name Plugin name
     * @return Plugin instance or nullptr
     */
    static std::shared_ptr<PluginBase> CreatePlugin(const std::string& name);
    
    /**
     * @brief Get all registered plugin names
     * @return Vector of plugin names
     */
    static std::vector<std::string> GetRegisteredPlugins();

private:
    static std::unordered_map<std::string, PluginFactoryFunction> factories_;
    static std::mutex registry_mutex_;
};

/**
 * @brief Macro for plugin registration
 */
#define REGISTER_PLUGIN(name, class_name) \
    namespace { \
        static bool registered_##class_name = []() { \
            PluginRegistry::RegisterPlugin(name, []() -> std::shared_ptr<PluginBase> { \
                return std::make_shared<class_name>(); \
            }); \
            return true; \
        }(); \
    }

/**
 * @brief Get plugin state name
 * @param state Plugin state
 * @return State name
 */
std::string GetPluginStateName(PluginState state);

/**
 * @brief Get plugin event type name
 * @param type Event type
 * @return Event type name
 */
std::string GetPluginEventTypeName(PluginEventType type);

} // namespace neo::plugins