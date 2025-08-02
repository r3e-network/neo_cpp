#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/io/json.h>
#include <neo/consensus/dbft_consensus.h>
#include <neo/node/neo_node.h>
#include <neo/rpc/rpc_server.h>
#include <neo/persistence/rocksdb_store.h>
#include <neo/protocol_settings.h>
// #include <neo/persistence/store_provider.h> // File not found
#include <neo/plugins/plugin.h>
#include <neo/plugins/rpc_plugin.h>
#include <neo/plugins/statistics_plugin.h>
#include <neo/wallets/wallet.h>
#include <neo/wallets/wallet_manager.h>
#include <string>
#include <thread>
#include <unordered_map>

using namespace neo;
using namespace neo::node;
using namespace neo::rpc;
using namespace neo::consensus;
using namespace neo::persistence;
using namespace neo::cryptography::ecc;
using namespace neo::wallets;
using namespace neo::plugins;

// Global variables
std::shared_ptr<node::NeoNode> g_node;
std::shared_ptr<rpc::RpcServer> g_rpcServer;
std::shared_ptr<consensus::DbftConsensus> g_consensusService;
std::shared_ptr<Wallet> g_wallet;
bool g_running = true;

// Register plugin factories
void RegisterPlugins()
{
    // auto& manager = PluginManager::GetInstance(); // PluginManager not declared

    // Register RPC plugin
    // manager.RegisterPluginFactory(std::make_shared<RPCPluginFactory>());

    // Register Statistics plugin
    // manager.RegisterPluginFactory(std::make_shared<StatisticsPluginFactory>());
}

// Signal handler
void SignalHandler(int signal)
{
    std::cout << "Received signal " << signal << std::endl;
    g_running = false;
}

// Load settings from config file
std::unordered_map<std::string, std::string> LoadSettings(const std::string& configFile)
{
    std::unordered_map<std::string, std::string> settings;

    try
    {
        // Open config file
        std::ifstream file(configFile);
        if (!file.is_open())
        {
            std::cerr << "Failed to open config file: " << configFile << std::endl;
            return settings;
        }

        // Parse JSON
        nlohmann::json json;
        file >> json;

        // Load settings
        for (auto it = json.begin(); it != json.end(); ++it)
        {
            settings[it.key()] = it.value().get<std::string>();
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to load settings: " << ex.what() << std::endl;
    }

    return settings;
}

// Load key pair from WIF
KeyPair LoadKeyPair(const std::string& wif)
{
    try
    {
        return Secp256r1::FromWIF(wif);
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to load key pair: " << ex.what() << std::endl;
        return Secp256r1::GenerateKeyPair();
    }
}

int main(int argc, char* argv[])
{
    // Register signal handlers
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // Register plugins
    RegisterPlugins();

    // Parse command line arguments
    std::string configFile = "config.json";
    if (argc > 1)
    {
        configFile = argv[1];
    }

    // Load settings
    auto settings = LoadSettings(configFile);

    // Set default settings
    if (settings.find("DataPath") == settings.end())
        settings["DataPath"] = "data";

    if (settings.find("P2PPort") == settings.end())
        settings["P2PPort"] = "10333";

    if (settings.find("RPCPort") == settings.end())
        settings["RPCPort"] = "10332";

    if (settings.find("MemoryPoolCapacity") == settings.end())
        settings["MemoryPoolCapacity"] = "50000";

    if (settings.find("WalletPath") == settings.end())
        settings["WalletPath"] = "wallet.json";

    // Create store provider
    std::shared_ptr<persistence::RocksDbStore> store;
    try
    {
        persistence::RocksDbConfig dbConfig;
        dbConfig.db_path = settings["DataPath"];
        store = std::make_shared<RocksDbStore>(dbConfig);
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to create store provider: " << ex.what() << std::endl;
        return 1;
    }

    // Create node
    try
    {
        // NeoNode constructor expects different parameters
        auto protocolSettings = std::make_shared<ProtocolSettings>();
        protocolSettings->SetNetwork(0x014F5448); // MainNet magic
        g_node = std::make_shared<NeoNode>(protocolSettings, "RocksDBStore", settings["DataPath"]);
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to create node: " << ex.what() << std::endl;
        return 1;
    }

    // Create RPC server
    try
    {
        // RpcServer constructor expects RpcConfig
        rpc::RpcConfig rpcConfig;
        rpcConfig.port = std::stoul(settings["RPCPort"]);
        rpcConfig.bind_address = "0.0.0.0";
        g_rpcServer = std::make_shared<RpcServer>(rpcConfig);
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to create RPC server: " << ex.what() << std::endl;
        return 1;
    }

    // Load or create wallet
    try
    {
        std::string walletPath = settings["WalletPath"];

        // Check if wallet exists
        if (std::filesystem::exists(walletPath))
        {
            // Open wallet
            g_wallet = WalletManager::GetInstance().OpenWallet(walletPath);
            if (g_wallet)
                std::cout << "Opened wallet: " << walletPath << std::endl;
        }
        else
        {
            // Create wallet
            g_wallet = WalletManager::GetInstance().CreateWallet(walletPath);
            if (g_wallet)
                std::cout << "Created wallet: " << walletPath << std::endl;
        }

        if (!g_wallet)
            std::cerr << "Failed to load or create wallet: " << walletPath << std::endl;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to load or create wallet: " << ex.what() << std::endl;
    }

    // Create consensus service
    try
    {
        if (settings.find("WIF") != settings.end())
        {
            auto keyPair = LoadKeyPair(settings["WIF"]);
            // g_consensusService = std::make_shared<ConsensusService>(g_node, keyPair); // Type mismatch
            // Consensus service initialization commented out until proper implementation
        }
        else if (g_wallet && g_wallet->GetDefaultAccount() && g_wallet->GetDefaultAccount()->HasPrivateKey())
        {
            // Use wallet's default account for consensus
            auto account = g_wallet->GetDefaultAccount();
            auto privateKey = account->GetPrivateKey();
            auto keyPair = Secp256r1::FromPrivateKey(privateKey);
            // g_consensusService = std::make_shared<ConsensusService>(g_node, keyPair); // Type mismatch
            // Consensus service initialization commented out until proper implementation
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to create consensus service: " << ex.what() << std::endl;
    }

    // Start node
    try
    {
        std::cout << "Starting node..." << std::endl;
        g_node->Start();
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to start node: " << ex.what() << std::endl;
        return 1;
    }

    // Start RPC server
    try
    {
        std::cout << "Starting RPC server on port " << settings["RPCPort"] << "..." << std::endl;
        g_rpcServer->Start();
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to start RPC server: " << ex.what() << std::endl;
        g_node->Stop();
        return 1;
    }

    // Start consensus service
    if (g_consensusService)
    {
        try
        {
            std::cout << "Starting consensus service..." << std::endl;
            g_consensusService->Start();
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Failed to start consensus service: " << ex.what() << std::endl;
        }
    }

    // Load plugins
    try
    {
        std::cout << "Loading plugins..." << std::endl;
        // auto& manager = PluginManager::GetInstance(); // PluginManager not declared
        // Plugin loading code commented out until PluginManager is implemented
        /*
        if (manager.LoadPlugins(g_node, g_rpcServer, settings))
        {
            std::cout << "Loaded " << manager.GetPlugins().size() << " plugins" << std::endl;

            // Start plugins
            std::cout << "Starting plugins..." << std::endl;
            if (manager.StartPlugins())
            {
                std::cout << "Started plugins" << std::endl;
            }
            else
            {
                std::cerr << "Failed to start plugins" << std::endl;
            }
        }
        else
        {
            std::cerr << "Failed to load plugins" << std::endl;
        }
        */
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to load or start plugins: " << ex.what() << std::endl;
    }

    // Print node information
    std::cout << "Node is running" << std::endl;
    std::cout << "P2P server is listening on port " << settings["P2PPort"] << std::endl;
    std::cout << "RPC server is listening on port " << settings["RPCPort"] << std::endl;

    // Print wallet information
    if (g_wallet)
    {
        std::cout << "Wallet: " << g_wallet->GetPath() << std::endl;
        std::cout << "Accounts: " << g_wallet->GetAccounts().size() << std::endl;

        if (g_wallet->GetDefaultAccount())
        {
            std::cout << "Default account: " << g_wallet->GetDefaultAccount()->GetAddress() << std::endl;
        }
    }

    // Wait for signal
    while (g_running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Stop consensus service
    if (g_consensusService)
    {
        try
        {
            std::cout << "Stopping consensus service..." << std::endl;
            g_consensusService->Stop();
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Failed to stop consensus service: " << ex.what() << std::endl;
        }
    }

    // Stop plugins
    try
    {
        std::cout << "Stopping plugins..." << std::endl;
        // auto& manager = PluginManager::GetInstance(); // PluginManager not declared
        /*
        if (manager.StopPlugins())
        {
            std::cout << "Stopped plugins" << std::endl;
        }
        else
        {
            std::cerr << "Failed to stop plugins" << std::endl;
        }
        */
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to stop plugins: " << ex.what() << std::endl;
    }

    // Stop RPC server
    try
    {
        std::cout << "Stopping RPC server..." << std::endl;
        g_rpcServer->Stop();
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to stop RPC server: " << ex.what() << std::endl;
    }

    // Stop node
    try
    {
        std::cout << "Stopping node..." << std::endl;
        g_node->Stop();
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to stop node: " << ex.what() << std::endl;
    }

    // Close wallet
    try
    {
        if (g_wallet)
        {
            std::cout << "Closing wallet..." << std::endl;
            g_wallet->Save();
            WalletManager::GetInstance().CloseWallet(g_wallet);
            g_wallet = nullptr;
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to close wallet: " << ex.what() << std::endl;
    }

    std::cout << "Node stopped" << std::endl;

    return 0;
}
