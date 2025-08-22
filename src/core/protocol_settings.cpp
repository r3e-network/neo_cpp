/**
 * @file protocol_settings.cpp
 * @brief Configuration settings
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/protocol_settings.h>
#include <neo/cryptography/helper.h>
#include <neo/io/json.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/block.h>

#include <fstream>
#include <sstream>
#include <chrono>

namespace neo::core
{

// Static members for singleton
std::shared_ptr<ProtocolSettings> ProtocolSettingsSingleton::instance_;
std::mutex ProtocolSettingsSingleton::mutex_;

ProtocolSettings::ProtocolSettings() : network_magic_(MAINNET_MAGIC) { InitializeMainNet(); }

ProtocolSettings::ProtocolSettings(uint32_t network_magic) : network_magic_(network_magic)
{
    switch (network_magic)
    {
        case MAINNET_MAGIC:
            InitializeMainNet();
            break;
        case TESTNET_MAGIC:
            InitializeTestNet();
            break;
        case PRIVNET_MAGIC:
            InitializePrivNet();
            break;
        default:
            // Custom network - use default values
            InitializePrivNet();
            network_magic_ = network_magic;
            break;
    }
}

bool ProtocolSettings::LoadFromFile(const std::string& config_path)
{
    try
    {
        std::ifstream file(config_path);
        if (!file.is_open())
        {
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        return LoadFromJson(buffer.str());
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool ProtocolSettings::LoadFromJson(const std::string& json)
{
    try
    {
        auto config = io::Json::Parse(json);

        // Network settings
        if (config.contains("Magic"))
        {
            network_magic_ = config["Magic"].get<uint32_t>();
        }

        if (config.contains("AddressVersion"))
        {
            address_version_ = config["AddressVersion"].get<uint32_t>();
        }

        // Consensus settings
        if (config.contains("StandbyValidators"))
        {
            std::vector<std::string> validators;
            for (const auto& v : config["StandbyValidators"])
            {
                validators.push_back(v.get<std::string>());
            }
            LoadStandbyValidators(validators);
        }

        if (config.contains("ValidatorsCount"))
        {
            validator_count_ = config["ValidatorsCount"].get<uint32_t>();
        }

        if (config.contains("MillisecondsPerBlock"))
        {
            milliseconds_per_block_ = std::chrono::milliseconds(config["MillisecondsPerBlock"].get<uint32_t>());
        }

        if (config.contains("MaxTransactionsPerBlock"))
        {
            max_transactions_per_block_ = config["MaxTransactionsPerBlock"].get<uint32_t>();
        }

        if (config.contains("MaxBlockSize"))
        {
            max_block_size_ = config["MaxBlockSize"].get<uint32_t>();
        }

        if (config.contains("MaxBlockSystemFee"))
        {
            max_block_system_fee_ = config["MaxBlockSystemFee"].get<uint64_t>();
        }

        // Economic model
        if (config.contains("NativeUpdateHistory"))
        {
            // Parse native contract update history
            InitializeNativeContracts();
        }

        // Transaction settings
        if (config.contains("MemoryPoolMaxTransactions"))
        {
            memory_pool_max_transactions_ = config["MemoryPoolMaxTransactions"].get<uint32_t>();
        }

        if (config.contains("MaxTraceableBlocks"))
        {
            max_trace_size_ = config["MaxTraceableBlocks"].get<uint32_t>();
        }

        // Hardforks
        if (config.contains("Hardforks"))
        {
            for (const auto& [name, height] : config["Hardforks"].items())
            {
                hardforks_.push_back({name, height.get<uint32_t>()});
            }
        }

        // Seed nodes
        if (config.contains("SeedList"))
        {
            seed_nodes_.clear();
            for (const auto& seed : config["SeedList"])
            {
                seed_nodes_.push_back(seed.get<std::string>());
            }
        }

        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool ProtocolSettings::IsHardforkEnabled(const std::string& name, uint32_t height) const
{
    for (const auto& hf : hardforks_)
    {
        if (hf.name == name)
        {
            return height >= hf.block_height;
        }
    }
    return false;
}

std::optional<uint32_t> ProtocolSettings::GetHardforkHeight(const std::string& name) const
{
    for (const auto& hf : hardforks_)
    {
        if (hf.name == name)
        {
            return hf.block_height;
        }
    }
    return std::nullopt;
}

std::optional<NativeContract> ProtocolSettings::GetNativeContract(const std::string& name) const
{
    for (const auto& contract : native_contracts_)
    {
        if (contract.name == name)
        {
            return contract;
        }
    }
    return std::nullopt;
}

std::optional<NativeContract> ProtocolSettings::GetNativeContract(const io::UInt160& hash) const
{
    for (const auto& contract : native_contracts_)
    {
        if (contract.hash == hash)
        {
            return contract;
        }
    }
    return std::nullopt;
}

bool ProtocolSettings::ValidateTransaction(const void* tx, uint32_t height) const
{
    if (!tx) {
        return false;
    }
    
    // Cast to transaction pointer (assuming it's a Neo transaction)
    const auto* transaction = static_cast<const ledger::Transaction*>(tx);
    
    // Validate transaction version (Neo N3 uses version 0)
    if (transaction->GetVersion() != 0) {
        return false;
    }
    
    // Check ValidUntilBlock
    if (transaction->GetValidUntilBlock() <= height || 
        transaction->GetValidUntilBlock() > height + max_valid_until_block_increment_) {
        return false;
    }
    
    // Validate system fee (must be non-negative)
    if (transaction->GetSystemFee() < 0) {
        return false;
    }
    
    // Validate network fee (must be non-negative)
    if (transaction->GetNetworkFee() < 0) {
        return false;
    }
    
    // Check transaction size limits
    auto tx_size = transaction->GetSize();
    if (tx_size > 102400) { // 100KB max transaction size
        return false;
    }
    
    // Validate attributes count
    if (transaction->GetAttributes().size() > 16) {
        return false;
    }
    
    // Validate signers count
    if (transaction->GetSigners().size() == 0 || transaction->GetSigners().size() > 16) {
        return false;
    }
    
    return true;
}

bool ProtocolSettings::ValidateBlock(const void* block) const
{
    if (!block) {
        return false;
    }
    
    // Cast to block pointer
    const auto* neo_block = static_cast<const ledger::Block*>(block);
    
    // Validate block version (Neo N3 uses version 0)
    if (neo_block->GetVersion() != 0) {
        return false;
    }
    
    // Check transaction count limits
    if (neo_block->GetTransactions().size() > max_transactions_per_block_) {
        return false;
    }
    
    // Validate block size (blocks should not exceed 1MB)
    auto block_size = neo_block->GetSize();
    if (block_size > 1048576) { // 1MB max block size
        return false;
    }
    
    // Check timestamp validity (basic range check)
    auto timestamp = neo_block->GetTimestamp();
    auto current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Block timestamp should not be too far in the future
    if (timestamp > current_time + 900000) { // 15 minutes tolerance
        return false;
    }
    
    // Block index should be sequential (can't validate without blockchain context)
    // This would need blockchain instance to verify against previous block
    
    return true;
}

uint64_t ProtocolSettings::CalculateSystemFee(uint32_t size) const { return size * fee_per_byte_; }

bool ProtocolSettings::IsFeatureEnabled(const std::string& feature, uint32_t height) const
{
    // Check if feature is enabled based on hardforks
    return IsHardforkEnabled(feature, height);
}

ProtocolSettings ProtocolSettings::MainNet() { return ProtocolSettings(MAINNET_MAGIC); }

ProtocolSettings ProtocolSettings::TestNet() { return ProtocolSettings(TESTNET_MAGIC); }

ProtocolSettings ProtocolSettings::PrivNet() { return ProtocolSettings(PRIVNET_MAGIC); }

ProtocolSettings ProtocolSettings::Custom(const std::string& config)
{
    ProtocolSettings settings;
    settings.LoadFromJson(config);
    return settings;
}

void ProtocolSettings::InitializeMainNet()
{
    // MainNet configuration
    address_version_ = 0x35;  // 53

    // Consensus
    validator_count_ = 7;
    milliseconds_per_block_ = std::chrono::milliseconds(15000);
    max_transactions_per_block_ = 512;
    max_block_size_ = 262144;              // 256 KB
    max_block_system_fee_ = 900000000000;  // 9000 GAS

    // Economic model
    native_gas_factor_ = 100000000;                // 10^8
    initial_gas_distribution_ = 5200000000000000;  // 52,000,000 GAS

    // Transaction settings
    memory_pool_max_transactions_ = 50000;
    max_trace_size_ = 2102400;
    free_gas_limit_ = 0;
    fee_per_byte_ = 1000;
    max_valid_until_block_increment_ = 5760;  // ~24 hours

    // State settings
    state_root_frequency_ = 1;
    max_state_root_delay_blocks_ = 2000;

    // Initialize validators (MainNet validator public keys)
    std::vector<std::string> validators = {"02486fd15702c4490a26703112a5cc1d0923fd697a33406bd5a1c00e0013b09a70",
                                           "024c7b7fb6c310fccf1ba33b082519d82964ea93868d67916631617e3b768bc09f",
                                           "02aaec38470f6aad0042c6e877cfd8087d2676b09a2651e753de65b58a2dd1e096",
                                           "02ca0e27697b9c248f6f16e085fd0061e26f44da85b58ee835c110caa5ec3ba554",
                                           "02df48f60e8f3e01c48ff40b9b7f1310d7a8b2a193188befe1c2e3df740e895093",
                                           "03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0f000a1b1",
                                           "03b8d9d5771d8f513aa0869b9cc8d50986403b78c6da36890638c3d46a5adce04a"};
    LoadStandbyValidators(validators);

    // Initialize native contracts
    InitializeNativeContracts();

    // Initialize hardforks
    InitializeHardforks();

    // Seed nodes (MainNet)
    seed_nodes_ = {"seed1.neo.org:10333", "seed2.neo.org:10333", "seed3.neo.org:10333", "seed4.neo.org:10333",
                   "seed5.neo.org:10333"};
}

void ProtocolSettings::InitializeTestNet()
{
    // TestNet configuration
    address_version_ = 0x35;  // Same as MainNet for N3

    // Consensus
    validator_count_ = 7;
    milliseconds_per_block_ = std::chrono::milliseconds(15000);
    max_transactions_per_block_ = 5000;
    max_block_size_ = 2097152;              // 2 MB
    max_block_system_fee_ = 1500000000000;  // 15000 GAS

    // Economic model
    native_gas_factor_ = 100000000;
    initial_gas_distribution_ = 5200000000000000;

    // Transaction settings
    memory_pool_max_transactions_ = 50000;
    max_trace_size_ = 2102400;
    free_gas_limit_ = 0;
    fee_per_byte_ = 1000;
    max_valid_until_block_increment_ = 5760;

    // State settings
    state_root_frequency_ = 1;
    max_state_root_delay_blocks_ = 2000;

    // TestNet validators
    std::vector<std::string> validators = {"03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c",
                                           "02df48f60e8f3e01c48ff40b9b7f1310d7a8b2a193188befe1c2e3df740e895093",
                                           "03b8d9d5771d8f513aa0869b9cc8d50986403b78c6da36890638c3d46a5adce04a",
                                           "02ca0e27697b9c248f6f16e085fd0061e26f44da85b58ee835c110caa5ec3ba554",
                                           "024c7b7fb6c310fccf1ba33b082519d82964ea93868d67916631617e3b768bc09f",
                                           "02aaec38470f6aad0042c6e877cfd8087d2676b09a2651e753de65b58a2dd1e096",
                                           "02486fd15702c4490a26703112a5cc1d0923fd697a33406bd5a1c00e0013b09a70"};
    LoadStandbyValidators(validators);

    InitializeNativeContracts();
    InitializeHardforks();

    // TestNet seed nodes
    seed_nodes_ = {"seed1.ngd.network:20333", "seed2.ngd.network:20333", "seed3.ngd.network:20333",
                   "seed4.ngd.network:20333", "seed5.ngd.network:20333"};
}

void ProtocolSettings::InitializePrivNet()
{
    // Private network configuration (for testing)
    address_version_ = 0x35;

    // Consensus
    validator_count_ = 1;
    milliseconds_per_block_ = std::chrono::milliseconds(1000);  // Faster for testing
    max_transactions_per_block_ = 5000;
    max_block_size_ = 2097152;
    max_block_system_fee_ = 1500000000000;

    // Economic model
    native_gas_factor_ = 100000000;
    initial_gas_distribution_ = 3000000000000000;  // 30,000,000 GAS for testing

    // Transaction settings
    memory_pool_max_transactions_ = 50000;
    max_trace_size_ = 2102400;
    free_gas_limit_ = 0;
    fee_per_byte_ = 0;  // Free transactions for testing
    max_valid_until_block_increment_ = 86400;

    // State settings
    state_root_frequency_ = 1;
    max_state_root_delay_blocks_ = 10000;

    // Single validator for private network
    std::vector<std::string> validators = {"02b3622bf4017bdfe317c58aed5f4c753f206b7db896046fa7d774bbc4bf7f8dc2"};
    LoadStandbyValidators(validators);

    InitializeNativeContracts();
    InitializeHardforks();

    // No seed nodes for private network
    seed_nodes_.clear();
}

void ProtocolSettings::InitializeNativeContracts()
{
    // Initialize native contracts with their hashes and IDs
    native_contracts_ = {
        {"ContractManagement", io::UInt160::Parse("0xfffdc93764dbaddd97c48f252a53ea4643faa3fd"), -1, "", ""},
        {"StdLib", io::UInt160::Parse("0xacce6fd80d44e1796aa0c2c625e9e4e0ce39efc0"), -2, "", ""},
        {"CryptoLib", io::UInt160::Parse("0x726cb6e0cd8628a1350a611384688911ab75f51b"), -3, "", ""},
        {"LedgerContract", io::UInt160::Parse("0xda65b600f7124ce6c79950c1772a36403104f2be"), -4, "", ""},
        {"NeoToken", io::UInt160::Parse("0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5"), -5, "", ""},
        {"GasToken", io::UInt160::Parse("0xd2a4cff31913016155e38e474a2c06d08be276cf"), -6, "", ""},
        {"PolicyContract", io::UInt160::Parse("0xcc5e4edd9f5f8dba8bb65734541df7a1c081c67b"), -7, "", ""},
        {"RoleManagement", io::UInt160::Parse("0x49cf4e5378ffcd4dec034fd98a174c5491e395e2"), -8, "", ""},
        {"OracleContract", io::UInt160::Parse("0xfe924b7cfe89ddd271abaf7210a80a7e11178758"), -9, "", ""}};
}

void ProtocolSettings::InitializeHardforks()
{
    // Initialize hardfork heights
    hardforks_ = {{"HF_Aspidochelone", 1730000}, {"HF_Basilisk", 4120000}};
}

void ProtocolSettings::LoadStandbyValidators(const std::vector<std::string>& public_keys)
{
    standby_validators_.clear();

    for (const auto& key_str : public_keys)
    {
        auto key_bytes = cryptography::Helper::HexToBytes(key_str);
        standby_validators_.emplace_back(key_bytes);
    }
}

// ProtocolSettingsSingleton implementation

void ProtocolSettingsSingleton::Initialize(std::shared_ptr<ProtocolSettings> settings)
{
    std::lock_guard<std::mutex> lock(mutex_);
    instance_ = settings;
}

std::shared_ptr<ProtocolSettings> ProtocolSettingsSingleton::GetInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!instance_)
    {
        // Create default MainNet settings if not initialized
        instance_ = std::make_shared<ProtocolSettings>();
    }

    return instance_;
}

bool ProtocolSettingsSingleton::Load(const std::string& config_path)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto settings = std::make_shared<ProtocolSettings>();
    if (settings->LoadFromFile(config_path))
    {
        instance_ = settings;
        return true;
    }

    return false;
}

}  // namespace neo::core