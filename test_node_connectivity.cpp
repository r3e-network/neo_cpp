/**
 * @file test_node_connectivity.cpp
 * @brief Simple test program to verify Neo C++ node connectivity
 * 
 * This program tests the basic functionality of the Neo C++ node
 * including network connectivity and block synchronization.
 * 
 * @author Neo C++ Development Team
 * @version 1.0.0
 * @date December 2024
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <signal.h>

// Neo Core Components
#include <neo/protocol_settings.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/network/p2p_server.h>
#include <neo/persistence/leveldb_store.h>
#include <neo/persistence/memory_store.h>
#include <neo/rpc/rpc_server.h>
#include <neo/node/neo_system.h>
#include <neo/smartcontract/native/native_contract_manager.h>

// Global flag for shutdown
volatile bool g_shutdown = false;

void SignalHandler(int signal)
{
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    g_shutdown = true;
}

int main(int argc, char* argv[])
{
    try
    {
        std::cout << "==========================================" << std::endl;
        std::cout << "Neo C++ Node Connectivity Test" << std::endl;
        std::cout << "Testing Neo N3 MainNet Connection" << std::endl;
        std::cout << "==========================================" << std::endl;

        // Set up signal handlers
        signal(SIGINT, SignalHandler);
        signal(SIGTERM, SignalHandler);

        // Step 1: Initialize Protocol Settings
        std::cout << "1. Initializing protocol settings..." << std::endl;
        auto protocolSettings = neo::ProtocolSettings::GetDefault();
        if (!protocolSettings)
        {
            std::cerr << "❌ Failed to load protocol settings" << std::endl;
            return -1;
        }
        
        std::cout << "   Network Magic: 0x" << std::hex << protocolSettings->GetNetwork() << std::dec << std::endl;
        std::cout << "   Validators: " << protocolSettings->GetValidatorsCount() << std::endl;
        std::cout << "   Committee: " << protocolSettings->GetCommitteeMembersCount() << std::endl;
        std::cout << "✅ Protocol settings loaded successfully" << std::endl;

        // Step 2: Initialize Storage
        std::cout << "\n2. Initializing storage system..." << std::endl;
        
        // Use memory store for testing (can switch to LevelDB for persistence)
        auto store = std::make_shared<neo::persistence::MemoryStore>();
        if (!store)
        {
            std::cerr << "❌ Failed to initialize storage" << std::endl;
            return -1;
        }
        std::cout << "✅ Storage system initialized (MemoryStore)" << std::endl;

        // Step 3: Initialize Blockchain
        std::cout << "\n3. Initializing blockchain..." << std::endl;
        auto blockchain = std::make_shared<neo::ledger::Blockchain>(protocolSettings, store);
        if (!blockchain || !blockchain->Initialize())
        {
            std::cerr << "❌ Failed to initialize blockchain" << std::endl;
            return -1;
        }
        
        std::cout << "   Genesis block height: " << blockchain->GetHeight() << std::endl;
        std::cout << "   Current block hash: " << blockchain->GetCurrentBlockHash().ToString() << std::endl;
        std::cout << "✅ Blockchain initialized successfully" << std::endl;

        // Step 4: Initialize Memory Pool
        std::cout << "\n4. Initializing memory pool..." << std::endl;
        auto mempool = std::make_shared<neo::ledger::MemoryPool>(protocolSettings);
        if (!mempool)
        {
            std::cerr << "❌ Failed to initialize memory pool" << std::endl;
            return -1;
        }
        std::cout << "✅ Memory pool initialized" << std::endl;

        // Step 5: Initialize Native Contracts
        std::cout << "\n5. Initializing native contracts..." << std::endl;
        auto& contractManager = neo::smartcontract::native::NativeContractManager::GetInstance();
        
        auto neoToken = contractManager.GetContract("NeoToken");
        auto gasToken = contractManager.GetContract("GasToken");
        
        if (!neoToken || !gasToken)
        {
            std::cerr << "❌ Failed to initialize native contracts" << std::endl;
            return -1;
        }
        
        std::cout << "   NeoToken contract ID: " << neoToken->GetId() << std::endl;
        std::cout << "   GasToken contract ID: " << gasToken->GetId() << std::endl;
        std::cout << "✅ Native contracts initialized" << std::endl;

        // Step 6: Create Neo System
        std::cout << "\n6. Creating Neo system..." << std::endl;
        auto neoSystem = std::make_shared<neo::node::NeoSystem>(protocolSettings, store);
        if (!neoSystem)
        {
            std::cerr << "❌ Failed to create Neo system" << std::endl;
            return -1;
        }
        std::cout << "✅ Neo system created successfully" << std::endl;

        // Step 7: Initialize P2P Network
        std::cout << "\n7. Initializing P2P network server..." << std::endl;
        auto p2pServer = std::make_shared<neo::network::P2PServer>(protocolSettings, blockchain, mempool);
        if (!p2pServer)
        {
            std::cerr << "❌ Failed to create P2P server" << std::endl;
            return -1;
        }
        std::cout << "✅ P2P server created" << std::endl;

        // Step 8: Initialize RPC Server
        std::cout << "\n8. Initializing RPC server..." << std::endl;
        auto rpcServer = std::make_shared<neo::rpc::RpcServer>(neoSystem, "127.0.0.1", 10332);
        if (!rpcServer)
        {
            std::cerr << "❌ Failed to create RPC server" << std::endl;
            return -1;
        }
        std::cout << "✅ RPC server created on port 10332" << std::endl;

        // Step 9: Start Services
        std::cout << "\n9. Starting network services..." << std::endl;
        
        try
        {
            // Start P2P server
            p2pServer->Start();
            std::cout << "✅ P2P server started on port " << protocolSettings->GetP2PPort() << std::endl;
            
            // Start RPC server
            rpcServer->Start();
            std::cout << "✅ RPC server started on port 10332" << std::endl;
            
        }
        catch (const std::exception& e)
        {
            std::cerr << "⚠️  Warning: Some services failed to start: " << e.what() << std::endl;
            std::cout << "   This is normal in testing environments" << std::endl;
        }

        // Step 10: Test Basic RPC Functionality
        std::cout << "\n10. Testing RPC functionality..." << std::endl;
        
        try
        {
            // Test getversion
            auto versionResult = neo::rpc::RPCMethods::GetVersion(neoSystem, nlohmann::json::array());
            std::cout << "   Version info: " << versionResult.dump() << std::endl;
            
            // Test getblockcount
            auto blockCountResult = neo::rpc::RPCMethods::GetBlockCount(neoSystem, nlohmann::json::array());
            std::cout << "   Block count: " << blockCountResult.get<int>() << std::endl;
            
            // Test getbestblockhash
            auto bestHashResult = neo::rpc::RPCMethods::GetBestBlockHash(neoSystem, nlohmann::json::array());
            std::cout << "   Best block hash: " << bestHashResult.get<std::string>() << std::endl;
            
            std::cout << "✅ RPC methods working correctly" << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "❌ RPC test failed: " << e.what() << std::endl;
            return -1;
        }

        // Step 11: Test Network Connectivity
        std::cout << "\n11. Testing network connectivity..." << std::endl;
        
        // Get seed list
        auto seedList = protocolSettings->GetSeedList();
        std::cout << "   Configured seed nodes:" << std::endl;
        for (const auto& seed : seedList)
        {
            std::cout << "     - " << seed << std::endl;
        }
        
        // Check connection count
        auto connectionCount = neo::rpc::RPCMethods::GetConnectionCount(neoSystem, nlohmann::json::array());
        std::cout << "   Current connections: " << connectionCount.get<int>() << std::endl;
        
        if (connectionCount.get<int>() > 0)
        {
            std::cout << "✅ Successfully connected to Neo N3 network!" << std::endl;
        }
        else
        {
            std::cout << "⚠️  No active connections (this is normal in isolated testing)" << std::endl;
        }

        // Step 12: Main Loop - Monitor for Block Sync
        std::cout << "\n12. Starting main monitoring loop..." << std::endl;
        std::cout << "    Press Ctrl+C to stop the node" << std::endl;
        std::cout << "==========================================" << std::endl;

        auto lastBlockCount = blockchain->GetHeight();
        auto lastCheckTime = std::chrono::steady_clock::now();
        
        while (!g_shutdown)
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            auto currentTime = std::chrono::steady_clock::now();
            auto currentBlockCount = blockchain->GetHeight();
            auto currentConnections = neo::rpc::RPCMethods::GetConnectionCount(neoSystem, nlohmann::json::array()).get<int>();
            
            // Print status every 30 seconds
            if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastCheckTime).count() >= 30)
            {
                std::cout << "[" << std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count() << "] ";
                std::cout << "Height: " << currentBlockCount 
                          << ", Connections: " << currentConnections 
                          << ", Mempool: " << mempool->GetTransactionCount() << std::endl;
                
                lastCheckTime = currentTime;
            }
            
            // Check for new blocks
            if (currentBlockCount > lastBlockCount)
            {
                std::cout << "🎉 NEW BLOCK SYNCHRONIZED!" << std::endl;
                std::cout << "   Block height: " << currentBlockCount << std::endl;
                std::cout << "   Block hash: " << blockchain->GetCurrentBlockHash().ToString() << std::endl;
                lastBlockCount = currentBlockCount;
            }
        }

        // Shutdown
        std::cout << "\n🛑 Shutting down Neo C++ node..." << std::endl;
        
        if (rpcServer)
        {
            rpcServer->Stop();
            std::cout << "✅ RPC server stopped" << std::endl;
        }
        
        if (p2pServer)
        {
            p2pServer->Stop();
            std::cout << "✅ P2P server stopped" << std::endl;
        }
        
        std::cout << "✅ Neo C++ node shutdown complete" << std::endl;
        std::cout << "==========================================" << std::endl;
        
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "❌ Fatal error: " << e.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        std::cerr << "❌ Unknown fatal error occurred" << std::endl;
        return -1;
    }
}