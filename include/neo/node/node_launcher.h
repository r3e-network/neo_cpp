/**
 * @file node_launcher.h
 * @brief Reusable console harness for starting Neo C++ nodes.
 *
 * This helper mirrors the behaviour of the C# node runner: it parses a small
 * set of CLI arguments, starts the Neo node, and periodically prints runtime
 * status. Use it from small entry-point executables to avoid duplicating
 * bootstrapping code.
 */

#pragma once

#include <neo/consensus/consensus_service.h>
#include <neo/network/p2p/local_node.h>
#include <neo/node/neo_node.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <csignal>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace neo::node::app
{
struct NodeAppConfig
{
    std::string appName{"Neo C++ Node"};
    std::string defaultConfigPath{"config.json"};
    std::string defaultDataPathOverride{};
    bool allowNetworkPreset{true};
    std::string binaryName{"neo_node"};
};

namespace detail
{
inline std::atomic<bool>& ShutdownRequested()
{
    static std::atomic<bool> flag{false};
    return flag;
}

inline std::chrono::seconds& StatusInterval()
{
    static std::chrono::seconds interval{std::chrono::seconds(30)};
    return interval;
}

inline void SignalHandler(int signal)
{
    ShutdownRequested().store(true);
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
}

inline void InstallSignalHandlers()
{
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
}

inline std::string ResolveNetworkConfigPath(const std::string& preset)
{
    namespace fs = std::filesystem;
    std::string normalized = preset;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    std::vector<std::string> candidates;
    if (normalized == "mainnet")
    {
        candidates = {"config/mainnet.config.json", "config/mainnet.json"};
    }
    else if (normalized == "testnet")
    {
        candidates = {"config/testnet.config.json", "config/testnet.json"};
    }
    else if (normalized == "privnet" || normalized == "private" || normalized == "private-net")
    {
        candidates = {"config/privnet.json"};
    }
    else
    {
        throw std::invalid_argument("Unknown network preset: " + preset);
    }

    for (const auto& candidate : candidates)
    {
        if (fs::exists(candidate))
        {
            return candidate;
        }

        fs::path parentCandidate = fs::path("..") / candidate;
        if (fs::exists(parentCandidate))
        {
            return parentCandidate.string();
        }
    }

    throw std::invalid_argument("No configuration found for preset: " + preset);
}

inline void PrintStatus(const std::shared_ptr<neo::node::NeoNode>& node,
                        const std::chrono::steady_clock::time_point& startTime)
{
    if (!node) return;

    const auto now = std::chrono::steady_clock::now();
    const auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);

    const uint32_t height = node->GetBlockHeight();
    const uint32_t headerHeight = node->GetHeaderHeight();
    const auto peers = node->GetConnectedPeersCount();
    const auto mempoolSize = node->GetMemoryPoolCount();

    uint32_t maxPeerHeight = 0;
    auto& localNode = neo::network::p2p::LocalNode::GetInstance();
    for (auto* peer : localNode.GetConnectedNodes())
    {
        if (peer)
        {
            maxPeerHeight = std::max(maxPeerHeight, peer->GetLastBlockIndex());
        }
    }

    std::cout << "[Status] Uptime=" << uptime.count() << "s"
              << " | Height=" << height << " (header=" << headerHeight << ")"
              << " | Peers=" << peers;
    if (maxPeerHeight > 0) std::cout << " (maxPeerHeight=" << maxPeerHeight << ")";
    std::cout << " | Mempool=" << mempoolSize;

    const auto consensus = node->GetConsensusService();
    if (consensus)
    {
        std::cout << " | Consensus=" << (consensus->IsRunning() ? "running" : "idle")
                  << " idx=" << consensus->GetBlockIndex() << " view=" << consensus->GetViewNumber()
                  << " validators=" << consensus->GetValidators().size();
    }

    std::cout << std::endl;
}
}  // namespace detail

inline void PrintUsage(const NodeAppConfig& config)
{
    std::cout << config.appName << std::endl;
    std::cout << "Usage: " << config.binaryName << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --config <path>          Configuration file path (default: " << config.defaultConfigPath << ")"
              << std::endl;
    std::cout << "  --network <preset>       Network preset (mainnet|testnet|privnet)" << std::endl;
    std::cout << "  --datadir <path>         Data directory override" << std::endl;
    std::cout << "  --db-engine <name>       Storage engine override" << std::endl;
    std::cout << "  --db-path <path>         Storage path override" << std::endl;
    std::cout << "  --no-rpc                 Disable RPC even if enabled in config" << std::endl;
    std::cout << "  --status-interval <sec>  Seconds between status reports (default 30)" << std::endl;
    std::cout << "  -h, --help               Show this help message" << std::endl;
    std::cout << "  -v, --version            Show version information" << std::endl;
}

inline void PrintVersion(const NodeAppConfig& config)
{
    std::cout << config.appName << std::endl;
    std::cout << "Build Date: " << __DATE__ << " " << __TIME__ << std::endl;
}

inline int RunNodeApp(
    int argc,
    char* argv[],
    NodeAppConfig config,
    std::function<void(const std::shared_ptr<neo::node::NeoNode>&)> onStarted = {})
{
    detail::InstallSignalHandlers();

    bool noRpc = false;
    bool showHelp = false;
    bool showVersion = false;
    std::string configPath = config.defaultConfigPath.empty() ? "config.json" : config.defaultConfigPath;
    std::string networkPreset;
    std::string dbEngine;
    std::string dbPath;
    std::string dataDir = config.defaultDataPathOverride;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "--help" || arg == "-h")
        {
            showHelp = true;
        }
        else if (arg == "--version" || arg == "-v")
        {
            showVersion = true;
        }
        else if (arg == "--config" && i + 1 < argc)
        {
            configPath = std::string(argv[++i]);
        }
        else if (arg == "--network" && i + 1 < argc)
        {
            networkPreset = std::string(argv[++i]);
        }
        else if (arg == "--datadir" && i + 1 < argc)
        {
            dataDir = std::string(argv[++i]);
        }
        else if (arg == "--db-engine" && i + 1 < argc)
        {
            dbEngine = std::string(argv[++i]);
        }
        else if (arg == "--db-path" && i + 1 < argc)
        {
            dbPath = std::string(argv[++i]);
        }
        else if (arg == "--no-rpc")
        {
            noRpc = true;
        }
        else if (arg == "--status-interval" && i + 1 < argc)
        {
            try
            {
                const auto value = std::stoul(argv[++i]);
                if (value == 0) throw std::invalid_argument("status interval must be > 0");
                detail::StatusInterval() = std::chrono::seconds(value);
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Invalid --status-interval value: " << ex.what() << std::endl;
                return 1;
            }
        }
    }

    if (showHelp)
    {
        PrintUsage(config);
        return 0;
    }

    if (showVersion)
    {
        PrintVersion(config);
        return 0;
    }

    try
    {
        if (!networkPreset.empty() && config.allowNetworkPreset)
        {
            configPath = detail::ResolveNetworkConfigPath(networkPreset);
            std::cout << "Selected network preset '" << networkPreset << "' -> " << configPath << std::endl;
        }

        auto node = std::make_shared<neo::node::NeoNode>(configPath, dataDir);
        if (!dbEngine.empty()) node->SetStorageEngineOverride(dbEngine);
        if (!dbPath.empty()) node->SetStoragePathOverride(dbPath);
        if (noRpc) node->SetRpcEnabledOverride(false);

        if (!node->Initialize())
        {
            std::cerr << "Failed to initialize Neo node" << std::endl;
            return 1;
        }

        if (!node->Start())
        {
            std::cerr << "Failed to start Neo node" << std::endl;
            return 1;
        }

        if (onStarted)
        {
            onStarted(node);
        }

        const auto startTime = std::chrono::steady_clock::now();
        std::cout << config.appName << " started. Press Ctrl+C to stop." << std::endl;

        while (!detail::ShutdownRequested().load() && node->IsRunning())
        {
            detail::PrintStatus(node, startTime);

            auto slept = std::chrono::seconds(0);
            while (slept < detail::StatusInterval() && !detail::ShutdownRequested().load() && node->IsRunning())
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                slept += std::chrono::seconds(1);
            }
        }

        std::cout << "Stopping " << config.appName << "..." << std::endl;
        node->Stop();
        std::cout << config.appName << " stopped cleanly." << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
}  // namespace neo::node::app
