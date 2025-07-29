#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <signal.h>
#include <thread>

// Core Neo components
#include <neo/consensus/dbft_consensus.h>
#include <neo/core/logging.h>
#include <neo/core/neo_system.h>
#include <neo/core/protocol_settings.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/memory_pool.h>
#include <neo/network/p2p_server.h>
#include <neo/persistence/data_cache.h>
#include <neo/persistence/rocksdb_store.h>
#include <neo/rpc/rpc_server.h>

// Native contracts
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/crypto_lib.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/oracle_contract.h>
#include <neo/smartcontract/native/policy_contract.h>
#include <neo/smartcontract/native/role_management.h>

// Wallet support
#include <neo/wallets/nep6/nep6_wallet.h>

// JSON configuration
#include <nlohmann/json.hpp>

using namespace neo;
using namespace neo::core;
using namespace neo::persistence;
using namespace neo::ledger;
using namespace neo::network;
using namespace neo::rpc;
using namespace neo::consensus;
using namespace neo::smartcontract::native;
using namespace neo::wallets;
using json = nlohmann::json;

// Global shutdown flag
std::atomic<bool> g_shutdown(false);

// Signal handler for graceful shutdown
void signal_handler(int signal)
{
    LOG_INFO("Received signal {}. Initiating graceful shutdown...", signal);
    g_shutdown = true;
}

/**
 * @brief Production-ready Neo blockchain node implementation
 *
 * This class implements a fully functional Neo node that is compatible
 * with the C# reference implementation and can participate in the Neo network.
 */
class ProductionNeoNode
{
  private:
    // Core components
    std::shared_ptr<NeoSystem> neoSystem_;
    std::shared_ptr<RocksDBStore> store_;
    std::shared_ptr<Blockchain> blockchain_;
    std::shared_ptr<MemoryPool> mempool_;
    std::shared_ptr<P2PServer> p2pServer_;
    std::shared_ptr<RpcServer> rpcServer_;
    std::shared_ptr<DbftConsensus> consensus_;

    // Configuration
    json config_;
    ProtocolSettings protocolSettings_;
    std::string dataPath_;
    std::string network_;

    // Wallet for consensus participation
    std::shared_ptr<NEP6Wallet> consensusWallet_;
    std::shared_ptr<cryptography::ecc::KeyPair> consensusKey_;

  public:
    ProductionNeoNode(const std::string& configPath = "config.json")
    {
        LOG_INFO("Initializing Neo C++ Production Node");

        // Load configuration
        LoadConfiguration(configPath);

        // Initialize all components in correct order
        InitializeLogging();
        InitializeProtocolSettings();
        InitializeStorage();
        InitializeNeoSystem();
        InitializeBlockchain();
        InitializeMemoryPool();
        InitializeP2PNetwork();
        InitializeRpcServer();
        InitializeConsensus();

        LOG_INFO("Neo node initialization complete");
    }

    ~ProductionNeoNode()
    {
        Shutdown();
    }

    void Start()
    {
        LOG_INFO("Starting Neo node on {} network", network_);

        try
        {
            // Start P2P network
            p2pServer_->Start();
            LOG_INFO("P2P server started on port {}", config_["P2P"]["Port"].get<uint16_t>());

            // Start RPC server
            rpcServer_->Start();
            LOG_INFO("RPC server started on port {}", config_["RPC"]["Port"].get<uint16_t>());

            // Start consensus if configured
            if (consensus_ && config_["Consensus"]["Enabled"].get<bool>())
            {
                consensus_->Start();
                LOG_INFO("Consensus service started");
            }

            // Display node information
            DisplayNodeInfo();

            // Main event loop
            MainLoop();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to start node: {}", e.what());
            throw;
        }
    }

    void Shutdown()
    {
        LOG_INFO("Shutting down Neo node...");

        // Stop services in reverse order
        if (consensus_)
            consensus_->Stop();
        if (rpcServer_)
            rpcServer_->Stop();
        if (p2pServer_)
            p2pServer_->Stop();

        // Flush blockchain data
        if (blockchain_)
            blockchain_->Flush();

        LOG_INFO("Neo node shutdown complete");
    }

  private:
    void LoadConfiguration(const std::string& configPath)
    {
        try
        {
            if (std::filesystem::exists(configPath))
            {
                std::ifstream file(configPath);
                file >> config_;
                LOG_INFO("Configuration loaded from {}", configPath);
            }
            else
            {
                // Use default configuration
                config_ = GetDefaultConfiguration();
                LOG_INFO("Using default configuration");
            }

            // Extract key settings
            network_ = config_["Network"].get<std::string>();
            dataPath_ = config_["DataPath"].get<std::string>();

            // Ensure data directory exists
            std::filesystem::create_directories(dataPath_);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to load configuration: {}", e.what());
            throw;
        }
    }

    json GetDefaultConfiguration()
    {
        return json{{"Network", "mainnet"},
                    {"DataPath", "./neo-data"},
                    {"P2P", {{"Port", 10333}, {"MaxConnections", 10}, {"MinDesiredConnections", 4}}},
                    {"RPC", {{"Port", 10332}, {"MaxConcurrentConnections", 40}, {"EnableCors", true}}},
                    {"Consensus", {{"Enabled", false}, {"WalletPath", ""}, {"WalletPassword", ""}}},
                    {"Logging", {{"Level", "info"}, {"Path", "./logs"}}}};
    }

    void InitializeLogging()
    {
        auto logPath = config_["Logging"]["Path"].get<std::string>();
        auto logLevel = config_["Logging"]["Level"].get<std::string>();

        std::filesystem::create_directories(logPath);

        Logger::Initialize("neo-node", logPath + "/neo-node.log");
        Logger::SetLevel(logLevel);

        LOG_INFO("Logging initialized - Level: {}", logLevel);
    }

    void InitializeProtocolSettings()
    {
        // Load protocol settings for the network
        if (network_ == "mainnet")
        {
            protocolSettings_ = ProtocolSettings::MainNet();
        }
        else if (network_ == "testnet")
        {
            protocolSettings_ = ProtocolSettings::TestNet();
        }
        else
        {
            protocolSettings_ = ProtocolSettings::Default();
        }

        LOG_INFO("Protocol settings loaded for {} network", network_);
    }

    void InitializeStorage()
    {
        try
        {
            auto dbPath = dataPath_ + "/chain";
            store_ = std::make_shared<RocksDBStore>(dbPath);
            LOG_INFO("Storage initialized at {}", dbPath);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to initialize storage: {}", e.what());
            throw;
        }
    }

    void InitializeNeoSystem()
    {
        neoSystem_ = std::make_shared<NeoSystem>(protocolSettings_, store_);

        // Initialize native contracts
        NeoToken::Initialize(neoSystem_.get());
        GasToken::Initialize(neoSystem_.get());
        ContractManagement::Initialize(neoSystem_.get());
        PolicyContract::Initialize(neoSystem_.get());
        RoleManagement::Initialize(neoSystem_.get());
        OracleContract::Initialize(neoSystem_.get());
        CryptoLib::Initialize(neoSystem_.get());

        LOG_INFO("NeoSystem and native contracts initialized");
    }

    void InitializeBlockchain()
    {
        blockchain_ = neoSystem_->GetBlockchain();

        // Load existing blockchain data or initialize genesis
        if (blockchain_->GetHeight() == 0)
        {
            LOG_INFO("Initializing genesis block");
            blockchain_->InitializeGenesis();
        }
        else
        {
            LOG_INFO("Blockchain loaded - Height: {}", blockchain_->GetHeight());
        }
    }

    void InitializeMemoryPool()
    {
        auto maxTxPerBlock = protocolSettings_.MaxTransactionsPerBlock;
        mempool_ = std::make_shared<MemoryPool>(maxTxPerBlock * 2);

        // Set transaction verifier
        mempool_->SetVerifier([this](const network::p2p::payloads::Neo3Transaction& tx)
                              { return blockchain_->VerifyTransaction(tx); });

        neoSystem_->SetMemoryPool(mempool_);
        LOG_INFO("Memory pool initialized - Capacity: {}", maxTxPerBlock * 2);
    }

    void InitializeP2PNetwork()
    {
        auto p2pPort = config_["P2P"]["Port"].get<uint16_t>();
        auto maxConnections = config_["P2P"]["MaxConnections"].get<uint32_t>();

        p2pServer_ = std::make_shared<P2PServer>(neoSystem_, p2pPort, maxConnections);

        // Load seed nodes for the network
        LoadSeedNodes();

        LOG_INFO("P2P network initialized on port {}", p2pPort);
    }

    void LoadSeedNodes()
    {
        std::vector<std::string> seedNodes;

        if (network_ == "mainnet")
        {
            seedNodes = {"seed1.neo.org:10333", "seed2.neo.org:10333", "seed3.neo.org:10333", "seed4.neo.org:10333",
                         "seed5.neo.org:10333"};
        }
        else if (network_ == "testnet")
        {
            seedNodes = {"seed1.testnet.neo.org:20333", "seed2.testnet.neo.org:20333", "seed3.testnet.neo.org:20333"};
        }

        for (const auto& seed : seedNodes)
        {
            p2pServer_->AddSeedNode(seed);
        }

        LOG_INFO("Loaded {} seed nodes", seedNodes.size());
    }

    void InitializeRpcServer()
    {
        auto rpcPort = config_["RPC"]["Port"].get<uint16_t>();
        auto maxConnections = config_["RPC"]["MaxConcurrentConnections"].get<uint32_t>();

        RpcConfig rpcConfig;
        rpcConfig.port = rpcPort;
        rpcConfig.bind_address = "0.0.0.0";
        rpcConfig.max_connections = maxConnections;
        rpcConfig.enable_cors = config_["RPC"]["EnableCors"].get<bool>();

        rpcServer_ = std::make_shared<RpcServer>(rpcConfig);
        rpcServer_->SetNeoSystem(neoSystem_);

        LOG_INFO("RPC server initialized on port {}", rpcPort);
    }

    void InitializeConsensus()
    {
        if (!config_["Consensus"]["Enabled"].get<bool>())
        {
            LOG_INFO("Consensus participation disabled");
            return;
        }

        try
        {
            // Load consensus wallet
            auto walletPath = config_["Consensus"]["WalletPath"].get<std::string>();
            auto walletPassword = config_["Consensus"]["WalletPassword"].get<std::string>();

            if (!std::filesystem::exists(walletPath))
            {
                LOG_ERROR("Consensus wallet not found: {}", walletPath);
                return;
            }

            consensusWallet_ = std::make_shared<NEP6Wallet>(walletPath);
            consensusWallet_->Unlock(walletPassword);

            // Get consensus key
            auto accounts = consensusWallet_->GetAccounts();
            if (accounts.empty())
            {
                LOG_ERROR("No accounts found in consensus wallet");
                return;
            }

            consensusKey_ = accounts[0]->GetKey();
            auto nodeId = consensusKey_->GetScriptHash();

            // Get current validators
            auto validators = blockchain_->GetValidators();

            // Initialize consensus
            ConsensusConfig consensusConfig;
            consensus_ = std::make_shared<DbftConsensus>(consensusConfig, nodeId, validators, mempool_, blockchain_);

            // Set callbacks
            consensus_->SetTransactionVerifier([this](const auto& tx) { return blockchain_->VerifyTransaction(tx); });

            consensus_->SetBlockPersister([this](const auto& block) { return blockchain_->Persist(block); });

            consensus_->SetMessageBroadcaster([this](const auto& msg) { p2pServer_->Broadcast(msg); });

            LOG_INFO("Consensus initialized - Node ID: {}", nodeId.ToString());
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to initialize consensus: {}", e.what());
        }
    }

    void DisplayNodeInfo()
    {
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "║              NEO C++ PRODUCTION NODE                      ║\n";
        std::cout << "║                  Version 3.6.0                            ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Network: " << std::left << std::setw(48) << network_ << "║\n";
        std::cout << "║ Block Height: " << std::left << std::setw(43) << blockchain_->GetHeight() << "║\n";
        std::cout << "║ P2P Port: " << std::left << std::setw(47) << config_["P2P"]["Port"].get<uint16_t>() << "║\n";
        std::cout << "║ RPC Port: " << std::left << std::setw(47) << config_["RPC"]["Port"].get<uint16_t>() << "║\n";

        if (consensus_)
        {
            std::cout << "║ Consensus: ACTIVE                                         ║\n";
        }
        else
        {
            std::cout << "║ Consensus: OBSERVER                                       ║\n";
        }

        std::cout << "╠══════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Native Contracts:                                         ║\n";
        std::cout << "║  • NeoToken     • GasToken      • ContractManagement    ║\n";
        std::cout << "║  • PolicyContract • RoleManagement • OracleContract      ║\n";
        std::cout << "║  • CryptoLib                                             ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        std::cout << "Node is running. Press Ctrl+C to stop.\n\n";
    }

    void MainLoop()
    {
        auto lastStats = std::chrono::steady_clock::now();

        while (!g_shutdown)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Display statistics every 30 seconds
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastStats).count() >= 30)
            {
                DisplayStatistics();
                lastStats = now;
            }

            // Process any pending tasks
            ProcessPendingTasks();
        }
    }

    void DisplayStatistics()
    {
        auto height = blockchain_->GetHeight();
        auto peers = p2pServer_->GetConnectedPeers();
        auto poolSize = mempool_->GetSize();

        LOG_INFO("=== NODE STATISTICS ===");
        LOG_INFO("Block Height: {}", height);
        LOG_INFO("Connected Peers: {}", peers);
        LOG_INFO("Memory Pool: {} transactions", poolSize);

        if (consensus_)
        {
            auto consensusState = consensus_->GetState();
            LOG_INFO("Consensus View: {}", consensusState.GetViewNumber());
        }

        LOG_INFO("=======================");
    }

    void ProcessPendingTasks()
    {
        // Process any pending blockchain tasks
        blockchain_->ProcessPendingBlocks();

        // Process memory pool cleanup
        mempool_->RemoveStaleTransactions();
    }
};

int main(int argc, char* argv[])
{
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try
    {
        // Parse command line arguments
        std::string configPath = "config.json";
        if (argc > 1)
        {
            configPath = argv[1];
        }

        std::cout << "Neo C++ Production Node v3.6.0\n";
        std::cout << "==============================\n\n";

        // Create and start the node
        ProductionNeoNode node(configPath);
        node.Start();

        std::cout << "\nNode stopped successfully.\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}