/**
 * @file configuration_manager.h
 * @brief Management components
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/core/logging.h>
#include <neo/io/json/jobject.h>

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace neo::config
{
/**
 * @brief Configuration source interface
 */
class IConfigurationSource
{
   public:
    virtual ~IConfigurationSource() = default;
    virtual bool Load() = 0;
    virtual std::string Get(const std::string& key) const = 0;
    virtual bool Contains(const std::string& key) const = 0;
    virtual json::JObject GetSection(const std::string& section) const = 0;
    virtual std::string GetName() const = 0;
    virtual int GetPriority() const = 0;
};

/**
 * @brief JSON file configuration source
 */
class JsonFileConfigSource : public IConfigurationSource
{
   private:
    std::string filepath_;
    json::JObject config_;
    int priority_;
    mutable std::mutex mutex_;

   public:
    JsonFileConfigSource(const std::string& filepath, int priority = 0);

    bool Load() override;
    std::string Get(const std::string& key) const override;
    bool Contains(const std::string& key) const override;
    json::JObject GetSection(const std::string& section) const override;
    std::string GetName() const override { return "JsonFile:" + filepath_; }
    int GetPriority() const override { return priority_; }
};

/**
 * @brief Environment variables configuration source
 */
class EnvironmentConfigSource : public IConfigurationSource
{
   private:
    std::string prefix_;
    mutable std::unordered_map<std::string, std::string> cache_;
    int priority_;
    mutable std::mutex mutex_;

   public:
    EnvironmentConfigSource(const std::string& prefix = "NEO_", int priority = 10);

    bool Load() override;
    std::string Get(const std::string& key) const override;
    bool Contains(const std::string& key) const override;
    json::JObject GetSection(const std::string& section) const override;
    std::string GetName() const override { return "Environment"; }
    int GetPriority() const override { return priority_; }
};

/**
 * @brief Command line arguments configuration source
 */
class CommandLineConfigSource : public IConfigurationSource
{
   private:
    std::unordered_map<std::string, std::string> args_;
    int priority_;

   public:
    CommandLineConfigSource(int argc, char* argv[], int priority = 20);

    bool Load() override { return true; }
    std::string Get(const std::string& key) const override;
    bool Contains(const std::string& key) const override;
    json::JObject GetSection(const std::string& section) const override;
    std::string GetName() const override { return "CommandLine"; }
    int GetPriority() const override { return priority_; }
};

/**
 * @brief Configuration change callback
 */
using ConfigChangeCallback =
    std::function<void(const std::string& key, const std::string& old_value, const std::string& new_value)>;

/**
 * @brief Production configuration manager
 *
 * Manages configuration from multiple sources with priority-based override.
 * Supports hot-reloading, validation, and change notifications.
 */
class ConfigurationManager
{
   private:
    std::vector<std::shared_ptr<IConfigurationSource>> sources_;
    mutable std::mutex mutex_;
    std::shared_ptr<core::Logger> logger_;

    // Cache for resolved values
    mutable std::unordered_map<std::string, std::string> cache_;
    mutable std::chrono::steady_clock::time_point cache_expiry_;
    std::chrono::seconds cache_duration_{60};

    // Change notifications
    std::unordered_map<std::string, std::vector<ConfigChangeCallback>> callbacks_;

    // Singleton
    static std::shared_ptr<ConfigurationManager> instance_;
    static std::mutex instance_mutex_;

    ConfigurationManager();

   public:
    static std::shared_ptr<ConfigurationManager> GetInstance();

    /**
     * @brief Add configuration source
     * @param source Configuration source to add
     */
    void AddSource(std::shared_ptr<IConfigurationSource> source);

    /**
     * @brief Remove configuration source by name
     * @param name Name of source to remove
     */
    void RemoveSource(const std::string& name);

    /**
     * @brief Reload all configuration sources
     * @return true if all sources loaded successfully
     */
    bool Reload();

    /**
     * @brief Get configuration value
     * @param key Configuration key
     * @param default_value Default value if key not found
     * @return Configuration value
     */
    std::string Get(const std::string& key, const std::string& default_value = "") const;

    /**
     * @brief Get configuration value as integer
     */
    int GetInt(const std::string& key, int default_value = 0) const;

    /**
     * @brief Get configuration value as boolean
     */
    bool GetBool(const std::string& key, bool default_value = false) const;

    /**
     * @brief Get configuration value as double
     */
    double GetDouble(const std::string& key, double default_value = 0.0) const;

    /**
     * @brief Get configuration section as JSON
     */
    json::JObject GetSection(const std::string& section) const;

    /**
     * @brief Check if configuration key exists
     */
    bool Contains(const std::string& key) const;

    /**
     * @brief Register callback for configuration changes
     * @param key Configuration key to watch (use "*" for all)
     * @param callback Callback function
     */
    void RegisterChangeCallback(const std::string& key, ConfigChangeCallback callback);

    /**
     * @brief Validate configuration against schema
     * @param schema JSON schema
     * @return Validation errors, empty if valid
     */
    std::vector<std::string> Validate(const json::JObject& schema) const;

    /**
     * @brief Export current configuration
     * @return Current configuration as JSON
     */
    json::JObject Export() const;

    /**
     * @brief Set cache duration
     * @param duration Cache duration in seconds
     */
    void SetCacheDuration(std::chrono::seconds duration) { cache_duration_ = duration; }

   private:
    /**
     * @brief Clear cache
     */
    void ClearCache();

    /**
     * @brief Check if cache is expired
     */
    bool IsCacheExpired() const;

    /**
     * @brief Notify change callbacks
     */
    void NotifyChange(const std::string& key, const std::string& old_value, const std::string& new_value);

    /**
     * @brief Resolve value from sources
     */
    std::string ResolveValue(const std::string& key) const;
};

/**
 * @brief Configuration validation rules
 */
class ConfigValidator
{
   public:
    struct Rule
    {
        std::string key;
        std::function<bool(const std::string&)> validator;
        std::string error_message;
    };

    static bool ValidatePort(const std::string& value);
    static bool ValidateIPAddress(const std::string& value);
    static bool ValidatePath(const std::string& value);
    static bool ValidatePositiveInteger(const std::string& value);
    static bool ValidatePercentage(const std::string& value);

    static std::vector<Rule> GetDefaultRules();
};

// Convenience macros
#define CONFIG_GET(key) neo::config::ConfigurationManager::GetInstance()->Get(key)
#define CONFIG_GET_INT(key) neo::config::ConfigurationManager::GetInstance()->GetInt(key)
#define CONFIG_GET_BOOL(key) neo::config::ConfigurationManager::GetInstance()->GetBool(key)
#define CONFIG_GET_DOUBLE(key) neo::config::ConfigurationManager::GetInstance()->GetDouble(key)
}  // namespace neo::config