#include <neo/wallets/wallet_factory.h>
#include <neo/wallets/nep6/nep6_wallet_factory.h>
#include <stdexcept>

namespace neo::wallets
{
    WalletFactoryManager::WalletFactoryManager()
    {
        // Register default factories
        RegisterFactory(nep6::NEP6WalletFactory::GetInstance());
    }

    WalletFactoryManager& WalletFactoryManager::GetInstance()
    {
        static WalletFactoryManager instance;
        return instance;
    }

    void WalletFactoryManager::RegisterFactory(std::shared_ptr<WalletFactory> factory)
    {
        factories_.push_back(factory);
    }

    std::shared_ptr<WalletFactory> WalletFactoryManager::GetFactory(const std::string& path) const
    {
        for (const auto& factory : factories_)
        {
            if (factory->CanHandle(path))
                return factory;
        }
        
        return nullptr;
    }

    std::shared_ptr<Wallet> WalletFactoryManager::CreateWallet(const std::string& path, const std::string& password, const std::string& name) const
    {
        auto factory = GetFactory(path);
        if (!factory)
            throw std::invalid_argument("No factory can handle the specified path.");
        
        return factory->CreateWallet(path, password, name);
    }

    std::shared_ptr<Wallet> WalletFactoryManager::OpenWallet(const std::string& path, const std::string& password) const
    {
        auto factory = GetFactory(path);
        if (!factory)
            throw std::invalid_argument("No factory can handle the specified path.");
        
        return factory->OpenWallet(path, password);
    }
}
