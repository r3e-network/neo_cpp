/**
 * @file neo_node.cpp
 * @brief Neo Node
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/node/neo_node.h>
#include <neo/core/configuration_manager.h>

#include <chrono>
#include <exception>

namespace neo::node
{
NeoNode::NeoNode(const std::string& configPath, const std::string& dataPath)
    : configPath_(configPath), dataPath_(dataPath), running_(false), shutdownRequested_(false)
{
    InitializeLogging();
}

NeoNode::~NeoNode()
{
    if (running_)
    {
        Stop();
    }
}

bool NeoNode::Initialize()
{
    try
    {
        logger_->Info("Initializing Neo C++ Node...");

        if (!LoadSettings())
        {
            logger_->Error("Failed to load node settings");
            return false;
        }

        if (!InitializeNeoSystem())
        {
            logger_->Error("Failed to initialize core Neo system");
            return false;
        }

        if (!InitializeNetwork())
        {
            logger_->Error("Failed to configure network");
            return false;
        }

        if (!InitializeRpcServer())
        {
            logger_->Error("Failed to set up RPC server");
            return false;
        }

        logger_->Info("Neo C++ Node initialized successfully");
        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error(std::string("Exception during initialization: ") + e.what());
        return false;
    }
}

bool NeoNode::Start()
{
    if (running_)
    {
        logger_->Warning("Node is already running");
        return true;
    }

    try
    {
        logger_->Info("Starting Neo C++ Node...");

        if (!neoSystem_)
        {
            logger_->Error("Neo system not initialized");
            return false;
        }

        if (!neoSystem_->Start())
        {
            logger_->Error("Failed to start Neo system");
            return false;
        }

        blockchain_ = neoSystem_->GetBlockchain();
        memoryPool_ = neoSystem_->GetMemoryPool();
        localNode_ = neoSystem_->GetLocalNode();

        if (!blockchain_ || !memoryPool_)
        {
            logger_->Error("Neo system not fully initialised (blockchain or mempool missing)");
            return false;
        }

        if (localNode_)
        {
            if (!localNode_->IsRunning() && !localNode_->Start(networkConfig_))
            {
                logger_->Error("Failed to start local P2P node");
                return false;
            }
        }
        else
        {
            logger_->Warning("Local node instance unavailable; network services are disabled");
        }

        if (!InitializeConsensus())
        {
            logger_->Error("Consensus initialization failed");
            return false;
        }

        if (consensusService_)
        {
            consensusAutoStart_ = core::ConfigurationManager::GetInstance().GetConsensusConfig().auto_start;
            if (!localNode_ || !localNode_->IsRunning())
            {
                logger_->Warning("Consensus configured but local node is not running; skipping consensus start");
            }
            else if (consensusAutoStart_)
            {
                consensusService_->SetAutoStartEnabled(true);
                consensusService_->Start();
                logger_->Info("Consensus service started with " +
                              std::to_string(consensusService_->GetValidators().size()) + " validators");
            }
            else
            {
                consensusService_->SetAutoStartEnabled(false);
                logger_->Info("Consensus service initialised but auto-start disabled");
            }
        }

        if (rpcServer_)
        {
            rpcServer_->Start();
        }

        running_ = true;
        shutdownRequested_ = false;

        // Start main processing thread
        mainThread_ = std::thread(&NeoNode::MainLoop, this);

        logger_->Info("Neo C++ Node started successfully");
        const auto networkMagic = protocolSettings_ ? protocolSettings_->GetNetwork() : 0;
        logger_->Info("Network magic: " + std::to_string(networkMagic));
        logger_->Info("P2P Port: " + std::to_string(networkConfig_.GetTcp().GetPort()));

        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error(std::string("Exception during startup: ") + e.what());
        return false;
    }
}

void NeoNode::Stop()
{
    if (!running_)
    {
        return;
    }

    logger_->Info("Stopping Neo C++ Node...");
    shutdownRequested_ = true;

    try
    {
        if (mainThread_.joinable())
        {
            mainThread_.join();
        }

        if (consensusService_)
        {
            consensusService_->Stop();
            consensusService_.reset();
        }
        network::p2p::LocalNode::GetInstance().SetConsensusService(nullptr);

        if (rpcServer_)
        {
            rpcServer_->Stop();
        }

        if (localNode_ && localNode_->IsRunning())
        {
            localNode_->Stop();
        }

        if (neoSystem_)
        {
            neoSystem_->Stop();
        }

        blockchain_.reset();
        memoryPool_.reset();
        localNode_.reset();

        running_ = false;
        logger_->Info("Neo C++ Node stopped successfully");
    }
    catch (const std::exception& e)
    {
        logger_->Error(std::string("Exception during shutdown: ") + e.what());
    }
}

bool NeoNode::IsRunning() const { return running_; }

uint32_t NeoNode::GetBlockHeight() const { return blockchain_ ? blockchain_->GetHeight() : 0; }
uint32_t NeoNode::GetHeaderHeight() const { return blockchain_ ? blockchain_->GetHeaderHeight() : 0; }

size_t NeoNode::GetConnectedPeersCount() const
{
    return localNode_ ? localNode_->GetConnectedCount() : 0;
}

size_t NeoNode::GetMemoryPoolCount() const { return memoryPool_ ? memoryPool_->GetSize() : 0; }

bool NeoNode::StartConsensusManually()
{
    if (!running_)
    {
        logger_->Warning("Cannot start consensus manually when node is not running");
        return false;
    }

    if (!consensusService_)
    {
        logger_->Warning("Consensus service not initialised; ensure consensus is enabled in configuration");
        return false;
    }

    if (!localNode_ || !localNode_->IsRunning())
    {
        logger_->Warning("Local node is not running; start networking before enabling consensus");
        return false;
    }

    if (consensusService_->StartManually())
    {
        logger_->Info("Consensus service started manually");
        return true;
    }

    logger_->Warning("Consensus service failed to start manually");
    return false;
}

bool NeoNode::RestartConsensus()
{
    if (!running_)
    {
        logger_->Warning("Cannot restart consensus when node is not running");
        return false;
    }

    if (!consensusService_)
    {
        logger_->Warning("Consensus service not initialised; ensure consensus is enabled in configuration");
        return false;
    }

    if (!localNode_ || !localNode_->IsRunning())
    {
        logger_->Warning("Local node is not running; cannot restart consensus");
        return false;
    }

    if (consensusService_->IsRunning())
    {
        consensusService_->Stop();
    }

    if (consensusService_->StartManually())
    {
        logger_->Info("Consensus service restarted successfully");
        return true;
    }

    logger_->Error("Consensus service restart failed");
    return false;
}
}  // namespace neo::node
