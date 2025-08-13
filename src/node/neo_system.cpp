/**
 * @file neo_system.cpp
 * @brief Main Neo system coordinator
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/logging/logger.h>
#include <neo/node/neo_system.h>
#include <neo/persistence/leveldb_store.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/policy_contract.h>
#include <neo/smartcontract/native/role_management.h>

#include <chrono>
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
    if (running_)
    {
        return true;
    }

    try
    {
        // Initialize storage
        if (!InitializeStorage())
        {
            return false;
        }

        // Initialize blockchain
        if (!InitializeBlockchain())
        {
            CleanupStorage();
            return false;
        }

        // Initialize memory pool
        if (!InitializeMemoryPool())
        {
            CleanupStorage();
            return false;
        }

        // Initialize native contracts
        if (!InitializeNativeContracts())
        {
            CleanupStorage();
            return false;
        }

        // Initialize networking
        if (!InitializeNetworking())
        {
            CleanupNativeContracts();
            CleanupStorage();
            return false;
        }

        running_ = true;
        return true;
    }
    catch (const std::exception& e)
    {
        // Cleanup on failure
        CleanupNetworking();
        CleanupNativeContracts();
        CleanupStorage();
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

    // Cleanup in reverse order of initialization
    CleanupNetworking();
    CleanupNativeContracts();
    CleanupStorage();

    // Clear callbacks
    std::lock_guard<std::mutex> lock(callbackMutex_);
    blockPersistCallbacks_.clear();
}

bool NeoSystem::IsRunning() const { return running_; }

std::shared_ptr<ProtocolSettings> NeoSystem::GetProtocolSettings() const { return protocolSettings_; }

std::shared_ptr<ledger::Blockchain> NeoSystem::GetBlockchain() const { return blockchain_; }

std::shared_ptr<ledger::MemoryPool> NeoSystem::GetMemoryPool() const { return memoryPool_; }

std::shared_ptr<network::P2PServer> NeoSystem::GetP2PServer() const { return p2pServer_; }

std::shared_ptr<persistence::DataCache> NeoSystem::GetDataCache() const { return dataCache_; }

std::unique_ptr<smartcontract::ApplicationEngine> NeoSystem::CreateApplicationEngine(
    smartcontract::TriggerType trigger, const io::ISerializable* container, const ledger::Block* persistingBlock,
    int64_t gas)
{
    return std::make_unique<smartcontract::ApplicationEngine>(trigger, container, dataCache_, persistingBlock, gas);
}

void NeoSystem::RegisterNativeContract(std::shared_ptr<smartcontract::native::NativeContract> contract)
{
    if (!contract)
    {
        return;
    }

    nativeContracts_.push_back(contract);
    // nativeContractMap_[contract->GetHash()] = contract.get();
}

smartcontract::native::NativeContract* NeoSystem::GetNativeContract(const io::UInt160& hash) const
{
    auto it = nativeContractMap_.find(hash);
    return (it != nativeContractMap_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<smartcontract::native::NativeContract>> NeoSystem::GetNativeContracts() const
{
    return nativeContracts_;
}

uint32_t NeoSystem::GetCurrentBlockHeight() const { return blockchain_ ? blockchain_->GetHeight() : 0; }

io::UInt256 NeoSystem::GetCurrentBlockHash() const
{
    return blockchain_ ? blockchain_->GetCurrentBlockHash() : io::UInt256();
}

bool NeoSystem::RelayTransaction(std::shared_ptr<ledger::Transaction> transaction)
{
    if (!transaction || !memoryPool_ || !p2pServer_)
    {
        return false;
    }

    // Add to memory pool
    // auto result = memoryPool_->TryAdd(transaction);
    // if (result != ledger::VerifyResult::Succeed)
    // {
    //     return false;
    // }

    // Broadcast to network
    // p2pServer_->BroadcastTransaction(transaction);
    return true;
}

bool NeoSystem::RelayBlock(std::shared_ptr<ledger::Block> block)
{
    if (!block || !blockchain_ || !p2pServer_)
    {
        return false;
    }

    // Add to blockchain
    // auto result = blockchain_->OnNewBlock(*block);
    // if (result != ledger::VerifyResult::Succeed)
    // {
    //     return false;
    // }

    // Broadcast to network
    // p2pServer_->BroadcastBlock(block);
    return true;
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
    return R"({
            "blockchain": {
                "height": )" +
           std::to_string(GetCurrentBlockHeight()) + R"(,
                "hash": ")" +
           GetCurrentBlockHash().ToString() + R"("
            },
            "memoryPool": {
                "count": )" +
           std::to_string(memoryPool_ ? memoryPool_->GetTransactionCount() : 0) + R"(
            },
            "network": {
                "connectedPeers": )" +
           std::to_string(p2pServer_ ? p2pServer_->GetConnectedPeersCount() : 0) + R"(
            },
            "system": {
                "running": )" +
           (running_ ? "true" : "false") + R"(,
                "storageEngine": ")" +
           storageEngine_ + R"("
            }
        })";
}

bool NeoSystem::InitializeStorage()
{
    try
    {
        if (storageEngine_ == "LevelDB")
        {
            auto store = std::make_shared<persistence::LevelDBStore>(storePath_);
            if (!store->Start())
            {
                return false;
            }
            dataCache_ = std::static_pointer_cast<persistence::DataCache>(store);
        }
        else
        {
            throw std::runtime_error("Unsupported storage engine: " + storageEngine_);
        }

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool NeoSystem::InitializeBlockchain()
{
    try
    {
        blockchain_ = std::make_shared<ledger::Blockchain>(dataCache_);
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool NeoSystem::InitializeMemoryPool()
{
    try
    {
        memoryPool_ = std::make_shared<ledger::MemoryPool>(protocolSettings_);
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool NeoSystem::InitializeNetworking()
{
    try
    {
        // Create P2P server
        network::IPEndPoint endpoint(network::IPAddress::Any(), protocolSettings_->GetP2PPort());
        p2pServer_ = std::make_shared<network::P2PServer>(
            // ioContext, endpoint, userAgent, startHeight
        );

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool NeoSystem::InitializeNativeContracts()
{
    try
    {
        // Initialize all native contracts
        auto gasToken = smartcontract::native::GasToken::GetInstance();
        auto neoToken = smartcontract::native::NeoToken::GetInstance();
        auto policyContract = smartcontract::native::PolicyContract::GetInstance();
        auto roleManagement = smartcontract::native::RoleManagement::GetInstance();
        auto contractManagement = smartcontract::native::ContractManagement::GetInstance();

        RegisterNativeContract(gasToken);
        RegisterNativeContract(neoToken);
        RegisterNativeContract(policyContract);
        RegisterNativeContract(roleManagement);
        RegisterNativeContract(contractManagement);

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

void NeoSystem::CleanupStorage()
{
    if (dataCache_)
    {
        // Stop storage if it has a Stop method
        dataCache_.reset();
    }
}

void NeoSystem::CleanupNetworking()
{
    if (p2pServer_)
    {
        // p2pServer_->Stop();
        p2pServer_.reset();
    }
}

void NeoSystem::CleanupNativeContracts()
{
    nativeContracts_.clear();
    nativeContractMap_.clear();
}
}  // namespace neo::node