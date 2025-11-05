/**
 * @file application_logs_plugin.cpp
 * @brief Application log storage for RPC consumption
 */

#include <neo/plugins/application_logs_plugin.h>

#include <neo/cryptography/base64.h>
#include <neo/core/neo_system.h>
#include <neo/ledger/block.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/neo_system.h>
#include <neo/node/neo_system.h>
#include <neo/persistence/data_cache.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/trigger_type.h>
#include <neo/vm/special_items.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/stack_item_types.h>

#include <algorithm>
#include <filesystem>
#include <unordered_set>

namespace neo::plugins
{
namespace
{
std::string StackItemTypeToString(vm::StackItemType type)
{
    using vm::StackItemType;
    switch (type)
    {
        case StackItemType::Any:
            return "Any";
        case StackItemType::Pointer:
            return "Pointer";
        case StackItemType::Boolean:
            return "Boolean";
        case StackItemType::Integer:
            return "Integer";
        case StackItemType::ByteString:
            return "ByteString";
        case StackItemType::Buffer:
            return "Buffer";
        case StackItemType::Array:
            return "Array";
        case StackItemType::Struct:
            return "Struct";
        case StackItemType::Map:
            return "Map";
        case StackItemType::InteropInterface:
            return "InteropInterface";
        case StackItemType::Null:
            return "Null";
        default:
            return "Any";
    }
}

nlohmann::json SerializeStackItemInternal(const std::shared_ptr<vm::StackItem>& item,
                                          std::unordered_set<const vm::StackItem*>& seen)
{
    nlohmann::json result = nlohmann::json::object();
    if (!item)
    {
        result["type"] = "Any";
        return result;
    }

    const auto type = item->GetType();
    result["type"] = StackItemTypeToString(type);

    switch (type)
    {
        case vm::StackItemType::Boolean:
            result["value"] = item->GetBoolean();
            break;
        case vm::StackItemType::Integer:
            result["value"] = std::to_string(item->GetInteger());
            break;
        case vm::StackItemType::ByteString:
        case vm::StackItemType::Buffer:
        {
            auto bytes = item->GetByteArray();
            result["value"] = cryptography::Base64::Encode(bytes.AsSpan());
            break;
        }
        case vm::StackItemType::Array:
        case vm::StackItemType::Struct:
        {
            const auto* raw = item.get();
            if (!seen.insert(raw).second)
            {
                result["value"] = nlohmann::json::array();
                break;
            }

            nlohmann::json arrayJson = nlohmann::json::array();
            for (const auto& element : item->GetArray())
            {
                arrayJson.push_back(SerializeStackItemInternal(element, seen));
            }
            seen.erase(raw);
            result["value"] = std::move(arrayJson);
            break;
        }
        case vm::StackItemType::Map:
        {
            const auto* raw = item.get();
            if (!seen.insert(raw).second)
            {
                result["value"] = nlohmann::json::array();
                break;
            }

            nlohmann::json mapJson = nlohmann::json::array();
            for (const auto& [key, value] : item->GetMap())
            {
                nlohmann::json entry = nlohmann::json::object();
                entry["key"] = SerializeStackItemInternal(key, seen);
                entry["value"] = SerializeStackItemInternal(value, seen);
                mapJson.push_back(std::move(entry));
            }
            seen.erase(raw);
            result["value"] = std::move(mapJson);
            break;
        }
        case vm::StackItemType::Pointer:
        {
            if (auto pointer = std::dynamic_pointer_cast<vm::PointerItem>(item))
            {
                result["value"] = pointer->GetPosition();
            }
            break;
        }
        case vm::StackItemType::InteropInterface:
        case vm::StackItemType::Any:
        case vm::StackItemType::Null:
        default:
            break;
    }

    return result;
}

nlohmann::json SerializeStackItem(const std::shared_ptr<vm::StackItem>& item)
{
    std::unordered_set<const vm::StackItem*> seen;
    return SerializeStackItemInternal(item, seen);
}
}  // namespace

ApplicationLogsPlugin::ApplicationLogsPlugin()
    : PluginBase("ApplicationLogs", "Provides application logs functionality", "1.0", "Neo C++ Team"),
      logPath_("ApplicationLogs")
{
}

bool ApplicationLogsPlugin::OnInitialize(const std::unordered_map<std::string, std::string>& settings)
{
    auto it = settings.find("LogPath");
    if (it != settings.end())
    {
        logPath_ = it->second;
    }

    auto maxIt = settings.find("MaxCachedLogs");
    if (maxIt != settings.end())
    {
        try
        {
            auto parsed = std::stoull(maxIt->second);
            if (parsed > 0)
            {
                maxCachedLogs_ = parsed;
            }
        }
        catch (const std::exception&)
        {
            // Ignore invalid value and keep the default
        }
    }

    std::error_code ec;
    std::filesystem::create_directories(logPath_, ec);
    (void)ec;
    return true;
}

bool ApplicationLogsPlugin::OnStart()
{
    auto neoSystem = GetNeoSystem();
    if (!neoSystem) return false;
    auto blockchain = neoSystem->GetBlockchain();
    if (!blockchain) return false;

    if (!handlerRegistered_)
    {
        std::weak_ptr<ApplicationLogsPlugin> weakSelf =
            std::static_pointer_cast<ApplicationLogsPlugin>(shared_from_this());
        blockchain->RegisterCommittingHandler(
            [weakSelf](std::shared_ptr<neo::NeoSystem>,
                       std::shared_ptr<ledger::Block> block,
                       std::shared_ptr<persistence::DataCache>,
                       const std::vector<ledger::ApplicationExecuted>& executions)
            {
                if (auto self = weakSelf.lock())
                {
                    self->HandleCommitting(std::move(block), executions);
                }
            });
        handlerRegistered_ = true;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        subscribed_ = true;
    }
    return true;
}

bool ApplicationLogsPlugin::OnStop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    subscribed_ = false;
    return true;
}

std::shared_ptr<ApplicationLog> ApplicationLogsPlugin::GetApplicationLog(const io::UInt256& hash) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = logs_.find(hash);
    if (it == logs_.end()) return nullptr;
    return it->second;
}

void ApplicationLogsPlugin::AddLog(std::shared_ptr<ApplicationLog> log)
{
    if (!log) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (log->TxHash.has_value())
    {
        auto key = *log->TxHash;
        StoreLog(key, std::move(log));
    }
    else if (log->BlockHash.has_value())
    {
        auto key = *log->BlockHash;
        StoreLog(key, std::move(log));
    }
}

void ApplicationLogsPlugin::HandleCommitting(std::shared_ptr<ledger::Block> block,
                                             const std::vector<ledger::ApplicationExecuted>& executions)
{
    if (!block) return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (!subscribed_) return;

    const auto blockHash = block->GetHash();
    RemoveKey(blockHash);

    auto blockLog = std::make_shared<ApplicationLog>();
    blockLog->BlockHash = blockHash;
    bool hasBlockExecutions = false;

    for (const auto& executed : executions)
    {
        auto execution = CreateExecution(executed);
        if (executed.transaction)
        {
            auto txHash = executed.transaction->GetHash();
            auto txLog = std::make_shared<ApplicationLog>();
            txLog->TxHash = txHash;
            txLog->BlockHash = blockHash;
            txLog->Executions.push_back(std::move(execution));
            StoreLog(txHash, std::move(txLog));
        }
        else
        {
            hasBlockExecutions = true;
            blockLog->Executions.push_back(std::move(execution));
        }
    }

    if (hasBlockExecutions)
    {
        StoreLog(blockHash, std::move(blockLog));
    }
}

ApplicationLog::Execution ApplicationLogsPlugin::CreateExecution(const ledger::ApplicationExecuted& executed) const
{
    ApplicationLog::Execution execution;
    execution.GasConsumed = static_cast<int64_t>(executed.gas_consumed);
    execution.VmState = executed.vm_state;
    execution.Exception = executed.exception_message;

    if (executed.engine)
    {
        execution.Trigger = executed.engine->GetTrigger();

        auto resultStack = executed.engine->GetResultStack();
        execution.Stack.reserve(resultStack.size());
        for (const auto& item : resultStack)
        {
            execution.Stack.push_back(SerializeStackItem(item));
        }
    }
    else
    {
        execution.Trigger = smartcontract::TriggerType::Application;
    }

    for (const auto& notify : executed.notifications)
    {
        ApplicationLog::Notification notification;
        notification.Contract = notify.script_hash;
        notification.EventName = notify.event_name;

        nlohmann::json stateArray = nlohmann::json::array();
        for (const auto& stateItem : notify.state)
        {
            stateArray.push_back(SerializeStackItem(stateItem));
        }
        notification.State = nlohmann::json{
            {"type", "Array"},
            {"value", std::move(stateArray)},
        };

        execution.Notifications.push_back(std::move(notification));
    }

    return execution;
}

void ApplicationLogsPlugin::StoreLog(const io::UInt256& key, std::shared_ptr<ApplicationLog> log)
{
    RemoveKey(key);
    logs_.insert_or_assign(key, std::move(log));
    cacheOrder_.push_back(key);
    PruneCacheIfNeeded();
}

void ApplicationLogsPlugin::RemoveKey(const io::UInt256& key)
{
    auto removed = logs_.erase(key);
    if (removed == 0)
    {
        return;
    }

    auto it = std::remove(cacheOrder_.begin(), cacheOrder_.end(), key);
    cacheOrder_.erase(it, cacheOrder_.end());
}

void ApplicationLogsPlugin::PruneCacheIfNeeded()
{
    while (maxCachedLogs_ > 0 && logs_.size() > maxCachedLogs_)
    {
        if (cacheOrder_.empty())
        {
            logs_.clear();
            break;
        }
        const auto& oldest = cacheOrder_.front();
        logs_.erase(oldest);
        cacheOrder_.pop_front();
    }
}
}  // namespace neo::plugins
