/**
 * @file wallet_account.h
 * @brief Wallet Account
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/cryptography/ecc/keypair.h>
#include <neo/io/json_serializable.h>
#include <neo/io/uint160.h>
#include <neo/smartcontract/contract.h>

#include <string>
#include <vector>

namespace neo::wallets
{
/**
 * @brief Represents a wallet account.
 */
class WalletAccount : public io::JsonSerializable
{
   public:
    /**
     * @brief Constructs an empty WalletAccount.
     */
    WalletAccount();

    /**
     * @brief Constructs a WalletAccount with the specified key pair.
     * @param keyPair The key pair.
     */
    explicit WalletAccount(const cryptography::ecc::KeyPair& keyPair);

    /**
     * @brief Constructs a WalletAccount with the specified script hash.
     * @param scriptHash The script hash.
     */
    explicit WalletAccount(const io::UInt160& scriptHash);

    /**
     * @brief Gets the script hash.
     * @return The script hash.
     */
    const io::UInt160& GetScriptHash() const;

    /**
     * @brief Sets the script hash.
     * @param scriptHash The script hash.
     */
    void SetScriptHash(const io::UInt160& scriptHash);

    /**
     * @brief Gets the public key.
     * @return The public key.
     */
    const cryptography::ecc::ECPoint& GetPublicKey() const;

    /**
     * @brief Sets the public key.
     * @param publicKey The public key.
     */
    void SetPublicKey(const cryptography::ecc::ECPoint& publicKey);

    /**
     * @brief Gets the private key.
     * @return The private key.
     */
    const std::vector<uint8_t>& GetPrivateKey() const;

    /**
     * @brief Sets the private key.
     * @param privateKey The private key.
     */
    void SetPrivateKey(const std::vector<uint8_t>& privateKey);

    /**
     * @brief Gets the contract.
     * @return The contract.
     */
    const smartcontract::Contract& GetContract() const;

    /**
     * @brief Sets the contract.
     * @param contract The contract.
     */
    void SetContract(const smartcontract::Contract& contract);

    /**
     * @brief Gets the label.
     * @return The label.
     */
    const std::string& GetLabel() const;

    /**
     * @brief Sets the label.
     * @param label The label.
     */
    void SetLabel(const std::string& label);

    /**
     * @brief Checks if the account is locked.
     * @return True if the account is locked, false otherwise.
     */
    bool IsLocked() const;

    /**
     * @brief Sets whether the account is locked.
     * @param locked True to lock the account, false to unlock it.
     */
    void SetLocked(bool locked);

    /**
     * @brief Gets the WIF.
     * @return The WIF.
     */
    std::string GetWIF() const;

    /**
     * @brief Gets the address.
     * @return The address.
     */
    std::string GetAddress() const;

    /**
     * @brief Checks if the account has a private key.
     * @return True if the account has a private key, false otherwise.
     */
    bool HasPrivateKey() const;

    /**
     * @brief Serializes the WalletAccount to a JSON object.
     * @return The JSON object.
     */
    nlohmann::json ToJson() const override;

    /**
     * @brief Deserializes the WalletAccount from a JSON object.
     * @param json The JSON object.
     */
    void FromJson(const nlohmann::json& json) override;

   private:
    io::UInt160 scriptHash_;
    cryptography::ecc::ECPoint publicKey_;
    std::vector<uint8_t> privateKey_;
    smartcontract::Contract contract_;
    std::string label_;
    bool locked_;
};
}  // namespace neo::wallets
