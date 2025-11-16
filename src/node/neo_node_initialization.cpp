/**
 * @file neo_node_initialization.cpp
 * @brief Initialization logic
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/node/neo_node.h>

#include <neo/core/configuration_manager.h>
#include <neo/core/protocol_settings.h>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/io/byte_vector.h>
#include <neo/network/ip_address.h>
#include <neo/network/ip_endpoint.h>
#include <neo/rpc/rpc_server.h>

#include <exception>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <vector>

namespace neo::node
{
namespace
{
bool LooksLikeWIF(const std::string& key)
{
    return key.size() == 51 || key.size() == 52;
}

network::IPEndPoint BuildBindEndpoint(const P2PSettings& settings)
{
    network::IPAddress address = network::IPAddress::Any();
    if (!settings.BindAddress.empty())
    {
        network::IPAddress parsed;
        if (network::IPAddress::TryParse(settings.BindAddress, parsed))
        {
            address = parsed;
        }
    }
    return network::IPEndPoint(address, static_cast<uint16_t>(settings.Port));
}

std::vector<network::IPEndPoint> BuildSeedEndpoints(const std::vector<std::string>& seeds, uint16_t fallbackPort)
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
        else if (!seed.empty())
        {
            endpoints.emplace_back(seed, fallbackPort);
        }
    }
    return endpoints;
}

std::string ResolvePeerListPath(const std::string& dataPath)
{
    namespace fs = std::filesystem;
    fs::path base = dataPath.empty() ? fs::path("./data") : fs::path(dataPath);
    std::error_code ec;
    if (fs::is_regular_file(base, ec))
    {
        base = base.parent_path();
    }
    if (base.empty())
    {
        base = fs::current_path();
    }
    fs::path peersFile = base / "peers.dat";
    if (auto parent = peersFile.parent_path(); !parent.empty())
    {
        fs::create_directories(parent, ec);
    }
    return peersFile.string();
}
}

void NeoNode::InitializeLogging()
{
    logger_ = logging::Logger::Create("neo-node");
    if (!logger_)
    {
        logger_ = std::shared_ptr<logging::Logger>(&logging::Logger::Instance(), [](logging::Logger*) {});
    }
    logger_->SetLevel(logging::Logger::Level::Info);
    logger_->Info("Neo C++ Node starting up...");
}

bool NeoNode::LoadSettings()
{
    try
    {
        settings_ = Settings::Load(configPath_);

        if (!dataPath_.empty())
        {
            settings_.Storage.Path = dataPath_;
            settings_.Application.DataPath = dataPath_;
        }

        if (!settings_.Protocol)
        {
            logger_->Warning(std::string("Protocol settings missing in ") + configPath_ + ", using defaults");
            settings_.Protocol = std::make_shared<ProtocolSettings>(ProtocolSettings::GetDefault());
        }

        protocolSettings_ = settings_.Protocol;

        auto& configManager = core::ConfigurationManager::GetInstance();
        if (!configManager.LoadFromFile(configPath_))
        {
            logger_->Warning(std::string("Failed to load extended configuration from ") + configPath_);
        }
        consensusAutoStart_ = configManager.GetConsensusConfig().auto_start;

        auto coreSettings = std::make_shared<core::ProtocolSettings>();
        nlohmann::json protocolJson;
        protocolJson["Magic"] = protocolSettings_->GetNetwork();
        protocolJson["AddressVersion"] = protocolSettings_->GetAddressVersion();
        protocolJson["MillisecondsPerBlock"] = protocolSettings_->GetMillisecondsPerBlock();
        protocolJson["MaxTransactionsPerBlock"] = protocolSettings_->GetMaxTransactionsPerBlock();
        protocolJson["MemoryPoolMaxTransactions"] = protocolSettings_->GetMemoryPoolMaxTransactions();
        protocolJson["MaxTraceableBlocks"] = protocolSettings_->GetMaxTraceableBlocks();
        protocolJson["MaxValidUntilBlockIncrement"] = protocolSettings_->GetMaxValidUntilBlockIncrement();
        protocolJson["ValidatorsCount"] = static_cast<uint32_t>(protocolSettings_->GetValidatorsCount());

        const auto appendPoints =
            [](const std::vector<cryptography::ecc::ECPoint>& points)
            {
                std::vector<std::string> hex;
                hex.reserve(points.size());
                for (const auto& point : points)
                {
                    hex.push_back(point.ToHex());
                }
                return hex;
            };

        protocolJson["StandbyCommittee"] = appendPoints(protocolSettings_->GetStandbyCommittee());
        protocolJson["StandbyValidators"] = appendPoints(protocolSettings_->GetStandbyValidators());
        protocolJson["SeedList"] = protocolSettings_->GetSeedList();

        if (!coreSettings->LoadFromJson(protocolJson.dump()))
        {
            logger_->Warning("Failed to synchronise core protocol settings; using defaults");
        }

        core::ProtocolSettingsSingleton::Initialize(coreSettings);

        logger_->Info("Protocol settings loaded successfully");
        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error(std::string("Failed to load node settings: ") + e.what());
        return false;
    }
}

bool NeoNode::InitializeNeoSystem()
{
    try
    {
        neoSystem_ = std::make_shared<NeoSystem>(protocolSettings_, settings_.Storage.Engine, settings_.Storage.Path);
        logger_->Info("Neo system prepared with storage engine '" + settings_.Storage.Engine + "' at " +
                      settings_.Storage.Path);
        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error(std::string("Failed to initialize Neo system: ") + e.what());
        return false;
    }
}

bool NeoNode::InitializeNetwork()
{
    try
    {
        const auto tcpEndpoint = BuildBindEndpoint(settings_.P2P);
        networkConfig_.SetTcp(tcpEndpoint);
        networkConfig_.SetMinDesiredConnections(static_cast<uint32_t>(settings_.P2P.MinDesiredConnections));
        networkConfig_.SetMaxConnections(static_cast<uint32_t>(settings_.P2P.MaxConnections));
        networkConfig_.SetMaxConnectionsPerAddress(static_cast<uint32_t>(settings_.P2P.MaxConnectionsPerAddress));
        networkConfig_.SetEnableCompression(settings_.P2P.EnableCompression);

        auto seedEndpoints = BuildSeedEndpoints(settings_.P2P.Seeds, static_cast<uint16_t>(settings_.P2P.Port));
        if (seedEndpoints.empty() && protocolSettings_)
        {
            seedEndpoints = BuildSeedEndpoints(protocolSettings_->GetSeedList(),
                                               static_cast<uint16_t>(settings_.P2P.Port));
        }
        if (!seedEndpoints.empty())
        {
            networkConfig_.SetSeedList(seedEndpoints);
        }

        const auto peerListPath = ResolvePeerListPath(settings_.Application.DataPath);
        network::p2p::LocalNode::GetInstance().SetPeerListPath(peerListPath);
        logger_->Info("Peer list path: " + peerListPath);

        if (neoSystem_)
        {
            neoSystem_->SetNetworkConfig(networkConfig_);
        }

        logger_->Info("Network configuration prepared (P2P endpoint " + tcpEndpoint.ToString() + ")");
        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error(std::string("Failed to configure network: ") + e.what());
        return false;
    }
}

bool NeoNode::InitializeRpcServer()
{
    try
    {
        if (!settings_.RPC.Enabled)
        {
            logger_->Info("RPC server disabled");
            rpcServer_.reset();
            return true;
        }

        rpc::RpcConfig config;
        config.bind_address = settings_.RPC.BindAddress;
        config.port = static_cast<uint16_t>(settings_.RPC.Port);
        config.max_concurrent_requests = static_cast<uint32_t>(settings_.RPC.MaxConnections);
        config.request_timeout_seconds =
            settings_.RPC.RequestTimeoutMs > 0 ? static_cast<uint32_t>(settings_.RPC.RequestTimeoutMs / 1000)
                                               : config.request_timeout_seconds;
        config.enable_cors = settings_.RPC.EnableCors;
        config.allowed_origins = settings_.RPC.AllowedOrigins;

        rpcServer_ = std::make_shared<rpc::RpcServer>(config, neoSystem_);

        logger_->Info("RPC server configured on " + config.bind_address + ":" + std::to_string(config.port));
        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error(std::string("Failed to initialize RPC server: ") + e.what());
        rpcServer_.reset();
        return false;
    }
}

bool NeoNode::InitializeConsensus()
{
    try
    {
        if (consensusService_)
        {
            return true;
        }

        const auto& consensusConfig = core::ConfigurationManager::GetInstance().GetConsensusConfig();
        if (!consensusConfig.enabled)
        {
            logger_->Info("Consensus service disabled");
            consensusService_.reset();
            consensusAutoStart_ = false;
            return true;
        }

        if (!blockchain_ || !memoryPool_)
        {
            logger_->Error("Consensus configuration enabled but blockchain or memory pool is unavailable");
            return false;
        }

        auto coreSettings = core::ProtocolSettingsSingleton::GetInstance();
        if (!coreSettings)
        {
            coreSettings = std::make_shared<core::ProtocolSettings>();
            core::ProtocolSettingsSingleton::Initialize(coreSettings);
        }

        consensusService_ =
            std::make_shared<consensus::ConsensusService>(coreSettings, blockchain_, memoryPool_);

        consensusService_->SetAutoStartEnabled(consensusAutoStart_);
        network::p2p::LocalNode::GetInstance().SetConsensusService(consensusService_);

        if (!consensusConfig.private_key.empty())
        {
            try
            {
                std::unique_ptr<cryptography::ecc::KeyPair> consensusKey;
                if (LooksLikeWIF(consensusConfig.private_key))
                {
                    consensusKey = cryptography::ecc::KeyPair::FromWIF(consensusConfig.private_key);
                }
                else
                {
                    auto rawKey = io::ByteVector::FromHexString(consensusConfig.private_key);
                    consensusKey = std::make_unique<cryptography::ecc::KeyPair>(rawKey);
                }

                if (consensusKey)
                {
                    consensusService_->SetKeyPair(std::move(consensusKey));
                }
            }
            catch (const std::exception& ex)
            {
                logger_->Warning("Failed to parse consensus private key: " + std::string(ex.what()));
            }
        }
        else
        {
            logger_->Warning("Consensus enabled but no private key configured; node will not sign payloads");
        }

        return true;
    }
    catch (const std::exception& e)
    {
        logger_->Error(std::string("Failed to initialise consensus: ") + e.what());
        consensusService_.reset();
        return false;
    }
}
}  // namespace neo::node
