/**
 * @file neo_system_stub.cpp
 * @brief Main Neo system coordinator
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/io/uint256.h>
#include <neo/node/neo_system.h>

namespace neo::node
{
NeoSystem::NeoSystem(std::shared_ptr<ProtocolSettings> protocolSettings, const std::string& storageEngine,
                     const std::string& storePath)
    : protocolSettings_(protocolSettings), running_(false), storageEngine_(storageEngine), storePath_(storePath)
{
    LOG_WARNING("Using lightweight implementation of NeoSystem");
}

NeoSystem::~NeoSystem() { Stop(); }

bool NeoSystem::Start()
{
    LOG_INFO(" NeoSystem::Start()");
    running_ = true;
    return true;
}

void NeoSystem::Stop()
{
    LOG_INFO(" NeoSystem::Stop()");
    running_ = false;
}

bool NeoSystem::IsRunning() const { return running_; }

std::shared_ptr<ProtocolSettings> NeoSystem::GetProtocolSettings() const { return protocolSettings_; }

std::shared_ptr<ledger::Blockchain> NeoSystem::GetBlockchain() const
{
    LOG_INFO(" NeoSystem::GetBlockchain() returning nullptr");
    return nullptr;
}

std::shared_ptr<ledger::MemoryPool> NeoSystem::GetMemoryPool() const
{
    LOG_INFO(" NeoSystem::GetMemoryPool() returning nullptr");
    return nullptr;
}

std::shared_ptr<network::P2PServer> NeoSystem::GetP2PServer() const
{
    LOG_INFO(" NeoSystem::GetP2PServer() returning nullptr");
    return nullptr;
}

std::shared_ptr<persistence::DataCache> NeoSystem::GetDataCache() const
{
    LOG_INFO(" NeoSystem::GetDataCache() returning nullptr");
    return nullptr;
}

uint32_t NeoSystem::GetCurrentBlockHeight() const
{
    LOG_INFO(" NeoSystem::GetCurrentBlockHeight() returning 0");
    return 0;
}

io::UInt256 NeoSystem::GetCurrentBlockHash() const
{
    LOG_INFO(" NeoSystem::GetCurrentBlockHash() returning zero hash");
    return io::UInt256::Zero();
}

// Initialize methods
bool NeoSystem::InitializeStorage()
{
    LOG_INFO(" NeoSystem::InitializeStorage()");
    return true;
}

bool NeoSystem::InitializeBlockchain()
{
    LOG_INFO(" NeoSystem::InitializeBlockchain()");
    return true;
}

bool NeoSystem::InitializeMemoryPool()
{
    LOG_INFO(" NeoSystem::InitializeMemoryPool()");
    return true;
}

bool NeoSystem::InitializeNativeContracts()
{
    LOG_INFO(" NeoSystem::InitializeNativeContracts()");
    return true;
}

bool NeoSystem::InitializeNetworking()
{
    LOG_INFO(" NeoSystem::InitializeNetworking()");
    return true;
}

}  // namespace neo::node