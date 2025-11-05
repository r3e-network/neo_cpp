/**
 * @file application_logs_plugin.h
 * @brief Application Logs Plugin
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/plugins/plugin_base.h>
#include <neo/smartcontract/trigger_type.h>
#include <neo/vm/vm_state.h>

#include <nlohmann/json.hpp>

#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo::ledger
{
class Block;
struct ApplicationExecuted;
}  // namespace neo::ledger

namespace neo::plugins
{
class ApplicationLogsPluginTestHelper;

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
     * @brief The transaction hash if applicable.
     */
    std::optional<io::UInt256> TxHash;

    /**
     * @brief Represents a notification emitted during execution.
     */
    struct Notification
    {
        io::UInt160 Contract;
        std::string EventName;
        nlohmann::json State;
    };

    /**
     * @brief Represents a single execution (trigger) entry.
     */
    struct Execution
    {
        smartcontract::TriggerType Trigger{smartcontract::TriggerType::Application};
        neo::vm::VMState VmState{neo::vm::VMState::Halt};
        int64_t GasConsumed{0};
        std::string Exception;
        std::vector<nlohmann::json> Stack;
        std::vector<Notification> Notifications;
    };

    /**
     * @brief The block hash when available.
     */
    std::optional<io::UInt256> BlockHash;

    /**
     * @brief The execution entries associated with the hash.
     */
    std::vector<Execution> Executions;
};

/**
 * @brief Represents an application logs plugin.
 */
class ApplicationLogsPlugin : public PluginBase, public std::enable_shared_from_this<ApplicationLogsPlugin>
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

    /**
     * @brief Adds a new application log entry.
     * @param log The log entry to add.
     */
    void AddLog(std::shared_ptr<ApplicationLog> log);

   protected:
    bool OnInitialize(const std::unordered_map<std::string, std::string>& settings) override;
    bool OnStart() override;
    bool OnStop() override;

   private:
    void HandleCommitting(std::shared_ptr<ledger::Block> block,
                          const std::vector<ledger::ApplicationExecuted>& executions);
    void StoreLog(const io::UInt256& key, std::shared_ptr<ApplicationLog> log);
    void RemoveKey(const io::UInt256& key);
    void PruneCacheIfNeeded();
    ApplicationLog::Execution CreateExecution(const ledger::ApplicationExecuted& executed) const;

    std::string logPath_;
    std::unordered_map<io::UInt256, std::shared_ptr<ApplicationLog>> logs_;
    std::deque<io::UInt256> cacheOrder_;
    mutable std::mutex mutex_;
    bool handlerRegistered_{false};
    bool subscribed_{false};
    size_t maxCachedLogs_{1000};

    friend class ApplicationLogsPluginTestHelper;
};

/**
 * @brief Represents an application logs plugin factory.
 */
// class ApplicationLogsPluginFactory : public PluginFactoryBase<ApplicationLogsPlugin>
// {
// };
}  // namespace neo::plugins
