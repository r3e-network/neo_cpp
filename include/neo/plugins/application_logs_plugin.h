/**
 * @file application_logs_plugin.h
 * @brief Application Logs Plugin
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/plugins/plugin_base.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/vm/vm_state.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo::plugins
{
/**
 * @brief Represents an application log.
 */
struct ApplicationLog
{
    /**
     * @brief Default constructor.
     */
    ApplicationLog() = default;

    /**
     * @brief The transaction hash.
     */
    io::UInt256 TxHash;

    /**
     * @brief The application engine state.
     */
    vm::VMState State = vm::VMState::None;

    /**
     * @brief The gas consumed.
     */
    int64_t GasConsumed = 0;

    /**
     * @brief The execution stack.
     */
    std::vector<std::string> Stack;

    /**
     * @brief The execution notifications.
     */
    std::vector<std::string> Notifications;

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
// class ApplicationLogsPluginFactory : public PluginFactoryBase<ApplicationLogsPlugin>
// {
// };
}  // namespace neo::plugins
