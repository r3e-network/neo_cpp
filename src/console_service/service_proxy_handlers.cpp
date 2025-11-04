/**
 * @file service_proxy_handlers.cpp
 * @brief Service implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/console_service/service_proxy.h>
#include <neo/ledger/transaction.h>
#include <neo/plugins/plugin_manager.h>
#include <neo/smartcontract/contract_state.h>
#include <neo/wallets/wallet.h>

#include <iomanip>
#include <sstream>

namespace neo::console_service
{
// Proper implementation for wallet commands
std::string ServiceProxy::HandleWalletCommandsProper(const std::string& command)
{
    std::istringstream iss(command);
    std::string cmd, subcmd;
    iss >> cmd >> subcmd;

    if (subcmd == "create")
    {
        std::string filename;
        iss >> filename;
        if (filename.empty())
        {
            return "Error: Please specify wallet filename";
        }

        try
        {
            // Create new wallet
            auto wallet = wallets::Wallet::Create(filename);
            if (wallet)
            {
                return "Wallet created successfully: " + filename;
            }
            return "Error: Failed to create wallet";
        }
        catch (const std::exception& e)
        {
            return "Error creating wallet: " + std::string(e.what());
        }
    }
    else if (subcmd == "open")
    {
        std::string filename;
        iss >> filename;
        if (filename.empty())
        {
            return "Error: Please specify wallet filename";
        }

        try
        {
            // Open existing wallet
            auto wallet = wallets::Wallet::Open(filename);
            if (wallet)
            {
                current_wallet_ = wallet;
                return "Wallet opened successfully: " + filename;
            }
            return "Error: Failed to open wallet";
        }
        catch (const std::exception& e)
        {
            return "Error opening wallet: " + std::string(e.what());
        }
    }
    else if (subcmd == "close")
    {
        if (current_wallet_)
        {
            current_wallet_->Close();
            current_wallet_.reset();
            return "Wallet closed successfully";
        }
        return "No wallet is currently open";
    }
    else if (subcmd == "list")
    {
        if (!current_wallet_)
        {
            return "Error: No wallet is open";
        }

        std::ostringstream result;
        result << "Addresses in wallet:\n";
        auto accounts = current_wallet_->GetAccounts();
        for (const auto& account : accounts)
        {
            result << " - " << account->GetAddress() << "\n";
        }
        return result.str();
    }
    else if (subcmd == "balance")
    {
        if (!current_wallet_)
        {
            return "Error: No wallet is open";
        }

        std::string address;
        iss >> address;

        try
        {
            // Get balance for address
            auto balance = current_wallet_->GetBalance(address);
            std::ostringstream result;
            result << "Balance for " << address << ":\n";
            result << " NEO: " << balance.neo << "\n";
            result << " GAS: " << std::fixed << std::setprecision(8) << balance.gas << "\n";
            return result.str();
        }
        catch (const std::exception& e)
        {
            return "Error getting balance: " + std::string(e.what());
        }
    }
    else if (subcmd == "claim")
    {
        if (!current_wallet_)
        {
            return "Error: No wallet is open";
        }

        try
        {
            // Claim GAS
            auto tx = current_wallet_->ClaimGas();
            if (tx)
            {
                return "GAS claim transaction created: " + tx->GetHash().ToString();
            }
            return "No GAS available to claim";
        }
        catch (const std::exception& e)
        {
            return "Error claiming GAS: " + std::string(e.what());
        }
    }

    return "Unknown wallet command: " + subcmd;
}

// Proper implementation for contract commands
std::string ServiceProxy::HandleContractCommandsProper(const std::string& command)
{
    std::istringstream iss(command);
    std::string cmd, subcmd;
    iss >> cmd >> subcmd;

    if (subcmd == "deploy")
    {
        std::string nefFile, manifestFile;
        iss >> nefFile >> manifestFile;

        if (nefFile.empty() || manifestFile.empty())
        {
            return "Error: Please specify NEF file and manifest file";
        }

        try
        {
            // Deploy contract
            auto tx = neo_system_->DeployContract(nefFile, manifestFile);
            if (tx)
            {
                return "Contract deployment transaction: " + tx->GetHash().ToString();
            }
            return "Error: Failed to create deployment transaction";
        }
        catch (const std::exception& e)
        {
            return "Error deploying contract: " + std::string(e.what());
        }
    }
    else if (subcmd == "invoke")
    {
        std::string scriptHash, method;
        iss >> scriptHash >> method;

        if (scriptHash.empty() || method.empty())
        {
            return "Error: Please specify script hash and method";
        }

        try
        {
            // Parse parameters
            std::vector<std::string> params;
            std::string param;
            while (iss >> param)
            {
                params.push_back(param);
            }

            // Invoke contract
            auto result = neo_system_->InvokeContract(scriptHash, method, params);
            return "Invocation result: " + result.ToString();
        }
        catch (const std::exception& e)
        {
            return "Error invoking contract: " + std::string(e.what());
        }
    }
    else if (subcmd == "get")
    {
        std::string scriptHash;
        iss >> scriptHash;

        if (scriptHash.empty())
        {
            return "Error: Please specify script hash";
        }

        try
        {
            // Get contract info
            io::UInt160 hash;
            if (!io::UInt160::TryParse(scriptHash, hash))
            {
                return "Error: Invalid script hash format";
            }

            auto contract = neo_system_->GetContract(hash);
            if (contract)
            {
                std::ostringstream result;
                result << "Contract: " << hash.ToString() << "\n";
                result << " ID: " << contract->GetId() << "\n";
                result << " UpdateCounter: " << contract->GetUpdateCounter() << "\n";
                result << " Hash: " << contract->GetScriptHash().ToString() << "\n";
                return result.str();
            }
            return "Contract not found";
        }
        catch (const std::exception& e)
        {
            return "Error getting contract: " + std::string(e.what());
        }
    }

    return "Unknown contract command: " + subcmd;
}

// Proper implementation for transaction commands
std::string ServiceProxy::HandleTransactionCommandsProper(const std::string& command)
{
    std::istringstream iss(command);
    std::string cmd, subcmd;
    iss >> cmd >> subcmd;

    if (subcmd == "send")
    {
        std::string asset, from, to, amount;
        iss >> asset >> from >> to >> amount;

        if (asset.empty() || from.empty() || to.empty() || amount.empty())
        {
            return "Error: Please specify asset, from, to, and amount";
        }

        try
        {
            // Create and send transaction
            auto tx = neo_system_->CreateTransfer(asset, from, to, amount);
            if (tx)
            {
                neo_system_->SendTransaction(tx);
                return "Transaction sent: " + tx->GetHash().ToString();
            }
            return "Error: Failed to create transaction";
        }
        catch (const std::exception& e)
        {
            return "Error sending transaction: " + std::string(e.what());
        }
    }
    else if (subcmd == "get")
    {
        std::string txHash;
        iss >> txHash;

        if (txHash.empty())
        {
            return "Error: Please specify transaction hash";
        }

        try
        {
            // Get transaction info
            io::UInt256 hash;
            if (!io::UInt256::TryParse(txHash, hash))
            {
                return "Error: Invalid transaction hash format";
            }

            auto tx = neo_system_->GetTransaction(hash);
            if (tx)
            {
                std::ostringstream result;
                result << "Transaction: " << hash.ToString() << "\n";
                result << " Type: " << static_cast<int>(tx->GetType()) << "\n";
                result << " Version: " << static_cast<int>(tx->GetVersion()) << "\n";
                result << " Size: " << tx->GetSize() << " bytes\n";
                result << " Attributes: " << tx->GetAttributes().size() << "\n";
                result << " Witnesses: " << tx->GetWitnesses().size() << "\n";
                return result.str();
            }
            return "Transaction not found";
        }
        catch (const std::exception& e)
        {
            return "Error getting transaction: " + std::string(e.what());
        }
    }
    else if (subcmd == "broadcast")
    {
        std::string txData;
        iss >> txData;

        if (txData.empty())
        {
            return "Error: Please specify transaction data (hex)";
        }

        try
        {
            // Broadcast raw transaction
            auto result = neo_system_->BroadcastTransaction(txData);
            if (result)
            {
                return "Transaction broadcast successfully";
            }
            return "Error: Failed to broadcast transaction";
        }
        catch (const std::exception& e)
        {
            return "Error broadcasting transaction: " + std::string(e.what());
        }
    }

    return "Unknown transaction command: " + subcmd;
}

// Proper implementation for plugin commands
std::string ServiceProxy::HandlePluginCommandsProper(const std::string& command)
{
    std::istringstream iss(command);
    std::string cmd, subcmd;
    iss >> cmd >> subcmd;

    auto pluginManager = neo_system_->GetPluginManager();
    if (!pluginManager)
    {
        return "Error: Plugin manager not available";
    }

    if (subcmd == "list")
    {
        auto plugins = pluginManager->GetLoadedPlugins();
        std::ostringstream result;
        result << "Loaded plugins:\n";
        for (const auto& plugin : plugins)
        {
            result << " - " << plugin->GetName() << " v" << plugin->GetVersion() << "\n";
        }
        return result.str();
    }
    else if (subcmd == "load")
    {
        std::string pluginName;
        iss >> pluginName;

        if (pluginName.empty())
        {
            return "Error: Please specify plugin name";
        }

        try
        {
            if (pluginManager->LoadPlugin(pluginName))
            {
                return "Plugin loaded successfully: " + pluginName;
            }
            return "Error: Failed to load plugin";
        }
        catch (const std::exception& e)
        {
            return "Error loading plugin: " + std::string(e.what());
        }
    }
    else if (subcmd == "unload")
    {
        std::string pluginName;
        iss >> pluginName;

        if (pluginName.empty())
        {
            return "Error: Please specify plugin name";
        }

        try
        {
            if (pluginManager->UnloadPlugin(pluginName))
            {
                return "Plugin unloaded successfully: " + pluginName;
            }
            return "Error: Failed to unload plugin";
        }
        catch (const std::exception& e)
        {
            return "Error unloading plugin: " + std::string(e.what());
        }
    }

    return "Unknown plugin command: " + subcmd;
}

// Proper implementation for config commands
std::string ServiceProxy::HandleConfigCommandsProper(const std::string& command)
{
    std::istringstream iss(command);
    std::string cmd, subcmd;
    iss >> cmd >> subcmd;

    if (subcmd == "get")
    {
        std::string key;
        iss >> key;

        if (key.empty())
        {
            // Show all config
            auto config = neo_system_->GetConfiguration();
            std::ostringstream result;
            result << "Current configuration:\n";
            result << config.ToString();
            return result.str();
        }

        try
        {
            // Get specific config value
            auto value = neo_system_->GetConfigValue(key);
            return key + " = " + value;
        }
        catch (const std::exception& e)
        {
            return "Error getting config: " + std::string(e.what());
        }
    }
    else if (subcmd == "set")
    {
        std::string key, value;
        iss >> key >> value;

        if (key.empty() || value.empty())
        {
            return "Error: Please specify key and value";
        }

        try
        {
            neo_system_->SetConfigValue(key, value);
            return "Configuration updated: " + key + " = " + value;
        }
        catch (const std::exception& e)
        {
            return "Error setting config: " + std::string(e.what());
        }
    }
    else if (subcmd == "save")
    {
        try
        {
            neo_system_->SaveConfiguration();
            return "Configuration saved successfully";
        }
        catch (const std::exception& e)
        {
            return "Error saving configuration: " + std::string(e.what());
        }
    }

    return "Unknown config command: " + subcmd;
}
}  // namespace neo::console_service
