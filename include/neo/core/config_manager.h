#pragma once

#include "safe_conversions.h"
#include <cstdlib>
#include <fstream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <unordered_map>

namespace neo::core
{

/**
 * @brief Configuration management with environment variable support
 *
 * Supports configuration hierarchy:
 * 1. Environment variables (highest priority)
 * 2. Configuration file
 * 3. Default values (lowest priority)
 *
 * Environment variables can be referenced in config as ${VAR_NAME}
 */
class ConfigManager
{
  public:
    /**
     * @brief Load configuration from file with environment variable substitution
     * @param configPath Path to configuration file
     * @throws std::runtime_error on parse error
     */
    void LoadFromFile(const std::string& configPath)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        std::ifstream file(configPath);
        if (!file.is_open())
        {
            throw std::runtime_error("Cannot open configuration file: " + configPath);
        }

        try
        {
            file >> config_;

            // Process environment variable substitutions
            ProcessEnvironmentVariables(config_);

            // Validate required fields
            ValidateConfiguration();
        }
        catch (const nlohmann::json::exception& e)
        {
            throw std::runtime_error("Invalid JSON in configuration file: " + std::string(e.what()));
        }
    }

    /**
     * @brief Get string configuration value
     * @param path JSON path (e.g., "ApplicationConfiguration.RPC.BindAddress")
     * @param defaultValue Default value if not found
     * @return Configuration value
     */
    std::string GetString(const std::string& path, const std::string& defaultValue = "") const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check environment variable first
        std::string envVar = PathToEnvVar(path);
        const char* envValue = std::getenv(envVar.c_str());
        if (envValue)
        {
            return std::string(envValue);
        }

        // Check JSON config
        try
        {
            nlohmann::json::json_pointer ptr(PathToJsonPointer(path));
            if (config_.contains(ptr))
            {
                return config_[ptr].get<std::string>();
            }
        }
        catch (...)
        {
            // Path not found
        }

        return defaultValue;
    }

    /**
     * @brief Get integer configuration value
     * @param path JSON path
     * @param defaultValue Default value if not found
     * @return Configuration value
     */
    int GetInt(const std::string& path, int defaultValue = 0) const
    {
        std::string strValue = GetString(path, std::to_string(defaultValue));
        auto maybeValue = SafeConversions::TryToInt32(strValue);
        return maybeValue ? *maybeValue : defaultValue;
    }

    /**
     * @brief Get boolean configuration value
     * @param path JSON path
     * @param defaultValue Default value if not found
     * @return Configuration value
     */
    bool GetBool(const std::string& path, bool defaultValue = false) const
    {
        std::string strValue = GetString(path, defaultValue ? "true" : "false");
        std::transform(strValue.begin(), strValue.end(), strValue.begin(), ::tolower);
        return strValue == "true" || strValue == "1" || strValue == "yes";
    }

    /**
     * @brief Get uint32_t configuration value
     * @param path JSON path
     * @param defaultValue Default value if not found
     * @return Configuration value
     */
    uint32_t GetUInt32(const std::string& path, uint32_t defaultValue = 0) const
    {
        std::string strValue = GetString(path, std::to_string(defaultValue));
        auto maybeValue = SafeConversions::TryToUInt32(strValue);
        return maybeValue ? *maybeValue : defaultValue;
    }

    /**
     * @brief Get uint16_t configuration value (for ports)
     * @param path JSON path
     * @param defaultValue Default value if not found
     * @return Configuration value
     */
    uint16_t GetPort(const std::string& path, uint16_t defaultValue = 0) const
    {
        std::string strValue = GetString(path, std::to_string(defaultValue));
        try
        {
            return SafeConversions::SafeToPort(strValue);
        }
        catch (...)
        {
            return defaultValue;
        }
    }

    /**
     * @brief Set configuration value (runtime override)
     * @param path JSON path
     * @param value Value to set
     */
    template <typename T>
    void Set(const std::string& path, const T& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        try
        {
            nlohmann::json::json_pointer ptr(PathToJsonPointer(path));
            config_[ptr] = value;
        }
        catch (const nlohmann::json::exception& e)
        {
            throw std::runtime_error("Failed to set config value: " + std::string(e.what()));
        }
    }

    /**
     * @brief Get the entire configuration as JSON
     * @return Configuration JSON
     */
    nlohmann::json GetJson() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }

    /**
     * @brief Get singleton instance
     * @return ConfigManager instance
     */
    static ConfigManager& GetInstance()
    {
        static ConfigManager instance;
        return instance;
    }

    /**
     * @brief Get default port for network
     * @param network Network name (mainnet, testnet, privnet)
     * @param service Service type (p2p, rpc, ws)
     * @return Default port number
     */
    static uint16_t GetDefaultPort(const std::string& network, const std::string& service)
    {
        if (network == "mainnet")
        {
            if (service == "p2p")
                return 10333;
            if (service == "rpc")
                return 10332;
            if (service == "ws")
                return 10334;
        }
        else if (network == "testnet")
        {
            if (service == "p2p")
                return 20333;
            if (service == "rpc")
                return 20332;
            if (service == "ws")
                return 20334;
        }
        else if (network == "privnet")
        {
            if (service == "p2p")
                return 30333;
            if (service == "rpc")
                return 30332;
            if (service == "ws")
                return 30334;
        }
        return 0;
    }

  private:
    ConfigManager() = default;

    /**
     * @brief Process environment variable substitutions in JSON
     * @param json JSON object to process
     */
    void ProcessEnvironmentVariables(nlohmann::json& json)
    {
        if (json.is_string())
        {
            std::string value = json.get<std::string>();
            json = SubstituteEnvironmentVariables(value);
        }
        else if (json.is_object())
        {
            for (auto& [key, value] : json.items())
            {
                ProcessEnvironmentVariables(value);
            }
        }
        else if (json.is_array())
        {
            for (auto& element : json)
            {
                ProcessEnvironmentVariables(element);
            }
        }
    }

    /**
     * @brief Substitute environment variables in string
     * @param str String containing ${VAR_NAME} placeholders
     * @return String with substitutions
     */
    std::string SubstituteEnvironmentVariables(const std::string& str)
    {
        std::regex envRegex(R"(\$\{([A-Z_][A-Z0-9_]*)\})");
        std::string result = str;

        std::smatch match;
        while (std::regex_search(result, match, envRegex))
        {
            std::string varName = match[1].str();
            const char* envValue = std::getenv(varName.c_str());

            if (envValue)
            {
                result.replace(match.position(), match.length(), envValue);
            }
            else
            {
                // Keep the placeholder if env var not found
                break;
            }
        }

        return result;
    }

    /**
     * @brief Convert JSON path to environment variable name
     * @param path Dot-separated path
     * @return Environment variable name
     */
    std::string PathToEnvVar(const std::string& path) const
    {
        std::string envVar = "NEO_";
        for (char c : path)
        {
            if (c == '.')
            {
                envVar += '_';
            }
            else
            {
                envVar += std::toupper(c);
            }
        }
        return envVar;
    }

    /**
     * @brief Convert dot-separated path to JSON pointer
     * @param path Dot-separated path
     * @return JSON pointer path
     */
    std::string PathToJsonPointer(const std::string& path) const
    {
        std::string pointer = "/";
        for (size_t i = 0; i < path.length(); ++i)
        {
            if (path[i] == '.')
            {
                pointer += '/';
            }
            else
            {
                pointer += path[i];
            }
        }
        return pointer;
    }

    /**
     * @brief Validate required configuration fields
     * @throws std::runtime_error if validation fails
     */
    void ValidateConfiguration()
    {
        // Check required fields
        std::vector<std::string> requiredFields = {"ProtocolConfiguration.Magic",
                                                   "ApplicationConfiguration.Storage.Engine",
                                                   "ApplicationConfiguration.P2P.Port"};

        for (const auto& field : requiredFields)
        {
            if (GetString(field).empty())
            {
                throw std::runtime_error("Required configuration field missing: " + field);
            }
        }

        // Validate port ranges
        uint16_t p2pPort = GetPort("ApplicationConfiguration.P2P.Port");
        if (p2pPort == 0)
        {
            throw std::runtime_error("Invalid P2P port configuration");
        }

        uint16_t rpcPort = GetPort("ApplicationConfiguration.RPC.Port");
        if (rpcPort == p2pPort)
        {
            throw std::runtime_error("RPC port cannot be the same as P2P port");
        }
    }

    nlohmann::json config_;
    mutable std::mutex mutex_;
};

/**
 * @brief Convenience class for accessing configuration
 */
class Config
{
  public:
    static std::string GetString(const std::string& path, const std::string& defaultValue = "")
    {
        return ConfigManager::GetInstance().GetString(path, defaultValue);
    }

    static int GetInt(const std::string& path, int defaultValue = 0)
    {
        return ConfigManager::GetInstance().GetInt(path, defaultValue);
    }

    static bool GetBool(const std::string& path, bool defaultValue = false)
    {
        return ConfigManager::GetInstance().GetBool(path, defaultValue);
    }

    static uint32_t GetUInt32(const std::string& path, uint32_t defaultValue = 0)
    {
        return ConfigManager::GetInstance().GetUInt32(path, defaultValue);
    }

    static uint16_t GetPort(const std::string& path, uint16_t defaultValue = 0)
    {
        return ConfigManager::GetInstance().GetPort(path, defaultValue);
    }
};

}  // namespace neo::core