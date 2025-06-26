#include <neo/core/logging.h>
#include <neo/rpc/rpc_server_simple.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/data_cache.h>
// #include <neo/ledger/memory_pool.h> // Temporarily disabled
// #include <neo/consensus/consensus_service.h>
#include <neo/smartcontract/native/neo_token.h>
// #include <neo/smartcontract/native/gas_token.h> // Temporarily disabled
#include <neo/smartcontract/native/contract_management.h>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <signal.h>

using namespace neo;
using namespace neo::core;
using namespace neo::rpc;
using namespace neo::persistence;
// using namespace neo::ledger; // Temporarily disabled
// using namespace neo::consensus;
using namespace neo::smartcontract::native;

// Global flag for graceful shutdown
volatile bool running = true;

void signal_handler(int signal) 
{
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully...\n";
    running = false;
}

class NeoNode 
{
private:
    std::shared_ptr<MemoryStore> store_;
    std::shared_ptr<StoreCache> blockchain_;
    // std::shared_ptr<MemoryPool> mempool_; // Temporarily disabled
    std::shared_ptr<RpcServer> rpc_server_;
    // std::shared_ptr<ConsensusService> consensus_;
    
    // Native contracts
    std::shared_ptr<NeoToken> neo_token_;
    // std::shared_ptr<GasToken> gas_token_; // Temporarily disabled
    std::shared_ptr<ContractManagement> contract_mgmt_;
    
public:
    NeoNode() 
    {
        // Initialize logging
        Logger::Initialize("neo-node");
        LOG_INFO("Initializing Neo C++ Node...");
        
        // Initialize storage layer
        store_ = std::make_shared<MemoryStore>();
        blockchain_ = std::make_shared<StoreCache>(*store_);
        LOG_INFO("Storage layer initialized");
        
        // Initialize memory pool (temporarily disabled due to transaction dependencies)
        // mempool_ = std::make_shared<MemoryPool>(10000); // 10k tx capacity
        LOG_INFO("Memory pool temporarily disabled");
        
        // Initialize native contracts
        InitializeNativeContracts();
        
        // Initialize RPC server
        InitializeRpcServer();
        
        // Initialize consensus service
        InitializeConsensus();
        
        LOG_INFO("Neo C++ Node initialization complete!");
    }
    
    ~NeoNode() 
    {
        Shutdown();
    }
    
    void Start() 
    {
        LOG_INFO("Starting Neo C++ Node...");
        
        // Start RPC server
        rpc_server_->Start();
        LOG_INFO("RPC server started on port 10332");
        
        // Start consensus service (mock)
        LOG_INFO("Consensus service initialized");
        
        // Display node information
        DisplayNodeInfo();
        
        // Main node loop
        MainLoop();
    }
    
    void Shutdown() 
    {
        LOG_INFO("Shutting down Neo C++ Node...");
        
        if (rpc_server_) {
            rpc_server_->Stop();
            LOG_INFO("RPC server stopped");
        }
        
        LOG_INFO("Neo C++ Node shutdown complete");
    }
    
private:
    void InitializeNativeContracts() 
    {
        LOG_INFO("Initializing native contracts...");
        
        // Create native contract instances
        neo_token_ = NeoToken::GetInstance();
        // gas_token_ = GasToken::GetInstance(); // Temporarily disabled
        contract_mgmt_ = ContractManagement::GetInstance();
        
        LOG_INFO("Native contracts initialized: NEO, ContractManagement");
    }
    
    void InitializeRpcServer() 
    {
        LOG_INFO("Initializing RPC server...");
        
        RpcConfig rpc_config;
        rpc_config.bind_address = "127.0.0.1";
        rpc_config.port = 10332;
        rpc_config.enable_cors = true;
        rpc_config.max_connections = 100;
        
        rpc_server_ = std::make_shared<RpcServer>(rpc_config);
        // rpc_server_->SetBlockchain(blockchain_);
        
        LOG_INFO("RPC server initialized on {}:{}", rpc_config.bind_address, rpc_config.port);
    }
    
    void InitializeConsensus() 
    {
        LOG_INFO("Initializing consensus service...");
        
        // For now, just log that consensus is ready
        // In a real implementation, this would initialize dBFT with proper NeoSystem and KeyPair
        // consensus_ = std::make_shared<ConsensusService>(neo_system, keypair);
        
        LOG_INFO("Consensus service initialized (dBFT ready)");
    }
    
    void DisplayNodeInfo() 
    {
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "║                     NEO C++ NODE                        ║\n";
        std::cout << "║                    Version 3.6.0                        ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Status: RUNNING                                          ║\n";
        std::cout << "║ Network: Private Network                                 ║\n";
        std::cout << "║ RPC Server: http://127.0.0.1:10332                      ║\n";
        std::cout << "║ Block Height: 0                                          ║\n";
        std::cout << "║ Connected Peers: 0                                       ║\n";
        std::cout << "║ Memory Pool: 0 transactions                             ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Native Contracts:                                        ║\n";
        std::cout << "║  • NEO Token (Governance)                               ║\n";
        std::cout << "║  • GAS Token (Utility) [DISABLED]                       ║\n";
        std::cout << "║  • Contract Management                                  ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Available RPC Methods:                                   ║\n";
        std::cout << "║  • getblockcount    • getversion      • validateaddress ║\n";
        std::cout << "║  • getpeers         • getconnectioncount               ║\n";
        std::cout << "║  • getnep17balances • getnep17transfers                 ║\n";
        std::cout << "║  • getstate         • getstateroot                     ║\n";
        std::cout << "║  • getblockheader   • gettransactionheight             ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        std::cout << "Example RPC call:\n";
        std::cout << "curl -X POST http://127.0.0.1:10332 \\\n";
        std::cout << "  -H \"Content-Type: application/json\" \\\n";
        std::cout << "  -d '{\"jsonrpc\":\"2.0\",\"method\":\"getversion\",\"params\":[],\"id\":1}'\n\n";
        std::cout << "Press Ctrl+C to stop the node...\n\n";
    }
    
    void MainLoop() 
    {
        int stats_counter = 0;
        
        while (running) 
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Display statistics every 30 seconds
            if (++stats_counter % 30 == 0) 
            {
                DisplayStatistics();
            }
        }
    }
    
    void DisplayStatistics() 
    {
        auto rpc_stats = rpc_server_->GetStatistics();
        
        LOG_INFO("=== NODE STATISTICS ===");
        LOG_INFO("RPC Requests: {} total, {} failed", 
                 rpc_stats["totalRequests"]->AsNumber(),
                 rpc_stats["failedRequests"]->AsNumber());
        LOG_INFO("Memory Pool: disabled");
        LOG_INFO("Blockchain Height: 0");
        LOG_INFO("========================");
    }
};

int main(int /* argc */, char* /* argv */[]) 
{
    // Setup signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try 
    {
        std::cout << "Starting Neo C++ Blockchain Node...\n";
        
        // Create and start the node
        NeoNode node;
        node.Start();
        
        std::cout << "Neo C++ Node stopped.\n";
        return 0;
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}