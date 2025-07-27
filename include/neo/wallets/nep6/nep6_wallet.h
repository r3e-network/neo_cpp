#pragma once

#include <memory>
#include <mutex>
#include <neo/cryptography/key_pair.h>
#include <neo/wallets/nep6/scrypt_parameters.h>
#include <neo/wallets/wallet.h>
#include <string>

namespace neo::wallets::nep6
{
/**
 * @brief Represents a NEP6 wallet account.
 */
class NEP6Account : public WalletAccount
{
  public:
    /**
     * @brief Constructs an empty NEP6Account.
     */
    NEP6Account();

    /**
     * @brief Constructs a NEP6Account with the specified script hash.
     * @param scriptHash The script hash.
     */
    explicit NEP6Account(const io::UInt160& scriptHash);

    /**
     * @brief Constructs a NEP6Account with the specified key pair.
     * @param keyPair The key pair.
     * @param password The password.
     * @param scrypt The scrypt parameters.
     */
    NEP6Account(const cryptography::ecc::KeyPair& keyPair, const std::string& password, const ScryptParameters& scrypt);

    /**
     * @brief Constructs a NEP6Account with the specified script hash and NEP2 key.
     * @param scriptHash The script hash.
     * @param nep2Key The NEP2 key.
     */
    NEP6Account(const io::UInt160& scriptHash, const std::string& nep2Key);

    /**
     * @brief Gets the NEP2 key.
     * @return The NEP2 key.
     */
    const std::string& GetNEP2Key() const;

    /**
     * @brief Sets the NEP2 key.
     * @param nep2Key The NEP2 key.
     */
    void SetNEP2Key(const std::string& nep2Key);

    /**
     * @brief Gets the extra data.
     * @return The extra data.
     */
    const nlohmann::json& GetExtra() const;

    /**
     * @brief Sets the extra data.
     * @param extra The extra data.
     */
    void SetExtra(const nlohmann::json& extra);

    /**
     * @brief Checks if the account is deployed.
     * @return True if the account is deployed, false otherwise.
     */
    bool IsDeployed() const;

    /**
     * @brief Sets whether the account is deployed.
     * @param deployed True to mark the account as deployed, false otherwise.
     */
    void SetDeployed(bool deployed);

    /**
     * @brief Gets the parameter names.
     * @return The parameter names.
     */
    const std::vector<std::string>& GetParameterNames() const;

    /**
     * @brief Sets the parameter names.
     * @param parameterNames The parameter names.
     */
    void SetParameterNames(const std::vector<std::string>& parameterNames);

    /**
     * @brief Decrypts the private key using the specified password.
     * @param password The password.
     * @param scrypt The scrypt parameters.
     * @return True if the decryption was successful, false otherwise.
     */
    bool DecryptPrivateKey(const std::string& password, const ScryptParameters& scrypt);

    /**
     * @brief Verifies the password.
     * @param password The password.
     * @param scrypt The scrypt parameters.
     * @return True if the password is correct, false otherwise.
     */
    bool VerifyPassword(const std::string& password, const ScryptParameters& scrypt) const;

    /**
     * @brief Serializes the NEP6Account to a JSON object.
     * @return The JSON object.
     */
    nlohmann::json ToJson() const override;

    /**
     * @brief Deserializes the NEP6Account from a JSON object.
     * @param json The JSON object.
     */
    void FromJson(const nlohmann::json& json) override;

  private:
    std::string nep2Key_;
    nlohmann::json extra_;
    bool deployed_;
    std::vector<std::string> parameterNames_;
};

/**
 * @brief Represents a NEP6 wallet.
 */
class NEP6Wallet : public Wallet
{
  public:
    /**
     * @brief Constructs a NEP6Wallet with the specified path and password.
     * @param path The path.
     * @param password The password.
     */
    NEP6Wallet(const std::string& path, const std::string& password);

    /**
     * @brief Constructs a NEP6Wallet with the specified path, password, and name.
     * @param path The path.
     * @param password The password.
     * @param name The name.
     */
    NEP6Wallet(const std::string& path, const std::string& password, const std::string& name);

    /**
     * @brief Gets the scrypt parameters.
     * @return The scrypt parameters.
     */
    const ScryptParameters& GetScrypt() const;

    /**
     * @brief Sets the scrypt parameters.
     * @param scrypt The scrypt parameters.
     */
    void SetScrypt(const ScryptParameters& scrypt);

    /**
     * @brief Gets the password.
     * @return The password.
     */
    const std::string& GetPassword() const;

    /**
     * @brief Sets the password.
     * @param password The password.
     * @return True if the password was changed, false otherwise.
     */
    bool ChangePassword(const std::string& oldPassword, const std::string& newPassword);

    /**
     * @brief Verifies the password.
     * @param password The password.
     * @return True if the password is correct, false otherwise.
     */
    bool VerifyPassword(const std::string& password) const;

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
     * @brief Imports an account from a NEP2 key.
     * @param nep2Key The NEP2 key.
     * @param password The password.
     * @return The imported account.
     */
    std::shared_ptr<WalletAccount> ImportFromNEP2(const std::string& nep2Key, const std::string& password);

    /**
     * @brief Serializes the NEP6Wallet to a JSON object.
     * @return The JSON object.
     */
    nlohmann::json ToJson() const override;

    /**
     * @brief Deserializes the NEP6Wallet from a JSON object.
     * @param json The JSON object.
     */
    void FromJson(const nlohmann::json& json) override;

  private:
    std::string password_;
    ScryptParameters scrypt_;
    nlohmann::json extra_;
};
}  // namespace neo::wallets::nep6
