/**
 * @file neo_system.cpp
 * @brief Main Neo system coordinator
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/node/neo_system.h>
#include <neo/smartcontract/native/native_contract_manager.h>
#include <neo/network/ip_address.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/channels_config.h>

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace neo::node
{
NeoSystem::NeoSystem(std::shared_ptr<ProtocolSettings> protocolSettings, const std::string& storageEngine,
                     const std::string& storePath)
    : protocolSettings_(protocolSettings),
      running_(false),
      storageEngine_(storageEngine),
      storePath_(storePath),
      nextCallbackId_(1)
{
    if (!protocolSettings_)
    {
        throw std::invalid_argument("Protocol settings cannot be null");
    }
}

NeoSystem::~NeoSystem() { Stop(); }

bool NeoSystem::Start()
{
    if (running_) return true;

    try
    {
        std::string provider = storageEngine_;
        if (provider.empty()) provider = "memory";
        std::string normalized = provider;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (normalized == "rocksdbstore")
        {
            provider = "rocksdb";
        }
        else if (normalized == "leveldbstore")
        {
            provider = "leveldb";
        }
        else if (normalized == "memorystore")
        {
            provider = "memory";
        }
        else if (normalized == "file" || normalized == "filestore" || normalized == "filestoreprovider")
        {
            provider = "file";
        }

        ledgerSystem_ = std::make_shared<ledger::NeoSystem>(protocolSettings_, provider, storePath_);

        if (!networkConfig_)
        {
            network::p2p::ChannelsConfig defaultConfig;
            defaultConfig.SetMinDesiredConnections(10);
            defaultConfig.SetMaxConnections(40);
            defaultConfig.SetMaxConnectionsPerAddress(3);
            const auto& seeds = protocolSettings_->GetSeedList();
            if (!seeds.empty())
            {
                std::vector<network::IPEndPoint> endpoints;
                endpoints.reserve(seeds.size());
                for (const auto& seed : seeds)
                {
                    network::IPEndPoint endpoint;
                    if (network::IPEndPoint::TryParse(seed, endpoint))
                    {
                        endpoints.push_back(endpoint);
                    }
                    else
                    {
                        // Fall back to default port if only host provided
                        endpoints.emplace_back(seed, static_cast<uint16_t>(10333));
                    }
                }

                if (!endpoints.empty())
                {
                    defaultConfig.SetSeedList(endpoints);
                    defaultConfig.SetTcp(network::IPEndPoint(network::IPAddress::Any(), endpoints.front().GetPort()));
                }
            }
            networkConfig_ = defaultConfig;
        }

        if (networkConfig_)
        {
            ledgerSystem_->SetNetworkConfig(*networkConfig_);
        }
        ledgerSystem_->Start();

        blockchain_ = ledgerSystem_->GetBlockchain();
        memoryPool_ = ledgerSystem_->GetMemoryPool();
        localNode_ = ledgerSystem_->GetLocalNode();
        p2pServer_.reset();

        running_ = true;
        return true;
    }
    catch (const std::exception&)
    {
        ledgerSystem_.reset();
        blockchain_.reset();
        memoryPool_.reset();
        return false;
    }
}

void NeoSystem::Stop()
{
    if (!running_)
    {
        return;
    }

    running_ = false;

    if (ledgerSystem_)
    {
        ledgerSystem_->Stop();
    }

    ledgerSystem_.reset();
    blockchain_.reset();
    memoryPool_.reset();
    localNode_.reset();
    p2pServer_.reset();

    std::lock_guard<std::mutex> lock(callbackMutex_);
    blockPersistCallbacks_.clear();
}

bool NeoSystem::IsRunning() const { return running_; }

std::shared_ptr<ProtocolSettings> NeoSystem::GetProtocolSettings() const { return protocolSettings_; }

std::shared_ptr<ledger::Blockchain> NeoSystem::GetBlockchain() const { return blockchain_; }

std::shared_ptr<ledger::MemoryPool> NeoSystem::GetMemoryPool() const { return memoryPool_; }

std::shared_ptr<network::P2PServer> NeoSystem::GetP2PServer() const { return p2pServer_; }

std::shared_ptr<network::p2p::LocalNode> NeoSystem::GetLocalNode() const { return localNode_; }

std::shared_ptr<network::p2p::NetworkSynchronizer> NeoSystem::GetNetworkSynchronizer() const
{
    return ledgerSystem_ ? ledgerSystem_->GetNetworkSynchronizer() : nullptr;
}

std::shared_ptr<persistence::DataCache> NeoSystem::GetDataCache() const
{
    return ledgerSystem_ ? ledgerSystem_->GetStoreView() : nullptr;
}

void NeoSystem::SetNetworkConfig(const network::p2p::ChannelsConfig& config)
{
    networkConfig_ = config;
    if (ledgerSystem_)
    {
        ledgerSystem_->SetNetworkConfig(config);
        localNode_ = ledgerSystem_->GetLocalNode();
    }
}

std::unique_ptr<smartcontract::ApplicationEngine> NeoSystem::CreateApplicationEngine(
    smartcontract::TriggerType trigger, const io::ISerializable* container, const ledger::Block* persistingBlock,
    int64_t gas)
{
    auto snapshot = ledgerSystem_ ? ledgerSystem_->GetStoreView() : nullptr;
    return std::make_unique<smartcontract::ApplicationEngine>(trigger, container, snapshot, persistingBlock, gas);
}

void NeoSystem::RegisterNativeContract(std::shared_ptr<smartcontract::native::NativeContract> contract)
{
    if (!contract) return;

    smartcontract::native::NativeContractManager::GetInstance().RegisterContract(contract);
}

smartcontract::native::NativeContract* NeoSystem::GetNativeContract(const io::UInt160& hash) const
{
    if (ledgerSystem_)
    {
        return ledgerSystem_->GetNativeContract(hash);
    }
    auto& manager = smartcontract::native::NativeContractManager::GetInstance();
    auto contract = manager.GetContract(hash);
    return contract.get();
}

std::vector<std::shared_ptr<smartcontract::native::NativeContract>> NeoSystem::GetNativeContracts() const
{
    if (ledgerSystem_)
    {
        return ledgerSystem_->GetNativeContracts();
    }
    auto& manager = smartcontract::native::NativeContractManager::GetInstance();
    return manager.GetContracts();
}

uint32_t NeoSystem::GetCurrentBlockHeight() const { return blockchain_ ? blockchain_->GetHeight() : 0; }

io::UInt256 NeoSystem::GetCurrentBlockHash() const
{
    return blockchain_ ? blockchain_->GetCurrentBlockHash() : io::UInt256();
}

bool NeoSystem::RelayTransaction(std::shared_ptr<ledger::Transaction> transaction)
{
    if (!transaction || !memoryPool_)
    {
        return false;
    }

    return memoryPool_->TryAdd(*transaction);
}

bool NeoSystem::RelayBlock(std::shared_ptr<ledger::Block> block)
{
    if (!block || !blockchain_)
    {
        return false;
    }

    auto result = blockchain_->OnNewBlock(block);
    return result == ledger::VerifyResult::Succeed;
}

int32_t NeoSystem::RegisterBlockPersistCallback(std::function<void(std::shared_ptr<ledger::Block>)> callback)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    int32_t id = nextCallbackId_++;
    blockPersistCallbacks_[id] = callback;
    return id;
}

void NeoSystem::UnregisterBlockPersistCallback(int32_t callbackId)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    blockPersistCallbacks_.erase(callbackId);
}

std::string NeoSystem::GetSystemStats() const
{
    // Return JSON-formatted system statistics
    auto poolCount = memoryPool_ ? memoryPool_->GetSize() : 0;
    return R"({
            "blockchain": {
                "height": )" +
           std::to_string(GetCurrentBlockHeight()) + R"(,
                "hash": ")" +
           GetCurrentBlockHash().ToString() + R"("
            },
            "memoryPool": {
                "count": )" +
           std::to_string(poolCount) + R"(
            },
            "network": {
                "connectedPeers": )" +
           std::to_string(localNode_ ? localNode_->GetConnectedCount() : 0U) + R"(,
                "listeningPort": )" +
           std::to_string(localNode_ ? localNode_->GetPort() : 0) + R"(
            },
            "system": {
                "running": )" +
           (running_ ? "true" : "false") + R"(,
                "storageEngine": ")" +
           storageEngine_ + R"("
            }
        })";
}

}  // namespace neo::node
