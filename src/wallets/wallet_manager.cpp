#include <neo/wallets/wallet_manager.h>

#include <iostream>

namespace neo::wallets
{
WalletManager& WalletManager::GetInstance()
{
    static WalletManager instance;
    return instance;
}

WalletManager::WalletManager() = default;

const std::vector<std::shared_ptr<Wallet>>& WalletManager::GetWallets() const { return wallets_; }

std::shared_ptr<Wallet> WalletManager::GetCurrentWallet() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return currentWallet_;
}

void WalletManager::SetCurrentWallet(std::shared_ptr<Wallet> wallet)
{
    std::lock_guard<std::mutex> lock(mutex_);
    currentWallet_ = wallet;
}

std::shared_ptr<Wallet> WalletManager::GetWallet(const std::string& path) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& wallet : wallets_)
    {
        if (wallet->GetPath() == path) return wallet;
    }

    return nullptr;
}

std::shared_ptr<Wallet> WalletManager::GetWalletByName(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& wallet : wallets_)
    {
        if (wallet->GetName() == name) return wallet;
    }

    return nullptr;
}

std::shared_ptr<Wallet> WalletManager::CreateWallet(const std::string& path)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if wallet already exists
    for (const auto& wallet : wallets_)
    {
        if (wallet->GetPath() == path) return wallet;
    }

    // Create new wallet
    auto wallet = std::make_shared<Wallet>(path);

    // Create default account
    wallet->CreateAccount();

    // Save wallet
    if (!wallet->Save())
    {
        std::cerr << "Failed to save wallet: " << path << std::endl;
        return nullptr;
    }

    // Add wallet to list
    wallets_.push_back(wallet);

    // Set as current wallet if none is set
    if (!currentWallet_) currentWallet_ = wallet;

    return wallet;
}

std::shared_ptr<Wallet> WalletManager::OpenWallet(const std::string& path)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if wallet is already open
    for (const auto& wallet : wallets_)
    {
        if (wallet->GetPath() == path) return wallet;
    }

    // Create wallet
    auto wallet = std::make_shared<Wallet>(path);

    // Load wallet
    if (!wallet->Load())
    {
        std::cerr << "Failed to load wallet: " << path << std::endl;
        return nullptr;
    }

    // Add wallet to list
    wallets_.push_back(wallet);

    // Set as current wallet if none is set
    if (!currentWallet_) currentWallet_ = wallet;

    return wallet;
}

bool WalletManager::CloseWallet(const std::string& path)
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = wallets_.begin(); it != wallets_.end(); ++it)
    {
        if ((*it)->GetPath() == path)
        {
            // Check if it's the current wallet
            if (currentWallet_ == *it) currentWallet_ = nullptr;

            wallets_.erase(it);
            return true;
        }
    }

    return false;
}

bool WalletManager::CloseWallet(std::shared_ptr<Wallet> wallet)
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = wallets_.begin(); it != wallets_.end(); ++it)
    {
        if (*it == wallet)
        {
            // Check if it's the current wallet
            if (currentWallet_ == *it) currentWallet_ = nullptr;

            wallets_.erase(it);
            return true;
        }
    }

    return false;
}

void WalletManager::CloseAllWallets()
{
    std::lock_guard<std::mutex> lock(mutex_);
    wallets_.clear();
    currentWallet_ = nullptr;
}
}  // namespace neo::wallets
