#include <neo/core/logging.h>
#include <neo/node/neo_system.h>

namespace neo::node
{
NeoSystem::NeoSystem(std::shared_ptr<ProtocolSettings> protocolSettings, const std::string& storageEngine,
                     const std::string& storePath)
    : protocolSettings_(protocolSettings), running_(false), storageEngine_(storageEngine), storePath_(storePath)
{
    LOG_WARNING("Using lightweight implementation of NeoSystem");
}

NeoSystem::~NeoSystem()
{
    Stop();
}

bool NeoSystem::Start()
{
    LOG_WARNING("Stub: NeoSystem::Start()");
    running_ = true;
    return true;
}

void NeoSystem::Stop()
{
    LOG_WARNING("Stub: NeoSystem::Stop()");
    running_ = false;
}

bool NeoSystem::IsRunning() const
{
    return running_;
}

std::shared_ptr<ProtocolSettings> NeoSystem::GetProtocolSettings() const
{
    return protocolSettings_;
}

std::shared_ptr<ledger::Blockchain> NeoSystem::GetBlockchain() const
{
    LOG_WARNING("Stub: NeoSystem::GetBlockchain() returning nullptr");
    return nullptr;
}

std::shared_ptr<ledger::MemoryPool> NeoSystem::GetMemoryPool() const
{
    LOG_WARNING("Stub: NeoSystem::GetMemoryPool() returning nullptr");
    return nullptr;
}

std::shared_ptr<network::P2PServer> NeoSystem::GetP2PServer() const
{
    LOG_WARNING("Stub: NeoSystem::GetP2PServer() returning nullptr");
    return nullptr;
}

std::shared_ptr<persistence::DataCache> NeoSystem::GetDataCache() const
{
    LOG_WARNING("Stub: NeoSystem::GetDataCache() returning nullptr");
    return nullptr;
}

// Initialize methods
bool NeoSystem::InitializeStorage()
{
    LOG_WARNING("Stub: NeoSystem::InitializeStorage()");
    return true;
}

bool NeoSystem::InitializeBlockchain()
{
    LOG_WARNING("Stub: NeoSystem::InitializeBlockchain()");
    return true;
}

bool NeoSystem::InitializeMemoryPool()
{
    LOG_WARNING("Stub: NeoSystem::InitializeMemoryPool()");
    return true;
}

bool NeoSystem::InitializeNativeContracts()
{
    LOG_WARNING("Stub: NeoSystem::InitializeNativeContracts()");
    return true;
}

bool NeoSystem::InitializeNetworking()
{
    LOG_WARNING("Stub: NeoSystem::InitializeNetworking()");
    return true;
}

}  // namespace neo::node