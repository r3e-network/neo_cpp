/**
 * @file protocol_settings.h
 * @brief Configuration settings
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo::core
{

/**
 * @brief Hardfork configuration
 */
struct Hardfork
{
    std::string name;
    uint32_t block_height;
};

/**
 * @brief Native contract configuration
 */
struct NativeContract
{
    std::string name;
    io::UInt160 hash;
    int32_t id;
    std::string nef_file;
    std::string manifest;
};

/**
 * @brief Protocol settings for Neo N3 blockchain
 *
 * This class contains all protocol-level configuration matching
 * the C# ProtocolSettings implementation for full compatibility.
 */
class ProtocolSettings
{
   public:
    // Network magic values (C# compatible)
    static constexpr uint32_t MAINNET_MAGIC = 0x4F454E;    // N3 MainNet
    static constexpr uint32_t TESTNET_MAGIC = 0x3154334E;  // N3T TestNet
    static constexpr uint32_t PRIVNET_MAGIC = 0x01020304;  // Private net

   private:
    // Network settings
    uint32_t network_magic_;
    uint32_t address_version_;

    // Consensus settings
    std::vector<cryptography::ecc::ECPoint> standby_validators_;
    uint32_t validator_count_;
    std::chrono::milliseconds milliseconds_per_block_;
    uint32_t max_transactions_per_block_;
    uint32_t max_block_size_;
    uint64_t max_block_system_fee_;

    // Economic model
    uint64_t native_gas_factor_;
    uint64_t initial_gas_distribution_;
    std::unordered_map<io::UInt160, uint64_t> genesis_allocation_;

    // Transaction settings
    uint32_t memory_pool_max_transactions_;
    uint32_t max_trace_size_;
    uint32_t free_gas_limit_;
    uint64_t fee_per_byte_;
    uint32_t max_valid_until_block_increment_;

    // State settings
    uint32_t state_root_frequency_;
    uint32_t max_state_root_delay_blocks_;

    // Hardfork settings
    std::vector<Hardfork> hardforks_;

    // Native contracts
    std::vector<NativeContract> native_contracts_;

    // Seed nodes
    std::vector<std::string> seed_nodes_;

   public:
    /**
     * @brief Default constructor with MainNet settings
     */
    ProtocolSettings();

    /**
     * @brief Constructor with custom network magic
     * @param network_magic Network identifier
     */
    explicit ProtocolSettings(uint32_t network_magic);

    /**
     * @brief Load settings from configuration file
     * @param config_path Path to config file
     * @return true if loaded successfully
     */
    bool LoadFromFile(const std::string& config_path);

    /**
     * @brief Load settings from JSON string
     * @param json JSON configuration
     * @return true if loaded successfully
     */
    bool LoadFromJson(const std::string& json);

    // Network settings
    uint32_t GetMagic() const { return network_magic_; }
    uint32_t GetAddressVersion() const { return address_version_; }
    bool IsMainNet() const { return network_magic_ == MAINNET_MAGIC; }
    bool IsTestNet() const { return network_magic_ == TESTNET_MAGIC; }

    // Consensus settings
    const std::vector<cryptography::ecc::ECPoint>& GetStandbyValidators() const { return standby_validators_; }
    uint32_t GetValidatorCount() const { return validator_count_; }
    std::chrono::milliseconds GetMillisecondsPerBlock() const { return milliseconds_per_block_; }
    uint32_t GetMaxTransactionsPerBlock() const { return max_transactions_per_block_; }
    uint32_t GetMaxBlockSize() const { return max_block_size_; }
    uint64_t GetMaxBlockSystemFee() const { return max_block_system_fee_; }

    // Economic model
    uint64_t GetNativeGasFactor() const { return native_gas_factor_; }
    uint64_t GetInitialGasDistribution() const { return initial_gas_distribution_; }
    const std::unordered_map<io::UInt160, uint64_t>& GetGenesisAllocation() const { return genesis_allocation_; }

    // Transaction settings
    uint32_t GetMemoryPoolMaxTransactions() const { return memory_pool_max_transactions_; }
    uint32_t GetMaxTraceSize() const { return max_trace_size_; }
    uint32_t GetFreeGasLimit() const { return free_gas_limit_; }
    uint64_t GetFeePerByte() const { return fee_per_byte_; }
    uint32_t GetMaxValidUntilBlockIncrement() const { return max_valid_until_block_increment_; }

    // State settings
    uint32_t GetStateRootFrequency() const { return state_root_frequency_; }
    uint32_t GetMaxStateRootDelayBlocks() const { return max_state_root_delay_blocks_; }

    // Hardfork management
    const std::vector<Hardfork>& GetHardforks() const { return hardforks_; }
    bool IsHardforkEnabled(const std::string& name, uint32_t height) const;
    std::optional<uint32_t> GetHardforkHeight(const std::string& name) const;

    // Native contracts
    const std::vector<NativeContract>& GetNativeContracts() const { return native_contracts_; }
    std::optional<NativeContract> GetNativeContract(const std::string& name) const;
    std::optional<NativeContract> GetNativeContract(const io::UInt160& hash) const;

    // Seed nodes
    const std::vector<std::string>& GetSeedNodes() const { return seed_nodes_; }

    // Validation
    /**
     * @brief Validate a transaction against protocol rules
     * @param tx Transaction to validate
     * @param height Current block height
     * @return true if valid
     */
    bool ValidateTransaction(const void* tx, uint32_t height) const;

    /**
     * @brief Validate a block against protocol rules
     * @param block Block to validate
     * @return true if valid
     */
    bool ValidateBlock(const void* block) const;

    // Utility methods
    /**
     * @brief Get time per block
     * @return Time duration per block
     */
    std::chrono::seconds GetTimePerBlock() const
    {
        return std::chrono::duration_cast<std::chrono::seconds>(milliseconds_per_block_);
    }

    /**
     * @brief Calculate system fee for transaction size
     * @param size Transaction size in bytes
     * @return System fee
     */
    uint64_t CalculateSystemFee(uint32_t size) const;

    /**
     * @brief Check if a feature is enabled at height
     * @param feature Feature name
     * @param height Block height
     * @return true if enabled
     */
    bool IsFeatureEnabled(const std::string& feature, uint32_t height) const;

    // Static factory methods
    /**
     * @brief Create MainNet settings
     * @return MainNet protocol settings
     */
    static ProtocolSettings MainNet();

    /**
     * @brief Create TestNet settings
     * @return TestNet protocol settings
     */
    static ProtocolSettings TestNet();

    /**
     * @brief Create PrivNet settings
     * @return PrivNet protocol settings
     */
    static ProtocolSettings PrivNet();

    /**
     * @brief Create custom settings
     * @param config Configuration JSON
     * @return Custom protocol settings
     */
    static ProtocolSettings Custom(const std::string& config);

   private:
    // Helper methods
    void InitializeMainNet();
    void InitializeTestNet();
    void InitializePrivNet();
    void InitializeNativeContracts();
    void InitializeHardforks();
    void LoadStandbyValidators(const std::vector<std::string>& public_keys);
};

/**
 * @brief Global protocol settings instance (C# compatible)
 */
class ProtocolSettingsSingleton
{
   private:
    static std::shared_ptr<ProtocolSettings> instance_;
    static std::mutex mutex_;

   public:
    /**
     * @brief Initialize global settings
     * @param settings Protocol settings
     */
    static void Initialize(std::shared_ptr<ProtocolSettings> settings);

    /**
     * @brief Get global settings instance
     * @return Protocol settings
     */
    static std::shared_ptr<ProtocolSettings> GetInstance();

    /**
     * @brief Load settings from file
     * @param config_path Configuration file path
     * @return true if loaded successfully
     */
    static bool Load(const std::string& config_path);
};

}  // namespace neo::core
