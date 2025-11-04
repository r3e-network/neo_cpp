#include <neo/ledger/neo_system.h>

#include <neo/core/neo_system.h>
#include <neo/network/p2p/local_node.h>
#include <neo/persistence/store_factory.h>
#include <neo/smartcontract/native/native_contract_manager.h>

#include <algorithm>
#include <cctype>
#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace neo::ledger
{
NeoSystem::NeoSystem(std::shared_ptr<neo::ProtocolSettings> settings)
    : NeoSystem(std::move(settings), "memory", std::string())
{
}

NeoSystem::NeoSystem(std::shared_ptr<neo::ProtocolSettings> settings, std::string storage_provider,
                     std::string storage_path)
    : settings_(std::move(settings)),
      storage_provider_(std::move(storage_provider)),
      storage_path_(std::move(storage_path)),
      is_running_(false),
      is_disposed_(false)
{
    if (!settings_)
    {
        throw std::invalid_argument("Protocol settings cannot be null");
    }
}

NeoSystem::~NeoSystem() { Dispose(); }

void NeoSystem::Start()
{
    if (is_running_) return;

    storage_provider_ = NormalizeProviderName(storage_provider_);
    smartcontract::native::NativeContractManager::GetInstance().Initialize();

    std::unordered_map<std::string, std::string> provider_config;
    if (!storage_path_.empty())
    {
        provider_config["db_path"] = storage_path_;
    }

    auto provider_ptr = persistence::StoreFactory::get_store_provider(storage_provider_, provider_config);
    if (!provider_ptr)
    {
        provider_ptr = persistence::StoreFactory::get_store_provider("memory", {});
    }

    auto settings_copy = std::make_unique<neo::ProtocolSettings>(*settings_);
    core_system_ = std::make_shared<neo::NeoSystem>(std::move(settings_copy), provider_ptr, storage_path_);
    core_system_->EnsureBlockchainInitialized();

    ledger_contract_ = core_system_->GetLedgerContract();

    if (auto raw_blockchain = core_system_->GetBlockchain())
    {
        blockchain_ = std::shared_ptr<Blockchain>(core_system_, raw_blockchain);
    }
    else
    {
        blockchain_.reset();
    }

    if (auto raw_mempool = core_system_->GetMemPool())
    {
        memory_pool_ = std::shared_ptr<MemoryPool>(core_system_, raw_mempool);
    }
    else
    {
        memory_pool_.reset();
    }

    auto& singleton = network::p2p::LocalNode::GetInstance();
    local_node_ = std::shared_ptr<network::p2p::LocalNode>(&singleton, [](network::p2p::LocalNode*) {});

    if (local_node_)
    {
        if (memory_pool_) local_node_->SetMemoryPool(memory_pool_);
        if (blockchain_)
        {
            local_node_->SetBlockchain(blockchain_);
            local_node_->SetLastBlockIndex(blockchain_->GetHeight());
        }

        if (channels_config_ && !local_node_->IsRunning())
        {
            local_node_->Start(*channels_config_);
        }
    }

    is_running_ = true;
    is_disposed_ = false;
}

void NeoSystem::Stop()
{
    if (!is_running_) return;

    if (local_node_ && local_node_->IsRunning())
    {
        local_node_->Stop();
    }

    if (core_system_)
    {
        core_system_->stop();
    }

    blockchain_.reset();
    memory_pool_.reset();
    ledger_contract_.reset();
    core_system_.reset();

    is_running_ = false;
}

bool NeoSystem::IsRunning() const { return is_running_; }

void NeoSystem::Dispose()
{
    if (is_disposed_) return;

    Stop();
    local_node_.reset();
    is_disposed_ = true;
}

void NeoSystem::SetNetworkConfig(const network::p2p::ChannelsConfig& config)
{
    channels_config_ = config;

    if (local_node_ && local_node_->IsRunning())
    {
        local_node_->Stop();
        local_node_->Start(*channels_config_);
    }
}

std::shared_ptr<Blockchain> NeoSystem::GetBlockchain() const { return blockchain_; }

std::shared_ptr<MemoryPool> NeoSystem::GetMemoryPool() const { return memory_pool_; }

std::shared_ptr<network::p2p::LocalNode> NeoSystem::GetLocalNode() const { return local_node_; }

std::shared_ptr<neo::ProtocolSettings> NeoSystem::GetSettings() const { return settings_; }

std::shared_ptr<persistence::DataCache> NeoSystem::GetStoreView() const
{
    if (!core_system_) return nullptr;

    auto snapshot = core_system_->get_snapshot_cache();
    if (!snapshot) return nullptr;

    return std::shared_ptr<persistence::DataCache>(snapshot.release(),
                                                   [](persistence::DataCache* cache) { delete cache; });
}

std::shared_ptr<smartcontract::native::LedgerContract> NeoSystem::GetLedgerContract() const
{
    if (ledger_contract_) return ledger_contract_;
    if (!core_system_) return nullptr;
    ledger_contract_ = core_system_->GetLedgerContract();
    return ledger_contract_;
}

std::shared_ptr<smartcontract::native::NeoToken> NeoSystem::GetNeoToken() const
{
    return core_system_ ? core_system_->GetNeoToken() : smartcontract::native::NeoToken::GetInstance();
}

std::shared_ptr<smartcontract::native::GasToken> NeoSystem::GetGasToken() const
{
    return core_system_ ? core_system_->GetGasToken() : smartcontract::native::GasToken::GetInstance();
}

std::shared_ptr<smartcontract::native::RoleManagement> NeoSystem::GetRoleManagement() const
{
    return core_system_ ? core_system_->GetRoleManagement()
                        : smartcontract::native::RoleManagement::GetInstance();
}

std::shared_ptr<Block> NeoSystem::GetGenesisBlock() const
{
    if (core_system_) return core_system_->GetGenesisBlock();
    return blockchain_ ? blockchain_->GetBlock(0) : nullptr;
}

smartcontract::native::NativeContract* NeoSystem::GetNativeContract(const io::UInt160& hash) const
{
    if (core_system_) return core_system_->GetNativeContract(hash);
    auto& manager = smartcontract::native::NativeContractManager::GetInstance();
    auto contract = manager.GetContract(hash);
    return contract.get();
}

std::vector<std::shared_ptr<smartcontract::native::NativeContract>> NeoSystem::GetNativeContracts() const
{
    auto& manager = smartcontract::native::NativeContractManager::GetInstance();
    return manager.GetContracts();
}

uint32_t NeoSystem::GetMaxTraceableBlocks() const
{
    if (core_system_) return core_system_->GetMaxTraceableBlocks();
    return settings_ ? settings_->GetMaxTraceableBlocks() : 0;
}

std::shared_ptr<persistence::DataCache> NeoSystem::GetSnapshot() const { return GetStoreView(); }

ContainsTransactionType NeoSystem::ContainsTransaction(const io::UInt256& hash) const
{
    if (core_system_) return core_system_->contains_transaction(hash);
    if (memory_pool_ && memory_pool_->Contains(hash)) return ContainsTransactionType::ExistsInPool;
    if (blockchain_ && blockchain_->ContainsTransaction(hash)) return ContainsTransactionType::ExistsInLedger;
    return ContainsTransactionType::NotExist;
}

bool NeoSystem::ContainsConflictHash(const io::UInt256& hash, const std::vector<io::UInt160>& signers) const
{
    if (core_system_) return core_system_->contains_conflict_hash(hash, signers);
    return false;
}

std::string NeoSystem::NormalizeProviderName(const std::string& provider) const
{
    std::string normalized = provider;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (normalized.empty() || normalized == "memory" || normalized == "memorystore")
    {
        return "memory";
    }

    if (normalized == "rocksdb" || normalized == "rocksdbstore" || normalized == "leveldb" ||
        normalized == "leveldbstore")
    {
        return "rocksdb";
    }

    if (normalized == "file" || normalized == "filestore" || normalized == "filestoreprovider")
    {
        return "file";
    }

    return normalized;
}

}  // namespace neo::ledger
