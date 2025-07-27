#include <iostream>
#include <neo/plugins/statistics_plugin.h>

namespace neo::plugins
{
StatisticsPlugin::StatisticsPlugin()
    : PluginBase("Statistics", "Collects and reports node statistics", "1.0", "Neo C++ Team"), blockCount_(0),
      transactionCount_(0), peerCount_(0), memoryPoolSize_(0), interval_(60), blockCallbackId_(-1),
      transactionCallbackId_(-1)
{
}

StatisticsPlugin::~StatisticsPlugin()
{
    OnStop();
}

bool StatisticsPlugin::OnInitialize(const std::unordered_map<std::string, std::string>& settings)
{
    // Parse settings
    if (settings.find("StatisticsInterval") != settings.end())
    {
        try
        {
            interval_ = std::chrono::seconds(std::stoul(settings.at("StatisticsInterval")));
        }
        catch (const std::exception&)
        {
            // Ignore
        }
    }

    return true;
}

bool StatisticsPlugin::OnStart()
{
    auto node = GetNode();
    if (!node)
        return false;

    // Register callbacks
    blockCallbackId_ = node->RegisterBlockPersistenceCallback(
        std::bind(&StatisticsPlugin::OnBlockPersisted, this, std::placeholders::_1));
    transactionCallbackId_ = node->RegisterTransactionExecutionCallback(
        std::bind(&StatisticsPlugin::OnTransactionExecuted, this, std::placeholders::_1));

    // Start statistics thread
    statisticsThread_ = std::thread(&StatisticsPlugin::RunStatistics, this);

    return true;
}

bool StatisticsPlugin::OnStop()
{
    auto node = GetNode();
    if (node)
    {
        // Unregister callbacks
        if (blockCallbackId_ >= 0)
        {
            node->UnregisterBlockPersistenceCallback(blockCallbackId_);
            blockCallbackId_ = -1;
        }

        if (transactionCallbackId_ >= 0)
        {
            node->UnregisterTransactionExecutionCallback(transactionCallbackId_);
            transactionCallbackId_ = -1;
        }
    }

    // Stop statistics thread
    {
        std::lock_guard<std::mutex> lock(mutex_);
        condition_.notify_all();
    }

    if (statisticsThread_.joinable())
        statisticsThread_.join();

    return true;
}

void StatisticsPlugin::RunStatistics()
{
    while (IsRunning())
    {
        // Collect statistics
        CollectStatistics();

        // Report statistics
        ReportStatistics();

        // Wait for interval
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait_for(lock, interval_);
    }
}

void StatisticsPlugin::OnBlockPersisted(std::shared_ptr<ledger::Block> block)
{
    blockCount_++;
    transactionCount_ += block->GetTransactions().size();
}

void StatisticsPlugin::OnTransactionExecuted(std::shared_ptr<ledger::Transaction> transaction)
{
    // Nothing to do here
}

void StatisticsPlugin::CollectStatistics()
{
    auto node = GetNode();
    if (!node)
        return;

    // Get peer count
    peerCount_ = node->GetP2PServer()->GetConnectedPeers().size();

    // Get memory pool size
    memoryPoolSize_ = node->GetMemoryPool()->GetCount();
}

void StatisticsPlugin::ReportStatistics()
{
    std::cout << "=== Node Statistics ===" << std::endl;
    std::cout << "Block count: " << blockCount_ << std::endl;
    std::cout << "Transaction count: " << transactionCount_ << std::endl;
    std::cout << "Peer count: " << peerCount_ << std::endl;
    std::cout << "Memory pool size: " << memoryPoolSize_ << std::endl;
    std::cout << "======================" << std::endl;
}
}  // namespace neo::plugins
