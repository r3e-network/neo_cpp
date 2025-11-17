#include <neo/logging/logger.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/channels_config.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/network_synchronizer.h>
#include <neo/node/neo_system.h>
#include <neo/rpc/rpc_methods.h>
#include <neo/rpc/rpc_server.h>
#include <neo/settings.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace
{
std::atomic<bool> g_shutdownRequested{false};
std::chrono::seconds g_statusInterval{std::chrono::seconds(30)};

void SignalHandler(int signal)
{
    g_shutdownRequested.store(true);
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
}

void PrintUsage()
{
    std::cout << "Neo C++ Node" << std::endl;
    std::cout << "Usage: neo_node [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --config <path>          Configuration file path" << std::endl;
    std::cout << "  --network <preset>       Network preset (mainnet|testnet|privnet)" << std::endl;
    std::cout << "  --db-engine <name>       Storage provider override" << std::endl;
    std::cout << "  --db-path <path>         Storage path override" << std::endl;
    std::cout << "  --no-rpc                 Disable RPC even if enabled in config" << std::endl;
    std::cout << "  --status-interval <sec>  Seconds between status reports (default 30)" << std::endl;
    std::cout << "  -h, --help               Show this help message" << std::endl;
    std::cout << "  -v, --version            Show version information" << std::endl;
}

void PrintVersion()
{
    std::cout << "Neo C++ Node" << std::endl;
    std::cout << "Build Date: " << __DATE__ << " " << __TIME__ << std::endl;
}

std::string ResolveNetworkConfigPath(const std::string& preset)
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

neo::network::IPEndPoint CreateBindEndpoint(const neo::P2PSettings& p2p)
{
    neo::network::IPAddress address = neo::network::IPAddress::Any();
    if (!p2p.BindAddress.empty())
    {
        neo::network::IPAddress parsed;
        if (neo::network::IPAddress::TryParse(p2p.BindAddress, parsed))
        {
            address = parsed;
        }
    }
    return neo::network::IPEndPoint(address, static_cast<uint16_t>(p2p.Port));
}

std::vector<neo::network::IPEndPoint> BuildSeedEndpoints(const std::vector<std::string>& seeds, uint16_t defaultPort)
{
    std::vector<neo::network::IPEndPoint> endpoints;
    endpoints.reserve(seeds.size());
    for (const auto& seed : seeds)
    {
        neo::network::IPEndPoint endpoint;
        if (neo::network::IPEndPoint::TryParse(seed, endpoint))
        {
            endpoints.push_back(endpoint);
        }
        else if (!seed.empty())
        {
            endpoints.emplace_back(seed, defaultPort);
        }
    }
    return endpoints;
}

std::string ResolvePeerListPath(const std::string& dataPath)
{
    namespace fs = std::filesystem;
    fs::path base = dataPath.empty() ? fs::path("./data") : fs::path(dataPath);
    std::error_code ec;
    if (fs::is_regular_file(base, ec))
    {
        base = base.parent_path();
    }
    if (base.empty())
    {
        base = fs::current_path();
    }
    fs::path peersFile = base / "peers.dat";
    if (auto parent = peersFile.parent_path(); !parent.empty())
    {
        fs::create_directories(parent, ec);
    }
    return peersFile.string();
}

void PrintStatus(const std::shared_ptr<neo::node::NeoSystem>& system,
                 const std::chrono::steady_clock::time_point& startTime)
{
    const auto now = std::chrono::steady_clock::now();
    const auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);

    uint32_t height = 0;
    size_t peers = 0;
    size_t mempool = 0;

    if (auto blockchain = system->GetBlockchain())
    {
        height = blockchain->GetHeight();
    }

    if (auto pool = system->GetMemoryPool())
    {
        mempool = pool->GetVerifiedTransactions().size() + pool->GetUnverifiedTransactions().size();
    }

    if (auto local = system->GetLocalNode())
    {
        peers = local->GetConnectedPeersCount();
    }

    std::cout << "[Status] Uptime=" << uptime.count() << "s"
              << " | Height=" << height << " | Peers=" << peers << " | Mempool=" << mempool << std::endl;
}
}  // namespace

int main(int argc, char* argv[])
{
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    bool noRpc = false;
    bool showHelp = false;
    bool showVersion = false;
    std::string configPath;
    std::string networkPreset;
    std::string dbEngine;
    std::string dbPath;

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
                g_statusInterval = std::chrono::seconds(value);
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
        PrintUsage();
        return 0;
    }

    if (showVersion)
    {
        PrintVersion();
        return 0;
    }

    try
    {
        if (configPath.empty() && !networkPreset.empty())
        {
            configPath = ResolveNetworkConfigPath(networkPreset);
            std::cout << "Selected network preset '" << networkPreset << "' -> " << configPath << std::endl;
        }

        neo::Settings settings = configPath.empty() ? neo::Settings::GetDefault() : neo::Settings::Load(configPath);

        if (!dbEngine.empty())
        {
            settings.Storage.Engine = dbEngine;
        }
        if (!dbPath.empty())
        {
            settings.Storage.Path = dbPath;
        }

        neo::rpc::RPCMethods::SetMaxFindResultItems(
            static_cast<size_t>(std::max(1, settings.RPC.MaxFindResultItems)));

        const auto peerListPath = ResolvePeerListPath(settings.Application.DataPath);
        neo::network::p2p::LocalNode::GetInstance().SetPeerListPath(peerListPath);
        std::cout << "Peer list path: " << peerListPath << std::endl;

        neo::network::p2p::ChannelsConfig channelsConfig;
        channelsConfig.SetTcp(CreateBindEndpoint(settings.P2P));
        channelsConfig.SetMinDesiredConnections(static_cast<uint32_t>(settings.P2P.MinDesiredConnections));
        channelsConfig.SetMaxConnections(static_cast<uint32_t>(settings.P2P.MaxConnections));
        channelsConfig.SetMaxConnectionsPerAddress(static_cast<uint32_t>(settings.P2P.MaxConnectionsPerAddress));
        channelsConfig.SetEnableCompression(settings.P2P.EnableCompression);
        channelsConfig.SetDialTimeoutMs(static_cast<uint32_t>(settings.P2P.DialTimeoutMs));

        auto seedEndpoints = BuildSeedEndpoints(settings.P2P.Seeds, static_cast<uint16_t>(settings.P2P.Port));
        if (seedEndpoints.empty() && settings.Protocol)
        {
            seedEndpoints = BuildSeedEndpoints(settings.Protocol->GetSeedList(),
                                               static_cast<uint16_t>(settings.P2P.Port));
        }
        if (!seedEndpoints.empty())
        {
            channelsConfig.SetSeedList(seedEndpoints);
        }

        auto system = std::make_shared<neo::node::NeoSystem>(settings.Protocol, settings.Storage.Engine,
                                                             settings.Storage.Path);
        system->SetNetworkConfig(channelsConfig);
        if (!system->Start())
        {
            std::cerr << "Failed to start Neo system" << std::endl;
            return 1;
        }

        std::shared_ptr<neo::rpc::RpcServer> rpc;
        if (settings.RPC.Enabled && !noRpc)
        {
            neo::rpc::RpcConfig rpcConfig;
            rpcConfig.port = static_cast<uint16_t>(settings.RPC.Port);
            rpcConfig.bind_address =
                settings.RPC.BindAddress.empty() ? std::string("0.0.0.0") : settings.RPC.BindAddress;
            rpcConfig.enable_cors = settings.RPC.EnableCors;
            rpcConfig.enable_authentication = !settings.RPC.Username.empty();
            rpcConfig.username = settings.RPC.Username;
            rpcConfig.password = settings.RPC.Password;
            rpcConfig.max_concurrent_requests = static_cast<uint32_t>(std::max(1, settings.RPC.MaxConnections));
            rpcConfig.max_iterator_items = static_cast<uint32_t>(std::max(1, settings.RPC.MaxIteratorResultItems));
            rpcConfig.max_request_size = static_cast<uint32_t>(settings.RPC.MaxRequestBodyBytes);
            rpcConfig.enable_rate_limiting = settings.RPC.EnableRateLimit;
            rpcConfig.max_requests_per_second =
                static_cast<uint32_t>(std::max(0, settings.RPC.MaxRequestsPerSecond));
            rpcConfig.rate_limit_window_seconds =
                static_cast<uint32_t>(std::max(1, settings.RPC.RateLimitWindowSeconds));
            rpcConfig.enable_ssl = settings.RPC.EnableSsl;
            rpcConfig.ssl_cert_path = settings.RPC.SslCert;
            rpcConfig.ssl_key_path = settings.RPC.SslKey;
            rpcConfig.trusted_authorities = settings.RPC.TrustedAuthorities;
            rpcConfig.ssl_ciphers = settings.RPC.SslCiphers;
            rpcConfig.min_tls_version = settings.RPC.MinTlsVersion;

            rpc = std::make_shared<neo::rpc::RpcServer>(rpcConfig, system);
            rpc->Start();
            std::cout << "RPC server listening on " << rpcConfig.bind_address << ":" << settings.RPC.Port << std::endl;
        }
        else
        {
            std::cout << "RPC server disabled" << std::endl;
        }

        if (auto synchronizer = system->GetNetworkSynchronizer())
        {
            synchronizer->SetStateChangedCallback(
                [](neo::network::p2p::SynchronizationState state)
                {
                    switch (state)
                    {
                        case neo::network::p2p::SynchronizationState::NotSynchronizing:
                            std::cout << "Synchronization: Not synchronizing" << std::endl;
                            break;
                        case neo::network::p2p::SynchronizationState::SynchronizingHeaders:
                            std::cout << "Synchronization: Synchronizing headers" << std::endl;
                            break;
                        case neo::network::p2p::SynchronizationState::SynchronizingBlocks:
                            std::cout << "Synchronization: Synchronizing blocks" << std::endl;
                            break;
                        case neo::network::p2p::SynchronizationState::Synchronized:
                            std::cout << "Synchronization: Synchronized" << std::endl;
                            break;
                    }
                });
        }

        const auto startTime = std::chrono::steady_clock::now();
        std::cout << "Neo node started on network " << (settings.Protocol ? settings.Protocol->GetNetwork() : 0)
                  << std::endl;
        std::cout << "P2P listening on " << settings.P2P.BindAddress << ":" << settings.P2P.Port << std::endl;

        while (!g_shutdownRequested.load())
        {
            PrintStatus(system, startTime);

            auto slept = std::chrono::seconds(0);
            while (slept < g_statusInterval && !g_shutdownRequested.load())
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                slept += std::chrono::seconds(1);
            }
        }

        if (rpc)
        {
            rpc->Stop();
        }
        system->Stop();
        std::cout << "Neo node stopped cleanly." << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
