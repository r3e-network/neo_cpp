// Simple app using NeoSystem and RPCServer
#include <neo/core/neo_system.h>
#include <neo/protocol_settings.h>
#include <neo/rpc/rpc_server.h>
#include <iostream>
#include <atomic>
#include <thread>

using namespace neo;
using namespace neo::core;
using namespace neo::rpc;

static std::atomic<bool> running{true};

static void signal_handler(int signal)
{
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully...\n";
    running = false;
}

static void DisplayNodeInfo()
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

static void DisplayStatistics(RpcServer& rpc)
{
    auto rpc_stats = rpc.GetStatistics();

    std::cout << "=== NODE STATISTICS ===" << std::endl;
    std::cout << "RPC Requests: " << rpc_stats["totalRequests"].GetInt64() << " total, "
              << rpc_stats["failedRequests"].GetInt64() << " failed" << std::endl;
    std::cout << "========================" << std::endl;
}

static void PrintUsage()
{
    std::cout << "Neo C++ Node v1.2.0\n"
              << "Usage: neo_node [--help] [--version] [--config <path>]\n"
              << "\nOptions:\n"
              << "  --help, -h       Show this help message\n"
              << "  --version, -v    Show version information\n"
              << "  --config <path>  Specify configuration file path\n" << std::endl;
}

static void PrintVersion()
{
    std::cout << "Neo C++ Node\n"
              << "Version: 1.2.0\n"
              << "Build Date: " << __DATE__ << " " << __TIME__ << "\n"
              << "Protocol Version: 3.6.0\n"
              << "Network ID: 860833102\n" << std::endl;
}

int main(int argc, char* argv[])
{
    // Setup signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try
    {
        // Basic CLI handling
        std::string configPath;
        bool noRpc = false;
        std::string dataPath = "./data";
        for (int i = 1; i < argc; ++i)
        {
            std::string arg(argv[i]);
            if (arg == "--help" || arg == "-h")
            {
                PrintUsage();
                return 0;
            }
            else if (arg == "--version" || arg == "-v")
            {
                PrintVersion();
                return 0;
            }
            if (arg == "--config" && i + 1 < argc)
            {
                configPath = std::string(argv[++i]);
                continue;
            }
            if (arg == "--no-rpc")
            {
                noRpc = true;
                continue;
            }
            if (arg == "--data" && i + 1 < argc)
            {
                dataPath = std::string(argv[++i]);
                continue;
            }
        }

        std::cout << "Starting Neo C++ Blockchain Node...\n";

        // Initialize logging
        Logger::Initialize("neo-node");

        // Initialize NeoSystem with RocksDB (fallback to memory if unavailable)
        auto settings = std::make_unique<ProtocolSettings>(ProtocolSettings::GetDefault());
        auto neoSystem = std::make_shared<neo::NeoSystem>(std::move(settings), "rocksdb", dataPath);
        neoSystem->EnsureBlockchainInitialized();

        // Initialize RPC server
        RpcConfig rpc_config;
        rpc_config.bind_address = "127.0.0.1";
        rpc_config.port = 10332;
        rpc_config.enable_cors = true;
        rpc_config.max_concurrent_requests = 100;

        std::unique_ptr<RpcServer> rpc;
        if (!noRpc)
        {
            rpc = std::make_unique<RpcServer>(rpc_config, neoSystem);
            rpc->Start();
        }

        DisplayNodeInfo();

        int stats_counter = 0;
        while (running.load())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (!noRpc && ++stats_counter % 30 == 0)
            {
                DisplayStatistics(*rpc);
            }
        }

        if (rpc) rpc->Stop();
        neoSystem->stop();
        std::cout << "Neo C++ Node stopped.\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
