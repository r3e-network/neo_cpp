/**
 * @file settings.h
 * @brief Configuration settings
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/protocol_settings.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo
{
/**
 * @brief Storage configuration settings
 */
struct StorageSettings
{
    std::string Engine = "LevelDB";  // "LevelDB", "RocksDB", "Memory"
    std::string Path = "./data";     // Storage path
    bool ReadOnly = false;           // Read-only mode
    int CacheSize = 100;             // Cache size in MB
    bool EnableCompression = true;   // Enable compression
    int MaxOpenFiles = 1000;         // Maximum open files
};

/**
 * @brief RPC server configuration settings
 */
struct RpcSettings
{
    bool Enabled = false;                     // Enable RPC server
    int Port = 10332;                         // RPC server port
    std::string BindAddress = "127.0.0.1";    // Bind address
    std::string Username;                     // Basic auth username
    std::string Password;                     // Basic auth password
    bool EnableCors = false;                  // Enable CORS
    std::vector<std::string> AllowedOrigins;  // Allowed CORS origins
    int MaxConnections = 40;                  // Maximum concurrent connections
    bool EnableSsl = false;                   // Enable SSL/TLS
    std::string SslCert;                      // SSL certificate path
    std::string SslKey;                       // SSL private key path
    std::vector<std::string> TrustedAuthorities;  // Client certificate authorities
    std::string SslCiphers;                   // Optional cipher list
    std::string MinTlsVersion = "1.2";        // Minimum TLS protocol
    int RequestTimeoutMs = 30000;             // Request timeout in milliseconds
    int MaxIteratorResultItems = 100;         // Maximum iterator items per RPC page
    bool EnableRateLimit = false;             // Enable global rate limiting
    int MaxRequestsPerSecond = 100;           // Maximum requests per window
    int RateLimitWindowSeconds = 1;           // Rate limit window size
    int MaxRequestBodyBytes = 10 * 1024 * 1024;  // Max HTTP payload size
    bool SessionEnabled = false;              // Enable RPC session tracking
    int SessionExpirationMinutes = 60;        // Session expiration interval
    bool EnableAuditTrail = false;            // Enable audit logging
    bool EnableSecurityLogging = false;       // Enable security logging
    int MaxFindResultItems = 100;             // Max iterator results for find RPCs
};

/**
 * @brief P2P network configuration settings
 */
struct P2PSettings
{
    int Port = 10333;                     // P2P listen port
    std::string BindAddress = "0.0.0.0";  // Bind address
    int MinDesiredConnections = 10;       // Minimum desired peer connections
    int MaxConnections = 40;              // Maximum peer connections
    int MaxConnectionsPerAddress = 3;     // Max connections per IP address
    int DialTimeoutMs = 5000;             // Connection timeout in milliseconds
    bool EnableUpnp = true;               // Enable UPnP port mapping
    bool EnableCompression = true;        // Enable P2P compression
    std::vector<std::string> Seeds;       // Seed node endpoints (host:port)
};

/**
 * @brief Application-level configuration settings
 */
struct ApplicationSettings
{
    std::string DataPath = "./data";  // Data directory path
    std::string LogPath = "./logs";   // Log directory path
    int LogLevel = 2;                 // 0=Error, 1=Warning, 2=Info, 3=Debug, 4=Trace
    bool LogToConsole = true;         // Log to console
    bool LogToFile = true;            // Log to file
    int MaxLogFileSizeMB = 100;       // Maximum log file size
    int MaxLogFiles = 10;             // Maximum number of log files
    bool EnableMetrics = false;       // Enable metrics collection
    int MetricsPort = 9090;           // Metrics server port
};

/**
 * @brief Plugin configuration settings
 */
struct PluginSettings
{
    std::vector<std::string> Plugins;      // List of plugins to load
    std::string PluginPath = "./plugins";  // Plugin directory path
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
        PluginConfigs;  // Plugin-specific configurations
};

/**
 * @brief Complete configuration settings for the Neo node
 */
class Settings
{
   public:
    /**
     * @brief Default constructor with default settings
     */
    Settings();

    /**
     * @brief Copy constructor
     */
    Settings(const Settings& other);

    /**
     * @brief Assignment operator
     */
    Settings& operator=(const Settings& other);

    /**
     * @brief Protocol configuration
     */
    std::shared_ptr<ProtocolSettings> Protocol;

    /**
     * @brief Storage configuration
     */
    StorageSettings Storage;

    /**
     * @brief RPC server configuration
     */
    RpcSettings RPC;

    /**
     * @brief P2P network configuration
     */
    P2PSettings P2P;

    /**
     * @brief Application configuration
     */
    ApplicationSettings Application;

    /**
     * @brief Plugin configuration
     */
    PluginSettings Plugins;

    /**
     * @brief Loads settings from a JSON configuration file
     * @param configPath Path to the configuration file
     * @return Settings object loaded from file, or default settings if file not found
     */
    static Settings Load(const std::string& configPath);

    /**
     * @brief Loads settings from a JSON string
     * @param jsonContent JSON configuration content
     * @return Settings object parsed from JSON
     */
    static Settings LoadFromJson(const std::string& jsonContent);

    /**
     * @brief Saves current settings to a JSON configuration file
     * @param configPath Path to save the configuration file
     * @return true if successful, false otherwise
     */
    bool Save(const std::string& configPath) const;

    /**
     * @brief Converts settings to JSON string
     * @return JSON representation of the settings
     */
    std::string ToJson() const;

    /**
     * @brief Validates the configuration settings
     * @return true if settings are valid, false otherwise
     */
    bool Validate() const;

    /**
     * @brief Gets the default settings
     * @return Default settings object
     */
    static Settings GetDefault();

    /**
     * @brief Creates settings for MainNet
     * @return MainNet settings
     */
    static Settings CreateMainNetSettings();

    /**
     * @brief Creates settings for TestNet
     * @return TestNet settings
     */
    static Settings CreateTestNetSettings();

    /**
     * @brief Creates settings for development/local testing
     * @return Development settings
     */
    static Settings CreateDevelopmentSettings();

    /**
     * @brief Merges settings from another settings object
     * @param other The settings to merge from
     * @param overwriteExisting Whether to overwrite existing values
     */
    void Merge(const Settings& other, bool overwriteExisting = true);

    /**
     * @brief Gets a string representation of the settings
     * @return String representation for debugging
     */
    std::string ToString() const;

   private:
    /**
     * @brief Loads protocol settings from JSON
     * @param json JSON object containing protocol settings
     */
    void LoadProtocolSettings(const std::string& json);

    /**
     * @brief Loads storage settings from JSON
     * @param json JSON object containing storage settings
     */
    void LoadStorageSettings(const std::string& json);

    /**
     * @brief Loads RPC settings from JSON
     * @param json JSON object containing RPC settings
     */
    void LoadRpcSettings(const std::string& json);

    /**
     * @brief Loads P2P settings from JSON
     * @param json JSON object containing P2P settings
     */
    void LoadP2PSettings(const std::string& json);

    /**
     * @brief Loads application settings from JSON
     * @param json JSON object containing application settings
     */
    void LoadApplicationSettings(const std::string& json);

    /**
     * @brief Loads plugin settings from JSON
     * @param json JSON object containing plugin settings
     */
    void LoadPluginSettings(const std::string& json);

    /**
     * @brief Validates storage settings
     * @return true if valid, false otherwise
     */
    bool ValidateStorageSettings() const;

    /**
     * @brief Validates RPC settings
     * @return true if valid, false otherwise
     */
    bool ValidateRpcSettings() const;

    /**
     * @brief Validates P2P settings
     * @return true if valid, false otherwise
     */
    bool ValidateP2PSettings() const;

    /**
     * @brief Validates application settings
     * @return true if valid, false otherwise
     */
    bool ValidateApplicationSettings() const;
};
}  // namespace neo
