#pragma once

#include <neo/plugins/plugin_base.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>

namespace neo::plugins
{
    /**
     * @brief Represents a statistics plugin.
     */
    class StatisticsPlugin : public PluginBase
    {
    public:
        /**
         * @brief Constructs a StatisticsPlugin.
         */
        StatisticsPlugin();

        /**
         * @brief Destructor.
         */
        virtual ~StatisticsPlugin();

    protected:
        /**
         * @brief Initializes the plugin.
         * @param settings The settings.
         * @return True if the plugin was initialized, false otherwise.
         */
        bool OnInitialize(const std::unordered_map<std::string, std::string>& settings) override;

        /**
         * @brief Starts the plugin.
         * @return True if the plugin was started, false otherwise.
         */
        bool OnStart() override;

        /**
         * @brief Stops the plugin.
         * @return True if the plugin was stopped, false otherwise.
         */
        bool OnStop() override;

    private:
        std::atomic<uint64_t> blockCount_;
        std::atomic<uint64_t> transactionCount_;
        std::atomic<uint64_t> peerCount_;
        std::atomic<uint64_t> memoryPoolSize_;
        std::chrono::seconds interval_;
        std::thread statisticsThread_;
        std::mutex mutex_;
        std::condition_variable condition_;
        int32_t blockCallbackId_;
        int32_t transactionCallbackId_;

        /**
         * @brief Runs the statistics thread.
         */
        void RunStatistics();

        /**
         * @brief Handles a block persistence.
         * @param block The block.
         */
        void OnBlockPersisted(std::shared_ptr<ledger::Block> block);

        /**
         * @brief Handles a transaction execution.
         * @param transaction The transaction.
         */
        void OnTransactionExecuted(std::shared_ptr<ledger::Transaction> transaction);

        /**
         * @brief Collects statistics.
         */
        void CollectStatistics();

        /**
         * @brief Reports statistics.
         */
        void ReportStatistics();
    };

    /**
     * @brief Represents a statistics plugin factory.
     */
    class StatisticsPluginFactory : public PluginFactoryBase<StatisticsPlugin>
    {
    };
}
