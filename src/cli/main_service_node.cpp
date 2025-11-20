/**
 * @file main_service_node.cpp
 * @brief Service implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cli/console_helper.h>
#include <neo/cli/main_service.h>
#include <neo/ledger/memory_pool.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <neo/network/p2p/remote_node.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>

namespace
{
bool IsVerboseArgument(const std::string& value)
{
    if (value.empty())
        return false;

    std::string normalized = value;
    normalized.erase(normalized.begin(),
                     std::find_if(normalized.begin(), normalized.end(), [](unsigned char c) { return c != '-'; }));
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return normalized == "verbose" || normalized == "v" || normalized == "true" || normalized == "1";
}

uint32_t GetMaxPeerBlockHeight(const std::shared_ptr<neo::network::p2p::LocalNode>& localNode)
{
    if (!localNode)
        return 0;

    uint32_t maxHeight = 0;
    for (auto* peer : localNode->GetConnectedNodes())
    {
        if (peer)
        {
            maxHeight = std::max(maxHeight, peer->GetLastBlockIndex());
        }
    }
    return maxHeight;
}

size_t GetUnconnectedPeerCount(const std::shared_ptr<neo::network::p2p::LocalNode>& localNode)
{
    if (!localNode)
        return 0;

    try
    {
        return localNode->GetPeerList().GetUnconnectedCount();
    }
    catch (...)
    {
        return 0;
    }
}

std::string FormatDuration(std::chrono::seconds duration)
{
    const auto totalSeconds = duration.count();
    const auto days = totalSeconds / 86400;
    const auto hours = (totalSeconds % 86400) / 3600;
    const auto minutes = (totalSeconds % 3600) / 60;
    const auto seconds = totalSeconds % 60;

    std::ostringstream oss;
    oss << days << "d " << std::setw(2) << std::setfill('0') << hours << "h " << std::setw(2) << minutes << "m "
        << std::setw(2) << seconds << "s";
    return oss.str();
}

std::string FormatTimestamp(const std::chrono::system_clock::time_point& when)
{
    std::time_t now = std::chrono::system_clock::to_time_t(when);
    std::tm tmNow{};
#ifdef _WIN32
    localtime_s(&tmNow, &now);
#else
    localtime_r(&now, &tmNow);
#endif
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tmNow);
    return buffer;
}

std::string FormatGasAmount(int64_t datoshi)
{
    constexpr double kGasFactor = 100000000.0;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(8) << static_cast<double>(datoshi) / kGasFactor;
    return oss.str();
}

struct NodeStateSnapshot
{
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point captureTime;
    std::chrono::system_clock::time_point wallClock;
    uint32_t blockHeight = 0;
    uint32_t headerHeight = 0;
    uint32_t targetHeight = 0;
    uint32_t maxPeerHeight = 0;
    size_t connectedPeers = 0;
    size_t unconnectedPeers = 0;
    size_t verifiedPool = 0;
    size_t unverifiedPool = 0;
};

NodeStateSnapshot CaptureNodeSnapshot(const std::shared_ptr<neo::node::NeoSystem>& system,
                                      std::chrono::steady_clock::time_point startTime)
{
    NodeStateSnapshot snapshot;
    snapshot.startTime = startTime;
    snapshot.captureTime = std::chrono::steady_clock::now();
    snapshot.wallClock = std::chrono::system_clock::now();

    if (!system)
        return snapshot;

    auto blockchain = system->GetBlockchain();
    auto mempool = system->GetMemPool();
    auto localNode = system->GetLocalNode();
    auto synchronizer = system->GetNetworkSynchronizer();

    snapshot.blockHeight = blockchain ? blockchain->GetCurrentBlockIndex() : 0;
    snapshot.headerHeight = blockchain ? blockchain->GetHeaderHeight() : snapshot.blockHeight;
    snapshot.targetHeight = synchronizer ? synchronizer->GetTargetBlockIndex() : snapshot.headerHeight;
    snapshot.maxPeerHeight = GetMaxPeerBlockHeight(localNode);
    snapshot.connectedPeers = localNode ? localNode->GetConnectedCount() : 0;
    snapshot.unconnectedPeers = GetUnconnectedPeerCount(localNode);
    snapshot.verifiedPool = mempool ? mempool->GetSize() : 0;
    snapshot.unverifiedPool = mempool ? mempool->GetUnverifiedSize() : 0;

    return snapshot;
}

void RenderNodeSnapshot(const NodeStateSnapshot& snapshot)
{
    neo::cli::ConsoleHelper::Clear();

    const auto uptime = std::chrono::duration_cast<std::chrono::seconds>(snapshot.captureTime - snapshot.startTime);
    const auto timestamp = FormatTimestamp(snapshot.wallClock);

    const uint32_t syncTarget = std::max({snapshot.targetHeight, snapshot.maxPeerHeight, snapshot.headerHeight});
    double syncPercent = 100.0;
    if (syncTarget > 0)
    {
        syncPercent = (static_cast<double>(snapshot.blockHeight) / syncTarget) * 100.0;
        syncPercent = std::clamp(syncPercent, 0.0, 100.0);
    }

    std::ostringstream syncLine;
    syncLine << std::fixed << std::setprecision(2) << syncPercent;

    neo::cli::ConsoleHelper::Info("=============================================");
    neo::cli::ConsoleHelper::Info("             NEO NODE STATUS                 ");
    neo::cli::ConsoleHelper::Info("=============================================");
    neo::cli::ConsoleHelper::Info("Time: " + timestamp + "    Uptime: " + FormatDuration(uptime));
    neo::cli::ConsoleHelper::Info("");

    neo::cli::ConsoleHelper::Info("Blockchain:");
    neo::cli::ConsoleHelper::Info("  Block Height : " + std::to_string(snapshot.blockHeight));
    neo::cli::ConsoleHelper::Info("  Header Height: " + std::to_string(snapshot.headerHeight));
    neo::cli::ConsoleHelper::Info("  Target Height: " + std::to_string(syncTarget));
    neo::cli::ConsoleHelper::Info("  Sync Progress: " + syncLine.str() + "%");
    neo::cli::ConsoleHelper::Info("");

    neo::cli::ConsoleHelper::Info("Network:");
    neo::cli::ConsoleHelper::Info("  Connected Peers  : " + std::to_string(snapshot.connectedPeers));
    neo::cli::ConsoleHelper::Info("  Unconnected Peers: " + std::to_string(snapshot.unconnectedPeers));
    neo::cli::ConsoleHelper::Info("");

    neo::cli::ConsoleHelper::Info("Memory Pool:");
    neo::cli::ConsoleHelper::Info("  Verified   : " + std::to_string(snapshot.verifiedPool));
    neo::cli::ConsoleHelper::Info("  Unverified : " + std::to_string(snapshot.unverifiedPool));
    neo::cli::ConsoleHelper::Info("  Total      : " + std::to_string(snapshot.verifiedPool + snapshot.unverifiedPool));

    neo::cli::ConsoleHelper::Info("");
    neo::cli::ConsoleHelper::Info("Press ENTER to exit | Refreshes every second");
}
}  // namespace

namespace neo::cli
{
void MainService::InitializeNodeCommands()
{
    // Node Commands
    RegisterCommand(
        "showstate",
        [this](const std::vector<std::string>& args)
        {
            OnShowState();
            return true;
        },
        "Node");

    RegisterCommand(
        "showpool",
        [this](const std::vector<std::string>& args)
        {
            bool verbose = false;
            if (!args.empty())
            {
                verbose = IsVerboseArgument(args[0]);
            }
            OnShowPool(verbose);
            return true;
        },
        "Node");

    RegisterCommand(
        "showpeers",
        [this](const std::vector<std::string>& args)
        {
            OnShowPeers();
            return true;
        },
        "Node");
}

void MainService::OnShowState()
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    auto system = neoSystem_;
    std::atomic<bool> cancel{false};
    const auto startTime = std::chrono::steady_clock::now();

    ConsoleHelper::Info("Entering live node state view...");

    auto displayLoop = [system, startTime](std::atomic<bool>& stopToken)
    {
        while (!stopToken.load())
        {
            try
            {
                RenderNodeSnapshot(CaptureNodeSnapshot(system, startTime));
            }
            catch (const std::exception& ex)
            {
                ConsoleHelper::Error(std::string("Unable to render node state: ") + ex.what());
                break;
            }

            for (int i = 0; i < 10 && !stopToken.load(); ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    };

    std::thread displayThread(displayLoop, std::ref(cancel));

    ConsoleHelper::Info("Press ENTER to exit the state view.");
    std::string line;
    std::getline(std::cin, line);

    cancel.store(true);
    if (displayThread.joinable())
    {
        displayThread.join();
    }

    ConsoleHelper::Clear();
}

void MainService::OnShowPool(bool verbose)
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto memPool = neoSystem_->GetMemPool();
        if (!memPool)
        {
            ConsoleHelper::Warning("Memory pool not available");
            return;
        }

        const auto verifiedCount = memPool->GetSize();
        const auto unverifiedCount = memPool->GetUnverifiedSize();
        const auto totalCount = verifiedCount + unverifiedCount;

        ConsoleHelper::Info("Memory Pool Summary:");
        ConsoleHelper::Info("  Total: " + std::to_string(totalCount));
        ConsoleHelper::Info("  Verified: " + std::to_string(verifiedCount));
        ConsoleHelper::Info("  Unverified: " + std::to_string(unverifiedCount));

        if (verbose)
        {
            std::vector<network::p2p::payloads::Neo3Transaction> verified;
            std::vector<network::p2p::payloads::Neo3Transaction> unverified;
            memPool->GetVerifiedAndUnverifiedTransactions(verified, unverified);

            if (verified.empty())
            {
                ConsoleHelper::Info("Verified Transactions: (none)");
            }
            else
            {
                ConsoleHelper::Info("Verified Transactions:");
                for (const auto& tx : verified)
                {
                    std::ostringstream line;
                    line << "  " << tx.GetHash().ToString() << " fee=" << FormatGasAmount(tx.GetNetworkFee())
                         << " GAS";
                    ConsoleHelper::Info(line.str());
                }
            }

            if (unverified.empty())
            {
                ConsoleHelper::Info("Unverified Transactions: (none)");
            }
            else
            {
                ConsoleHelper::Info("Unverified Transactions:");
                for (const auto& tx : unverified)
                {
                    std::ostringstream line;
                    line << "  " << tx.GetHash().ToString() << " fee=" << FormatGasAmount(tx.GetNetworkFee())
                         << " GAS";
                    ConsoleHelper::Info(line.str());
                }
            }
        }
        else
        {
            ConsoleHelper::Info("Use 'showpool verbose' to list individual transactions.");
        }
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}

void MainService::OnShowPeers()
{
    if (!neoSystem_)
    {
        ConsoleHelper::Error("Neo system not initialized");
        return;
    }

    try
    {
        auto localNode = neoSystem_->GetLocalNode();
        auto peers = localNode ? localNode->GetConnectedNodes() : std::vector<network::p2p::RemoteNode*>{};

        ConsoleHelper::Info("Connected Peers: " + std::to_string(peers.size()));
        for (const auto& peer : peers)
        {
            ConsoleHelper::Info("  " + peer->GetRemoteEndPoint().ToString());
        }
    }
    catch (const std::exception& ex)
    {
        ConsoleHelper::Error(ex.what());
    }
}
}  // namespace neo::cli
