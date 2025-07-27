#include <iostream>
#include <fstream>
#include <memory>
#include <thread>
#include <chrono>
#include <signal.h>
#include <neo/core/neo_system.h>
#include <neo/core/logging.h>
#include <neo/protocol_settings.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_cache.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/policy_contract.h>
#include <neo/rpc/rpc_server.h>
#include <neo/ledger/blockchain.h>
#include <neo/io/json.h>
#include <nlohmann/json.hpp>

using namespace neo;
using namespace neo::core;

volatile sig_atomic_t g_shutdown = 0;

void signal_handler(int signal) 
{
    std::cout << "\nReceived signal " << signal << ", shutting down...\n";
    g_shutdown = 1;
}

class TestnetNode 
{
private:
    std::unique_ptr<NeoSystem> neo_system_;
    std::unique_ptr<rpc::RpcServer> rpc_server_;
    std::unique_ptr<ProtocolSettings> settings_;
    
public:
    TestnetNode() 
    {
        // Display banner
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════╗\n";
        std::cout << "║           NEO C++ BLOCKCHAIN NODE - TESTNET            ║\n";
        std::cout << "║                    Version 3.6.0                       ║\n";
        std::cout << "╚════════════════════════════════════════════════════════╝\n\n";
    }
    
    bool Initialize(const std::string& config_path) 
    {
        try {
            std::cout << "[INFO] Loading testnet configuration from: " << config_path << "\n";
            
            // Load configuration
            std::ifstream config_file(config_path);
            if (!config_file.is_open()) {
                std::cerr << "[ERROR] Failed to open configuration file: " << config_path << "\n";
                return false;
            }
            
            nlohmann::json config;
            config_file >> config;
            config_file.close();
            
            // Create protocol settings with testnet parameters
            settings_ = std::make_unique<ProtocolSettings>();
            
            // Set testnet magic number
            if (config.contains("ProtocolConfiguration")) {
                auto& protocol = config["ProtocolConfiguration"];
                
                if (protocol.contains("Magic")) {
                    uint32_t magic = protocol["Magic"];
                    std::cout << "[INFO] Testnet Magic Number: " << magic << " (0x" 
                              << std::hex << magic << std::dec << ")\n";
                    // Note: In a complete implementation, we'd set this on the settings object
                }
                
                if (protocol.contains("MillisecondsPerBlock")) {
                    uint32_t ms_per_block = protocol["MillisecondsPerBlock"];
                    std::cout << "[INFO] Block Time: " << ms_per_block << " ms\n";
                }
                
                if (protocol.contains("ValidatorsCount")) {
                    int validators = protocol["ValidatorsCount"];
                    std::cout << "[INFO] Validators Count: " << validators << "\n";
                }
            }
            
            // Initialize logging
            Logger::Initialize("neo-testnet");
            LOG_INFO("Neo Testnet Node starting...");
            
            // Initialize Neo system with memory storage (since network is stubbed)
            std::cout << "[INFO] Initializing Neo system...\n";
            neo_system_ = std::make_unique<NeoSystem>(
                std::move(settings_), 
                "memory",  // Use memory storage since network is stubbed
                "./TestNetChain"
            );
            
            std::cout << "[INFO] Neo system initialized successfully\n";
            
            // Initialize RPC server if configured
            if (config.contains("ApplicationConfiguration") && 
                config["ApplicationConfiguration"].contains("RPC")) {
                auto& rpc_config = config["ApplicationConfiguration"]["RPC"];
                int rpc_port = rpc_config.value("Port", 20332);
                std::string bind_address = rpc_config.value("BindAddress", "127.0.0.1");
                
                std::cout << "[INFO] Starting RPC server on " << bind_address << ":" << rpc_port << "\n";
                
                // Create RPC config
                rpc::RpcConfig rpc_cfg;
                rpc_cfg.bind_address = bind_address;
                rpc_cfg.port = rpc_port;
                
                rpc_server_ = std::make_unique<rpc::RpcServer>(rpc_cfg);
                
                rpc_server_->Start();
                std::cout << "[INFO] RPC server started\n";
            }
            
            // Display seed nodes (P2P not implemented, just for info)
            if (config.contains("ProtocolConfiguration") && 
                config["ProtocolConfiguration"].contains("SeedList")) {
                std::cout << "[INFO] Testnet seed nodes:\n";
                for (const auto& seed : config["ProtocolConfiguration"]["SeedList"]) {
                    std::cout << "         - " << seed << "\n";
                }
                std::cout << "[NOTE] P2P networking is currently stubbed\n";
            }
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to initialize testnet node: " << e.what() << "\n";
            return false;
        }
    }
    
    void Run() 
    {
        std::cout << "\n";
        std::cout << "══════════════════════════════════════════════════════════\n";
        std::cout << "Neo Testnet Node is running!\n";
        std::cout << "══════════════════════════════════════════════════════════\n";
        std::cout << "\n";
        std::cout << "Available commands:\n";
        std::cout << "  help    - Show this help message\n";
        std::cout << "  info    - Display node information\n";
        std::cout << "  height  - Show current block height\n";
        std::cout << "  peers   - Show connected peers (stubbed)\n";
        std::cout << "  exit    - Shutdown the node\n";
        std::cout << "\n";
        std::cout << "RPC endpoints available at: http://localhost:20332\n";
        std::cout << "  Example: curl -X POST http://localhost:20332 -d '{\"jsonrpc\":\"2.0\",\"method\":\"getblockcount\",\"params\":[],\"id\":1}'\n";
        std::cout << "\n";
        
        // Main loop
        std::string command;
        while (!g_shutdown) {
            std::cout << "neo-testnet> ";
            std::cout.flush();
            
            if (!std::getline(std::cin, command)) {
                break; // EOF or error
            }
            
            if (command.empty()) {
                continue;
            }
            
            if (command == "exit" || command == "quit") {
                break;
            }
            else if (command == "help") {
                std::cout << "Commands:\n";
                std::cout << "  help    - Show this help message\n";
                std::cout << "  info    - Display node information\n";
                std::cout << "  height  - Show current block height\n";
                std::cout << "  peers   - Show connected peers\n";
                std::cout << "  exit    - Shutdown the node\n";
            }
            else if (command == "info") {
                std::cout << "Neo Testnet Node Information:\n";
                std::cout << "  Version: 3.6.0\n";
                std::cout << "  Network: TestNet (Magic: 894710606)\n";
                std::cout << "  Storage: In-Memory\n";
                std::cout << "  RPC Port: 20332\n";
                std::cout << "  P2P Port: 20333 (stubbed)\n";
                
                // In a complete implementation, we'd get the blockchain height
                std::cout << "  Block Height: 0 (Genesis)\n";
            }
            else if (command == "height") {
                // In a complete implementation, we'd get the blockchain height
                std::cout << "Current block height: 0 (Genesis)\n";
                std::cout << "Note: Full blockchain sync requires P2P implementation\n";
            }
            else if (command == "peers") {
                std::cout << "P2P networking is currently stubbed\n";
                std::cout << "In a full implementation, this would show:\n";
                std::cout << "  - seed1.neo.org:20333\n";
                std::cout << "  - seed2.neo.org:20333\n";
                std::cout << "  - seed3.neo.org:20333\n";
                std::cout << "  - seed4.neo.org:20333\n";
                std::cout << "  - seed5.neo.org:20333\n";
            }
            else {
                std::cout << "Unknown command: " << command << "\n";
                std::cout << "Type 'help' for available commands\n";
            }
        }
    }
    
    void Shutdown() 
    {
        std::cout << "\n[INFO] Shutting down Neo testnet node...\n";
        
        if (rpc_server_) {
            std::cout << "[INFO] Stopping RPC server...\n";
            rpc_server_->Stop();
        }
        
        if (neo_system_) {
            std::cout << "[INFO] Stopping Neo system...\n";
            neo_system_->stop();
        }
        
        std::cout << "[INFO] Shutdown complete\n";
    }
};

int main(int argc, char* argv[]) 
{
    // Install signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        // Determine config path
        std::string config_path = "config/testnet.json";
        if (argc > 1) {
            config_path = argv[1];
        }
        
        // Create and initialize node
        TestnetNode node;
        
        if (!node.Initialize(config_path)) {
            std::cerr << "Failed to initialize testnet node\n";
            return 1;
        }
        
        // Run the node
        node.Run();
        
        // Cleanup
        node.Shutdown();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}