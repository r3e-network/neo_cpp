#include <iostream>
#include <neo/plugins/statistics/statistics_plugin.h>

namespace neo::plugins::statistics
{
StatisticsPlugin::StatisticsPlugin()
    : running_(false), blockCount_(0), transactionCount_(0), peerCount_(0), memoryPoolSize_(0),
      interval_(10), enableRPC_(true)
{
}

StatisticsPlugin::~StatisticsPlugin()
{
    Stop();
}

std::string StatisticsPlugin::GetName() const
{
    return "Statistics";
}

std::string StatisticsPlugin::GetDescription() const
{
    return "Collects and displays node statistics";
}

std::string StatisticsPlugin::GetVersion() const
{
    return "1.0.0";
}

std::string StatisticsPlugin::GetAuthor() const
{
    return "Neo C++ Team";
}

bool StatisticsPlugin::Initialize(std::shared_ptr<node::NeoSystem> neoSystem,
                                  const std::unordered_map<std::string, std::string>& settings)
{
    neoSystem_ = neoSystem;

    // Parse settings
    for (const auto& [key, value] : settings)
    {
        if (key == "Statistics.Interval")
        {
            try
            {
                interval_ = std::chrono::seconds(std::stoi(value));
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Failed to parse Statistics.Interval: " << ex.what() << std::endl;
                return false;
            }
        }
        else if (key == "Statistics.EnableRPC")
        {
            enableRPC_ = value == "true" || value == "1";
        }
    }

    // RPC registration would be handled by the RPC system
    // if (enableRPC_)
    // {
    //     // Register RPC methods through the RPC system
    // }

    return true;
}

bool StatisticsPlugin::Start()
{
    if (running_)
        return true;

    running_ = true;
    statisticsThread_ = std::thread(&StatisticsPlugin::CollectStatistics, this);

    std::cout << "Statistics plugin started" << std::endl;

    return true;
}

bool StatisticsPlugin::Stop()
{
    if (!running_)
        return true;

    running_ = false;

    if (statisticsThread_.joinable())
        statisticsThread_.join();

    // RPC cleanup would be handled by the RPC system
    // if (enableRPC_)
    // {
    //     // Unregister RPC methods
    // }

    std::cout << "Statistics plugin stopped" << std::endl;

    return true;
}

bool StatisticsPlugin::IsRunning() const
{
    return running_;
}

void StatisticsPlugin::CollectStatistics()
{
    while (running_)
    {
        try
        {
            // Collect basic statistics from NeoSystem
            if (neoSystem_)
            {
                // Block count would come from blockchain
                blockCount_ = 0;  // Would need GetCurrentBlockHeight() method
                
                // Transaction count would come from database
                transactionCount_ = 0;
                
                // Peer count would come from network
                peerCount_ = 0;
                
                // Memory pool size would come from mempool
                memoryPoolSize_ = 0;
            }

            // Log statistics
            std::cout << "Statistics: " << std::endl;
            std::cout << "  Block count: " << blockCount_ << std::endl;
            std::cout << "  Transaction count: " << transactionCount_ << std::endl;
            std::cout << "  Peer count: " << peerCount_ << std::endl;
            std::cout << "  Memory pool size: " << memoryPoolSize_ << std::endl;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Failed to collect statistics: " << ex.what() << std::endl;
        }

        // Sleep for the specified interval
        std::this_thread::sleep_for(interval_);
    }
}

nlohmann::json StatisticsPlugin::HandleGetStatistics(const nlohmann::json& params)
{
    nlohmann::json result;
    result["blockCount"] = blockCount_.load();
    result["transactionCount"] = transactionCount_.load();
    result["peerCount"] = peerCount_.load();
    result["memoryPoolSize"] = memoryPoolSize_.load();
    return result;
}
}  // namespace neo::plugins::statistics
