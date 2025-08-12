#pragma once

#include <neo/cryptography/ecc_point.h>
#include <neo/io/json.h>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace neo::config
{

/**
 * @brief Network configuration settings
 */
struct NetworkConfig
{
    std::string network = "mainnet";
    uint32_t magic = 860833102;  // MainNet magic number
    uint8_t addressVersion = 53;
    std::vector<std::string> seedList;
    uint16_t port = 10333;
    size_t maxConnections = 100;
    size_t minDesiredConnections = 10;
    size_t maxConnectionsPerAddress = 3;
};

/**
 * @brief RPC server configuration
 */
struct RpcConfig
{
    bool enabled = true;
    uint16_t port = 10332;
    std::string sslCertificate;
    std::string sslCertificatePassword;
    size_t maxConcurrentConnections = 40;
    uint64_t maxGasInvoke = 50000000;
    size_t maxIteratorResultItems = 100;
    size_t maxStackSize = 2048;
    std::vector<std::string> disabledMethods;
};

/**
 * @brief Storage configuration
 */
struct StorageConfig
{
    std::string engine = "LevelDB";
    std::string path = "./data";
};

/**
 * @brief Logging configuration
 */
struct LoggingConfig
{
    std::string path = "./logs";
    std::string level = "INFO";
    bool console = true;
    bool file = true;
    size_t maxFileSize = 10485760;  // 10MB
    size_t maxBackupFiles = 10;
};

/**
 * @brief Wallet configuration
 */
struct WalletConfig
{
    std::string path;
    std::string password;
    bool isActive = false;
};

/**
 * @brief Protocol configuration
 */
struct ProtocolConfig
{
    uint32_t network = 860833102;
    uint8_t addressVersion = 53;
    std::chrono::milliseconds millisecondsPerBlock{15000};
    size_t maxTransactionsPerBlock = 512;
    size_t memoryPoolMaxTransactions = 50000;
    size_t maxTraceableBlocks = 2102400;
    uint64_t initialGasDistribution = 5200000000000000;
    size_t validatorsCount = 7;
    std::vector<cryptography::ECPoint> standbyCommittee;
};

/**
 * @brief Plugin configuration
 */
struct PluginConfig
{
    bool enabled = false;
    std::string name;
    io::JsonValue settings;
};

/**
 * @brief Complete Neo node configuration
 */
class Configuration
{
   public:
    /**
     * @brief Load configuration from file
     * @param path Path to configuration file
     * @return Configuration instance
     */
    static std::shared_ptr<Configuration> Load(const std::string& path);

    /**
     * @brief Load configuration from JSON
     * @param json JSON configuration object
     * @return Configuration instance
     */
    static std::shared_ptr<Configuration> LoadFromJson(const io::JsonValue& json);

    /**
     * @brief Save configuration to file
     * @param path Path to save configuration
     * @return True if successful
     */
    bool Save(const std::string& path) const;

    /**
     * @brief Convert configuration to JSON
     * @return JSON representation
     */
    io::JsonValue ToJson() const;

    /**
     * @brief Get network configuration
     */
    const NetworkConfig& GetNetwork() const { return network_; }
    NetworkConfig& GetNetwork() { return network_; }

    /**
     * @brief Get RPC configuration
     */
    const RpcConfig& GetRpc() const { return rpc_; }
    RpcConfig& GetRpc() { return rpc_; }

    /**
     * @brief Get storage configuration
     */
    const StorageConfig& GetStorage() const { return storage_; }
    StorageConfig& GetStorage() { return storage_; }

    /**
     * @brief Get logging configuration
     */
    const LoggingConfig& GetLogging() const { return logging_; }
    LoggingConfig& GetLogging() { return logging_; }

    /**
     * @brief Get wallet configuration
     */
    const WalletConfig& GetWallet() const { return wallet_; }
    WalletConfig& GetWallet() { return wallet_; }

    /**
     * @brief Get protocol configuration
     */
    const ProtocolConfig& GetProtocol() const { return protocol_; }
    ProtocolConfig& GetProtocol() { return protocol_; }

    /**
     * @brief Get plugin configurations
     */
    const std::vector<PluginConfig>& GetPlugins() const { return plugins_; }
    std::vector<PluginConfig>& GetPlugins() { return plugins_; }

    /**
     * @brief Get default configuration for mainnet
     */
    static std::shared_ptr<Configuration> GetMainnetConfig();

    /**
     * @brief Get default configuration for testnet
     */
    static std::shared_ptr<Configuration> GetTestnetConfig();

    /**
     * @brief Get default configuration for private net
     */
    static std::shared_ptr<Configuration> GetPrivateNetConfig();

    /**
     * @brief Validate configuration
     * @return True if configuration is valid
     */
    bool Validate() const;

   private:
    NetworkConfig network_;
    RpcConfig rpc_;
    StorageConfig storage_;
    LoggingConfig logging_;
    WalletConfig wallet_;
    ProtocolConfig protocol_;
    std::vector<PluginConfig> plugins_;

    void ParseNetworkConfig(const io::JsonValue& json);
    void ParseRpcConfig(const io::JsonValue& json);
    void ParseStorageConfig(const io::JsonValue& json);
    void ParseLoggingConfig(const io::JsonValue& json);
    void ParseWalletConfig(const io::JsonValue& json);
    void ParseProtocolConfig(const io::JsonValue& json);
    void ParsePluginConfigs(const io::JsonValue& json);
};

/**
 * @brief Global configuration instance
 */
class ConfigurationManager
{
   public:
    /**
     * @brief Get the singleton instance
     */
    static ConfigurationManager& GetInstance();

    /**
     * @brief Initialize with configuration file
     * @param configPath Path to configuration file
     * @return True if successful
     */
    bool Initialize(const std::string& configPath);

    /**
     * @brief Initialize with configuration
     * @param config Configuration instance
     * @return True if successful
     */
    bool Initialize(std::shared_ptr<Configuration> config);

    /**
     * @brief Get current configuration
     * @return Current configuration or nullptr if not initialized
     */
    std::shared_ptr<Configuration> GetConfiguration() const;

    /**
     * @brief Check if initialized
     * @return True if initialized
     */
    bool IsInitialized() const;

    /**
     * @brief Reload configuration from file
     * @return True if successful
     */
    bool Reload();

   private:
    ConfigurationManager() = default;
    ~ConfigurationManager() = default;
    ConfigurationManager(const ConfigurationManager&) = delete;
    ConfigurationManager& operator=(const ConfigurationManager&) = delete;

    std::shared_ptr<Configuration> config_;
    std::string configPath_;
    mutable std::mutex mutex_;
};

}  // namespace neo::config