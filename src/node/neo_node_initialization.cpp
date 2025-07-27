#include <exception>
#include <neo/node/neo_node.h>

namespace neo::node
{
void NeoNode::InitializeLogging()
{
    logger_ = logging::Logger::GetInstance();
    logger_->SetLevel(logging::LogLevel::Info);
    logger_->Info("Neo C++ Node starting up...");
}

bool NeoNode::LoadProtocolSettings()
{
    try
    {
        protocolSettings_ = ProtocolSettings::Load(configPath_);
        if (!protocolSettings_)
        {
            logger_->Warning("Failed to load config from {}, using defaults", configPath_);
            protocolSettings_ = std::make_unique<ProtocolSettings>(ProtocolSettings::GetDefault());
        }

        logger_->Info("Protocol settings loaded successfully");
        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error("Failed to load protocol settings: {}", e.what());
        return false;
    }
}

bool NeoNode::InitializeStorage()
{
    try
    {
        store_ = std::make_shared<persistence::LevelDBStore>(dataPath_);
        logger_->Info("Storage initialized at: {}", dataPath_);
        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error("Failed to initialize storage: {}", e.what());
        return false;
    }
}

bool NeoNode::InitializeBlockchain()
{
    try
    {
        blockchain_ = std::make_shared<ledger::Blockchain>(protocolSettings_, store_);
        memoryPool_ = std::make_shared<ledger::MemoryPool>(protocolSettings_);

        logger_->Info("Blockchain initialized");
        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error("Failed to initialize blockchain: {}", e.what());
        return false;
    }
}

bool NeoNode::InitializeSmartContracts()
{
    try
    {
        // Initialize application engine
        applicationEngine_ = std::make_shared<smartcontract::ApplicationEngine>(protocolSettings_, blockchain_);

        // Initialize native contracts
        gasToken_ = smartcontract::native::GasToken::GetInstance();
        neoToken_ = smartcontract::native::NeoToken::GetInstance();
        policyContract_ = smartcontract::native::PolicyContract::GetInstance();
        roleManagement_ = smartcontract::native::RoleManagement::GetInstance();

        // Register native contracts with application engine
        applicationEngine_->RegisterNativeContract(gasToken_);
        applicationEngine_->RegisterNativeContract(neoToken_);
        applicationEngine_->RegisterNativeContract(policyContract_);
        applicationEngine_->RegisterNativeContract(roleManagement_);

        logger_->Info("Smart contract system initialized");
        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error("Failed to initialize smart contracts: {}", e.what());
        return false;
    }
}

bool NeoNode::InitializeNetwork()
{
    try
    {
        // Create P2P server
        auto listenEndpoint = network::IPEndPoint(network::IPAddress::Any(), protocolSettings_->GetP2PPort());

        p2pServer_ = std::make_shared<network::P2PServer>(listenEndpoint, protocolSettings_, blockchain_, memoryPool_);

        // Create peer discovery service
        peerDiscovery_ = std::make_shared<network::PeerDiscoveryService>(protocolSettings_, p2pServer_);

        logger_->Info("Network layer initialized");
        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error("Failed to initialize network: {}", e.what());
        return false;
    }
}

bool NeoNode::InitializeRPC()
{
    try
    {
        if (protocolSettings_->IsRpcEnabled())
        {
            rpcServer_ = std::make_shared<rpc::RpcServer>(protocolSettings_->GetRpcPort(), blockchain_, memoryPool_,
                                                          applicationEngine_);

            logger_->Info("RPC server initialized on port {}", protocolSettings_->GetRpcPort());
        }
        else
        {
            logger_->Info("RPC server disabled");
        }

        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error("Failed to initialize RPC: {}", e.what());
        return false;
    }
}

bool NeoNode::InitializeConsensus()
{
    try
    {
        if (protocolSettings_->IsConsensusEnabled())
        {
            consensusService_ =
                std::make_shared<consensus::ConsensusService>(protocolSettings_, blockchain_, memoryPool_, p2pServer_);

            logger_->Info("Consensus service initialized");
        }
        else
        {
            logger_->Info("Consensus service disabled");
        }

        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error("Failed to initialize consensus: {}", e.what());
        return false;
    }
}
}  // namespace neo::node