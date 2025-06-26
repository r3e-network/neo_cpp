#include <neo/core/logging.h>
#include <neo/rpc/rpc_server_simple.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/data_cache.h>
// #include <neo/wallets/wallet_manager.h> // Available but disabled due to crypto deps
// #include <neo/wallets/wallet.h> // Available but disabled due to crypto deps
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <signal.h>

using namespace neo;
using namespace neo::core;
using namespace neo::rpc;
using namespace neo::persistence;
// using namespace neo::wallets; // Available but disabled

// Global flag for graceful shutdown
volatile bool running = true;

void signal_handler(int signal) 
{
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully...\n";
    running = false;
}

class MinimalNeoNode 
{
private:
    std::shared_ptr<MemoryStore> store_;
    std::shared_ptr<StoreCache> blockchain_;
    std::shared_ptr<RpcServer> rpc_server_;
    // std::shared_ptr<Wallet> wallet_; // Available but disabled
    
public:
    MinimalNeoNode() 
    {
        // Initialize logging
        Logger::Initialize("neo-node");
        LOG_INFO("Initializing Minimal Neo C++ Node...");
        
        // Initialize storage layer
        store_ = std::make_shared<MemoryStore>();
        blockchain_ = std::make_shared<StoreCache>(*store_);
        LOG_INFO("Storage layer initialized");
        
        // Initialize wallet (temporarily disabled)
        // InitializeWallet();
        
        // Initialize RPC server
        InitializeRpcServer();
        
        LOG_INFO("Minimal Neo C++ Node initialization complete!");
    }
    
    ~MinimalNeoNode() 
    {
        Shutdown();
    }
    
    void Start() 
    {
        LOG_INFO("Starting Minimal Neo C++ Node...");
        
        // Start RPC server
        rpc_server_->Start();
        LOG_INFO("RPC server started on port 10332");
        
        // Display node information
        DisplayNodeInfo();
        
        // Main node loop
        MainLoop();
    }
    
    void Shutdown() 
    {
        LOG_INFO("Shutting down Minimal Neo C++ Node...");
        
        if (rpc_server_) {
            rpc_server_->Stop();
            LOG_INFO("RPC server stopped");
        }
        
        LOG_INFO("Minimal Neo C++ Node shutdown complete");
    }
    
private:
    // Wallet functionality available but temporarily disabled
    // void InitializeWallet() { ... }

    void InitializeRpcServer() 
    {
        LOG_INFO("Initializing RPC server...");
        
        RpcConfig rpc_config;
        rpc_config.bind_address = "127.0.0.1";
        rpc_config.port = 10332;
        rpc_config.enable_cors = true;
        rpc_config.max_connections = 100;
        
        rpc_server_ = std::make_shared<RpcServer>(rpc_config);
        
        LOG_INFO("RPC server initialized on {}:{}", rpc_config.bind_address, rpc_config.port);
    }
    
    void DisplayNodeInfo() 
    {
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "║                 MINIMAL NEO C++ NODE                    ║\n";
        std::cout << "║                    Version 3.6.0                        ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Status: RUNNING                                          ║\n";
        std::cout << "║ Network: Development Mode                                ║\n";
        std::cout << "║ RPC Server: http://127.0.0.1:10332                      ║\n";
        std::cout << "║ Block Height: 0                                          ║\n";
        std::cout << "║ Connected Peers: 0                                       ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Wallet Information:                                      ║\n";
        std::cout << "║  • Status: Available (requires crypto completion)      ║\n";
        std::cout << "║  • Infrastructure: Headers and classes implemented     ║\n";
        std::cout << "║  • Note: Full wallet needs cryptographic functions     ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Features:                                                ║\n";
        std::cout << "║  • Basic RPC Server                                     ║\n";
        std::cout << "║  • Memory Storage                                       ║\n";
        std::cout << "║  • Wallet Infrastructure (pending crypto completion)   ║\n";
        std::cout << "║  • Development Environment                              ║\n";
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
        std::cout << "Starting Minimal Neo C++ Blockchain Node...\n";
        
        // Create and start the node
        MinimalNeoNode node;
        node.Start();
        
        std::cout << "Minimal Neo C++ Node stopped.\n";
        return 0;
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}