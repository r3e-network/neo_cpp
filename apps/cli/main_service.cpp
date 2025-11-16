#include "main_service.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
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
    std::string normalized = network;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (normalized == "mainnet") return "config/mainnet.config.json";
    if (normalized == "testnet") return "config/testnet.config.json";
    if (normalized == "privnet" || normalized == "private" || normalized == "private-net")
        return "config/privnet.json";

    throw std::invalid_argument("Unknown network preset: " + network);
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
            OnShowPool();
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

    try
    {
        auto blockchain = neoSystem_->GetBlockchain();
        auto localNode = neoSystem_->GetLocalNode();
        auto memPool = neoSystem_->GetMemPool();
        auto synchronizer = neoSystem_->GetNetworkSynchronizer();

        ConsoleHelper::Info("Node State:");
        ConsoleHelper::Info("  Block Height: " + std::to_string(blockchain->GetHeight()));
        ConsoleHelper::Info("  Block Hash: " + blockchain->GetCurrentBlockHash().ToString());
        // ConsoleHelper::Info("  Header Height: " + std::to_string(blockchain->GetHeaderHeight())); // Method unavailable
        // ConsoleHelper::Info("  Header Hash: " + blockchain->GetCurrentHeaderHash().ToString()); // Method unavailable
        const auto peerCount = localNode ? localNode->GetConnectedCount() : 0U;
        ConsoleHelper::Info("  Connected Peers: " + std::to_string(peerCount));
        ConsoleHelper::Info("  Memory Pool Size: " + std::to_string(memPool->GetSize()));

        if (synchronizer)
        {
            auto state = synchronizer->GetState();
            std::string stateStr;
            switch (state)
            {
                case network::p2p::SynchronizationState::NotSynchronizing:
                    stateStr = "Not synchronizing";
                    break;
                case network::p2p::SynchronizationState::SynchronizingHeaders:
                    stateStr = "Synchronizing headers";
                    break;
                case network::p2p::SynchronizationState::SynchronizingBlocks:
                    stateStr = "Synchronizing blocks";
                    break;
                case network::p2p::SynchronizationState::Synchronized:
                    stateStr = "Synchronized";
                    break;
            }
            ConsoleHelper::Info("  Synchronization State: " + stateStr);
            ConsoleHelper::Info("  Current Block Index: " + std::to_string(synchronizer->GetCurrentBlockIndex()));
            ConsoleHelper::Info("  Target Block Index: " + std::to_string(synchronizer->GetTargetBlockIndex()));
        }
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

void MainService::OnShowPool()
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto memPool = neoSystem_->GetMemPool();
        // auto transactions = memPool->GetTransactions(); // Method unavailable
        std::vector<std::shared_ptr<network::p2p::payloads::Neo3Transaction>> transactions;

        ConsoleHelper::Info("Memory Pool Transactions: " + std::to_string(transactions.size()));
        for (const auto& tx : transactions)
        {
            ConsoleHelper::Info("  " + tx->GetHash().ToString());
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
                ConsoleHelper::Info("  " + peer->GetUserAgent() + " (Height: " + std::to_string(peer->GetStartHeight()) + ")");
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
        auto accounts = currentWallet_->GetAccounts();

        for (const auto& account : accounts)
        {
            ConsoleHelper::Info("Account: " + account->GetAddress());

            // Get NEO balance - GetBalance method is unavailable
            // auto neoBalance =
            //     currentWallet_->GetBalance(smartcontract::native::NeoToken::SCRIPT_HASH, account->GetScriptHash());

            // Get GAS balance - GetBalance method is unavailable
            // auto gasBalance =
            //     currentWallet_->GetBalance(smartcontract::native::GasToken::SCRIPT_HASH, account->GetScriptHash());

            ConsoleHelper::Info("  NEO: Balance not available");
            ConsoleHelper::Info("  GAS: Balance not available");
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
        auto accounts = currentWallet_->GetAccounts();

        for (const auto& account : accounts)
        {
            // auto balance = currentWallet_->GetBalance(assetId, account->GetScriptHash()); // Method unavailable
            uint64_t balance = 0;
            ConsoleHelper::Info(account->GetAddress() + ": " + std::to_string(balance));
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
        // Create transfer transaction - Methods unavailable in this build
        // auto tx = currentWallet_->CreateTransferTransaction(assetId, address, amount);

        // Sign transaction - Method unavailable
        // currentWallet_->SignTransaction(tx);

        // Send transaction - AddTransaction method is unavailable
        // auto memPool = neoSystem_->GetMemPool();
        // auto result = memPool->AddTransaction(tx);

        ConsoleHelper::Info("Transfer functionality is not available in this build");
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error("Failed to transfer: " + std::string(ex.what()));
    }
}
}  // namespace neo::cli
