#pragma once

#include <neo/sdk/core/types.h>
#include <neo/sdk/wallet/account.h>
#include <neo/wallets/nep6/nep6_wallet.h>
#include <memory>
#include <vector>
#include <string>

namespace neo::sdk::wallet {

/**
 * @brief High-level wallet interface for Neo blockchain
 * 
 * This class provides a simplified interface for wallet operations,
 * wrapping the underlying NEP-6 wallet implementation.
 */
class Wallet {
public:
    /**
     * @brief Create a new wallet
     * @param path Path where the wallet file will be saved
     * @param password Password to encrypt the wallet
     * @param name Optional wallet name
     * @return Shared pointer to the created wallet
     */
    static std::shared_ptr<Wallet> Create(
        const std::string& path,
        const std::string& password,
        const std::string& name = "Neo Wallet"
    );
    
    /**
     * @brief Open an existing wallet
     * @param path Path to the wallet file
     * @param password Password to decrypt the wallet
     * @return Shared pointer to the opened wallet
     * @throws std::runtime_error if wallet cannot be opened
     */
    static std::shared_ptr<Wallet> Open(
        const std::string& path,
        const std::string& password
    );
    
    /**
     * @brief Create a new account in the wallet
     * @param label Optional label for the account
     * @return The created account
     */
    Account CreateAccount(const std::string& label = "");
    
    /**
     * @brief Import an account from WIF
     * @param wif The private key in WIF format
     * @param label Optional label for the account
     * @return The imported account
     */
    Account ImportAccount(const std::string& wif, const std::string& label = "");
    
    /**
     * @brief Import an account from private key
     * @param privateKey The private key bytes
     * @param label Optional label for the account
     * @return The imported account
     */
    Account ImportAccount(const std::vector<uint8_t>& privateKey, const std::string& label = "");
    
    /**
     * @brief Get all accounts in the wallet
     * @return Vector of accounts
     */
    std::vector<Account> GetAccounts() const;
    
    /**
     * @brief Get account by address
     * @param address The account address
     * @return The account if found, nullptr otherwise
     */
    std::shared_ptr<Account> GetAccount(const std::string& address) const;
    
    /**
     * @brief Get account by script hash
     * @param scriptHash The account script hash
     * @return The account if found, nullptr otherwise
     */
    std::shared_ptr<Account> GetAccount(const core::UInt160& scriptHash) const;
    
    /**
     * @brief Delete an account from the wallet
     * @param address The account address to delete
     * @return true if account was deleted
     */
    bool DeleteAccount(const std::string& address);
    
    /**
     * @brief Get the default account
     * @return The default account, or first account if no default set
     */
    Account GetDefaultAccount() const;
    
    /**
     * @brief Set the default account
     * @param address The address to set as default
     * @return true if successful
     */
    bool SetDefaultAccount(const std::string& address);
    
    /**
     * @brief Check if wallet contains an account
     * @param address The account address
     * @return true if wallet contains the account
     */
    bool ContainsAccount(const std::string& address) const;
    
    /**
     * @brief Get wallet name
     * @return The wallet name
     */
    std::string GetName() const;
    
    /**
     * @brief Set wallet name
     * @param name The new wallet name
     */
    void SetName(const std::string& name);
    
    /**
     * @brief Get wallet version
     * @return The wallet version
     */
    std::string GetVersion() const;
    
    /**
     * @brief Save wallet to file
     * @return true if successful
     */
    bool Save();
    
    /**
     * @brief Save wallet to a different file
     * @param path The new file path
     * @return true if successful
     */
    bool SaveAs(const std::string& path);
    
    /**
     * @brief Lock the wallet
     */
    void Lock();
    
    /**
     * @brief Unlock the wallet
     * @param password The wallet password
     * @return true if successfully unlocked
     */
    bool Unlock(const std::string& password);
    
    /**
     * @brief Check if wallet is locked
     * @return true if wallet is locked
     */
    bool IsLocked() const;
    
    /**
     * @brief Change wallet password
     * @param oldPassword The current password
     * @param newPassword The new password
     * @return true if password was changed
     */
    bool ChangePassword(const std::string& oldPassword, const std::string& newPassword);
    
    /**
     * @brief Sign a message with an account
     * @param message The message to sign
     * @param account The account to sign with
     * @return The signature
     */
    std::vector<uint8_t> Sign(
        const std::vector<uint8_t>& message,
        const Account& account
    );
    
    /**
     * @brief Sign a transaction
     * @param transaction The transaction to sign
     * @return true if transaction was signed
     */
    bool SignTransaction(std::shared_ptr<core::Transaction> transaction);
    
    /**
     * @brief Get wallet file path
     * @return The file path
     */
    std::string GetPath() const;

private:
    // Private constructor - use Create() or Open()
    Wallet();
    
    // Implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace neo::sdk::wallet