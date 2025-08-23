/**
 * @file configuration_manager.cpp
 * @brief Production-ready configuration management system implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/configuration_manager.h>
#include <neo/core/logging.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <cstdlib>

#ifdef YAML_SUPPORT
#include <yaml-cpp/yaml.h>
#endif

namespace neo
{
namespace core
{

ConfigurationManager::ConfigurationManager()
{
    // Initialize with secure defaults
    network_config_.seed_nodes = {
        "seed1.neo.org:10333",
        "seed2.neo.org:10333",
        "seed3.neo.org:10333",
        "seed4.neo.org:10333",
        "seed5.neo.org:10333"
    };
}

bool ConfigurationManager::LoadFromFile(const std::string& config_file_path)
{
    try 
    {
        if (!std::filesystem::exists(config_file_path))
        {
            LOG_ERROR("Configuration file not found: {}", config_file_path);
            return false;
        }

        std::ifstream file(config_file_path);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open configuration file: {}", config_file_path);
            return false;
        }

        nlohmann::json config_json;
        file >> config_json;
        file.close();

        config_file_path_ = config_file_path;
        loaded_from_file_ = true;

        // Load each configuration section
        if (config_json.contains("network"))
            LoadNetworkConfig(config_json["network"]);
        if (config_json.contains("rpc"))
            LoadRpcConfig(config_json["rpc"]);
        if (config_json.contains("database"))
            LoadDatabaseConfig(config_json["database"]);
        if (config_json.contains("consensus"))
            LoadConsensusConfig(config_json["consensus"]);
        if (config_json.contains("logging"))
            LoadLoggingConfig(config_json["logging"]);
        if (config_json.contains("monitoring"))
            LoadMonitoringConfig(config_json["monitoring"]);
        if (config_json.contains("performance"))
            LoadPerformanceConfig(config_json["performance"]);
        if (config_json.contains("security"))
            LoadSecurityConfig(config_json["security"]);
        if (config_json.contains("backup"))
            LoadBackupConfig(config_json["backup"]);
        if (config_json.contains("advanced"))
            LoadAdvancedConfig(config_json["advanced"]);

        // Validate loaded configuration
        Validate();

        LOG_INFO("Configuration loaded successfully from: {}", config_file_path);
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to load configuration from {}: {}", config_file_path, e.what());
        return false;
    }
}

bool ConfigurationManager::LoadFromYamlFile(const std::string& config_file_path)
{
#ifdef YAML_SUPPORT
    try 
    {
        if (!std::filesystem::exists(config_file_path))
        {
            LOG_ERROR("YAML configuration file not found: {}", config_file_path);
            return false;
        }

        YAML::Node config_yaml = YAML::LoadFile(config_file_path);
        
        // Convert YAML to JSON for unified processing
        nlohmann::json config_json;
        
        // This is a simplified conversion - in production, you'd want a more robust YAML to JSON converter
        if (config_yaml["node"])
        {
            auto node = config_yaml["node"];
            config_json["network"]["max_connections"] = node["max_peers"].as<int>(100);
            config_json["network"]["min_connections"] = node["min_peers"].as<int>(10);
        }

        if (config_yaml["network"])
        {
            auto network = config_yaml["network"];
            config_json["network"]["bind_address"] = network["bind_address"].as<std::string>("0.0.0.0");
            config_json["network"]["p2p_port"] = network["port"].as<int>(10333);
            config_json["network"]["max_connections"] = network["max_connections"].as<int>(100);
            config_json["network"]["connection_timeout_seconds"] = network["connection_timeout"].as<int>(30);
            config_json["network"]["enable_upnp"] = network["enable_upnp"].as<bool>(false);
        }

        if (config_yaml["rpc"])
        {
            auto rpc = config_yaml["rpc"];
            config_json["rpc"]["enabled"] = rpc["enabled"].as<bool>(true);
            config_json["rpc"]["bind_address"] = rpc["bind_address"].as<std::string>("127.0.0.1");
            config_json["rpc"]["port"] = rpc["port"].as<int>(10332);
            config_json["rpc"]["max_concurrent_requests"] = rpc["max_concurrent_requests"].as<int>(100);
            config_json["rpc"]["max_request_size"] = rpc["max_request_size"].as<int>(10485760);
            config_json["rpc"]["request_timeout_seconds"] = rpc["request_timeout"].as<int>(30);
            config_json["rpc"]["enable_cors"] = rpc["enable_cors"].as<bool>(true);
            config_json["rpc"]["enable_authentication"] = rpc["enable_authentication"].as<bool>(false);
            config_json["rpc"]["username"] = rpc["username"].as<std::string>("");
            config_json["rpc"]["password"] = rpc["password"].as<std::string>("");
            
            if (rpc["allowed_origins"])
            {
                std::vector<std::string> origins;
                for (const auto& origin : rpc["allowed_origins"])
                {
                    origins.push_back(origin.as<std::string>());
                }
                config_json["rpc"]["allowed_origins"] = origins;
            }
        }

        // Continue with other sections...
        if (config_yaml["database"])
        {
            auto db = config_yaml["database"];
            config_json["database"]["backend"] = db["backend"].as<std::string>("rocksdb");
            config_json["database"]["path"] = db["path"].as<std::string>("./data/chain");
            config_json["database"]["cache_size_mb"] = db["cache_size"].as<int>(512);
            config_json["database"]["write_buffer_size_mb"] = db["write_buffer_size"].as<int>(128);
            config_json["database"]["use_bloom_filter"] = db["use_bloom_filter"].as<bool>(true);
            config_json["database"]["compression_enabled"] = db["compression_enabled"].as<bool>(true);
        }

        if (config_yaml["consensus"])
        {
            auto consensus = config_yaml["consensus"];
            config_json["consensus"]["enabled"] = consensus["enabled"].as<bool>(false);
            config_json["consensus"]["private_key"] = consensus["private_key"].as<std::string>("");
            config_json["consensus"]["block_time_ms"] = consensus["block_time"].as<int>(15) * 1000; // Convert seconds to ms
            config_json["consensus"]["view_timeout_ms"] = consensus["view_timeout"].as<int>(60) * 1000; // Convert seconds to ms
            config_json["consensus"]["max_transactions_per_block"] = consensus["max_transactions_per_block"].as<int>(512);
            config_json["consensus"]["max_block_size"] = consensus["max_block_size"].as<int>(262144);
        }

        if (config_yaml["logging"])
        {
            auto logging = config_yaml["logging"];
            config_json["logging"]["level"] = logging["level"].as<std::string>("info");
            config_json["logging"]["console_output"] = logging["console_output"].as<bool>(true);
            config_json["logging"]["file_output"] = logging["file_output"].as<bool>(true);
            config_json["logging"]["log_file_path"] = logging["log_file_path"].as<std::string>("./logs/neo.log");
            config_json["logging"]["max_file_size_mb"] = logging["max_file_size"].as<int>(100);
            config_json["logging"]["max_files"] = logging["max_files"].as<int>(10);
            config_json["logging"]["async_logging"] = logging["async_logging"].as<bool>(true);
        }

        if (config_yaml["monitoring"])
        {
            auto monitoring = config_yaml["monitoring"];
            config_json["monitoring"]["metrics_enabled"] = monitoring["metrics_enabled"].as<bool>(true);
            config_json["monitoring"]["metrics_port"] = monitoring["metrics_port"].as<int>(9090);
            config_json["monitoring"]["health_checks_enabled"] = monitoring["health_checks_enabled"].as<bool>(true);
            config_json["monitoring"]["health_check_port"] = monitoring["health_check_port"].as<int>(8080);
            config_json["monitoring"]["health_check_interval_seconds"] = monitoring["health_check_interval"].as<int>(30);
        }

        if (config_yaml["performance"])
        {
            auto perf = config_yaml["performance"];
            config_json["performance"]["worker_threads"] = perf["worker_threads"].as<int>(0);
            config_json["performance"]["tx_pool_size"] = perf["tx_pool_size"].as<int>(50000);
            config_json["performance"]["max_memory_gb"] = perf["max_memory_gb"].as<int>(8);
            config_json["performance"]["enable_memory_pooling"] = perf["enable_memory_pooling"].as<bool>(true);
            
            if (perf["cache"])
            {
                auto cache = perf["cache"];
                config_json["performance"]["block_cache_size"] = cache["block_cache_size"].as<int>(1000);
                config_json["performance"]["transaction_cache_size"] = cache["transaction_cache_size"].as<int>(10000);
                config_json["performance"]["contract_cache_size"] = cache["contract_cache_size"].as<int>(100);
            }
        }

        if (config_yaml["security"])
        {
            auto security = config_yaml["security"];
            config_json["security"]["enable_tls"] = security["enable_tls"].as<bool>(false);
            config_json["security"]["tls_cert_file"] = security["tls_cert_file"].as<std::string>("");
            config_json["security"]["tls_key_file"] = security["tls_key_file"].as<std::string>("");
            config_json["security"]["enable_rate_limiting"] = security["enable_rate_limiting"].as<bool>(true);
            config_json["security"]["rate_limit_rps"] = security["rate_limit_rps"].as<int>(100);
            config_json["security"]["ban_duration_seconds"] = security["ban_duration"].as<int>(3600);
        }

        if (config_yaml["backup"])
        {
            auto backup = config_yaml["backup"];
            config_json["backup"]["enabled"] = backup["enabled"].as<bool>(false);
            config_json["backup"]["interval_hours"] = backup["interval_hours"].as<int>(24);
            config_json["backup"]["path"] = backup["path"].as<std::string>("./backups");
            config_json["backup"]["max_backups"] = backup["max_backups"].as<int>(7);
        }

        if (config_yaml["advanced"])
        {
            auto advanced = config_yaml["advanced"];
            config_json["advanced"]["experimental_features"] = advanced["experimental_features"].as<bool>(false);
        }

        config_file_path_ = config_file_path;
        loaded_from_file_ = true;

        // Now load from the converted JSON
        return LoadFromJson(config_json.dump());
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to load YAML configuration from {}: {}", config_file_path, e.what());
        return false;
    }
#else
    LOG_ERROR("YAML support not compiled in. Please use JSON configuration files.");
    return false;
#endif
}

bool ConfigurationManager::LoadFromJson(const std::string& json_content)
{
    try 
    {
        nlohmann::json config_json = nlohmann::json::parse(json_content);

        // Load each configuration section
        if (config_json.contains("network"))
            LoadNetworkConfig(config_json["network"]);
        if (config_json.contains("rpc"))
            LoadRpcConfig(config_json["rpc"]);
        if (config_json.contains("database"))
            LoadDatabaseConfig(config_json["database"]);
        if (config_json.contains("consensus"))
            LoadConsensusConfig(config_json["consensus"]);
        if (config_json.contains("logging"))
            LoadLoggingConfig(config_json["logging"]);
        if (config_json.contains("monitoring"))
            LoadMonitoringConfig(config_json["monitoring"]);
        if (config_json.contains("performance"))
            LoadPerformanceConfig(config_json["performance"]);
        if (config_json.contains("security"))
            LoadSecurityConfig(config_json["security"]);
        if (config_json.contains("backup"))
            LoadBackupConfig(config_json["backup"]);
        if (config_json.contains("advanced"))
            LoadAdvancedConfig(config_json["advanced"]);

        // Validate loaded configuration
        Validate();

        LOG_INFO("Configuration loaded successfully from JSON content");
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to load configuration from JSON: {}", e.what());
        return false;
    }
}

bool ConfigurationManager::LoadFromEnvironment()
{
    try 
    {
        LOG_INFO("Loading configuration from environment variables...");

        // Network configuration
        network_config_.bind_address = GetEnvironmentVariable("NEO_NETWORK_BIND_ADDRESS", network_config_.bind_address);
        network_config_.p2p_port = static_cast<uint16_t>(GetEnvironmentVariableInt("NEO_NETWORK_P2P_PORT", network_config_.p2p_port));
        network_config_.max_connections = static_cast<uint16_t>(GetEnvironmentVariableInt("NEO_NETWORK_MAX_CONNECTIONS", network_config_.max_connections));
        network_config_.min_connections = static_cast<uint16_t>(GetEnvironmentVariableInt("NEO_NETWORK_MIN_CONNECTIONS", network_config_.min_connections));
        network_config_.connection_timeout_seconds = static_cast<uint32_t>(GetEnvironmentVariableInt("NEO_NETWORK_CONNECTION_TIMEOUT", network_config_.connection_timeout_seconds));
        network_config_.enable_upnp = GetEnvironmentVariableBool("NEO_NETWORK_ENABLE_UPNP", network_config_.enable_upnp);

        // RPC configuration
        rpc_config_.enabled = GetEnvironmentVariableBool("NEO_RPC_ENABLED", rpc_config_.enabled);
        rpc_config_.bind_address = GetEnvironmentVariable("NEO_RPC_BIND_ADDRESS", rpc_config_.bind_address);
        rpc_config_.port = static_cast<uint16_t>(GetEnvironmentVariableInt("NEO_RPC_PORT", rpc_config_.port));
        rpc_config_.max_concurrent_requests = static_cast<uint32_t>(GetEnvironmentVariableInt("NEO_RPC_MAX_CONCURRENT_REQUESTS", rpc_config_.max_concurrent_requests));
        rpc_config_.enable_authentication = GetEnvironmentVariableBool("NEO_RPC_ENABLE_AUTH", rpc_config_.enable_authentication);
        rpc_config_.username = GetEnvironmentVariable("NEO_RPC_USERNAME", rpc_config_.username);
        rpc_config_.password = GetEnvironmentVariable("NEO_RPC_PASSWORD", rpc_config_.password);

        // Database configuration
        database_config_.backend = GetEnvironmentVariable("NEO_DB_BACKEND", database_config_.backend);
        database_config_.path = GetEnvironmentVariable("NEO_DB_PATH", database_config_.path);
        database_config_.cache_size_mb = static_cast<uint32_t>(GetEnvironmentVariableInt("NEO_DB_CACHE_SIZE_MB", database_config_.cache_size_mb));

        // Consensus configuration
        consensus_config_.enabled = GetEnvironmentVariableBool("NEO_CONSENSUS_ENABLED", consensus_config_.enabled);
        consensus_config_.wallet_path = GetEnvironmentVariable("NEO_CONSENSUS_WALLET_PATH", consensus_config_.wallet_path);
        consensus_config_.wallet_password = GetEnvironmentVariable("NEO_CONSENSUS_WALLET_PASSWORD", consensus_config_.wallet_password);

        // Logging configuration
        logging_config_.level = GetEnvironmentVariable("NEO_LOG_LEVEL", logging_config_.level);
        logging_config_.console_output = GetEnvironmentVariableBool("NEO_LOG_CONSOLE", logging_config_.console_output);
        logging_config_.file_output = GetEnvironmentVariableBool("NEO_LOG_FILE", logging_config_.file_output);
        logging_config_.log_file_path = GetEnvironmentVariable("NEO_LOG_FILE_PATH", logging_config_.log_file_path);

        // Monitoring configuration
        monitoring_config_.metrics_enabled = GetEnvironmentVariableBool("NEO_METRICS_ENABLED", monitoring_config_.metrics_enabled);
        monitoring_config_.metrics_port = static_cast<uint16_t>(GetEnvironmentVariableInt("NEO_METRICS_PORT", monitoring_config_.metrics_port));
        monitoring_config_.health_checks_enabled = GetEnvironmentVariableBool("NEO_HEALTH_CHECKS_ENABLED", monitoring_config_.health_checks_enabled);
        monitoring_config_.health_check_port = static_cast<uint16_t>(GetEnvironmentVariableInt("NEO_HEALTH_CHECK_PORT", monitoring_config_.health_check_port));

        // Security configuration
        security_config_.enable_rate_limiting = GetEnvironmentVariableBool("NEO_SECURITY_RATE_LIMITING", security_config_.enable_rate_limiting);
        security_config_.rate_limit_rps = static_cast<uint32_t>(GetEnvironmentVariableInt("NEO_SECURITY_RATE_LIMIT_RPS", security_config_.rate_limit_rps));

        // Validate loaded configuration
        Validate();

        LOG_INFO("Configuration loaded successfully from environment variables");
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to load configuration from environment: {}", e.what());
        return false;
    }
}

void ConfigurationManager::Validate() const
{
    ValidateNetworkConfig();
    ValidateRpcConfig();
    ValidateDatabaseConfig();
    ValidateConsensusConfig();
    ValidateLoggingConfig();
    ValidateMonitoringConfig();
    ValidatePerformanceConfig();
    ValidateSecurityConfig();
    ValidateBackupConfig();
}

bool ConfigurationManager::SaveToFile(const std::string& config_file_path) const
{
    try 
    {
        nlohmann::json config_json;
        
        config_json["network"] = NetworkConfigToJson();
        config_json["rpc"] = RpcConfigToJson();
        config_json["database"] = DatabaseConfigToJson();
        config_json["consensus"] = ConsensusConfigToJson();
        config_json["logging"] = LoggingConfigToJson();
        config_json["monitoring"] = MonitoringConfigToJson();
        config_json["performance"] = PerformanceConfigToJson();
        config_json["security"] = SecurityConfigToJson();
        config_json["backup"] = BackupConfigToJson();
        config_json["advanced"] = AdvancedConfigToJson();

        // Create directory if it doesn't exist
        std::filesystem::create_directories(std::filesystem::path(config_file_path).parent_path());

        std::ofstream file(config_file_path);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open configuration file for writing: {}", config_file_path);
            return false;
        }

        file << config_json.dump(2);
        file.close();

        LOG_INFO("Configuration saved successfully to: {}", config_file_path);
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to save configuration to {}: {}", config_file_path, e.what());
        return false;
    }
}

ConfigurationManager& ConfigurationManager::GetInstance()
{
    static ConfigurationManager instance;
    return instance;
}

// Private helper method implementations
void ConfigurationManager::LoadNetworkConfig(const nlohmann::json& json)
{
    if (json.contains("bind_address"))
        network_config_.bind_address = json["bind_address"].get<std::string>();
    if (json.contains("p2p_port"))
        network_config_.p2p_port = json["p2p_port"].get<uint16_t>();
    if (json.contains("max_connections"))
        network_config_.max_connections = json["max_connections"].get<uint16_t>();
    if (json.contains("min_connections"))
        network_config_.min_connections = json["min_connections"].get<uint16_t>();
    if (json.contains("connection_timeout_seconds"))
        network_config_.connection_timeout_seconds = json["connection_timeout_seconds"].get<uint32_t>();
    if (json.contains("enable_upnp"))
        network_config_.enable_upnp = json["enable_upnp"].get<bool>();
    if (json.contains("seed_nodes"))
        network_config_.seed_nodes = json["seed_nodes"].get<std::vector<std::string>>();
    if (json.contains("user_agent"))
        network_config_.user_agent = json["user_agent"].get<std::string>();
    if (json.contains("network_magic"))
        network_config_.network_magic = json["network_magic"].get<uint32_t>();
}

void ConfigurationManager::LoadRpcConfig(const nlohmann::json& json)
{
    if (json.contains("enabled"))
        rpc_config_.enabled = json["enabled"].get<bool>();
    if (json.contains("bind_address"))
        rpc_config_.bind_address = json["bind_address"].get<std::string>();
    if (json.contains("port"))
        rpc_config_.port = json["port"].get<uint16_t>();
    if (json.contains("ssl_port"))
        rpc_config_.ssl_port = json["ssl_port"].get<uint16_t>();
    if (json.contains("max_concurrent_requests"))
        rpc_config_.max_concurrent_requests = json["max_concurrent_requests"].get<uint32_t>();
    if (json.contains("max_request_size"))
        rpc_config_.max_request_size = json["max_request_size"].get<uint32_t>();
    if (json.contains("request_timeout_seconds"))
        rpc_config_.request_timeout_seconds = json["request_timeout_seconds"].get<uint32_t>();
    if (json.contains("enable_cors"))
        rpc_config_.enable_cors = json["enable_cors"].get<bool>();
    if (json.contains("allowed_origins"))
        rpc_config_.allowed_origins = json["allowed_origins"].get<std::vector<std::string>>();
    if (json.contains("enable_authentication"))
        rpc_config_.enable_authentication = json["enable_authentication"].get<bool>();
    if (json.contains("username"))
        rpc_config_.username = json["username"].get<std::string>();
    if (json.contains("password"))
        rpc_config_.password = json["password"].get<std::string>();
    if (json.contains("ssl_cert_file"))
        rpc_config_.ssl_cert_file = json["ssl_cert_file"].get<std::string>();
    if (json.contains("ssl_cert_password"))
        rpc_config_.ssl_cert_password = json["ssl_cert_password"].get<std::string>();
    if (json.contains("trusted_authorities"))
        rpc_config_.trusted_authorities = json["trusted_authorities"].get<std::vector<std::string>>();
    if (json.contains("disabled_methods"))
        rpc_config_.disabled_methods = json["disabled_methods"].get<std::vector<std::string>>();
    if (json.contains("session_enabled"))
        rpc_config_.session_enabled = json["session_enabled"].get<bool>();
    if (json.contains("session_expiration_seconds"))
        rpc_config_.session_expiration_seconds = json["session_expiration_seconds"].get<uint32_t>();
    if (json.contains("max_gas_invoke"))
        rpc_config_.max_gas_invoke = json["max_gas_invoke"].get<uint32_t>();
    if (json.contains("max_fee"))
        rpc_config_.max_fee = json["max_fee"].get<uint64_t>();
    if (json.contains("max_iterator_result_items"))
        rpc_config_.max_iterator_result_items = json["max_iterator_result_items"].get<uint32_t>();
    if (json.contains("max_stack_size"))
        rpc_config_.max_stack_size = json["max_stack_size"].get<uint32_t>();
}

void ConfigurationManager::LoadDatabaseConfig(const nlohmann::json& json)
{
    if (json.contains("backend"))
        database_config_.backend = json["backend"].get<std::string>();
    if (json.contains("path"))
        database_config_.path = json["path"].get<std::string>();
    if (json.contains("cache_size_mb"))
        database_config_.cache_size_mb = json["cache_size_mb"].get<uint32_t>();
    if (json.contains("write_buffer_size_mb"))
        database_config_.write_buffer_size_mb = json["write_buffer_size_mb"].get<uint32_t>();
    if (json.contains("use_bloom_filter"))
        database_config_.use_bloom_filter = json["use_bloom_filter"].get<bool>();
    if (json.contains("compression_enabled"))
        database_config_.compression_enabled = json["compression_enabled"].get<bool>();
    if (json.contains("read_only"))
        database_config_.read_only = json["read_only"].get<bool>();
}

void ConfigurationManager::LoadConsensusConfig(const nlohmann::json& json)
{
    if (json.contains("enabled"))
        consensus_config_.enabled = json["enabled"].get<bool>();
    if (json.contains("wallet_path"))
        consensus_config_.wallet_path = json["wallet_path"].get<std::string>();
    if (json.contains("wallet_password"))
        consensus_config_.wallet_password = json["wallet_password"].get<std::string>();
    if (json.contains("private_key"))
        consensus_config_.private_key = json["private_key"].get<std::string>();
    if (json.contains("block_time_ms"))
        consensus_config_.block_time_ms = json["block_time_ms"].get<uint32_t>();
    if (json.contains("view_timeout_ms"))
        consensus_config_.view_timeout_ms = json["view_timeout_ms"].get<uint32_t>();
    if (json.contains("max_transactions_per_block"))
        consensus_config_.max_transactions_per_block = json["max_transactions_per_block"].get<uint32_t>();
    if (json.contains("max_block_size"))
        consensus_config_.max_block_size = json["max_block_size"].get<uint32_t>();
    if (json.contains("max_block_system_fee"))
        consensus_config_.max_block_system_fee = json["max_block_system_fee"].get<uint64_t>();
    if (json.contains("auto_start"))
        consensus_config_.auto_start = json["auto_start"].get<bool>();
}

void ConfigurationManager::LoadLoggingConfig(const nlohmann::json& json)
{
    if (json.contains("level"))
        logging_config_.level = json["level"].get<std::string>();
    if (json.contains("console_output"))
        logging_config_.console_output = json["console_output"].get<bool>();
    if (json.contains("file_output"))
        logging_config_.file_output = json["file_output"].get<bool>();
    if (json.contains("log_file_path"))
        logging_config_.log_file_path = json["log_file_path"].get<std::string>();
    if (json.contains("max_file_size_mb"))
        logging_config_.max_file_size_mb = json["max_file_size_mb"].get<uint32_t>();
    if (json.contains("max_files"))
        logging_config_.max_files = json["max_files"].get<uint32_t>();
    if (json.contains("async_logging"))
        logging_config_.async_logging = json["async_logging"].get<bool>();
    if (json.contains("enable_file_rotation"))
        logging_config_.enable_file_rotation = json["enable_file_rotation"].get<bool>();
}

void ConfigurationManager::LoadMonitoringConfig(const nlohmann::json& json)
{
    if (json.contains("metrics_enabled"))
        monitoring_config_.metrics_enabled = json["metrics_enabled"].get<bool>();
    if (json.contains("metrics_port"))
        monitoring_config_.metrics_port = json["metrics_port"].get<uint16_t>();
    if (json.contains("metrics_bind_address"))
        monitoring_config_.metrics_bind_address = json["metrics_bind_address"].get<std::string>();
    if (json.contains("health_checks_enabled"))
        monitoring_config_.health_checks_enabled = json["health_checks_enabled"].get<bool>();
    if (json.contains("health_check_port"))
        monitoring_config_.health_check_port = json["health_check_port"].get<uint16_t>();
    if (json.contains("health_check_bind_address"))
        monitoring_config_.health_check_bind_address = json["health_check_bind_address"].get<std::string>();
    if (json.contains("health_check_interval_seconds"))
        monitoring_config_.health_check_interval_seconds = json["health_check_interval_seconds"].get<uint32_t>();
    if (json.contains("enable_performance_counters"))
        monitoring_config_.enable_performance_counters = json["enable_performance_counters"].get<bool>();
}

void ConfigurationManager::LoadPerformanceConfig(const nlohmann::json& json)
{
    if (json.contains("worker_threads"))
        performance_config_.worker_threads = json["worker_threads"].get<uint32_t>();
    if (json.contains("tx_pool_size"))
        performance_config_.tx_pool_size = json["tx_pool_size"].get<uint32_t>();
    if (json.contains("max_memory_gb"))
        performance_config_.max_memory_gb = json["max_memory_gb"].get<uint32_t>();
    if (json.contains("enable_memory_pooling"))
        performance_config_.enable_memory_pooling = json["enable_memory_pooling"].get<bool>();
    if (json.contains("block_cache_size"))
        performance_config_.block_cache_size = json["block_cache_size"].get<uint32_t>();
    if (json.contains("transaction_cache_size"))
        performance_config_.transaction_cache_size = json["transaction_cache_size"].get<uint32_t>();
    if (json.contains("contract_cache_size"))
        performance_config_.contract_cache_size = json["contract_cache_size"].get<uint32_t>();
    if (json.contains("max_concurrent_transactions"))
        performance_config_.max_concurrent_transactions = json["max_concurrent_transactions"].get<uint32_t>();
    if (json.contains("max_concurrent_blocks"))
        performance_config_.max_concurrent_blocks = json["max_concurrent_blocks"].get<uint32_t>();
}

void ConfigurationManager::LoadSecurityConfig(const nlohmann::json& json)
{
    if (json.contains("enable_tls"))
        security_config_.enable_tls = json["enable_tls"].get<bool>();
    if (json.contains("tls_cert_file"))
        security_config_.tls_cert_file = json["tls_cert_file"].get<std::string>();
    if (json.contains("tls_key_file"))
        security_config_.tls_key_file = json["tls_key_file"].get<std::string>();
    if (json.contains("enable_rate_limiting"))
        security_config_.enable_rate_limiting = json["enable_rate_limiting"].get<bool>();
    if (json.contains("rate_limit_rps"))
        security_config_.rate_limit_rps = json["rate_limit_rps"].get<uint32_t>();
    if (json.contains("ban_duration_seconds"))
        security_config_.ban_duration_seconds = json["ban_duration_seconds"].get<uint32_t>();
    if (json.contains("enable_whitelist"))
        security_config_.enable_whitelist = json["enable_whitelist"].get<bool>();
    if (json.contains("whitelisted_addresses"))
        security_config_.whitelisted_addresses = json["whitelisted_addresses"].get<std::vector<std::string>>();
    if (json.contains("enable_blacklist"))
        security_config_.enable_blacklist = json["enable_blacklist"].get<bool>();
    if (json.contains("blacklisted_addresses"))
        security_config_.blacklisted_addresses = json["blacklisted_addresses"].get<std::vector<std::string>>();
    if (json.contains("max_requests_per_second"))
        security_config_.max_requests_per_second = json["max_requests_per_second"].get<uint32_t>();
}

void ConfigurationManager::LoadBackupConfig(const nlohmann::json& json)
{
    if (json.contains("enabled"))
        backup_config_.enabled = json["enabled"].get<bool>();
    if (json.contains("interval_hours"))
        backup_config_.interval_hours = json["interval_hours"].get<uint32_t>();
    if (json.contains("path"))
        backup_config_.path = json["path"].get<std::string>();
    if (json.contains("max_backups"))
        backup_config_.max_backups = json["max_backups"].get<uint32_t>();
    if (json.contains("compress_backups"))
        backup_config_.compress_backups = json["compress_backups"].get<bool>();
}

void ConfigurationManager::LoadAdvancedConfig(const nlohmann::json& json)
{
    if (json.contains("experimental_features"))
        advanced_config_.experimental_features = json["experimental_features"].get<bool>();
    if (json.contains("protocol_settings"))
        advanced_config_.protocol_settings = json["protocol_settings"].get<std::unordered_map<std::string, nlohmann::json>>();
    if (json.contains("plugin_settings"))
        advanced_config_.plugin_settings = json["plugin_settings"].get<std::unordered_map<std::string, nlohmann::json>>();
}

// Validation methods
void ConfigurationManager::ValidateNetworkConfig() const
{
    if (network_config_.p2p_port == 0 || network_config_.p2p_port > 65535)
        throw std::invalid_argument("Invalid P2P port: " + std::to_string(network_config_.p2p_port));
    
    if (network_config_.max_connections < network_config_.min_connections)
        throw std::invalid_argument("Max connections must be >= min connections");
    
    if (network_config_.connection_timeout_seconds < 5 || network_config_.connection_timeout_seconds > 300)
        throw std::invalid_argument("Connection timeout must be between 5 and 300 seconds");
}

void ConfigurationManager::ValidateRpcConfig() const
{
    if (rpc_config_.enabled)
    {
        if (rpc_config_.port == 0 || rpc_config_.port > 65535)
            throw std::invalid_argument("Invalid RPC port: " + std::to_string(rpc_config_.port));
        
        if (rpc_config_.ssl_port == 0 || rpc_config_.ssl_port > 65535)
            throw std::invalid_argument("Invalid RPC SSL port: " + std::to_string(rpc_config_.ssl_port));
        
        if (rpc_config_.max_concurrent_requests == 0)
            throw std::invalid_argument("Max concurrent requests must be > 0");
        
        if (rpc_config_.max_request_size < 1024) // 1KB minimum
            throw std::invalid_argument("Max request size too small (minimum 1KB)");
        
        if (rpc_config_.enable_authentication && (rpc_config_.username.empty() || rpc_config_.password.empty()))
            throw std::invalid_argument("Username and password required when authentication is enabled");
    }
}

void ConfigurationManager::ValidateDatabaseConfig() const
{
    if (database_config_.backend != "leveldb" && database_config_.backend != "rocksdb")
        throw std::invalid_argument("Invalid database backend: " + database_config_.backend);
    
    if (database_config_.path.empty())
        throw std::invalid_argument("Database path cannot be empty");
    
    if (database_config_.cache_size_mb < 16) // 16MB minimum
        throw std::invalid_argument("Database cache size too small (minimum 16MB)");
}

void ConfigurationManager::ValidateConsensusConfig() const
{
    if (consensus_config_.enabled)
    {
        if (consensus_config_.block_time_ms < 1000) // 1 second minimum
            throw std::invalid_argument("Block time too small (minimum 1 second)");
        
        if (consensus_config_.max_transactions_per_block == 0)
            throw std::invalid_argument("Max transactions per block must be > 0");
        
        if (consensus_config_.max_block_size < 1024) // 1KB minimum
            throw std::invalid_argument("Max block size too small (minimum 1KB)");
    }
}

void ConfigurationManager::ValidateLoggingConfig() const
{
    if (logging_config_.level != "trace" && logging_config_.level != "debug" && 
        logging_config_.level != "info" && logging_config_.level != "warning" && 
        logging_config_.level != "error" && logging_config_.level != "critical")
        throw std::invalid_argument("Invalid log level: " + logging_config_.level);
    
    if (logging_config_.file_output && logging_config_.log_file_path.empty())
        throw std::invalid_argument("Log file path cannot be empty when file output is enabled");
    
    if (logging_config_.max_file_size_mb < 1)
        throw std::invalid_argument("Max log file size must be >= 1MB");
}

void ConfigurationManager::ValidateMonitoringConfig() const
{
    if (monitoring_config_.metrics_enabled)
    {
        if (monitoring_config_.metrics_port == 0 || monitoring_config_.metrics_port > 65535)
            throw std::invalid_argument("Invalid metrics port: " + std::to_string(monitoring_config_.metrics_port));
    }
    
    if (monitoring_config_.health_checks_enabled)
    {
        if (monitoring_config_.health_check_port == 0 || monitoring_config_.health_check_port > 65535)
            throw std::invalid_argument("Invalid health check port: " + std::to_string(monitoring_config_.health_check_port));
        
        if (monitoring_config_.health_check_interval_seconds < 5)
            throw std::invalid_argument("Health check interval too small (minimum 5 seconds)");
    }
}

void ConfigurationManager::ValidatePerformanceConfig() const
{
    if (performance_config_.tx_pool_size < 1000)
        throw std::invalid_argument("Transaction pool size too small (minimum 1000)");
    
    if (performance_config_.max_memory_gb < 1)
        throw std::invalid_argument("Max memory must be >= 1GB");
    
    if (performance_config_.block_cache_size == 0)
        throw std::invalid_argument("Block cache size must be > 0");
}

void ConfigurationManager::ValidateSecurityConfig() const
{
    if (security_config_.enable_tls)
    {
        if (security_config_.tls_cert_file.empty())
            throw std::invalid_argument("TLS certificate file path required when TLS is enabled");
        
        if (security_config_.tls_key_file.empty())
            throw std::invalid_argument("TLS key file path required when TLS is enabled");
    }
    
    if (security_config_.rate_limit_rps == 0)
        throw std::invalid_argument("Rate limit RPS must be > 0");
}

void ConfigurationManager::ValidateBackupConfig() const
{
    if (backup_config_.enabled)
    {
        if (backup_config_.interval_hours == 0)
            throw std::invalid_argument("Backup interval must be > 0 hours");
        
        if (backup_config_.path.empty())
            throw std::invalid_argument("Backup path cannot be empty when backups are enabled");
        
        if (backup_config_.max_backups == 0)
            throw std::invalid_argument("Max backups must be > 0");
    }
}

// JSON serialization methods
nlohmann::json ConfigurationManager::NetworkConfigToJson() const
{
    nlohmann::json json;
    json["bind_address"] = network_config_.bind_address;
    json["p2p_port"] = network_config_.p2p_port;
    json["max_connections"] = network_config_.max_connections;
    json["min_connections"] = network_config_.min_connections;
    json["connection_timeout_seconds"] = network_config_.connection_timeout_seconds;
    json["enable_upnp"] = network_config_.enable_upnp;
    json["seed_nodes"] = network_config_.seed_nodes;
    json["user_agent"] = network_config_.user_agent;
    json["network_magic"] = network_config_.network_magic;
    return json;
}

nlohmann::json ConfigurationManager::RpcConfigToJson() const
{
    nlohmann::json json;
    json["enabled"] = rpc_config_.enabled;
    json["bind_address"] = rpc_config_.bind_address;
    json["port"] = rpc_config_.port;
    json["ssl_port"] = rpc_config_.ssl_port;
    json["max_concurrent_requests"] = rpc_config_.max_concurrent_requests;
    json["max_request_size"] = rpc_config_.max_request_size;
    json["request_timeout_seconds"] = rpc_config_.request_timeout_seconds;
    json["enable_cors"] = rpc_config_.enable_cors;
    json["allowed_origins"] = rpc_config_.allowed_origins;
    json["enable_authentication"] = rpc_config_.enable_authentication;
    json["username"] = rpc_config_.username;
    json["password"] = rpc_config_.password; // In production, this should be encrypted or excluded
    json["ssl_cert_file"] = rpc_config_.ssl_cert_file;
    json["ssl_cert_password"] = rpc_config_.ssl_cert_password; // In production, this should be encrypted or excluded
    json["trusted_authorities"] = rpc_config_.trusted_authorities;
    json["disabled_methods"] = rpc_config_.disabled_methods;
    json["session_enabled"] = rpc_config_.session_enabled;
    json["session_expiration_seconds"] = rpc_config_.session_expiration_seconds;
    json["max_gas_invoke"] = rpc_config_.max_gas_invoke;
    json["max_fee"] = rpc_config_.max_fee;
    json["max_iterator_result_items"] = rpc_config_.max_iterator_result_items;
    json["max_stack_size"] = rpc_config_.max_stack_size;
    return json;
}

nlohmann::json ConfigurationManager::DatabaseConfigToJson() const
{
    nlohmann::json json;
    json["backend"] = database_config_.backend;
    json["path"] = database_config_.path;
    json["cache_size_mb"] = database_config_.cache_size_mb;
    json["write_buffer_size_mb"] = database_config_.write_buffer_size_mb;
    json["use_bloom_filter"] = database_config_.use_bloom_filter;
    json["compression_enabled"] = database_config_.compression_enabled;
    json["read_only"] = database_config_.read_only;
    return json;
}

nlohmann::json ConfigurationManager::ConsensusConfigToJson() const
{
    nlohmann::json json;
    json["enabled"] = consensus_config_.enabled;
    json["wallet_path"] = consensus_config_.wallet_path;
    json["wallet_password"] = consensus_config_.wallet_password; // In production, this should be encrypted or excluded
    json["private_key"] = consensus_config_.private_key; // In production, this should be encrypted or excluded
    json["block_time_ms"] = consensus_config_.block_time_ms;
    json["view_timeout_ms"] = consensus_config_.view_timeout_ms;
    json["max_transactions_per_block"] = consensus_config_.max_transactions_per_block;
    json["max_block_size"] = consensus_config_.max_block_size;
    json["max_block_system_fee"] = consensus_config_.max_block_system_fee;
    json["auto_start"] = consensus_config_.auto_start;
    return json;
}

nlohmann::json ConfigurationManager::LoggingConfigToJson() const
{
    nlohmann::json json;
    json["level"] = logging_config_.level;
    json["console_output"] = logging_config_.console_output;
    json["file_output"] = logging_config_.file_output;
    json["log_file_path"] = logging_config_.log_file_path;
    json["max_file_size_mb"] = logging_config_.max_file_size_mb;
    json["max_files"] = logging_config_.max_files;
    json["async_logging"] = logging_config_.async_logging;
    json["enable_file_rotation"] = logging_config_.enable_file_rotation;
    return json;
}

nlohmann::json ConfigurationManager::MonitoringConfigToJson() const
{
    nlohmann::json json;
    json["metrics_enabled"] = monitoring_config_.metrics_enabled;
    json["metrics_port"] = monitoring_config_.metrics_port;
    json["metrics_bind_address"] = monitoring_config_.metrics_bind_address;
    json["health_checks_enabled"] = monitoring_config_.health_checks_enabled;
    json["health_check_port"] = monitoring_config_.health_check_port;
    json["health_check_bind_address"] = monitoring_config_.health_check_bind_address;
    json["health_check_interval_seconds"] = monitoring_config_.health_check_interval_seconds;
    json["enable_performance_counters"] = monitoring_config_.enable_performance_counters;
    return json;
}

nlohmann::json ConfigurationManager::PerformanceConfigToJson() const
{
    nlohmann::json json;
    json["worker_threads"] = performance_config_.worker_threads;
    json["tx_pool_size"] = performance_config_.tx_pool_size;
    json["max_memory_gb"] = performance_config_.max_memory_gb;
    json["enable_memory_pooling"] = performance_config_.enable_memory_pooling;
    json["block_cache_size"] = performance_config_.block_cache_size;
    json["transaction_cache_size"] = performance_config_.transaction_cache_size;
    json["contract_cache_size"] = performance_config_.contract_cache_size;
    json["max_concurrent_transactions"] = performance_config_.max_concurrent_transactions;
    json["max_concurrent_blocks"] = performance_config_.max_concurrent_blocks;
    return json;
}

nlohmann::json ConfigurationManager::SecurityConfigToJson() const
{
    nlohmann::json json;
    json["enable_tls"] = security_config_.enable_tls;
    json["tls_cert_file"] = security_config_.tls_cert_file;
    json["tls_key_file"] = security_config_.tls_key_file;
    json["enable_rate_limiting"] = security_config_.enable_rate_limiting;
    json["rate_limit_rps"] = security_config_.rate_limit_rps;
    json["ban_duration_seconds"] = security_config_.ban_duration_seconds;
    json["enable_whitelist"] = security_config_.enable_whitelist;
    json["whitelisted_addresses"] = security_config_.whitelisted_addresses;
    json["enable_blacklist"] = security_config_.enable_blacklist;
    json["blacklisted_addresses"] = security_config_.blacklisted_addresses;
    json["max_requests_per_second"] = security_config_.max_requests_per_second;
    return json;
}

nlohmann::json ConfigurationManager::BackupConfigToJson() const
{
    nlohmann::json json;
    json["enabled"] = backup_config_.enabled;
    json["interval_hours"] = backup_config_.interval_hours;
    json["path"] = backup_config_.path;
    json["max_backups"] = backup_config_.max_backups;
    json["compress_backups"] = backup_config_.compress_backups;
    return json;
}

nlohmann::json ConfigurationManager::AdvancedConfigToJson() const
{
    nlohmann::json json;
    json["experimental_features"] = advanced_config_.experimental_features;
    json["protocol_settings"] = advanced_config_.protocol_settings;
    json["plugin_settings"] = advanced_config_.plugin_settings;
    return json;
}

// Environment variable helpers
std::string ConfigurationManager::GetEnvironmentVariable(const std::string& var_name, const std::string& default_value) const
{
    const char* value = std::getenv(var_name.c_str());
    return value ? std::string(value) : default_value;
}

int ConfigurationManager::GetEnvironmentVariableInt(const std::string& var_name, int default_value) const
{
    const char* value = std::getenv(var_name.c_str());
    if (value)
    {
        try 
        {
            return std::stoi(value);
        }
        catch (const std::exception&)
        {
            LOG_WARNING("Invalid integer value for environment variable {}: {}", var_name, value);
        }
    }
    return default_value;
}

bool ConfigurationManager::GetEnvironmentVariableBool(const std::string& var_name, bool default_value) const
{
    const char* value = std::getenv(var_name.c_str());
    if (value)
    {
        std::string val_str(value);
        std::transform(val_str.begin(), val_str.end(), val_str.begin(), ::tolower);
        return (val_str == "true" || val_str == "1" || val_str == "yes" || val_str == "on");
    }
    return default_value;
}

} // namespace core
} // namespace neo