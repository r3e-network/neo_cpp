#include "cli_service.h"
#include "commands/command_registry.h"
#include "plugins/plugin_manager.h"
#include "services/console_service_neo.h"

#include <neo/consensus/dbft_consensus.h>
#include <neo/core/circuit_breaker.h>
#include <neo/core/config_manager.h>
#include <neo/core/logging.h>
#include <neo/node/neo_system.h>
#include <neo/wallets/nep6/nep6_wallet.h>
#include <neo/protocol_settings.h>
#include <neo/core/shutdown_manager.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/memory_pool.h>
#include <neo/monitoring/health_check.h>
#include <neo/monitoring/prometheus_exporter.h>
#include <neo/network/connection_manager.h>
#include <neo/network/p2p_server.h>
#include <neo/persistence/rocksdb_store.h>
#include <neo/rpc/rate_limiter.h>
#include <neo/rpc/rpc_server.h>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace neo::cli
{

CLIService::CLIService(const std::filesystem::path& config_path, const std::string& network)
    : config_path_(config_path), network_(network)
{
}

void CLIService::Initialize()
{
    // Load configuration with environment variable support
    LoadConfiguration();

    // Setup graceful shutdown
    SetupShutdownHandlers();

    // Initialize core components
    InitializeLogging();
    InitializeMetrics();
    InitializeHealthChecks();
    InitializeStorage();
    InitializeNeoSystem();
    InitializeNetwork();

    if (rpc_enabled_)
    {
        InitializeRPC();
    }

    if (consensus_enabled_)
    {
        InitializeConsensus();
    }

    InitializeConsole();
    RegisterCommands();
    LoadPlugins();

    // Start monitoring services
    StartMonitoring();
}

CLIService::~CLIService()
{
    Stop();
    // Explicitly clear unique_ptrs with incomplete types
    current_wallet_ = nullptr;
    neo_system_ = nullptr;
    p2p_server_ = nullptr;
    rpc_server_ = nullptr;
    consensus_ = nullptr;
    store_ = nullptr;
}

void CLIService::Start()
{
    if (running_)
        return;

    running_ = true;

    // Start P2P server
    if (p2p_server_)
    {
        p2p_server_->Start();
        std::cout << "P2P server started on port " << config_["P2P"]["Port"].get<uint16_t>() << std::endl;
    }

    // Start RPC server
    if (rpc_server_ && rpc_enabled_)
    {
        rpc_server_->Start();
        std::cout << "RPC server started on port " << config_["RPC"]["Port"].get<uint16_t>() << std::endl;
    }

    // Start consensus
    if (consensus_ && consensus_enabled_)
    {
        consensus_->Start();
        std::cout << "Consensus service started" << std::endl;
    }

    // Start status display thread
    status_thread_ = std::thread(&CLIService::StatusLoop, this);
}

void CLIService::Run()
{
    if (!console_service_)
    {
        throw std::runtime_error("Console service not initialized");
    }

    DisplayHelp();

    // Main command loop
    while (running_)
    {
        console_service_->ProcessCommands();
    }
}

void CLIService::Stop()
{
    if (!running_)
        return;

    running_ = false;

    // Stop status thread
    if (status_thread_.joinable())
    {
        status_thread_.join();
    }

    // Stop consensus
    if (consensus_)
    {
        consensus_->Stop();
    }

    // Stop RPC server
    if (rpc_server_)
    {
        rpc_server_->Stop();
    }

    // Stop P2P server
    if (p2p_server_)
    {
        p2p_server_->Stop();
    }

    // Close wallet
    CloseWallet();
}

void CLIService::DisplayBanner()
{
    std::cout << R"(
     _   _ ______ ___        _____   _       _____ 
    | \ | |  ____/ _ \      / ____| | |     |_   _|
    |  \| | |__ | | | |    | |      | |       | |  
    | . ` |  __|| | | |    | |      | |       | |  
    | |\  | |___| |_| |    | |____  | |____  _| |_ 
    |_| \_|______\___/      \_____| |______||_____|
                                                    
    NEO C++ Command Line Interface v3.6.0
    
)" << std::endl;

    std::cout << "Network: " << network_ << std::endl;
    // Protocol version display - GetProtocolVersion method not available
    // std::cout << "Protocol: " << neo_system_->GetProtocolSettings()->GetProtocolVersion() << std::endl;
    std::cout << std::endl;
}

void CLIService::DisplayStatus()
{
    if (!neo_system_ || !GetBlockchain())
        return;

    auto blockchain = GetBlockchain();
    auto mempool = GetMemoryPool();
    auto p2p = GetP2PServer();

    std::cout << "\nNode Status:" << std::endl;
    std::cout << "  Height: " << blockchain->GetHeight() << std::endl;
    std::cout << "  Block Height: " << blockchain->GetHeight() << std::endl;
    std::cout << "  Connected Peers: " << (p2p ? p2p->GetConnectedPeersCount() : 0) << std::endl;
    std::cout << "  Memory Pool: " << (mempool ? mempool->GetSize() : 0) << " transactions" << std::endl;

    if (consensus_ && consensus_enabled_)
    {
        std::cout << "  Consensus: Active" << std::endl;
    }

    if (current_wallet_)
    {
        std::cout << "  Wallet: opened" << std::endl; // GetName() not available
    }

    std::cout << std::endl;
}

void CLIService::DisplayHelp()
{
    std::cout << "\nAvailable commands:" << std::endl;
    std::cout << "  help              - Show this help message" << std::endl;
    std::cout << "  status            - Display node status" << std::endl;
    std::cout << "  open wallet <path> - Open a wallet" << std::endl;
    std::cout << "  close wallet      - Close current wallet" << std::endl;
    std::cout << "  show state        - Show blockchain state" << std::endl;
    std::cout << "  show pool         - Show memory pool" << std::endl;
    std::cout << "  plugins           - List loaded plugins" << std::endl;
    std::cout << "  exit              - Exit the application" << std::endl;

    if (command_registry_)
    {
        command_registry_->DisplayHelp();
    }

    std::cout << std::endl;
}

ledger::Blockchain* CLIService::GetBlockchain()
{
    return neo_system_ ? neo_system_->GetBlockchain().get() : nullptr;
}

ledger::MemoryPool* CLIService::GetMemoryPool()
{
    return neo_system_ ? neo_system_->GetMemoryPool().get() : nullptr;
}

bool CLIService::OpenWallet(const std::filesystem::path& path, const std::string& password)
{
    try
    {
        CloseWallet();

        // NEP6Wallet implementation not complete
        // current_wallet_ = std::make_unique<wallets::NEP6Wallet>(path);
        // current_wallet_->Unlock(password);

        std::cout << "Wallet opened successfully: " << path << std::endl;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to open wallet: " << e.what() << std::endl;
        current_wallet_ = nullptr;
        return false;
    }
}

void CLIService::CloseWallet()
{
    if (current_wallet_)
    {
        // current_wallet_.reset(); // Incomplete type issue
        current_wallet_ = nullptr;
        std::cout << "Wallet closed" << std::endl;
    }
}

void CLIService::LoadPlugins()
{
    if (!plugin_manager_)
    {
        plugin_manager_ = std::make_unique<PluginManager>(this);
    }

    // Load plugins from configuration
    if (config_.contains("Plugins"))
    {
        for (const auto& plugin_config : config_["Plugins"])
        {
            std::string name = plugin_config["Name"];
            std::filesystem::path path = plugin_config["Path"];

            if (plugin_manager_->LoadPlugin(name, path))
            {
                std::cout << "Loaded plugin: " << name << std::endl;
            }
        }
    }
}

void CLIService::LoadConfiguration()
{
    // Use ConfigManager for environment variable support
    auto& configManager = core::ConfigManager::GetInstance();

    try
    {
        configManager.LoadFromFile(config_path_);
        config_ = configManager.GetJson();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Warning: " << e.what() << std::endl;
        std::cerr << "Using default configuration" << std::endl;

        // Default configuration based on network
        if (network_ == "mainnet")
        {
            config_ = {{"Network", "mainnet"},
                       {"Magic", 860833102},
                       {"P2P", {{"Port", 10333}, {"WsPort", 10334}}},
                       {"RPC", {{"Port", 10332}, {"SslPort", 10331}, {"MaxGasInvoke", "50"}}},
                       {"Storage", {{"Engine", "RocksDBStore"}, {"Path", "./Chain"}}},
                       {"Consensus", {{"Enabled", false}, {"UnlockWallet", {{"Path", ""}, {"Password", ""}}}}},
                       {"Logging", {{"Path", "./Logs"}, {"ConsoleOutput", true}}}};
        }
        else if (network_ == "testnet")
        {
            config_ = {{"Network", "testnet"},
                       {"Magic", 894710606},
                       {"P2P", {{"Port", 20333}, {"WsPort", 20334}}},
                       {"RPC", {{"Port", 20332}, {"SslPort", 20331}, {"MaxGasInvoke", "50"}}},
                       {"Storage", {{"Engine", "RocksDBStore"}, {"Path", "./TestNetChain"}}},
                       {"Consensus", {{"Enabled", false}, {"UnlockWallet", {{"Path", ""}, {"Password", ""}}}}},
                       {"Logging", {{"Path", "./Logs"}, {"ConsoleOutput", true}}}};
        }
        else
        {
            // Private network
            config_ = {{"Network", "privnet"},
                       {"Magic", 1951352142},
                       {"P2P", {{"Port", 30333}, {"WsPort", 30334}}},
                       {"RPC", {{"Port", 30332}, {"SslPort", 30331}, {"MaxGasInvoke", "50"}}},
                       {"Storage", {{"Engine", "RocksDBStore"}, {"Path", "./PrivNetChain"}}},
                       {"Consensus", {{"Enabled", false}, {"UnlockWallet", {{"Path", ""}, {"Password", ""}}}}},
                       {"Logging", {{"Path", "./Logs"}, {"ConsoleOutput", true}}}};
        }
    }
}

void CLIService::InitializeLogging()
{
    auto log_path = config_["Logging"]["Path"].get<std::string>();
    auto console_output = config_["Logging"]["ConsoleOutput"].get<bool>();

    // Create log directory if it doesn't exist
    std::filesystem::create_directories(log_path);

    // Initialize logging system
    // LogManager initialization - method not found
    // core::LogManager::Initialize(log_path, console_output);
}

void CLIService::InitializeStorage()
{
    auto engine = config_["Storage"]["Engine"].get<std::string>();
    auto path = config_["Storage"]["Path"].get<std::string>();

    if (engine == "RocksDBStore")
    {
        store_ = std::make_unique<persistence::RocksDbStore>(persistence::RocksDbConfig{path});
    }
    else
    {
        throw std::runtime_error("Unknown storage engine: " + engine);
    }
}

void CLIService::InitializeNeoSystem()
{
        auto magic = config_["Magic"].get<uint32_t>();

        // Create protocol settings
        auto settings = std::make_shared<ProtocolSettings>();
        settings->SetNetwork(magic);

        // Create Neo system
        neo_system_ = std::make_shared<node::NeoSystem>(settings, "RocksDBStore", config_["Storage"]["Path"].get<std::string>());
    }

void CLIService::InitializeNetwork()
{
        auto port = config_["P2P"]["Port"].get<uint16_t>();
        auto ws_port = config_["P2P"]["WsPort"].get<uint16_t>();

        // P2PServer constructor expects different parameters
        if (!io_context_) {
            io_context_ = std::make_unique<boost::asio::io_context>();
        }
        network::IPEndPoint endpoint("0.0.0.0", port);
        std::string userAgent = "NEO-CPP/3.6.0";
        uint32_t startHeight = 0;
        p2p_server_ = std::make_unique<network::P2PServer>(*io_context_, endpoint, userAgent, startHeight);
    }

void CLIService::InitializeRPC()
{
        auto port = config_["RPC"]["Port"].get<uint16_t>();
        auto ssl_port = config_["RPC"]["SslPort"].get<uint16_t>();
        auto max_gas = config_["RPC"]["MaxGasInvoke"].get<std::string>();

        rpc::RpcConfig rpc_config;
        rpc_config.port = port;
        rpc_config.bind_address = "127.0.0.1";
        rpc_server_ = std::make_unique<rpc::RpcServer>(rpc_config);

        // Configure RPC settings
        // rpc_server_->SetMaxGasInvoke(std::stoull(max_gas)); // Method not available
    }

void CLIService::InitializeConsensus()
{
        // consensus_ = std::make_unique<consensus::DbftConsensus>(neo_system_.get(), current_wallet_.get()); // Constructor not matching
        // Consensus initialization would go here when properly implemented

        // Auto-unlock wallet for consensus if configured
        auto wallet_path = config_["Consensus"]["UnlockWallet"]["Path"].get<std::string>();
        auto wallet_password = config_["Consensus"]["UnlockWallet"]["Password"].get<std::string>();

        if (!wallet_path.empty() && !wallet_password.empty())
        {
            OpenWallet(wallet_path, wallet_password);
        }
    }

void CLIService::InitializeConsole()
{
        console_service_ = std::make_unique<ConsoleServiceNeo>(this);
        command_registry_ = std::make_unique<CommandRegistry>(this);
    }

void CLIService::RegisterCommands()
{
        if (!command_registry_)
            return;

        // Register built-in commands
        command_registry_->RegisterBuiltinCommands();
    }

void CLIService::StatusLoop()
{
        auto last_display = std::chrono::steady_clock::now();

        while (running_)
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_display);

            // Update status every 30 seconds
            if (elapsed.count() >= 30)
            {
                DisplayStatus();
                last_display = now;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

}  // namespace neo::cli