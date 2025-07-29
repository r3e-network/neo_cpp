/**
 * Neo C++ CLI - Complete command-line interface for Neo blockchain
 * Compatible with Neo N3 C# implementation
 */

#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <signal.h>
#include <string>
#include <vector>

// Core Neo includes
#include <neo/consensus/dbft_consensus.h>
#include <neo/console_service/service_proxy.h>
#include <neo/core/logging.h>
#include <neo/core/neo_system.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/memory_pool.h>
#include <neo/network/p2p_server.h>
#include <neo/persistence/rocksdb_store.h>
#include <neo/rpc/rpc_server.h>
#include <neo/vm/execution_engine.h>
#include <neo/wallets/nep6/nep6_wallet.h>

// CLI includes
#include "cli_service.h"
#include "commands/command_registry.h"
#include "plugins/plugin_manager.h"
#include "services/console_service_neo.h"

using namespace neo;
using namespace neo::core;
using namespace neo::cli;

// Global instance for signal handling
std::unique_ptr<CLIService> g_cli_service;

void signal_handler(int signal)
{
    if (g_cli_service)
    {
        g_cli_service->Stop();
    }
}

int main(int argc, char* argv[])
{
    try
    {
        // Setup signal handlers
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);

        // Parse command line arguments
        std::filesystem::path config_path = "config.json";
        std::string network = "mainnet";
        bool enable_rpc = true;
        bool enable_consensus = false;

        for (int i = 1; i < argc; i++)
        {
            std::string arg = argv[i];
            if (arg == "--config" && i + 1 < argc)
            {
                config_path = argv[++i];
            }
            else if (arg == "--testnet")
            {
                network = "testnet";
            }
            else if (arg == "--mainnet")
            {
                network = "mainnet";
            }
            else if (arg == "--privnet")
            {
                network = "privnet";
            }
            else if (arg == "--norpc")
            {
                enable_rpc = false;
            }
            else if (arg == "--consensus")
            {
                enable_consensus = true;
            }
            else if (arg == "--help" || arg == "-h")
            {
                std::cout << "Neo C++ CLI v3.6.0\n\n";
                std::cout << "Usage: neo-cli [options]\n\n";
                std::cout << "Options:\n";
                std::cout << "  --config <path>    Configuration file path (default: config.json)\n";
                std::cout << "  --mainnet          Use MainNet configuration\n";
                std::cout << "  --testnet          Use TestNet configuration\n";
                std::cout << "  --privnet          Use private network configuration\n";
                std::cout << "  --norpc            Disable RPC server\n";
                std::cout << "  --consensus        Enable consensus participation\n";
                std::cout << "  --help, -h         Show this help message\n";
                return 0;
            }
        }

        // Create and start CLI service
        g_cli_service = std::make_unique<CLIService>(config_path, network);
        g_cli_service->SetRPCEnabled(enable_rpc);
        g_cli_service->SetConsensusEnabled(enable_consensus);

        // Display startup banner
        g_cli_service->DisplayBanner();

        // Initialize and start the node
        g_cli_service->Initialize();
        g_cli_service->Start();

        // Run the CLI main loop
        g_cli_service->Run();

        // Cleanup
        g_cli_service->Stop();
        g_cli_service.reset();

        std::cout << "\nNeo CLI stopped successfully.\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\nFatal error: " << e.what() << std::endl;
        return 1;
    }
}