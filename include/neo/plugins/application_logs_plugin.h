#pragma once

#include <neo/plugins/plugin_base.h>
#include <neo/ledger/block.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/io/uint256.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace neo::plugins
{
    /**
     * @brief Represents an application log.
     */
    struct ApplicationLog
    {
        /**
         * @brief The transaction hash.
         */
        io::UInt256 TxHash;

        /**
         * @brief The application engine state.
         */
        smartcontract::VMState State;

        /**
         * @brief The gas consumed.
         */
        int64_t GasConsumed;

        /**
         * @brief The stack.
         */
        std::vector<std::shared_ptr<smartcontract::vm::StackItem>> Stack;

        /**
         * @brief The notifications.
         */
        std::vector<smartcontract::NotifyEventArgs> Notifications;

        /**
         * @brief The exception.
         */
        std::string Exception;
    };

    /**
     * @brief Represents an application logs plugin.
     */
    class ApplicationLogsPlugin : public PluginBase
    {
    public:
        /**
         * @brief Constructs an ApplicationLogsPlugin.
         */
        ApplicationLogsPlugin();

        /**
         * @brief Gets the application log for a transaction.
         * @param txHash The transaction hash.
         * @return The application log, or nullptr if not found.
         */
        std::shared_ptr<ApplicationLog> GetApplicationLog(const io::UInt256& txHash) const;

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
        std::string logPath_;
        std::unordered_map<io::UInt256, std::shared_ptr<ApplicationLog>> logs_;
        mutable std::mutex mutex_;

        void OnBlockPersisted(std::shared_ptr<ledger::Block> block);
        void OnTransactionExecuted(std::shared_ptr<ledger::Transaction> transaction);
        void SaveLogs();
        void LoadLogs();
    };

    /**
     * @brief Represents an application logs plugin factory.
     */
    class ApplicationLogsPluginFactory : public PluginFactoryBase<ApplicationLogsPlugin>
    {
    };
}
