#include <neo/plugins/statistics/statistics_plugin.h>
#include <iostream>

namespace neo::plugins::statistics
{
    StatisticsPlugin::StatisticsPlugin()
        : Plugin("Statistics"), running_(false), blockCount_(0), transactionCount_(0), peerCount_(0), memoryPoolSize_(0), interval_(10), enableRPC_(true)
    {
    }
    
    StatisticsPlugin::~StatisticsPlugin()
    {
        Stop();
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
    
    bool StatisticsPlugin::Initialize(std::shared_ptr<node::Node> node, std::shared_ptr<rpc::RPCServer> rpcServer, const std::unordered_map<std::string, std::string>& settings)
    {
        node_ = node;
        rpcServer_ = rpcServer;
        
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
        
        // Register RPC methods
        if (enableRPC_ && rpcServer_)
        {
            rpcServer_->RegisterMethod("getstatistics", [this](const nlohmann::json& params) { return HandleGetStatistics(params); });
        }
        
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
        
        // Unregister RPC methods
        if (enableRPC_ && rpcServer_)
        {
            rpcServer_->UnregisterMethod("getstatistics");
        }
        
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
                // Collect block count
                blockCount_ = node_->GetBlockchain()->GetCurrentBlockIndex() + 1;
                
                // Collect transaction count
                transactionCount_ = 0; // This would require a database query
                
                // Collect peer count
                peerCount_ = node_->GetP2PServer()->GetConnectedPeers().size();
                
                // Collect memory pool size
                memoryPoolSize_ = node_->GetMemoryPool()->GetTransactions().size();
                
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
        result["blockCount"] = blockCount_;
        result["transactionCount"] = transactionCount_;
        result["peerCount"] = peerCount_;
        result["memoryPoolSize"] = memoryPoolSize_;
        return result;
    }
}
