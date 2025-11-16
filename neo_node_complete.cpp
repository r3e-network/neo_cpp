/**
 * @file neo_node_complete.cpp
 * @brief Console harness for the production Neo C++ node.
 */

#include <neo/consensus/consensus_service.h>
#include <neo/node/neo_node.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <algorithm>

namespace
{
std::atomic<bool> g_shutdownRequested{false};
std::shared_ptr<neo::node::NeoNode> g_node;
std::chrono::seconds g_statusInterval{std::chrono::seconds(30)};

void SignalHandler(int signal)
{
    g_shutdownRequested.store(true);
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
}

void PrintBanner()
{
    std::cout << "============================================\n";
    std::cout << "        Neo N3 C++ Full Node Console        \n";
    std::cout << "============================================\n";
    std::cout << "Build: " << __DATE__ << " " << __TIME__ << "\n";
    std::cout << "Parity target: Neo C# Node 3.7.x\n";
    std::cout << "============================================\n\n";
}

std::string ResolveNetworkPreset(const std::string& preset)
{
    std::string normalized = preset;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (normalized == "testnet") return "config/testnet.config.json";
    if (normalized == "mainnet") return "config/mainnet.config.json";
    if (normalized == "privnet" || normalized == "private" || normalized == "private-net") return "config/privnet.json";

    throw std::invalid_argument("Unknown network preset: " + preset);
}

void PrintUsage(const char* exe)
{
    std::cout << "Usage: " << exe << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --config <path>          Configuration file path (default: config.json)\n";
    std::cout << "  --network <name>         Network preset (mainnet|testnet|privnet)\n";
    std::cout << "  --datadir <path>         Blockchain data directory (default: ./data)\n";
    std::cout << "  --status-interval <sec>  Seconds between status reports (default: 30)\n";
    std::cout << "  -h, --help               Show this help message\n";
}

void PrintConfiguration(const std::string& config, const std::string& dataDir)
{
    std::cout << "Configuration:\n";
    std::cout << "  Config file : " << config << "\n";
    std::cout << "  Data path   : " << dataDir << "\n";
    std::cout << "  Status freq : every " << g_statusInterval.count() << "s\n\n";
}

void PrintCompatibilitySummary(const std::shared_ptr<neo::node::NeoNode>& node)
{
    std::cout << "Runtime services mapped to C# components:\n";
    std::cout << "  - Ledger engine     : neo::ledger::Blockchain\n";
    std::cout << "  - Transaction pool  : neo::ledger::MemoryPool\n";
    std::cout << "  - Networking stack  : neo::network::p2p::LocalNode + BlockSyncManager\n";
    std::cout << "  - RPC surface       : neo::rpc::RpcServer (JSON-RPC 2.0)\n";
    std::cout << "  - VM / contracts    : neo::vm + neo::smartcontract::native\n";
    const auto consensus = node->GetConsensusService();
    std::cout << "  - dBFT consensus    : " << (consensus ? "available" : "disabled");
    if (consensus)
    {
        std::cout << " (validators=" << consensus->GetValidators().size()
                  << ", autostart=" << (node->IsConsensusAutoStartEnabled() ? "true" : "false") << ")";
    }
    std::cout << "\n\n";
}

void PrintNodeStatus(const std::shared_ptr<neo::node::NeoNode>& node,
                     const std::chrono::steady_clock::time_point& startTime)
{
    const auto now = std::chrono::steady_clock::now();
    const auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
    std::cout << "[Status] Uptime=" << uptime.count() << "s"
              << " | Height=" << node->GetBlockHeight()
              << " | Peers=" << node->GetConnectedPeersCount()
              << " | Mempool=" << node->GetMemoryPoolCount() << '\n';

    const auto consensus = node->GetConsensusService();
    if (consensus)
    {
        std::cout << "         Consensus: " << (consensus->IsRunning() ? "running" : "idle")
                  << " | BlockIndex=" << consensus->GetBlockIndex()
                  << " | View=" << consensus->GetViewNumber()
                  << " | Validators=" << consensus->GetValidators().size() << '\n';
    }
    else
    {
        std::cout << "         Consensus: disabled\n";
    }
}
}  // namespace

int main(int argc, char* argv[])
{
    PrintBanner();

    std::string configPath = "config.json";
    std::string dataPath = "./data";
    std::string networkPreset;
    bool showHelp = false;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc)
        {
            configPath = argv[++i];
        }
        else if (arg == "--datadir" && i + 1 < argc)
        {
            dataPath = argv[++i];
        }
        else if (arg == "--status-interval" && i + 1 < argc)
        {
            try
            {
                const auto value = std::stoul(argv[++i]);
                if (value == 0)
                {
                    throw std::invalid_argument("status interval must be > 0");
                }
                g_statusInterval = std::chrono::seconds(value);
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Invalid --status-interval value: " << ex.what() << '\n';
                return 1;
            }
        }
        else if (arg == "--help" || arg == "-h")
        {
            showHelp = true;
        }
        else if (arg == "--network" && i + 1 < argc)
        {
            networkPreset = argv[++i];
        }
        else
        {
            std::cerr << "Unknown option: " << arg << '\n';
            showHelp = true;
        }
    }

    if (showHelp)
    {
        PrintUsage(argv[0]);
        return 0;
    }

    if (!networkPreset.empty())
    {
        try
        {
            configPath = ResolveNetworkPreset(networkPreset);
            std::cout << "Using network preset '" << networkPreset << "' -> " << configPath << "\n";
        }
        catch (const std::exception& ex)
        {
            std::cerr << ex.what() << '\n';
            return 1;
        }
    }

    PrintConfiguration(configPath, dataPath);

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    try
    {
        g_node = std::make_shared<neo::node::NeoNode>(configPath, dataPath);
        if (!g_node->Initialize())
        {
            std::cerr << "Failed to initialise Neo node\n";
            return 1;
        }

        if (!g_node->Start())
        {
            std::cerr << "Failed to start Neo node\n";
            return 1;
        }

        const auto startTime = std::chrono::steady_clock::now();
        PrintCompatibilitySummary(g_node);
        std::cout << "Node started. Press Ctrl+C to stop.\n";

        while (!g_shutdownRequested.load() && g_node->IsRunning())
        {
            PrintNodeStatus(g_node, startTime);

            auto slept = std::chrono::seconds(0);
            while (slept < g_statusInterval && !g_shutdownRequested.load() && g_node->IsRunning())
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                slept += std::chrono::seconds(1);
            }
        }

        std::cout << "Stopping Neo node...\n";
        g_node->Stop();
        g_node.reset();

        std::cout << "Neo node stopped cleanly.\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << '\n';
        if (g_node)
        {
            g_node->Stop();
            g_node.reset();
        }
        return 1;
    }
}
