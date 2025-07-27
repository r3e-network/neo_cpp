#pragma once

#include <memory>
#include <neo/wallets/wallet.h>
#include <string>
#include <vector>

namespace neo::wallets
{
/**
 * @brief Interface for wallet factories.
 */
class WalletFactory
{
  public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~WalletFactory() = default;

    /**
     * @brief Checks if the factory can handle the specified path.
     * @param path The path.
     * @return True if the factory can handle the path, false otherwise.
     */
    virtual bool CanHandle(const std::string& path) const = 0;

    /**
     * @brief Creates a new wallet.
     * @param path The path.
     * @param password The password.
     * @param name The name.
     * @return The new wallet.
     */
    virtual std::shared_ptr<Wallet> CreateWallet(const std::string& path, const std::string& password,
                                                 const std::string& name) const = 0;

    /**
     * @brief Opens an existing wallet.
     * @param path The path.
     * @param password The password.
     * @return The opened wallet.
     */
    virtual std::shared_ptr<Wallet> OpenWallet(const std::string& path, const std::string& password) const = 0;
};

/**
 * @brief Manages wallet factories.
 */
class WalletFactoryManager
{
  public:
    /**
     * @brief Gets the singleton instance of the WalletFactoryManager.
     * @return The singleton instance.
     */
    static WalletFactoryManager& GetInstance();

    /**
     * @brief Registers a wallet factory.
     * @param factory The factory.
     */
    void RegisterFactory(std::shared_ptr<WalletFactory> factory);

    /**
     * @brief Gets a factory that can handle the specified path.
     * @param path The path.
     * @return The factory, or nullptr if no factory can handle the path.
     */
    std::shared_ptr<WalletFactory> GetFactory(const std::string& path) const;

    /**
     * @brief Creates a new wallet.
     * @param path The path.
     * @param password The password.
     * @param name The name.
     * @return The new wallet.
     */
    std::shared_ptr<Wallet> CreateWallet(const std::string& path, const std::string& password,
                                         const std::string& name) const;

    /**
     * @brief Opens an existing wallet.
     * @param path The path.
     * @param password The password.
     * @return The opened wallet.
     */
    std::shared_ptr<Wallet> OpenWallet(const std::string& path, const std::string& password) const;

  private:
    WalletFactoryManager();
    std::vector<std::shared_ptr<WalletFactory>> factories_;
};
}  // namespace neo::wallets
