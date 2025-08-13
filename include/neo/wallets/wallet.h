/**
 * @file wallet.h
 * @brief Wallet
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/cryptography/ecc/keypair.h>
#include <neo/io/json_serializable.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/wallets/wallet_account.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo::wallets
{
/**
 * @brief Represents a wallet.
 */
class Wallet : public io::JsonSerializable
{
   public:
    /**
     * @brief Constructs an empty Wallet.
     */
    Wallet();

    /**
     * @brief Constructs a Wallet with the specified path.
     * @param path The path.
     */
    explicit Wallet(const std::string& path);

    /**
     * @brief Gets the path.
     * @return The path.
     */
    const std::string& GetPath() const;

    /**
     * @brief Sets the path.
     * @param path The path.
     */
    void SetPath(const std::string& path);

    /**
     * @brief Gets the name.
     * @return The name.
     */
    const std::string& GetName() const;

    /**
     * @brief Sets the name.
     * @param name The name.
     */
    void SetName(const std::string& name);

    /**
     * @brief Gets the version.
     * @return The version.
     */
    int32_t GetVersion() const;

    /**
     * @brief Sets the version.
     * @param version The version.
     */
    void SetVersion(int32_t version);

    /**
     * @brief Gets the accounts.
     * @return The accounts.
     */
    const std::vector<std::shared_ptr<WalletAccount>>& GetAccounts() const;

    /**
     * @brief Gets the default account.
     * @return The default account.
     */
    std::shared_ptr<WalletAccount> GetDefaultAccount() const;

    /**
     * @brief Sets the default account.
     * @param account The default account.
     */
    void SetDefaultAccount(std::shared_ptr<WalletAccount> account);

    /**
     * @brief Gets an account by script hash.
     * @param scriptHash The script hash.
     * @return The account, or nullptr if not found.
     */
    std::shared_ptr<WalletAccount> GetAccount(const io::UInt160& scriptHash) const;

    /**
     * @brief Gets an account by address.
     * @param address The address.
     * @return The account, or nullptr if not found.
     */
    std::shared_ptr<WalletAccount> GetAccount(const std::string& address) const;

    /**
     * @brief Creates a new account.
     * @return The new account.
     */
    std::shared_ptr<WalletAccount> CreateAccount();

    /**
     * @brief Creates a new account with the specified private key.
     * @param privateKey The private key.
     * @return The new account.
     */
    std::shared_ptr<WalletAccount> CreateAccount(const std::vector<uint8_t>& privateKey);

    /**
     * @brief Creates a new account with the specified key pair.
     * @param keyPair The key pair.
     * @return The new account.
     */
    std::shared_ptr<WalletAccount> CreateAccount(const cryptography::ecc::KeyPair& keyPair);

    /**
     * @brief Creates a new account with the specified WIF.
     * @param wif The WIF.
     * @return The new account.
     */
    std::shared_ptr<WalletAccount> CreateAccountFromWIF(const std::string& wif);

    /**
     * @brief Creates a new account with the specified script hash.
     * @param scriptHash The script hash.
     * @return The new account.
     */
    std::shared_ptr<WalletAccount> CreateAccount(const io::UInt160& scriptHash);

    /**
     * @brief Adds an account.
     * @param account The account.
     */
    void AddAccount(std::shared_ptr<WalletAccount> account);

    /**
     * @brief Removes an account.
     * @param scriptHash The script hash.
     * @return True if the account was removed, false otherwise.
     */
    bool RemoveAccount(const io::UInt160& scriptHash);

    /**
     * @brief Removes an account.
     * @param address The address.
     * @return True if the account was removed, false otherwise.
     */
    bool RemoveAccount(const std::string& address);

    /**
     * @brief Saves the wallet.
     * @return True if the wallet was saved, false otherwise.
     */
    bool Save();

    /**
     * @brief Saves the wallet to the specified path.
     * @param path The path.
     * @return True if the wallet was saved, false otherwise.
     */
    bool SaveAs(const std::string& path);

    /**
     * @brief Loads the wallet.
     * @return True if the wallet was loaded, false otherwise.
     */
    bool Load();

    /**
     * @brief Loads the wallet from the specified path.
     * @param path The path.
     * @return True if the wallet was loaded, false otherwise.
     */
    bool LoadFrom(const std::string& path);

    /**
     * @brief Serializes the Wallet to a JSON object.
     * @return The JSON object.
     */
    nlohmann::json ToJson() const override;

    /**
     * @brief Deserializes the Wallet from a JSON object.
     * @param json The JSON object.
     */
    void FromJson(const nlohmann::json& json) override;

   private:
    std::string path_;
    std::string name_;
    int32_t version_;
    std::vector<std::shared_ptr<WalletAccount>> accounts_;
    std::shared_ptr<WalletAccount> defaultAccount_;
    mutable std::mutex mutex_;
};
}  // namespace neo::wallets
