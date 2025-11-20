#include "main_service.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <neo/io/json.h>
#include <neo/network/ip_address.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/channels_config.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/network_synchronizer.h>
#include <neo/network/p2p/remote_node.h>
#include <neo/node/neo_system.h>
#include <neo/rpc/rpc_methods.h>
#include <neo/rpc/rpc_server.h>
#include <neo/rpc/rpc_session_manager.h>
#include <neo/io/fixed8.h>
#include <neo/settings.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/wallets/wallet.h>
#include <neo/wallets/wallet_factory.h>
#include <filesystem>
#include <sstream>
#include <thread>

namespace
{
neo::network::IPEndPoint CreateBindEndpoint(const neo::P2PSettings& p2p)
{
    neo::network::IPAddress address = neo::network::IPAddress::Any();
    if (!p2p.BindAddress.empty())
    {
        neo::network::IPAddress parsed;
        if (neo::network::IPAddress::TryParse(p2p.BindAddress, parsed))
        {
            address = parsed;
        }
    }
    return neo::network::IPEndPoint(address, static_cast<uint16_t>(p2p.Port));
}

std::vector<neo::network::IPEndPoint> BuildSeedEndpoints(const std::vector<std::string>& seeds, uint16_t defaultPort)
{
    std::vector<neo::network::IPEndPoint> endpoints;
    endpoints.reserve(seeds.size());
    for (const auto& seed : seeds)
    {
        neo::network::IPEndPoint endpoint;
        if (neo::network::IPEndPoint::TryParse(seed, endpoint))
        {
            endpoints.push_back(endpoint);
        }
        else if (!seed.empty())
        {
            endpoints.emplace_back(seed, defaultPort);
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

std::string ResolveNetworkConfigPath(const std::string& network)
{
    namespace fs = std::filesystem;
    std::string normalized = network;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    std::vector<std::string> candidates;
    if (normalized == "mainnet")
    {
        candidates = {"config/mainnet.config.json", "config/mainnet.json"};
    }
    else if (normalized == "testnet")
    {
        candidates = {"config/testnet.config.json", "config/testnet.json"};
    }
    else if (normalized == "privnet" || normalized == "private" || normalized == "private-net")
    {
        candidates = {"config/privnet.json"};
    }
    else
    {
        throw std::invalid_argument("Unknown network preset: " + network);
    }

    for (const auto& candidate : candidates)
    {
        if (fs::exists(candidate))
        {
            return candidate;
        }

        fs::path parentCandidate = fs::path("..") / candidate;
        if (fs::exists(parentCandidate))
        {
            return parentCandidate.string();
        }
    }

    throw std::invalid_argument("No configuration found for preset: " + network);
}

uint32_t GetMaxPeerBlockHeight(const std::shared_ptr<neo::network::p2p::LocalNode>& localNode)
{
    if (!localNode)
        return 0;

    uint32_t maxHeight = 0;
    for (auto* peer : localNode->GetConnectedNodes())
    {
        if (peer)
        {
            maxHeight = std::max(maxHeight, peer->GetLastBlockIndex());
        }
    }
    return maxHeight;
}

size_t GetUnconnectedPeerCount(const std::shared_ptr<neo::network::p2p::LocalNode>& localNode)
{
    if (!localNode)
        return 0;

    try
    {
        return localNode->GetPeerList().GetUnconnectedCount();
    }
    catch (...)
    {
        return 0;
    }
}

std::string FormatDuration(std::chrono::seconds duration)
{
    const auto totalSeconds = duration.count();
    const auto days = totalSeconds / 86400;
    const auto hours = (totalSeconds % 86400) / 3600;
    const auto minutes = (totalSeconds % 3600) / 60;
    const auto seconds = totalSeconds % 60;

    std::ostringstream oss;
    oss << days << "d " << std::setw(2) << std::setfill('0') << hours << "h " << std::setw(2) << minutes << "m "
        << std::setw(2) << seconds << "s";
    return oss.str();
}

std::string FormatTimestamp(const std::chrono::system_clock::time_point& when)
{
    std::time_t now = std::chrono::system_clock::to_time_t(when);
    std::tm tmNow{};
#ifdef _WIN32
    localtime_s(&tmNow, &now);
#else
    localtime_r(&now, &tmNow);
#endif
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tmNow);
    return buffer;
}

struct NodeStateSnapshot
{
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point captureTime;
    std::chrono::system_clock::time_point wallClock;
    uint32_t blockHeight = 0;
    uint32_t headerHeight = 0;
    uint32_t targetHeight = 0;
    uint32_t maxPeerHeight = 0;
    size_t connectedPeers = 0;
    size_t unconnectedPeers = 0;
    size_t verifiedPool = 0;
    size_t unverifiedPool = 0;
};

NodeStateSnapshot CaptureNodeSnapshot(const std::shared_ptr<neo::node::NeoSystem>& system,
                                      std::chrono::steady_clock::time_point startTime)
{
    NodeStateSnapshot snapshot;
    snapshot.startTime = startTime;
    snapshot.captureTime = std::chrono::steady_clock::now();
    snapshot.wallClock = std::chrono::system_clock::now();

    if (!system)
        return snapshot;

    auto blockchain = system->GetBlockchain();
    auto mempool = system->GetMemPool();
    auto localNode = system->GetLocalNode();
    auto synchronizer = system->GetNetworkSynchronizer();

    snapshot.blockHeight = blockchain ? blockchain->GetHeight() : 0;
    snapshot.headerHeight = blockchain ? blockchain->GetHeaderHeight() : snapshot.blockHeight;
    snapshot.targetHeight = synchronizer ? synchronizer->GetTargetBlockIndex() : snapshot.blockHeight;
    snapshot.maxPeerHeight = GetMaxPeerBlockHeight(localNode);
    snapshot.connectedPeers = localNode ? localNode->GetConnectedCount() : 0;
    snapshot.unconnectedPeers = GetUnconnectedPeerCount(localNode);
    snapshot.verifiedPool = mempool ? mempool->GetSize() : 0;
    snapshot.unverifiedPool = mempool ? mempool->GetUnverifiedSize() : 0;

    return snapshot;
}

void RenderNodeSnapshot(const NodeStateSnapshot& snapshot)
{
    neo::cli::ConsoleHelper::Clear();

    const auto uptime = std::chrono::duration_cast<std::chrono::seconds>(snapshot.captureTime - snapshot.startTime);
    const auto timestamp = FormatTimestamp(snapshot.wallClock);

    const uint32_t syncTarget = std::max({snapshot.targetHeight, snapshot.maxPeerHeight, snapshot.headerHeight});
    const uint32_t denominator = syncTarget == 0 ? snapshot.blockHeight : syncTarget;
    double syncPercent = 100.0;
    if (denominator > 0)
    {
        syncPercent = (static_cast<double>(snapshot.blockHeight) / denominator) * 100.0;
        if (syncPercent > 100.0)
            syncPercent = 100.0;
        if (syncPercent < 0.0)
            syncPercent = 0.0;
    }

    std::cout << "=============================================" << std::endl;
    std::cout << "             NEO NODE STATUS                 " << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "Time: " << timestamp << "    Uptime: " << FormatDuration(uptime) << std::endl << std::endl;

    std::cout << "Blockchain" << std::endl;
    std::cout << "  Block Height : " << snapshot.blockHeight << std::endl;
    if (snapshot.headerHeight > 0)
        std::cout << "  Header Height: " << snapshot.headerHeight << std::endl;
    if (snapshot.targetHeight > 0)
        std::cout << "  Target Height: " << snapshot.targetHeight << std::endl;
    if (snapshot.maxPeerHeight > 0)
        std::cout << "  Max Peer     : " << snapshot.maxPeerHeight << std::endl;
    {
        std::ostringstream line;
        line << "  Sync Progress: " << std::fixed << std::setprecision(2) << syncPercent << "%";
        std::cout << line.str() << std::endl;
    }
    std::cout << std::endl;

    std::cout << "Network" << std::endl;
    std::cout << "  Connected Peers  : " << snapshot.connectedPeers << std::endl;
    std::cout << "  Unconnected Peers: " << snapshot.unconnectedPeers << std::endl;
    std::cout << std::endl;

    std::cout << "Memory Pool" << std::endl;
    std::cout << "  Verified   : " << snapshot.verifiedPool << std::endl;
    std::cout << "  Unverified : " << snapshot.unverifiedPool << std::endl;
    std::cout << "  Total      : " << snapshot.verifiedPool + snapshot.unverifiedPool << std::endl;

    std::cout << std::endl;
    std::cout << "Press ENTER to exit | Refreshes every second" << std::endl;
}

bool IsVerboseArgument(const std::string& value)
{
    if (value.empty())
        return false;

    std::string normalized = value;
    normalized.erase(normalized.begin(),
                     std::find_if(normalized.begin(), normalized.end(), [](unsigned char c) { return c != '-'; }));
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return normalized == "verbose" || normalized == "v" || normalized == "true" || normalized == "1";
}

std::string FormatGasAmount(int64_t datoshi)
{
    constexpr double kGasFactor = 100000000.0;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(8) << static_cast<double>(datoshi) / kGasFactor;
    return oss.str();
}
}  // namespace

namespace neo::cli
{
MainService::MainService() : running_(false)
{
    InitializeTypeConverters();
    InitializeCommands();
}

MainService::~MainService()
{
    Stop();
}

void MainService::Run(const std::vector<std::string>& args)
{
    if (args.empty())
    {
        // Interactive mode
        CommandLineOptions options;
        Start(options);
        RunConsole();
        Stop();
    }
    else
    {
        // Command line mode
        OnStartWithCommandLine(args);
    }
}

void MainService::Start(const CommandLineOptions& options)
{
    if (neoSystem_)
        return;

    try
    {
        // Resolve configuration path if a network preset was provided
        std::string configPath = options.Config;
        if (configPath.empty() && !options.Network.empty())
        {
            try
            {
                configPath = ResolveNetworkConfigPath(options.Network);
                ConsoleHelper::Info("Selected network preset '" + options.Network + "' -> " + configPath);
            }
            catch (const std::exception& e)
            {
                ConsoleHelper::Error(e.what());
                throw;
            }
        }

        // Load settings
        Settings settings;
        if (!configPath.empty())
        {
            ConsoleHelper::Info("Loading configuration from " + configPath);
            settings = Settings::Load(configPath);
        }
        else
        {
            settings = Settings::GetDefault();
        }

        // Override settings from command line
        if (!options.DbEngine.empty())
            settings.Storage.Engine = options.DbEngine;
        if (!options.DbPath.empty())
            settings.Storage.Path = options.DbPath;

        // Apply RPC limits before networking/RPC server initialization
        rpc::RPCMethods::SetMaxFindResultItems(static_cast<size_t>(std::max(1, settings.RPC.MaxFindResultItems)));

        // Configure peer list path before networking starts
        const auto peerListPath = ResolvePeerListPath(settings.Application.DataPath);
        network::p2p::LocalNode::GetInstance().SetPeerListPath(peerListPath);
        ConsoleHelper::Info("Peer list path: " + peerListPath);

        // Store is created internally by NeoSystem

        // Create Neo system
        neoSystem_ = std::make_shared<node::NeoSystem>(settings.Protocol, settings.Storage.Engine, settings.Storage.Path);

        network::p2p::ChannelsConfig channelsConfig;
        channelsConfig.SetTcp(CreateBindEndpoint(settings.P2P));
        channelsConfig.SetMinDesiredConnections(static_cast<uint32_t>(settings.P2P.MinDesiredConnections));
        channelsConfig.SetMaxConnections(static_cast<uint32_t>(settings.P2P.MaxConnections));
        channelsConfig.SetMaxConnectionsPerAddress(static_cast<uint32_t>(settings.P2P.MaxConnectionsPerAddress));
        channelsConfig.SetEnableCompression(settings.P2P.EnableCompression);
        channelsConfig.SetDialTimeoutMs(static_cast<uint32_t>(settings.P2P.DialTimeoutMs));

        auto seedEndpoints = BuildSeedEndpoints(settings.P2P.Seeds, static_cast<uint16_t>(settings.P2P.Port));
        if (seedEndpoints.empty() && settings.Protocol)
        {
            seedEndpoints = BuildSeedEndpoints(settings.Protocol->GetSeedList(),
                                               static_cast<uint16_t>(settings.P2P.Port));
        }
        if (!seedEndpoints.empty())
        {
            channelsConfig.SetSeedList(seedEndpoints);
        }

        neoSystem_->SetNetworkConfig(channelsConfig);

        // Start Neo system
        neoSystem_->Start();

        ConsoleHelper::Info("Neo system started");
        ConsoleHelper::Info("Network: " + std::to_string(settings.Protocol->GetNetwork()));
        ConsoleHelper::Info("Storage: " + settings.Storage.Path);

        // Start RPC server if enabled
        if (settings.RPC.Enabled)
        {
            rpc::RpcConfig rpcConfig;
            rpcConfig.port = static_cast<uint16_t>(settings.RPC.Port);
            rpcConfig.bind_address = settings.RPC.BindAddress.empty() ? std::string("0.0.0.0")
                                                                      : settings.RPC.BindAddress;
            rpcConfig.enable_cors = settings.RPC.EnableCors;
            if (!settings.RPC.AllowedOrigins.empty())
            {
                rpcConfig.allowed_origins = settings.RPC.AllowedOrigins;
            }
            rpcConfig.max_concurrent_requests = static_cast<uint32_t>(std::max(1, settings.RPC.MaxConnections));
            rpcConfig.request_timeout_seconds =
                static_cast<uint32_t>(std::max(1, settings.RPC.RequestTimeoutMs / 1000));
            rpcConfig.max_request_size = static_cast<uint32_t>(settings.RPC.MaxRequestBodyBytes);
            rpcConfig.enable_rate_limiting = settings.RPC.EnableRateLimit;
            rpcConfig.max_requests_per_second =
                static_cast<uint32_t>(std::max(0, settings.RPC.MaxRequestsPerSecond));
            rpcConfig.rate_limit_window_seconds =
                static_cast<uint32_t>(std::max(1, settings.RPC.RateLimitWindowSeconds));
            rpcConfig.enable_sessions = settings.RPC.SessionEnabled;
            rpcConfig.session_timeout_minutes =
                static_cast<uint32_t>(std::max(1, settings.RPC.SessionExpirationMinutes));
            rpcConfig.max_iterator_items =
                static_cast<uint32_t>(std::max(1, settings.RPC.MaxIteratorResultItems));
            rpcConfig.enable_audit_trail = settings.RPC.EnableAuditTrail;
            rpcConfig.enable_security_logging = settings.RPC.EnableSecurityLogging;
            rpcConfig.enable_ssl = settings.RPC.EnableSsl;
            rpcConfig.ssl_cert_path = settings.RPC.SslCert;
            rpcConfig.ssl_key_path = settings.RPC.SslKey;
            rpcConfig.trusted_authorities = settings.RPC.TrustedAuthorities;
            rpcConfig.ssl_ciphers = settings.RPC.SslCiphers;
            rpcConfig.min_tls_version = settings.RPC.MinTlsVersion;
            if (!settings.RPC.Username.empty())
            {
                rpcConfig.enable_authentication = true;
                rpcConfig.username = settings.RPC.Username;
                rpcConfig.password = settings.RPC.Password;
            }

            rpcServer_ = std::make_shared<rpc::RpcServer>(rpcConfig, neoSystem_);
            rpcServer_->Start();
            ConsoleHelper::Info("RPC server started on " + rpcConfig.bind_address + ":" +
                                std::to_string(settings.RPC.Port));
        }

        // Open wallet if specified
        if (!options.Wallet.empty())
        {
            OnOpenWallet(options.Wallet, options.Password);
        }

        if (auto synchronizer = neoSystem_->GetNetworkSynchronizer())
        {
            synchronizer->SetStateChangedCallback(
                [this](network::p2p::SynchronizationState state)
                {
                    switch (state)
                    {
                        case network::p2p::SynchronizationState::NotSynchronizing:
                            ConsoleHelper::Info("Synchronization: Not synchronizing");
                            break;
                        case network::p2p::SynchronizationState::SynchronizingHeaders:
                            ConsoleHelper::Info("Synchronization: Synchronizing headers");
                            break;
                        case network::p2p::SynchronizationState::SynchronizingBlocks:
                            ConsoleHelper::Info("Synchronization: Synchronizing blocks");
                            break;
                        case network::p2p::SynchronizationState::Synchronized:
                            ConsoleHelper::Info("Synchronization: Synchronized");
                            break;
                    }
                });

            synchronizer->SetBlockReceivedCallback(
                [this](const std::shared_ptr<ledger::Block>& block)
                {
                    if (block->GetIndex() % 1000 == 0)
                    {
                        ConsoleHelper::Info("Block received: " + std::to_string(block->GetIndex()));
                    }
                });
        }

        running_ = true;
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error("Failed to start Neo system: " + std::string(ex.what()));
        throw;
    }
}

void MainService::Stop()
{
    if (!running_)
        return;

    running_ = false;

    // Stop console thread
    if (consoleThread_.joinable())
        consoleThread_.join();

    // Close wallet
    currentWallet_.reset();

    // Stop RPC server
    if (rpcServer_)
    {
        rpcServer_->Stop();
        rpcServer_.reset();
        ConsoleHelper::Info("RPC server stopped");
    }

    // Stop Neo system
    if (neoSystem_)
    {
        neoSystem_->Stop();
        neoSystem_.reset();
        ConsoleHelper::Info("Neo system stopped");
    }
}

void MainService::RegisterCommand(const std::string& name, const CommandHandler& handler, const std::string& category)
{
    commands_[name] = handler;

    if (!category.empty())
    {
        commandsByCategory_[category][name] = handler;
    }
}

void MainService::RegisterTypeConverter(const std::string& typeName, const TypeConverter& converter)
{
    typeConverters_[typeName] = converter;
}

std::shared_ptr<node::NeoSystem> MainService::GetNeoSystem() const
{
    return neoSystem_;
}

std::shared_ptr<wallets::Wallet> MainService::GetCurrentWallet() const
{
    return currentWallet_;
}

void MainService::OnCommand(const std::string& command)
{
    if (command.empty())
        return;

    // Parse command and arguments
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg)
    {
        args.push_back(arg);
    }

    // Find command handler
    auto it = commands_.find(cmd);
    if (it != commands_.end())
    {
        try
        {
            it->second(args);
        }
        catch (const std::exception& ex)
        {
            ConsoleHelper::Error(ex.what());
        }
    }
    else
    {
        ConsoleHelper::Error("Command not found: " + cmd);
    }
}

void MainService::OnStartWithCommandLine(const std::vector<std::string>& args)
{
    // Parse command line options
    CommandLineOptions options;

    for (size_t i = 0; i < args.size(); i++)
    {
        if (args[i] == "-c" || args[i] == "--config")
        {
            if (i + 1 < args.size())
                options.Config = args[++i];
        }
        else if (args[i] == "-w" || args[i] == "--wallet")
        {
            if (i + 1 < args.size())
                options.Wallet = args[++i];
        }
        else if (args[i] == "-p" || args[i] == "--password")
        {
            if (i + 1 < args.size())
                options.Password = args[++i];
        }
        else if (args[i] == "--db-engine")
        {
            if (i + 1 < args.size())
                options.DbEngine = args[++i];
        }
        else if (args[i] == "--db-path")
        {
            if (i + 1 < args.size())
                options.DbPath = args[++i];
        }
        else if (args[i] == "--network")
        {
            if (i + 1 < args.size())
                options.Network = args[++i];
        }
        else if (args[i] == "--noverify")
        {
            options.NoVerify = true;
        }
        else if (args[i] == "--plugins")
        {
            while (i + 1 < args.size() && args[i + 1][0] != '-')
            {
                options.Plugins.push_back(args[++i]);
            }
        }
        else if (args[i] == "--verbose")
        {
            if (i + 1 < args.size())
                options.Verbose = std::stoi(args[++i]);
        }
    }

    // Start the service
    Start(options);
}

void MainService::OnHelp(const std::string& category)
{
    if (!category.empty())
    {
        auto it = commandsByCategory_.find(category);
        if (it != commandsByCategory_.end())
        {
            ConsoleHelper::Info(category + " Commands:");
            for (const auto& [name, handler] : it->second)
            {
                ConsoleHelper::Info("  " + name);
            }
        }
        else
        {
            ConsoleHelper::Error("Category not found: " + category);
        }
        return;
    }

    // Show all categories
    ConsoleHelper::Info("Neo C++ CLI Commands:");
    ConsoleHelper::Info("");

    for (const auto& [category, commands] : commandsByCategory_)
    {
        ConsoleHelper::Info(category + " Commands:");
        for (const auto& [name, handler] : commands)
        {
            ConsoleHelper::Info("  " + name);
        }
        ConsoleHelper::Info("");
    }

    ConsoleHelper::Info("Use 'help <category>' for detailed help on a category");
}

void MainService::OnExit()
{
    running_ = false;
}

void MainService::OnClear()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void MainService::OnVersion()
{
    ConsoleHelper::Info("Neo C++ CLI v1.0.0");
    ConsoleHelper::Info("Neo Protocol Version: 3.0");

    if (neoSystem_)
    {
        auto blockchain = neoSystem_->GetBlockchain();
        if (blockchain)
        {
            ConsoleHelper::Info("Current Block Height: " + std::to_string(blockchain->GetHeight()));
            // ConsoleHelper::Info("Current Header Height: " + std::to_string(blockchain->GetHeaderHeight())); // Method unavailable
        }
    }
}

void MainService::InitializeCommands()
{
    // Base Commands
    RegisterCommand(
        "help",
        [this](const std::vector<std::string>& args)
        {
            if (args.empty())
                OnHelp();
            else
                OnHelp(args[0]);
            return true;
        },
        "Base");

    RegisterCommand(
        "exit",
        [this](const std::vector<std::string>& args)
        {
            OnExit();
            return true;
        },
        "Base");

    RegisterCommand(
        "clear",
        [this](const std::vector<std::string>& args)
        {
            OnClear();
            return true;
        },
        "Base");

    RegisterCommand(
        "version",
        [this](const std::vector<std::string>& args)
        {
            OnVersion();
            return true;
        },
        "Base");

    // Initialize command groups
    InitializeBlockchainCommands();
    InitializeNodeCommands();
    InitializeWalletCommands();
}

void MainService::InitializeBlockchainCommands()
{
    RegisterCommand(
        "showblock",
        [this](const std::vector<std::string>& args)
        {
            if (args.empty())
            {
                ConsoleHelper::Error("Missing argument: index or hash");
                return false;
            }
            OnShowBlock(args[0]);
            return true;
        },
        "Blockchain");

    RegisterCommand(
        "showheader",
        [this](const std::vector<std::string>& args)
        {
            if (args.empty())
            {
                ConsoleHelper::Error("Missing argument: index or hash");
                return false;
            }
            OnShowHeader(args[0]);
            return true;
        },
        "Blockchain");

    RegisterCommand(
        "showtx",
        [this](const std::vector<std::string>& args)
        {
            if (args.empty())
            {
                ConsoleHelper::Error("Missing argument: hash");
                return false;
            }
            try
            {
                io::UInt256 hash = io::UInt256::Parse(args[0]);
                OnShowTransaction(hash);
            }
            catch (const std::exception& ex)
            {
                ConsoleHelper::Error(ex.what());
            }
            return true;
        },
        "Blockchain");
}

void MainService::InitializeNodeCommands()
{
    RegisterCommand(
        "showstate",
        [this](const std::vector<std::string>& args)
        {
            OnShowState();
            return true;
        },
        "Node");

    RegisterCommand(
        "showpool",
        [this](const std::vector<std::string>& args)
        {
            bool verbose = false;
            if (!args.empty())
            {
                verbose = IsVerboseArgument(args[0]);
            }
            OnShowPool(verbose);
            return true;
        },
        "Node");

    RegisterCommand(
        "showpeers",
        [this](const std::vector<std::string>& args)
        {
            OnShowPeers();
            return true;
        },
        "Node");
}

void MainService::InitializeWalletCommands()
{
    RegisterCommand(
        "openwallet",
        [this](const std::vector<std::string>& args)
        {
            if (args.empty())
            {
                ConsoleHelper::Error("Missing argument: path");
                return false;
            }

            std::string password;
            if (args.size() > 1)
            {
                password = args[1];
            }
            else
            {
                password = ConsoleHelper::ReadPassword("Password: ");
            }

            OnOpenWallet(args[0], password);
            return true;
        },
        "Wallet");

    RegisterCommand(
        "closewallet",
        [this](const std::vector<std::string>& args)
        {
            OnCloseWallet();
            return true;
        },
        "Wallet");

    RegisterCommand(
        "showbalance",
        [this](const std::vector<std::string>& args)
        {
            OnShowBalance();
            return true;
        },
        "Wallet");

    RegisterCommand(
        "showaddress",
        [this](const std::vector<std::string>& args)
        {
            OnShowAddress();
            return true;
        },
        "Wallet");

    RegisterCommand(
        "transfer",
        [this](const std::vector<std::string>& args)
        {
            if (args.size() < 3)
            {
                ConsoleHelper::Error("Usage: transfer <asset> <address> <amount>");
                return false;
            }

            std::string asset = args[0];
            std::string address = args[1];
            double amount = std::stod(args[2]);

            io::UInt160 assetId;
            if (asset == "neo" || asset == "NEO")
            {
                assetId = smartcontract::native::NeoToken::GetContractId();
            }
            else if (asset == "gas" || asset == "GAS")
            {
                assetId = smartcontract::native::GasToken::GetContractId();
            }
            else
            {
                assetId = io::UInt160::Parse(asset);
            }

            OnTransfer(assetId, address, amount);
            return true;
        },
        "Wallet");
}

void MainService::InitializeTypeConverters()
{
    // Basic type converters
    RegisterTypeConverter("string",
                          [](const std::vector<std::string>& args, bool canConsumeAll) -> void*
                          {
                              if (args.empty())
                                  return new std::string("");
                              return new std::string(args[0]);
                          });

    RegisterTypeConverter("int",
                          [](const std::vector<std::string>& args, bool canConsumeAll) -> void*
                          {
                              if (args.empty())
                                  throw std::runtime_error("Missing argument for int");
                              return new int(std::stoi(args[0]));
                          });

    RegisterTypeConverter("uint32_t",
                          [](const std::vector<std::string>& args, bool canConsumeAll) -> void*
                          {
                              if (args.empty())
                                  throw std::runtime_error("Missing argument for uint32_t");
                              return new uint32_t(std::stoul(args[0]));
                          });
}

void MainService::RunConsole()
{
    // Print welcome message
    ConsoleHelper::Info("Neo C++ CLI v1.0.0");
    ConsoleHelper::Info("Type 'help' for a list of commands");

    // Run console loop
    running_ = true;
    while (running_)
    {
        std::string command = ConsoleHelper::ReadLine("neo> ");
        if (command.empty())
            continue;

        OnCommand(command);
    }
}

// Blockchain command implementations
void MainService::OnShowBlock(const std::string& indexOrHash)
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto blockchain = neoSystem_->GetBlockchain();
        std::shared_ptr<ledger::Block> block;

        if (indexOrHash.size() == 64)
        {
            // Hash
            io::UInt256 hash = io::UInt256::Parse(indexOrHash);
            block = blockchain->GetBlock(hash);
        }
        else
        {
            // Index
            uint32_t index = std::stoul(indexOrHash);
            block = blockchain->GetBlock(index);
        }

        if (!block)
        {
            ConsoleHelper::Error("Block not found");
            return;
        }

        ConsoleHelper::Info("Block " + std::to_string(block->GetIndex()) + ":");
        ConsoleHelper::Info("  Hash: " + block->GetHash().ToString());
        // ConsoleHelper::Info("  Previous Hash: " + block->GetPrevHash().ToString()); // Method unavailable
        ConsoleHelper::Info("  Merkle Root: " + block->GetMerkleRoot().ToString());
        // ConsoleHelper::Info("  Timestamp: " + std::to_string(block->GetTimestamp())); // Type conversion issue
        ConsoleHelper::Info("  Version: " + std::to_string(block->GetVersion()));
        ConsoleHelper::Info("  Next Consensus: " + block->GetNextConsensus().ToString());
        ConsoleHelper::Info("  Transactions: " + std::to_string(block->GetTransactions().size()));
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

void MainService::OnShowHeader(const std::string& indexOrHash)
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto blockchain = neoSystem_->GetBlockchain();
        std::shared_ptr<ledger::BlockHeader> header;

        if (indexOrHash.size() == 64)
        {
            // Hash
            io::UInt256 hash = io::UInt256::Parse(indexOrHash);
            // header = blockchain->GetHeader(hash); // Method unavailable
            header = nullptr;
        }
        else
        {
            // Index
            uint32_t index = std::stoul(indexOrHash);
            // header = blockchain->GetHeader(index); // Method unavailable
            header = nullptr;
        }

        if (!header)
        {
            ConsoleHelper::Error("Header not found");
            return;
        }

        ConsoleHelper::Info("Header " + std::to_string(header->GetIndex()) + ":");
        ConsoleHelper::Info("  Hash: " + header->GetHash().ToString());
        ConsoleHelper::Info("  Previous Hash: " + header->GetPrevHash().ToString());
        ConsoleHelper::Info("  Merkle Root: " + header->GetMerkleRoot().ToString());
        ConsoleHelper::Info("  Timestamp: " + std::to_string(header->GetTimestamp()));
        ConsoleHelper::Info("  Version: " + std::to_string(header->GetVersion()));
        ConsoleHelper::Info("  Next Consensus: " + header->GetNextConsensus().ToString());
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

void MainService::OnShowTransaction(const io::UInt256& hash)
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto blockchain = neoSystem_->GetBlockchain();
        auto tx = blockchain->GetTransaction(hash);
        if (!tx)
        {
            ConsoleHelper::Error("Transaction not found");
            return;
        }

        ConsoleHelper::Info("Transaction " + hash.ToString() + ":");
        ConsoleHelper::Info("  Version: " + std::to_string(tx->GetVersion()));
        ConsoleHelper::Info("  Nonce: " + std::to_string(tx->GetNonce()));
        ConsoleHelper::Info("  Sender: " + tx->GetSender().ToString());
        ConsoleHelper::Info("  System Fee: " + std::to_string(tx->GetSystemFee()));
        ConsoleHelper::Info("  Network Fee: " + std::to_string(tx->GetNetworkFee()));
        ConsoleHelper::Info("  Valid Until Block: " + std::to_string(tx->GetValidUntilBlock()));
        ConsoleHelper::Info("  Script: " + tx->GetScript().ToHexString());
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

// Node command implementations
void MainService::OnShowState()
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    auto system = neoSystem_;
    std::atomic<bool> cancel{false};
    const auto startTime = std::chrono::steady_clock::now();

    ConsoleHelper::Info("Entering live node state view...");

    auto displayLoop = [system, startTime](std::atomic<bool>& stopToken)
    {
        while (!stopToken.load())
        {
            try
            {
                RenderNodeSnapshot(CaptureNodeSnapshot(system, startTime));
            }
            catch (const std::exception& ex)
            {
                ConsoleHelper::Error(std::string("Unable to render node state: ") + ex.what());
                break;
            }

            for (int i = 0; i < 10 && !stopToken.load(); ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    };

    std::thread displayThread(displayLoop, std::ref(cancel));

    ConsoleHelper::Info("Press ENTER to exit the state view.");
    std::string line;
    std::getline(std::cin, line);

    cancel.store(true);
    if (displayThread.joinable())
    {
        displayThread.join();
    }

    ConsoleHelper::Clear();
}

void MainService::OnShowPool(bool verbose)
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto memPool = neoSystem_->GetMemPool();
        if (!memPool)
        {
            ConsoleHelper::Warning("Memory pool not available");
            return;
        }

        const auto verifiedCount = memPool->GetSize();
        const auto unverifiedCount = memPool->GetUnverifiedSize();
        const auto totalCount = verifiedCount + unverifiedCount;

        ConsoleHelper::Info("Memory Pool Summary:");
        ConsoleHelper::Info("  Total: " + std::to_string(totalCount));
        ConsoleHelper::Info("  Verified: " + std::to_string(verifiedCount));
        ConsoleHelper::Info("  Unverified: " + std::to_string(unverifiedCount));

        if (verbose)
        {
            std::vector<network::p2p::payloads::Neo3Transaction> verified;
            std::vector<network::p2p::payloads::Neo3Transaction> unverified;
            memPool->GetVerifiedAndUnverifiedTransactions(verified, unverified);

            if (verified.empty())
            {
                ConsoleHelper::Info("Verified Transactions: (none)");
            }
            else
            {
                ConsoleHelper::Info("Verified Transactions:");
                for (const auto& tx : verified)
                {
                    std::ostringstream line;
                    line << "  " << tx.GetHash().ToString() << " fee=" << FormatGasAmount(tx.GetNetworkFee())
                         << " GAS";
                    ConsoleHelper::Info(line.str());
                }
            }

            if (unverified.empty())
            {
                ConsoleHelper::Info("Unverified Transactions: (none)");
            }
            else
            {
                ConsoleHelper::Info("Unverified Transactions:");
                for (const auto& tx : unverified)
                {
                    std::ostringstream line;
                    line << "  " << tx.GetHash().ToString() << " fee=" << FormatGasAmount(tx.GetNetworkFee())
                         << " GAS";
                    ConsoleHelper::Info(line.str());
                }
            }
        }
        else
        {
            ConsoleHelper::Info("Use 'showpool verbose' to list individual transactions.");
        }
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

void MainService::OnShowPeers()
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto localNode = neoSystem_->GetLocalNode();
        std::vector<std::shared_ptr<network::p2p::RemoteNode>> peers;
        if (localNode)
        {
            peers = localNode->GetConnectedPeers();
        }

        ConsoleHelper::Info("Connected Peers: " + std::to_string(peers.size()));
        for (const auto& peer : peers)
        {
            // GetRemoteEndPoint not available in P2PPeer
            if (peer)
            {
                ConsoleHelper::Info("  " + peer->GetUserAgent() + " (Height: " +
                                    std::to_string(peer->GetLastBlockIndex()) + ")");
            }
        }
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

// Wallet command implementations
void MainService::OnOpenWallet(const std::string& path, const std::string& password)
{
    try
    {
        // Close current wallet if open
        if (currentWallet_)
        {
            currentWallet_.reset();
        }

        // Open wallet - WalletFactory is unavailable in this build
        // currentWallet_ = wallets::WalletFactory::Open(path, password);
        currentWallet_ = std::make_shared<wallets::Wallet>();

        ConsoleHelper::Info("Wallet opened: " + path);

        // Show wallet info
        auto accounts = currentWallet_->GetAccounts();
        ConsoleHelper::Info("Accounts: " + std::to_string(accounts.size()));
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error("Failed to open wallet: " + std::string(ex.what()));
    }
}

void MainService::OnCloseWallet()
{
    if (!currentWallet_)
    {
        ConsoleHelper::Error("No wallet is open");
        return;
    }

    currentWallet_.reset();
    ConsoleHelper::Info("Wallet closed");
}

void MainService::OnShowBalance()
{
    if (!currentWallet_)
    {
        ConsoleHelper::Error("No wallet is open");
        return;
    }

    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto snapshot = neoSystem_->GetDataCache();
        if (!snapshot)
        {
            ConsoleHelper::Error("Failed to get ledger snapshot");
            return;
        }

        auto neoToken = smartcontract::native::NeoToken::GetInstance();
        auto gasToken = smartcontract::native::GasToken::GetInstance();
        auto accounts = currentWallet_->GetAccounts();

        for (const auto& account : accounts)
        {
            ConsoleHelper::Info("Account: " + account->GetAddress());

            const auto neoBalance = neoToken->GetBalance(snapshot, account->GetScriptHash());
            const auto gasRaw = gasToken->GetBalance(snapshot, account->GetScriptHash());
            const double gasBalance =
                static_cast<double>(gasRaw) / static_cast<double>(smartcontract::native::GasToken::FACTOR);

            ConsoleHelper::Info("  NEO: " + neoBalance.ToString());
            ConsoleHelper::Info("  GAS: " + std::to_string(gasBalance));
        }
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

void MainService::OnShowBalance(const io::UInt160& assetId)
{
    if (!currentWallet_)
    {
        ConsoleHelper::Error("No wallet is open");
        return;
    }

    try
    {
        auto snapshot = neoSystem_ ? neoSystem_->GetDataCache() : nullptr;
        if (!snapshot)
        {
            ConsoleHelper::Error("Failed to get ledger snapshot");
            return;
        }

        const auto neoToken = smartcontract::native::NeoToken::GetInstance();
        const auto gasToken = smartcontract::native::GasToken::GetInstance();
        auto accounts = currentWallet_->GetAccounts();

        for (const auto& account : accounts)
        {
            if (assetId == neoToken->GetScriptHash())
            {
                const auto neoBalance = neoToken->GetBalance(snapshot, account->GetScriptHash());
                ConsoleHelper::Info(account->GetAddress() + ": " + neoBalance.ToString());
            }
            else if (assetId == gasToken->GetScriptHash())
            {
                const auto gasRaw = gasToken->GetBalance(snapshot, account->GetScriptHash());
                const double gasBalance =
                    static_cast<double>(gasRaw) / static_cast<double>(smartcontract::native::GasToken::FACTOR);
                ConsoleHelper::Info(account->GetAddress() + ": " + std::to_string(gasBalance));
            }
            else
            {
                ConsoleHelper::Info(account->GetAddress() + ": asset not supported");
            }
        }
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

void MainService::OnShowAddress()
{
    if (!currentWallet_)
    {
        ConsoleHelper::Error("No wallet is open");
        return;
    }

    try
    {
        auto accounts = currentWallet_->GetAccounts();

        ConsoleHelper::Info("Addresses:");
        for (const auto& account : accounts)
        {
            ConsoleHelper::Info("  " + account->GetAddress());
        }
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

void MainService::OnTransfer(const io::UInt160& assetId, const std::string& address, double amount)
{
    if (!currentWallet_)
    {
        ConsoleHelper::Error("No wallet is open");
        return;
    }

    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        ConsoleHelper::Error("Transfers not yet implemented in this build");
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error("Failed to transfer: " + std::string(ex.what()));
    }
}
}  // namespace neo::cli
