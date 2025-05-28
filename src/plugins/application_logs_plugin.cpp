#include <neo/plugins/application_logs_plugin.h>
#include <neo/ledger/blockchain.h>
#include <neo/io/json.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <fstream>
#include <iostream>
#include <filesystem>

namespace neo::plugins
{
    ApplicationLogsPlugin::ApplicationLogsPlugin()
        : PluginBase("ApplicationLogs", "Provides application logs functionality", "1.0", "Neo C++ Team"),
          logPath_("ApplicationLogs")
    {
    }

    bool ApplicationLogsPlugin::OnInitialize(const std::unordered_map<std::string, std::string>& settings)
    {
        // Parse settings
        for (const auto& [key, value] : settings)
        {
            if (key == "LogPath")
            {
                logPath_ = value;
            }
        }

        // Create log directory if it doesn't exist
        std::filesystem::create_directories(logPath_);

        // Load logs
        LoadLogs();

        return true;
    }

    bool ApplicationLogsPlugin::OnStart()
    {
        // Register callbacks
        auto& blockchain = GetNeoSystem()->GetBlockchain();
        blockchain.RegisterBlockPersistenceCallback([this](std::shared_ptr<ledger::Block> block) {
            OnBlockPersisted(block);
        });

        blockchain.RegisterTransactionExecutionCallback([this](std::shared_ptr<ledger::Transaction> transaction) {
            OnTransactionExecuted(transaction);
        });

        return true;
    }

    bool ApplicationLogsPlugin::OnStop()
    {
        // Save logs
        SaveLogs();

        return true;
    }

    std::shared_ptr<ApplicationLog> ApplicationLogsPlugin::GetApplicationLog(const io::UInt256& txHash) const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = logs_.find(txHash);
        if (it != logs_.end())
            return it->second;

        return nullptr;
    }

    void ApplicationLogsPlugin::OnBlockPersisted(std::shared_ptr<ledger::Block> block)
    {
        // Save logs
        SaveLogs();
    }

    void ApplicationLogsPlugin::OnTransactionExecuted(std::shared_ptr<ledger::Transaction> transaction)
    {
        // Create application log
        auto log = std::make_shared<ApplicationLog>();
        log->TxHash = transaction->GetHash();

        // Get application engine
        auto engine = transaction->GetApplicationEngine();
        if (engine)
        {
            log->State = engine->GetState();
            log->GasConsumed = engine->GetGasConsumed();
            log->Stack = engine->GetResultStack();
            log->Notifications = engine->GetNotifications();
            log->Exception = engine->GetException();
        }

        // Add log
        std::lock_guard<std::mutex> lock(mutex_);
        logs_[log->TxHash] = log;
    }

    void ApplicationLogsPlugin::SaveLogs()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (const auto& [txHash, log] : logs_)
        {
            // Create log file
            std::string logFile = logPath_ + "/" + txHash.ToString() + ".json";

            // Create log JSON
            nlohmann::json json;
            json["txid"] = txHash.ToString();
            json["state"] = log->State == smartcontract::VMState::HALT ? "HALT" : "FAULT";
            json["gasconsumed"] = log->GasConsumed;
            json["exception"] = log->Exception;

            // Add stack
            json["stack"] = nlohmann::json::array();
            for (const auto& item : log->Stack)
            {
                nlohmann::json stackItem;

                if (item->GetType() == smartcontract::vm::StackItemType::Integer)
                {
                    stackItem["type"] = "Integer";
                    stackItem["value"] = item->GetInteger();
                }
                else if (item->GetType() == smartcontract::vm::StackItemType::Boolean)
                {
                    stackItem["type"] = "Boolean";
                    stackItem["value"] = item->GetBoolean();
                }
                else if (item->GetType() == smartcontract::vm::StackItemType::ByteString)
                {
                    stackItem["type"] = "ByteString";
                    stackItem["value"] = item->GetByteArray().ToHexString();
                }
                else if (item->GetType() == smartcontract::vm::StackItemType::Buffer)
                {
                    stackItem["type"] = "Buffer";
                    stackItem["value"] = item->GetByteArray().ToHexString();
                }
                else
                {
                    stackItem["type"] = "Unknown";
                    stackItem["value"] = nullptr;
                }

                json["stack"].push_back(stackItem);
            }

            // Add notifications
            json["notifications"] = nlohmann::json::array();
            for (const auto& notification : log->Notifications)
            {
                nlohmann::json notificationJson;
                notificationJson["contract"] = notification.ScriptHash.ToString();
                notificationJson["eventname"] = notification.EventName;

                // Add arguments
                notificationJson["state"] = nlohmann::json::array();
                for (const auto& arg : notification.State)
                {
                    nlohmann::json argJson;

                    if (arg->GetType() == smartcontract::vm::StackItemType::Integer)
                    {
                        argJson["type"] = "Integer";
                        argJson["value"] = arg->GetInteger();
                    }
                    else if (arg->GetType() == smartcontract::vm::StackItemType::Boolean)
                    {
                        argJson["type"] = "Boolean";
                        argJson["value"] = arg->GetBoolean();
                    }
                    else if (arg->GetType() == smartcontract::vm::StackItemType::ByteString)
                    {
                        argJson["type"] = "ByteString";
                        argJson["value"] = arg->GetByteArray().ToHexString();
                    }
                    else if (arg->GetType() == smartcontract::vm::StackItemType::Buffer)
                    {
                        argJson["type"] = "Buffer";
                        argJson["value"] = arg->GetByteArray().ToHexString();
                    }
                    else
                    {
                        argJson["type"] = "Unknown";
                        argJson["value"] = nullptr;
                    }

                    notificationJson["state"].push_back(argJson);
                }

                json["notifications"].push_back(notificationJson);
            }

            // Write log file
            std::ofstream file(logFile);
            file << json.dump(4);
            file.close();
        }
    }

    void ApplicationLogsPlugin::LoadLogs()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Clear logs
        logs_.clear();

        // Check if log directory exists
        if (!std::filesystem::exists(logPath_))
            return;

        // Load log files
        for (const auto& entry : std::filesystem::directory_iterator(logPath_))
        {
            if (entry.path().extension() != ".json")
                continue;

            try
            {
                // Read log file
                std::ifstream file(entry.path());
                nlohmann::json json = nlohmann::json::parse(file);
                file.close();

                // Create application log
                auto log = std::make_shared<ApplicationLog>();
                log->TxHash = io::UInt256::Parse(json["txid"].get<std::string>());
                log->State = json["state"].get<std::string>() == "HALT" ? smartcontract::VMState::HALT : smartcontract::VMState::FAULT;
                log->GasConsumed = json["gasconsumed"].get<int64_t>();
                log->Exception = json["exception"].get<std::string>();

                // Add log
                logs_[log->TxHash] = log;
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Failed to load log file: " << entry.path() << " - " << ex.what() << std::endl;
            }
        }
    }
}
