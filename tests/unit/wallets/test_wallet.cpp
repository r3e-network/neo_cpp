#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/io/json.h>
#include <neo/wallets/wallet.h>
#include <neo/wallets/wallet_manager.h>

using namespace neo::wallets;
using namespace neo::cryptography::ecc;
using namespace neo::io;

TEST(WalletAccountTest, Constructor)
{
    // Default constructor
    WalletAccount account1;
    EXPECT_EQ(account1.GetScriptHash(), UInt160());
    EXPECT_TRUE(account1.GetPublicKey().IsInfinity());
    EXPECT_TRUE(account1.GetPrivateKey().empty());
    EXPECT_TRUE(account1.GetContract().GetScript().IsEmpty());
    EXPECT_TRUE(account1.GetLabel().empty());
    EXPECT_FALSE(account1.IsLocked());

    // KeyPair constructor
    auto keyPair = Secp256r1::GenerateKeyPair();
    WalletAccount account2(keyPair);
    EXPECT_NE(account2.GetScriptHash(), UInt160());
    EXPECT_EQ(account2.GetPublicKey(), keyPair.PublicKey);
    EXPECT_EQ(account2.GetPrivateKey(), keyPair.PrivateKey);
    EXPECT_FALSE(account2.GetContract().GetScript().IsEmpty());
    EXPECT_TRUE(account2.GetLabel().empty());
    EXPECT_FALSE(account2.IsLocked());

    // ScriptHash constructor
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    WalletAccount account3(scriptHash);
    EXPECT_EQ(account3.GetScriptHash(), scriptHash);
    EXPECT_TRUE(account3.GetPublicKey().IsInfinity());
    EXPECT_TRUE(account3.GetPrivateKey().empty());
    EXPECT_TRUE(account3.GetContract().GetScript().IsEmpty());
    EXPECT_TRUE(account3.GetLabel().empty());
    EXPECT_FALSE(account3.IsLocked());
}

TEST(WalletAccountTest, SettersAndGetters)
{
    WalletAccount account;

    // ScriptHash
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    account.SetScriptHash(scriptHash);
    EXPECT_EQ(account.GetScriptHash(), scriptHash);

    // PublicKey
    auto keyPair = Secp256r1::GenerateKeyPair();
    account.SetPublicKey(keyPair.PublicKey);
    EXPECT_EQ(account.GetPublicKey(), keyPair.PublicKey);

    // PrivateKey
    account.SetPrivateKey(keyPair.PrivateKey);
    EXPECT_EQ(account.GetPrivateKey(), keyPair.PrivateKey);

    // Contract
    auto contract = smartcontract::Contract::CreateSignatureContract(keyPair.PublicKey);
    account.SetContract(contract);
    EXPECT_EQ(account.GetContract().GetScript(), contract.GetScript());

    // Label
    account.SetLabel("Test Account");
    EXPECT_EQ(account.GetLabel(), "Test Account");

    // Locked
    account.SetLocked(true);
    EXPECT_TRUE(account.IsLocked());
}

TEST(WalletAccountTest, GetWIF)
{
    // Account with private key
    auto keyPair = Secp256r1::GenerateKeyPair();
    WalletAccount account1(keyPair);
    EXPECT_FALSE(account1.GetWIF().empty());

    // Account without private key
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    WalletAccount account2(scriptHash);
    EXPECT_TRUE(account2.GetWIF().empty());
}

TEST(WalletAccountTest, GetAddress)
{
    // Account with script hash
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    WalletAccount account(scriptHash);
    EXPECT_FALSE(account.GetAddress().empty());
}

TEST(WalletAccountTest, HasPrivateKey)
{
    // Account with private key
    auto keyPair = Secp256r1::GenerateKeyPair();
    WalletAccount account1(keyPair);
    EXPECT_TRUE(account1.HasPrivateKey());

    // Account without private key
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    WalletAccount account2(scriptHash);
    EXPECT_FALSE(account2.HasPrivateKey());
}

TEST(WalletAccountTest, Serialization)
{
    // Create account
    auto keyPair = Secp256r1::GenerateKeyPair();
    WalletAccount account(keyPair);
    account.SetLabel("Test Account");
    account.SetLocked(true);

    // Serialize
    nlohmann::json json = account.ToJson();

    // Check
    EXPECT_FALSE(json["address"].get<std::string>().empty());
    EXPECT_EQ(json["script_hash"].get<std::string>(), account.GetScriptHash().ToString());
    EXPECT_EQ(json["public_key"].get<std::string>(), account.GetPublicKey().ToString());
    EXPECT_FALSE(json["private_key"].get<std::string>().empty());
    EXPECT_FALSE(json["contract"]["script"].get<std::string>().empty());
    EXPECT_EQ(json["label"].get<std::string>(), "Test Account");
    EXPECT_TRUE(json["locked"].get<bool>());

    // Deserialize
    WalletAccount account2;
    account2.FromJson(json);

    // Check
    EXPECT_EQ(account2.GetScriptHash(), account.GetScriptHash());
    EXPECT_EQ(account2.GetPublicKey(), account.GetPublicKey());
    EXPECT_EQ(account2.GetPrivateKey(), account.GetPrivateKey());
    EXPECT_EQ(account2.GetContract().GetScript(), account.GetContract().GetScript());
    EXPECT_EQ(account2.GetLabel(), account.GetLabel());
    EXPECT_EQ(account2.IsLocked(), account.IsLocked());
}

TEST(WalletTest, Constructor)
{
    // Default constructor
    Wallet wallet1;
    EXPECT_TRUE(wallet1.GetPath().empty());
    EXPECT_TRUE(wallet1.GetName().empty());
    EXPECT_EQ(wallet1.GetVersion(), 1);
    EXPECT_TRUE(wallet1.GetAccounts().empty());
    EXPECT_EQ(wallet1.GetDefaultAccount(), nullptr);

    // Path constructor
    Wallet wallet2("test.json");
    EXPECT_EQ(wallet2.GetPath(), "test.json");
    EXPECT_EQ(wallet2.GetName(), "test");
    EXPECT_EQ(wallet2.GetVersion(), 1);
    EXPECT_TRUE(wallet2.GetAccounts().empty());
    EXPECT_EQ(wallet2.GetDefaultAccount(), nullptr);
}

TEST(WalletTest, SettersAndGetters)
{
    Wallet wallet;

    // Path
    wallet.SetPath("test.json");
    EXPECT_EQ(wallet.GetPath(), "test.json");
    EXPECT_EQ(wallet.GetName(), "test");

    // Name
    wallet.SetName("Test Wallet");
    EXPECT_EQ(wallet.GetName(), "Test Wallet");

    // Version
    wallet.SetVersion(2);
    EXPECT_EQ(wallet.GetVersion(), 2);
}

TEST(WalletTest, CreateAccount)
{
    Wallet wallet;

    // Create account
    auto account1 = wallet.CreateAccount();
    EXPECT_NE(account1, nullptr);
    EXPECT_EQ(wallet.GetAccounts().size(), 1);
    EXPECT_EQ(wallet.GetDefaultAccount(), account1);

    // Create account with private key
    auto keyPair = Secp256r1::GenerateKeyPair();
    auto account2 = wallet.CreateAccount(keyPair.PrivateKey);
    EXPECT_NE(account2, nullptr);
    EXPECT_EQ(wallet.GetAccounts().size(), 2);
    EXPECT_EQ(wallet.GetDefaultAccount(), account1);

    // Create account with key pair
    auto account3 = wallet.CreateAccount(keyPair);
    EXPECT_NE(account3, nullptr);
    EXPECT_EQ(wallet.GetAccounts().size(), 3);
    EXPECT_EQ(wallet.GetDefaultAccount(), account1);

    // Create account from WIF
    auto wif = Secp256r1::ToWIF(keyPair.PrivateKey);
    auto account4 = wallet.CreateAccountFromWIF(wif);
    EXPECT_NE(account4, nullptr);
    EXPECT_EQ(wallet.GetAccounts().size(), 4);
    EXPECT_EQ(wallet.GetDefaultAccount(), account1);

    // Create account with script hash
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    auto account5 = wallet.CreateAccount(scriptHash);
    EXPECT_NE(account5, nullptr);
    EXPECT_EQ(wallet.GetAccounts().size(), 5);
    EXPECT_EQ(wallet.GetDefaultAccount(), account1);
}

TEST(WalletTest, GetAccount)
{
    Wallet wallet;

    // Create accounts
    auto account1 = wallet.CreateAccount();
    auto account2 = wallet.CreateAccount();

    // Get account by script hash
    auto account3 = wallet.GetAccount(account1->GetScriptHash());
    EXPECT_EQ(account3, account1);

    // Get account by address
    auto account4 = wallet.GetAccount(account2->GetAddress());
    EXPECT_EQ(account4, account2);

    // Get non-existent account
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    auto account5 = wallet.GetAccount(scriptHash);
    EXPECT_EQ(account5, nullptr);

    auto account6 = wallet.GetAccount("invalid_address");
    EXPECT_EQ(account6, nullptr);
}

TEST(WalletTest, AddAccount)
{
    Wallet wallet;

    // Create account
    auto keyPair = Secp256r1::GenerateKeyPair();
    auto account = std::make_shared<WalletAccount>(keyPair);

    // Add account
    wallet.AddAccount(account);
    EXPECT_EQ(wallet.GetAccounts().size(), 1);
    EXPECT_EQ(wallet.GetDefaultAccount(), account);

    // Add same account again
    wallet.AddAccount(account);
    EXPECT_EQ(wallet.GetAccounts().size(), 1);
}

TEST(WalletTest, RemoveAccount)
{
    Wallet wallet;

    // Create accounts
    auto account1 = wallet.CreateAccount();
    auto account2 = wallet.CreateAccount();

    // Remove account by script hash
    bool result1 = wallet.RemoveAccount(account1->GetScriptHash());
    EXPECT_TRUE(result1);
    EXPECT_EQ(wallet.GetAccounts().size(), 1);
    EXPECT_EQ(wallet.GetDefaultAccount(), nullptr);

    // Remove account by address
    bool result2 = wallet.RemoveAccount(account2->GetAddress());
    EXPECT_TRUE(result2);
    EXPECT_EQ(wallet.GetAccounts().size(), 0);

    // Remove non-existent account
    UInt160 scriptHash = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    bool result3 = wallet.RemoveAccount(scriptHash);
    EXPECT_FALSE(result3);

    bool result4 = wallet.RemoveAccount("invalid_address");
    EXPECT_FALSE(result4);
}

TEST(WalletTest, SaveLoad)
{
    // Create temporary file
    std::string path = "test_wallet.json";

    {
        // Create wallet
        Wallet wallet(path);

        // Create accounts
        auto account1 = wallet.CreateAccount();
        account1->SetLabel("Account 1");

        auto account2 = wallet.CreateAccount();
        account2->SetLabel("Account 2");

        // Set default account
        wallet.SetDefaultAccount(account2);

        // Save wallet
        bool result = wallet.Save();
        EXPECT_TRUE(result);
    }

    {
        // Load wallet
        Wallet wallet;
        bool result = wallet.LoadFrom(path);
        EXPECT_TRUE(result);

        // Check wallet
        EXPECT_EQ(wallet.GetPath(), path);
        EXPECT_EQ(wallet.GetName(), "test_wallet");
        EXPECT_EQ(wallet.GetVersion(), 1);
        EXPECT_EQ(wallet.GetAccounts().size(), 2);

        // Check accounts
        auto account1 = wallet.GetAccounts()[0];
        EXPECT_EQ(account1->GetLabel(), "Account 1");

        auto account2 = wallet.GetAccounts()[1];
        EXPECT_EQ(account2->GetLabel(), "Account 2");

        // Check default account
        EXPECT_EQ(wallet.GetDefaultAccount()->GetLabel(), "Account 2");
    }

    // Clean up
    std::filesystem::remove(path);
}

TEST(WalletTest, Serialization)
{
    // Create wallet
    Wallet wallet("test.json");

    // Create accounts
    auto account1 = wallet.CreateAccount();
    account1->SetLabel("Account 1");

    auto account2 = wallet.CreateAccount();
    account2->SetLabel("Account 2");

    // Set default account
    wallet.SetDefaultAccount(account2);

    // Serialize
    nlohmann::json json = wallet.ToJson();

    // Check
    EXPECT_EQ(json["name"].get<std::string>(), "test");
    EXPECT_EQ(json["version"].get<int32_t>(), 1);
    EXPECT_EQ(json["accounts"].size(), 2);
    EXPECT_EQ(json["accounts"][0]["label"].get<std::string>(), "Account 1");
    EXPECT_EQ(json["accounts"][1]["label"].get<std::string>(), "Account 2");
    EXPECT_EQ(json["default_account"].get<std::string>(), account2->GetAddress());

    // Deserialize
    Wallet wallet2;
    wallet2.FromJson(json);

    // Check
    EXPECT_EQ(wallet2.GetName(), "test");
    EXPECT_EQ(wallet2.GetVersion(), 1);
    EXPECT_EQ(wallet2.GetAccounts().size(), 2);
    EXPECT_EQ(wallet2.GetAccounts()[0]->GetLabel(), "Account 1");
    EXPECT_EQ(wallet2.GetAccounts()[1]->GetLabel(), "Account 2");
    EXPECT_EQ(wallet2.GetDefaultAccount()->GetLabel(), "Account 2");
}

TEST(WalletManagerTest, GetInstance)
{
    auto& manager1 = WalletManager::GetInstance();
    auto& manager2 = WalletManager::GetInstance();

    EXPECT_EQ(&manager1, &manager2);
}

TEST(WalletManagerTest, CreateOpenCloseWallet)
{
    auto& manager = WalletManager::GetInstance();

    // Close all wallets
    manager.CloseAllWallets();
    EXPECT_TRUE(manager.GetWallets().empty());
    EXPECT_EQ(manager.GetCurrentWallet(), nullptr);

    // Create temporary file
    std::string path = "test_wallet_manager.json";

    // Create wallet
    auto wallet1 = manager.CreateWallet(path);
    EXPECT_NE(wallet1, nullptr);
    EXPECT_EQ(manager.GetWallets().size(), 1);
    EXPECT_EQ(manager.GetCurrentWallet(), wallet1);

    // Close wallet
    bool result1 = manager.CloseWallet(path);
    EXPECT_TRUE(result1);
    EXPECT_TRUE(manager.GetWallets().empty());
    EXPECT_EQ(manager.GetCurrentWallet(), nullptr);

    // Open wallet
    auto wallet2 = manager.OpenWallet(path);
    EXPECT_NE(wallet2, nullptr);
    EXPECT_EQ(manager.GetWallets().size(), 1);
    EXPECT_EQ(manager.GetCurrentWallet(), wallet2);

    // Close wallet
    bool result2 = manager.CloseWallet(wallet2);
    EXPECT_TRUE(result2);
    EXPECT_TRUE(manager.GetWallets().empty());
    EXPECT_EQ(manager.GetCurrentWallet(), nullptr);

    // Clean up
    std::filesystem::remove(path);
}

TEST(WalletManagerTest, GetWallet)
{
    auto& manager = WalletManager::GetInstance();

    // Close all wallets
    manager.CloseAllWallets();

    // Create temporary files
    std::string path1 = "test_wallet_manager1.json";
    std::string path2 = "test_wallet_manager2.json";

    // Create wallets
    auto wallet1 = manager.CreateWallet(path1);
    auto wallet2 = manager.CreateWallet(path2);

    // Get wallet by path
    auto wallet3 = manager.GetWallet(path1);
    EXPECT_EQ(wallet3, wallet1);

    // Get wallet by name
    auto wallet4 = manager.GetWalletByName("test_wallet_manager2");
    EXPECT_EQ(wallet4, wallet2);

    // Get non-existent wallet
    auto wallet5 = manager.GetWallet("non_existent.json");
    EXPECT_EQ(wallet5, nullptr);

    auto wallet6 = manager.GetWalletByName("non_existent");
    EXPECT_EQ(wallet6, nullptr);

    // Close all wallets
    manager.CloseAllWallets();

    // Clean up
    std::filesystem::remove(path1);
    std::filesystem::remove(path2);
}

TEST(WalletManagerTest, SetCurrentWallet)
{
    auto& manager = WalletManager::GetInstance();

    // Close all wallets
    manager.CloseAllWallets();

    // Create temporary files
    std::string path1 = "test_wallet_manager1.json";
    std::string path2 = "test_wallet_manager2.json";

    // Create wallets
    auto wallet1 = manager.CreateWallet(path1);
    auto wallet2 = manager.CreateWallet(path2);

    // Check current wallet
    EXPECT_EQ(manager.GetCurrentWallet(), wallet1);

    // Set current wallet
    manager.SetCurrentWallet(wallet2);
    EXPECT_EQ(manager.GetCurrentWallet(), wallet2);

    // Close all wallets
    manager.CloseAllWallets();

    // Clean up
    std::filesystem::remove(path1);
    std::filesystem::remove(path2);
}
