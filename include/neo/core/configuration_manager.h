/**
 * @file configuration_manager.h
 * @brief Production-ready configuration management system
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/core/logging.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo
{
namespace core
{

/**
 * @brief Network configuration settings
 */
struct NetworkConfig
{
    std::string bind_address{"0.0.0.0"};
    uint16_t p2p_port{10333};
    uint16_t max_connections{100};
    uint16_t min_connections{10};
    uint32_t connection_timeout_seconds{30};
    bool enable_upnp{false};
    std::vector<std::string> seed_nodes;
    std::string user_agent{"/NEO:3.6.0-cpp/"};
    uint32_t network_magic{860833102};
};

/**
 * @brief RPC server configuration settings
 */
struct RpcConfig
{
    bool enabled{true};
    std::string bind_address{"127.0.0.1"};
    uint16_t port{10332};
    uint16_t ssl_port{10331};
    uint32_t max_concurrent_requests{100};
    uint32_t max_request_size{10485760}; // 10MB
    uint32_t request_timeout_seconds{30};
    bool enable_cors{true};
    std::vector<std::string> allowed_origins{"*"};
    bool enable_authentication{false};
    std::string username;
    std::string password;
    std::string ssl_cert_file;
    std::string ssl_cert_password;
    std::vector<std::string> trusted_authorities;
    std::vector<std::string> disabled_methods;
    bool session_enabled{false};
    uint32_t session_expiration_seconds{60};
    uint32_t max_gas_invoke{20000000};
    uint64_t max_fee{1000000000};
    uint32_t max_iterator_result_items{100};
    uint32_t max_stack_size{65536};
};

/**
 * @brief Database configuration settings
 */
struct DatabaseConfig
{
    std::string backend{"rocksdb"};
    std::string path{"./data/chain"};
    uint32_t cache_size_mb{512};
    uint32_t write_buffer_size_mb{128};
    bool use_bloom_filter{true};
    bool compression_enabled{true};
    bool read_only{false};
};

/**
 * @brief Consensus configuration settings
 */
struct ConsensusConfig
{
    bool enabled{false};
    std::string wallet_path;
    std::string wallet_password;
    std::string private_key;
    uint32_t block_time_ms{15000};
    uint32_t view_timeout_ms{60000};
    uint32_t max_transactions_per_block{512};
    uint32_t max_block_size{262144}; // 256KB
    uint64_t max_block_system_fee{900000000000};
    bool auto_start{false};
};

/**
 * @brief Logging configuration settings
 */
struct LoggingConfig
{
    std::string level{"info"};
    bool console_output{true};
    bool file_output{true};
    std::string log_file_path{"./logs/neo.log"};
    uint32_t max_file_size_mb{100};
    uint32_t max_files{10};
    bool async_logging{true};
    bool enable_file_rotation{true};
};

/**
 * @brief Monitoring and metrics configuration
 */
struct MonitoringConfig
{
    bool metrics_enabled{true};
    uint16_t metrics_port{9090};
    std::string metrics_bind_address{"127.0.0.1"};
    bool health_checks_enabled{true};
    uint16_t health_check_port{8080};
    std::string health_check_bind_address{"127.0.0.1"};
    uint32_t health_check_interval_seconds{30};
    bool enable_performance_counters{true};
};

/**
 * @brief Performance tuning configuration
 */
struct PerformanceConfig
{
    uint32_t worker_threads{0}; // 0 = auto-detect
    uint32_t tx_pool_size{50000};
    uint32_t max_memory_gb{8};
    bool enable_memory_pooling{true};
    uint32_t block_cache_size{1000};
    uint32_t transaction_cache_size{10000};
    uint32_t contract_cache_size{100};
    uint32_t max_concurrent_transactions{1000};
    uint32_t max_concurrent_blocks{100};
};

/**
 * @brief Security configuration settings
 */
struct SecurityConfig
{
    bool enable_tls{false};
    std::string tls_cert_file;
    std::string tls_key_file;
    bool enable_rate_limiting{true};
    uint32_t rate_limit_rps{100};
    uint32_t ban_duration_seconds{3600};
    bool enable_whitelist{false};
    std::vector<std::string> whitelisted_addresses;
    bool enable_blacklist{false};
    std::vector<std::string> blacklisted_addresses;
    uint32_t max_requests_per_second{100};
};

/**
 * @brief Backup configuration settings
 */
struct BackupConfig
{
    bool enabled{false};
    uint32_t interval_hours{24};
    std::string path{"./backups"};
    uint32_t max_backups{7};
    bool compress_backups{true};
};

/**
 * @brief Advanced configuration settings
 */
struct AdvancedConfig
{
    bool experimental_features{false};
    std::unordered_map<std::string, nlohmann::json> protocol_settings;
    std::unordered_map<std::string, nlohmann::json> plugin_settings;
};

/**
 * @brief Main configuration container for Neo C++ node
 */
class ConfigurationManager
{
  public:
    /**
     * @brief Creates a ConfigurationManager with default settings
     */
    ConfigurationManager();

    /**
     * @brief Loads configuration from a JSON file
     * @param config_file_path Path to the configuration file
     * @return true if successful, false otherwise
     */
    bool LoadFromFile(const std::string& config_file_path);

    /**
     * @brief Loads configuration from a YAML file
     * @param config_file_path Path to the YAML configuration file
     * @return true if successful, false otherwise
     */
    bool LoadFromYamlFile(const std::string& config_file_path);

    /**
     * @brief Loads configuration from JSON string
     * @param json_content JSON configuration content
     * @return true if successful, false otherwise
     */
    bool LoadFromJson(const std::string& json_content);

    /**
     * @brief Loads configuration from environment variables
     * Environment variables should be prefixed with NEO_
     * @return true if successful, false otherwise
     */
    bool LoadFromEnvironment();

    /**
     * @brief Validates all configuration settings
     * @throws std::invalid_argument if validation fails
     */
    void Validate() const;

    /**
     * @brief Saves current configuration to a JSON file
     * @param config_file_path Path where to save the configuration
     * @return true if successful, false otherwise
     */
    bool SaveToFile(const std::string& config_file_path) const;

    /**
     * @brief Gets the singleton instance
     * @return Reference to the configuration manager instance
     */
    static ConfigurationManager& GetInstance();

    // Configuration accessors
    const NetworkConfig& GetNetworkConfig() const { return network_config_; }
    NetworkConfig& GetNetworkConfig() { return network_config_; }

    const RpcConfig& GetRpcConfig() const { return rpc_config_; }
    RpcConfig& GetRpcConfig() { return rpc_config_; }

    const DatabaseConfig& GetDatabaseConfig() const { return database_config_; }
    DatabaseConfig& GetDatabaseConfig() { return database_config_; }

    const ConsensusConfig& GetConsensusConfig() const { return consensus_config_; }
    ConsensusConfig& GetConsensusConfig() { return consensus_config_; }

    const LoggingConfig& GetLoggingConfig() const { return logging_config_; }
    LoggingConfig& GetLoggingConfig() { return logging_config_; }

    const MonitoringConfig& GetMonitoringConfig() const { return monitoring_config_; }
    MonitoringConfig& GetMonitoringConfig() { return monitoring_config_; }

    const PerformanceConfig& GetPerformanceConfig() const { return performance_config_; }
    PerformanceConfig& GetPerformanceConfig() { return performance_config_; }

    const SecurityConfig& GetSecurityConfig() const { return security_config_; }
    SecurityConfig& GetSecurityConfig() { return security_config_; }

    const BackupConfig& GetBackupConfig() const { return backup_config_; }
    BackupConfig& GetBackupConfig() { return backup_config_; }

    const AdvancedConfig& GetAdvancedConfig() const { return advanced_config_; }
    AdvancedConfig& GetAdvancedConfig() { return advanced_config_; }

    /**
     * @brief Gets the current configuration file path
     * @return The path to the loaded configuration file
     */
    const std::string& GetConfigFilePath() const { return config_file_path_; }

    /**
     * @brief Checks if configuration was loaded from a file
     * @return true if loaded from file, false if using defaults
     */
    bool IsLoadedFromFile() const { return loaded_from_file_; }

  private:
    NetworkConfig network_config_;
    RpcConfig rpc_config_;
    DatabaseConfig database_config_;
    ConsensusConfig consensus_config_;
    LoggingConfig logging_config_;
    MonitoringConfig monitoring_config_;
    PerformanceConfig performance_config_;
    SecurityConfig security_config_;
    BackupConfig backup_config_;
    AdvancedConfig advanced_config_;

    std::string config_file_path_;
    bool loaded_from_file_{false};

    // Private helper methods
    void LoadNetworkConfig(const nlohmann::json& json);
    void LoadRpcConfig(const nlohmann::json& json);
    void LoadDatabaseConfig(const nlohmann::json& json);
    void LoadConsensusConfig(const nlohmann::json& json);
    void LoadLoggingConfig(const nlohmann::json& json);
    void LoadMonitoringConfig(const nlohmann::json& json);
    void LoadPerformanceConfig(const nlohmann::json& json);
    void LoadSecurityConfig(const nlohmann::json& json);
    void LoadBackupConfig(const nlohmann::json& json);
    void LoadAdvancedConfig(const nlohmann::json& json);

    void ValidateNetworkConfig() const;
    void ValidateRpcConfig() const;
    void ValidateDatabaseConfig() const;
    void ValidateConsensusConfig() const;
    void ValidateLoggingConfig() const;
    void ValidateMonitoringConfig() const;
    void ValidatePerformanceConfig() const;
    void ValidateSecurityConfig() const;
    void ValidateBackupConfig() const;

    nlohmann::json NetworkConfigToJson() const;
    nlohmann::json RpcConfigToJson() const;
    nlohmann::json DatabaseConfigToJson() const;
    nlohmann::json ConsensusConfigToJson() const;
    nlohmann::json LoggingConfigToJson() const;
    nlohmann::json MonitoringConfigToJson() const;
    nlohmann::json PerformanceConfigToJson() const;
    nlohmann::json SecurityConfigToJson() const;
    nlohmann::json BackupConfigToJson() const;
    nlohmann::json AdvancedConfigToJson() const;

    std::string GetEnvironmentVariable(const std::string& var_name, const std::string& default_value = "") const;
    int GetEnvironmentVariableInt(const std::string& var_name, int default_value) const;
    bool GetEnvironmentVariableBool(const std::string& var_name, bool default_value) const;
};

} // namespace core
} // namespace neo