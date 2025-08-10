#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/scrypt.h>
#include <neo/io/json.h>
#include <neo/wallets/nep6/nep6_wallet.h>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace neo::wallets::nep6
{
// NEP6Account implementation
NEP6Account::NEP6Account() : deployed_(false) {}

NEP6Account::NEP6Account(const io::UInt160& scriptHash) : WalletAccount(scriptHash), deployed_(false) {}

NEP6Account::NEP6Account(const cryptography::ecc::KeyPair& keyPair, const std::string& password,
                         const ScryptParameters& scrypt)
    : WalletAccount(keyPair), deployed_(false)
{
    // Generate NEP2 key
    nep2Key_ = cryptography::ecc::Secp256r1::ToNEP2(keyPair.PrivateKey(), password, scrypt.GetN(), scrypt.GetR(),
                                                    scrypt.GetP());
}

NEP6Account::NEP6Account(const io::UInt160& scriptHash, const std::string& nep2Key)
    : WalletAccount(scriptHash), nep2Key_(nep2Key), deployed_(false)
{
}

const std::string& NEP6Account::GetNEP2Key() const { return nep2Key_; }

void NEP6Account::SetNEP2Key(const std::string& nep2Key) { nep2Key_ = nep2Key; }

const nlohmann::json& NEP6Account::GetExtra() const { return extra_; }

void NEP6Account::SetExtra(const nlohmann::json& extra) { extra_ = extra; }

bool NEP6Account::IsDeployed() const { return deployed_; }

void NEP6Account::SetDeployed(bool deployed) { deployed_ = deployed; }

const std::vector<std::string>& NEP6Account::GetParameterNames() const { return parameterNames_; }

void NEP6Account::SetParameterNames(const std::vector<std::string>& parameterNames)
{
    parameterNames_ = parameterNames;
}

bool NEP6Account::DecryptPrivateKey(const std::string& password, const ScryptParameters& scrypt)
{
    if (nep2Key_.empty()) return false;

    try
    {
        auto privateKey =
            cryptography::ecc::Secp256r1::FromNEP2(nep2Key_, password, scrypt.GetN(), scrypt.GetR(), scrypt.GetP());
        SetPrivateKey(privateKey);

        // Calculate public key
        auto keyPair = cryptography::ecc::Secp256r1::FromPrivateKey(privateKey);
        SetPublicKey(keyPair.PublicKey());

        return true;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to decrypt private key: " << ex.what() << std::endl;
        return false;
    }
}

bool NEP6Account::VerifyPassword(const std::string& password, const ScryptParameters& scrypt) const
{
    if (nep2Key_.empty()) return false;

    try
    {
        cryptography::ecc::Secp256r1::FromNEP2(nep2Key_, password, scrypt.GetN(), scrypt.GetR(), scrypt.GetP());
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

nlohmann::json NEP6Account::ToJson() const
{
    nlohmann::json json;
    json["address"] = GetAddress();

    if (!nep2Key_.empty()) json["key"] = nep2Key_;

    if (!GetLabel().empty()) json["label"] = GetLabel();

    json["isDefault"] = false;  // Will be set by the wallet
    json["lock"] = IsLocked();

    if (!GetContract().GetScript().IsEmpty())
    {
        nlohmann::json contractJson;
        contractJson["script"] = GetContract().GetScript().ToHexString();

        nlohmann::json parametersJson = nlohmann::json::array();
        for (size_t i = 0; i < GetContract().GetParameterList().size(); i++)
        {
            nlohmann::json parameterJson;
            parameterJson["name"] = i < parameterNames_.size() ? parameterNames_[i] : "parameter" + std::to_string(i);
            parameterJson["type"] = static_cast<uint8_t>(GetContract().GetParameterList()[i]);
            parametersJson.push_back(parameterJson);
        }
        contractJson["parameters"] = parametersJson;
        contractJson["deployed"] = deployed_;

        json["contract"] = contractJson;
    }

    if (!extra_.is_null()) json["extra"] = extra_;

    return json;
}

void NEP6Account::FromJson(const nlohmann::json& json)
{
    if (json.contains("address"))
    {
        auto address = json["address"].get<std::string>();
        SetScriptHash(io::UInt160::FromAddress(address));
    }

    if (json.contains("key")) nep2Key_ = json["key"].get<std::string>();

    if (json.contains("label")) SetLabel(json["label"].get<std::string>());

    if (json.contains("lock")) SetLocked(json["lock"].get<bool>());

    if (json.contains("contract") && !json["contract"].is_null())
    {
        auto contractJson = json["contract"];

        io::ByteVector script;
        if (contractJson.contains("script")) script = io::ByteVector::Parse(contractJson["script"].get<std::string>());

        std::vector<smartcontract::ContractParameterType> parameterList;
        parameterNames_.clear();

        if (contractJson.contains("parameters") && contractJson["parameters"].is_array())
        {
            for (const auto& parameterJson : contractJson["parameters"])
            {
                parameterList.push_back(
                    static_cast<smartcontract::ContractParameterType>(parameterJson["type"].get<uint8_t>()));

                if (parameterJson.contains("name"))
                    parameterNames_.push_back(parameterJson["name"].get<std::string>());
                else
                    parameterNames_.push_back("parameter" + std::to_string(parameterNames_.size()));
            }
        }

        SetContract(smartcontract::Contract(script, parameterList));

        if (contractJson.contains("deployed")) deployed_ = contractJson["deployed"].get<bool>();
    }

    if (json.contains("extra") && !json["extra"].is_null()) extra_ = json["extra"];
}

// NEP6Wallet implementation
NEP6Wallet::NEP6Wallet(const std::string& path, const std::string& password)
    : Wallet(path), password_(password), scrypt_(ScryptParameters::Default())
{
    if (std::filesystem::exists(path))
    {
        Load();
    }
}

NEP6Wallet::NEP6Wallet(const std::string& path, const std::string& password, const std::string& name)
    : Wallet(path), password_(password), scrypt_(ScryptParameters::Default())
{
    SetName(name);
    SetVersion(1);
}

const ScryptParameters& NEP6Wallet::GetScrypt() const { return scrypt_; }

void NEP6Wallet::SetScrypt(const ScryptParameters& scrypt) { scrypt_ = scrypt; }

const std::string& NEP6Wallet::GetPassword() const { return password_; }

bool NEP6Wallet::ChangePassword(const std::string& oldPassword, const std::string& newPassword)
{
    if (!VerifyPassword(oldPassword)) return false;

    // Change password for all accounts
    for (const auto& account : GetAccounts())
    {
        auto nep6Account = std::dynamic_pointer_cast<NEP6Account>(account);
        if (nep6Account && !nep6Account->GetNEP2Key().empty())
        {
            // Decrypt private key
            if (!nep6Account->DecryptPrivateKey(oldPassword, scrypt_)) return false;

            // Re-encrypt with new password
            auto privateKey = nep6Account->GetPrivateKey();
            auto nep2Key = cryptography::ecc::Secp256r1::ToNEP2(privateKey, newPassword, scrypt_.GetN(), scrypt_.GetR(),
                                                                scrypt_.GetP());
            nep6Account->SetNEP2Key(nep2Key);
        }
    }

    password_ = newPassword;
    return true;
}

bool NEP6Wallet::VerifyPassword(const std::string& password) const
{
    if (password_ != password) return false;

    // Verify password for all accounts
    for (const auto& account : GetAccounts())
    {
        auto nep6Account = std::dynamic_pointer_cast<NEP6Account>(account);
        if (nep6Account && !nep6Account->GetNEP2Key().empty())
        {
            if (!nep6Account->VerifyPassword(password, scrypt_)) return false;
        }
    }

    return true;
}

std::shared_ptr<WalletAccount> NEP6Wallet::CreateAccount()
{
    auto keyPair = cryptography::ecc::Secp256r1::GenerateKeyPair();
    return CreateAccount(keyPair);
}

std::shared_ptr<WalletAccount> NEP6Wallet::CreateAccount(const std::vector<uint8_t>& privateKey)
{
    auto keyPair = cryptography::ecc::Secp256r1::FromPrivateKey(privateKey);
    return CreateAccount(keyPair);
}

std::shared_ptr<WalletAccount> NEP6Wallet::CreateAccount(const cryptography::ecc::KeyPair& keyPair)
{
    auto account = std::make_shared<NEP6Account>(keyPair, password_, scrypt_);

    // Create contract
    auto contract = smartcontract::Contract::CreateSignatureContract(keyPair.PublicKey());
    account->SetContract(contract);

    // Set parameter names
    std::vector<std::string> parameterNames = {"signature"};
    account->SetParameterNames(parameterNames);

    AddAccount(account);
    return account;
}

std::shared_ptr<WalletAccount> NEP6Wallet::CreateAccountFromWIF(const std::string& wif)
{
    auto keyPair = cryptography::ecc::Secp256r1::FromWIF(wif);
    return CreateAccount(keyPair);
}

std::shared_ptr<WalletAccount> NEP6Wallet::CreateAccount(const io::UInt160& scriptHash)
{
    auto account = std::make_shared<NEP6Account>(scriptHash);
    AddAccount(account);
    return account;
}

std::shared_ptr<WalletAccount> NEP6Wallet::ImportFromNEP2(const std::string& nep2Key, const std::string& password)
{
    // Decrypt private key
    auto privateKey =
        cryptography::ecc::Secp256r1::FromNEP2(nep2Key, password, scrypt_.GetN(), scrypt_.GetR(), scrypt_.GetP());

    // Create key pair
    auto keyPair = cryptography::ecc::Secp256r1::FromPrivateKey(privateKey);

    // Create account
    auto account = std::make_shared<NEP6Account>(keyPair.GetScriptHash(), nep2Key);

    // Set public key
    account->SetPublicKey(keyPair.PublicKey());

    // Create contract
    auto contract = smartcontract::Contract::CreateSignatureContract(keyPair.PublicKey());
    account->SetContract(contract);

    // Set parameter names
    std::vector<std::string> parameterNames = {"signature"};
    account->SetParameterNames(parameterNames);

    AddAccount(account);
    return account;
}

nlohmann::json NEP6Wallet::ToJson() const
{
    nlohmann::json json;
    json["name"] = GetName();
    json["version"] = GetVersion();
    json["scrypt"] = scrypt_.ToJson();

    nlohmann::json accountsJson = nlohmann::json::array();
    auto defaultAccount = GetDefaultAccount();

    for (const auto& account : GetAccounts())
    {
        auto accountJson = account->ToJson();
        accountJson["isDefault"] = (account == defaultAccount);
        accountsJson.push_back(accountJson);
    }

    json["accounts"] = accountsJson;

    if (!extra_.is_null()) json["extra"] = extra_;

    return json;
}

void NEP6Wallet::FromJson(const nlohmann::json& json)
{
    if (json.contains("name")) SetName(json["name"].get<std::string>());

    if (json.contains("version")) SetVersion(json["version"].get<int32_t>());

    if (json.contains("scrypt")) scrypt_.FromJson(json["scrypt"]);

    if (json.contains("accounts") && json["accounts"].is_array())
    {
        std::shared_ptr<WalletAccount> defaultAccount;

        for (const auto& accountJson : json["accounts"])
        {
            auto account = std::make_shared<NEP6Account>();
            account->FromJson(accountJson);

            AddAccount(account);

            if (accountJson.contains("isDefault") && accountJson["isDefault"].get<bool>()) defaultAccount = account;
        }

        if (defaultAccount) SetDefaultAccount(defaultAccount);
    }

    if (json.contains("extra") && !json["extra"].is_null()) extra_ = json["extra"];
}
}  // namespace neo::wallets::nep6
