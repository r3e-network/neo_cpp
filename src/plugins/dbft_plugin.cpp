/**
 * @file dbft_plugin.cpp
 * @brief Dbft Plugin
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/consensus/consensus_service.h>
#include <neo/cryptography/ecc/key_pair.h>
#include <neo/plugins/dbft_plugin.h>
#include <neo/wallets/wallet.h>

#include <iostream>

namespace neo::plugins
{
DBFTPlugin::DBFTPlugin()
    : PluginBase("DBFT", "Provides DBFT consensus functionality", "1.0", "Neo C++ Team"), autoStart_(false)
{
}

bool DBFTPlugin::OnInitialize(const std::unordered_map<std::string, std::string>& settings)
{
    // Parse settings
    for (const auto& [key, value] : settings)
    {
        if (key == "WalletPath")
        {
            walletPath_ = value;
        }
        else if (key == "WalletPassword")
        {
            walletPassword_ = value;
        }
        else if (key == "AutoStart")
        {
            autoStart_ = (value == "true" || value == "1");
        }
    }

    return true;
}

bool DBFTPlugin::OnStart()
{
    // Auto start consensus
    if (autoStart_ && !walletPath_.empty())
    {
        try
        {
            // Open wallet
            auto wallet = wallets::Wallet::Open(walletPath_, walletPassword_);

            // Start consensus
            return StartConsensus(wallet);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Failed to auto start consensus: " << ex.what() << std::endl;
        }
    }

    return true;
}

bool DBFTPlugin::OnStop() { return StopConsensus(); }

bool DBFTPlugin::StartConsensus(std::shared_ptr<wallets::Wallet> wallet)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if consensus is already running
    if (consensusService_) return true;

    try
    {
        // Get default account
        auto account = wallet->GetDefaultAccount();
        if (!account)
        {
            std::cerr << "No default account found in wallet" << std::endl;
            return false;
        }

        // Get key pair
        auto keyPair = account->GetKeyPair();
        if (!keyPair)
        {
            std::cerr << "No key pair found for default account" << std::endl;
            return false;
        }

        // Create consensus service
        consensusService_ = std::make_shared<consensus::ConsensusService>(GetNeoSystem(), keyPair);

        // Start consensus service
        consensusService_->Start();

        std::cout << "Started consensus with account: " << account->GetScriptHash().ToString() << std::endl;
        return true;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to start consensus: " << ex.what() << std::endl;
        consensusService_.reset();
        return false;
    }
}

bool DBFTPlugin::StopConsensus()
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if consensus is running
    if (!consensusService_) return true;

    try
    {
        // Stop consensus service
        consensusService_->Stop();
        consensusService_.reset();

        std::cout << "Stopped consensus" << std::endl;
        return true;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to stop consensus: " << ex.what() << std::endl;
        return false;
    }
}

bool DBFTPlugin::IsConsensusRunning() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return consensusService_ != nullptr;
}

void DBFTPlugin::SetConsensusAutoStart(bool value)
{
    std::lock_guard<std::mutex> lock(mutex_);
    autoStart_ = value;
}
}  // namespace neo::plugins
