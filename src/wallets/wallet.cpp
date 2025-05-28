#include <neo/wallets/wallet.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/io/json.h>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace neo::wallets
{

    // Wallet implementation
    Wallet::Wallet()
        : version_(1)
    {
    }

    Wallet::Wallet(const std::string& path)
        : path_(path), version_(1)
    {
        // Extract name from path
        std::filesystem::path fsPath(path);
        name_ = fsPath.stem().string();
    }

    const std::string& Wallet::GetPath() const
    {
        return path_;
    }

    void Wallet::SetPath(const std::string& path)
    {
        path_ = path;

        // Extract name from path
        std::filesystem::path fsPath(path);
        name_ = fsPath.stem().string();
    }

    const std::string& Wallet::GetName() const
    {
        return name_;
    }

    void Wallet::SetName(const std::string& name)
    {
        name_ = name;
    }

    int32_t Wallet::GetVersion() const
    {
        return version_;
    }

    void Wallet::SetVersion(int32_t version)
    {
        version_ = version;
    }

    const std::vector<std::shared_ptr<WalletAccount>>& Wallet::GetAccounts() const
    {
        return accounts_;
    }

    std::shared_ptr<WalletAccount> Wallet::GetDefaultAccount() const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (defaultAccount_)
            return defaultAccount_;

        if (!accounts_.empty())
            return accounts_[0];

        return nullptr;
    }

    void Wallet::SetDefaultAccount(std::shared_ptr<WalletAccount> account)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        defaultAccount_ = account;
    }

    std::shared_ptr<WalletAccount> Wallet::GetAccount(const io::UInt160& scriptHash) const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (const auto& account : accounts_)
        {
            if (account->GetScriptHash() == scriptHash)
                return account;
        }

        return nullptr;
    }

    std::shared_ptr<WalletAccount> Wallet::GetAccount(const std::string& address) const
    {
        try
        {
            auto scriptHash = io::UInt160::FromAddress(address);
            return GetAccount(scriptHash);
        }
        catch (const std::exception&)
        {
            return nullptr;
        }
    }

    std::shared_ptr<WalletAccount> Wallet::CreateAccount()
    {
        auto keyPair = cryptography::ecc::Secp256r1::GenerateKeyPair();
        return CreateAccount(keyPair);
    }

    std::shared_ptr<WalletAccount> Wallet::CreateAccount(const std::vector<uint8_t>& privateKey)
    {
        auto keyPair = cryptography::ecc::Secp256r1::FromPrivateKey(privateKey);
        return CreateAccount(keyPair);
    }

    std::shared_ptr<WalletAccount> Wallet::CreateAccount(const cryptography::ecc::KeyPair& keyPair)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto account = std::make_shared<WalletAccount>(keyPair);
        accounts_.push_back(account);

        if (!defaultAccount_)
            defaultAccount_ = account;

        return account;
    }

    std::shared_ptr<WalletAccount> Wallet::CreateAccountFromWIF(const std::string& wif)
    {
        auto keyPair = cryptography::ecc::Secp256r1::FromWIF(wif);
        return CreateAccount(keyPair);
    }

    std::shared_ptr<WalletAccount> Wallet::CreateAccount(const io::UInt160& scriptHash)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto account = std::make_shared<WalletAccount>(scriptHash);
        accounts_.push_back(account);

        if (!defaultAccount_)
            defaultAccount_ = account;

        return account;
    }

    void Wallet::AddAccount(std::shared_ptr<WalletAccount> account)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if account already exists
        for (const auto& existingAccount : accounts_)
        {
            if (existingAccount->GetScriptHash() == account->GetScriptHash())
                return;
        }

        accounts_.push_back(account);

        if (!defaultAccount_)
            defaultAccount_ = account;
    }

    bool Wallet::RemoveAccount(const io::UInt160& scriptHash)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto it = accounts_.begin(); it != accounts_.end(); ++it)
        {
            if ((*it)->GetScriptHash() == scriptHash)
            {
                // Check if it's the default account
                if (defaultAccount_ == *it)
                    defaultAccount_ = nullptr;

                accounts_.erase(it);
                return true;
            }
        }

        return false;
    }

    bool Wallet::RemoveAccount(const std::string& address)
    {
        try
        {
            auto scriptHash = io::UInt160::FromAddress(address);
            return RemoveAccount(scriptHash);
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    bool Wallet::Save()
    {
        return SaveAs(path_);
    }

    bool Wallet::SaveAs(const std::string& path)
    {
        try
        {
            // Create directory if it doesn't exist
            std::filesystem::path fsPath(path);
            std::filesystem::create_directories(fsPath.parent_path());

            // Open file
            std::ofstream file(path);
            if (!file.is_open())
                return false;

            // Serialize wallet
            nlohmann::json json = ToJson();

            // Write to file
            file << json.dump(4);

            // Update path
            path_ = path;

            return true;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Failed to save wallet file: " << ex.what() << std::endl;
            return false;
        }
    }

    bool Wallet::Load()
    {
        return LoadFrom(path_);
    }

    bool Wallet::LoadFrom(const std::string& path)
    {
        try
        {
            // Open file
            std::ifstream file(path);
            if (!file.is_open())
                return false;

            // Parse JSON
            nlohmann::json json;
            file >> json;

            // Deserialize wallet
            FromJson(json);

            // Update path
            path_ = path;

            return true;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Failed to load wallet file: " << ex.what() << std::endl;
            return false;
        }
    }

    nlohmann::json Wallet::ToJson() const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        nlohmann::json json;
        json["name"] = name_;
        json["version"] = version_;

        nlohmann::json accountsJson = nlohmann::json::array();
        for (const auto& account : accounts_)
        {
            accountsJson.push_back(account->ToJson());
        }
        json["accounts"] = accountsJson;

        if (defaultAccount_)
            json["default_account"] = defaultAccount_->GetAddress();

        return json;
    }

    void Wallet::FromJson(const nlohmann::json& json)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (json.contains("name"))
            name_ = json["name"].get<std::string>();

        if (json.contains("version"))
            version_ = json["version"].get<int32_t>();

        accounts_.clear();
        defaultAccount_ = nullptr;

        if (json.contains("accounts") && json["accounts"].is_array())
        {
            for (const auto& accountJson : json["accounts"])
            {
                auto account = std::make_shared<WalletAccount>();
                account->FromJson(accountJson);
                accounts_.push_back(account);
            }
        }

        if (json.contains("default_account") && !accounts_.empty())
        {
            auto defaultAddress = json["default_account"].get<std::string>();
            for (const auto& account : accounts_)
            {
                if (account->GetAddress() == defaultAddress)
                {
                    defaultAccount_ = account;
                    break;
                }
            }
        }
    }
}
