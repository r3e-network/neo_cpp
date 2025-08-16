#include <gtest/gtest.h>
#include <neo/wallets/wallet.h>
#include <neo/wallets/account.h>
#include <neo/wallets/key_pair.h>
#include <neo/wallets/nep6_wallet.h>
#include <neo/wallets/wallet_account.h>
#include <neo/wallets/asset_descriptor.h>
#include <neo/io/byte_vector.h>
#include <filesystem>

using namespace neo::wallets;
using namespace neo::io;
namespace fs = std::filesystem;

class WalletExtendedTest : public ::testing::Test
{
protected:
    std::string testWalletPath;
    std::string testPassword;
    
    void SetUp() override
    {
        testWalletPath = "test_wallet.json";
        testPassword = "TestPassword123!";
        
        // Clean up any existing test wallet
        if (fs::exists(testWalletPath))
        {
            fs::remove(testWalletPath);
        }
    }
    
    void TearDown() override
    {
        // Clean up test wallet
        if (fs::exists(testWalletPath))
        {
            fs::remove(testWalletPath);
        }
    }
};

TEST_F(WalletExtendedTest, TestWallet)
{
    // Create new wallet
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    
    EXPECT_NE(wallet, nullptr);
    EXPECT_EQ(wallet->GetName(), "TestWallet");
    EXPECT_EQ(wallet->GetPath(), testWalletPath);
    EXPECT_EQ(wallet->GetAccountCount(), 0);
    
    // Create account
    auto account = wallet->CreateAccount();
    EXPECT_NE(account, nullptr);
    EXPECT_EQ(wallet->GetAccountCount(), 1);
    
    // Save wallet
    EXPECT_TRUE(wallet->Save());
    EXPECT_TRUE(fs::exists(testWalletPath));
    
    // Load wallet
    auto loadedWallet = Wallet::Open(testWalletPath, testPassword);
    EXPECT_NE(loadedWallet, nullptr);
    EXPECT_EQ(loadedWallet->GetAccountCount(), 1);
}

TEST_F(WalletExtendedTest, TestAccount)
{
    // Create account from private key
    ByteVector privateKey = ByteVector::GenerateRandom(32);
    Account account(privateKey);
    
    EXPECT_FALSE(account.GetAddress().empty());
    EXPECT_FALSE(account.GetScriptHash().IsZero());
    EXPECT_EQ(account.GetContract().Script.Size(), 40); // Standard verification script size
    
    // Test WIF conversion
    std::string wif = account.GetWIF();
    EXPECT_FALSE(wif.empty());
    
    // Create account from WIF
    Account accountFromWIF(wif);
    EXPECT_EQ(accountFromWIF.GetAddress(), account.GetAddress());
    EXPECT_EQ(accountFromWIF.GetScriptHash(), account.GetScriptHash());
}

TEST_F(WalletExtendedTest, TestKeyPair)
{
    // Generate new key pair
    KeyPair keyPair;
    
    EXPECT_EQ(keyPair.GetPrivateKey().Size(), 32);
    EXPECT_EQ(keyPair.GetPublicKey().Size(), 33); // Compressed public key
    
    // Create key pair from private key
    ByteVector privateKey = ByteVector::Parse("7177f0d04c79fa0b8c91fe90c1cf1d44772d1fba6e5eb9b281a22cd3aafb51fe");
    KeyPair keyPair2(privateKey);
    
    EXPECT_EQ(keyPair2.GetPrivateKey(), privateKey);
    
    // Test signing
    ByteVector message = ByteVector::FromString("Test message");
    auto signature = keyPair.Sign(message);
    
    EXPECT_GT(signature.Size(), 0);
    EXPECT_TRUE(keyPair.Verify(message, signature));
    
    // Wrong message should fail verification
    ByteVector wrongMessage = ByteVector::FromString("Wrong message");
    EXPECT_FALSE(keyPair.Verify(wrongMessage, signature));
}

TEST_F(WalletExtendedTest, TestNEP6Wallet)
{
    // Create NEP6 wallet
    NEP6Wallet wallet("TestNEP6", testWalletPath);
    wallet.Unlock(testPassword);
    
    // Add account
    auto account = wallet.CreateAccount();
    EXPECT_NE(account, nullptr);
    
    // Set metadata
    wallet.SetVersion("3.0");
    wallet.SetScrypt(16384, 8, 1);
    
    // Add extra data
    wallet.SetExtra("created", std::to_string(std::time(nullptr)));
    wallet.SetExtra("platform", "neo-cpp");
    
    // Save as NEP6 format
    EXPECT_TRUE(wallet.Save());
    
    // Load and verify NEP6 format
    NEP6Wallet loadedWallet(testWalletPath);
    EXPECT_TRUE(loadedWallet.Load());
    EXPECT_EQ(loadedWallet.GetVersion(), "3.0");
    EXPECT_EQ(loadedWallet.GetAccountCount(), 1);
    
    // Unlock and access account
    EXPECT_TRUE(loadedWallet.Unlock(testPassword));
    auto loadedAccount = loadedWallet.GetAccount(0);
    EXPECT_NE(loadedAccount, nullptr);
    EXPECT_EQ(loadedAccount->GetAddress(), account->GetAddress());
}

TEST_F(WalletExtendedTest, TestWalletAccount)
{
    // Create wallet account
    ByteVector privateKey = ByteVector::GenerateRandom(32);
    KeyPair keyPair(privateKey);
    
    WalletAccount account(keyPair.GetScriptHash());
    account.SetLabel("Main Account");
    account.SetIsDefault(true);
    account.SetLock(false);
    
    // Set contract
    Contract contract;
    contract.Script = keyPair.GetVerificationScript();
    contract.Parameters = {ContractParameterType::Signature};
    contract.Deployed = false;
    account.SetContract(contract);
    
    // Set key (encrypted)
    ByteVector salt = ByteVector::GenerateRandom(8);
    ByteVector encryptedKey = account.EncryptPrivateKey(privateKey, testPassword, salt);
    account.SetKey(encryptedKey);
    
    EXPECT_EQ(account.GetLabel(), "Main Account");
    EXPECT_TRUE(account.IsDefault());
    EXPECT_FALSE(account.IsLocked());
    EXPECT_FALSE(account.GetContract().Script.empty());
    
    // Decrypt key
    ByteVector decryptedKey = account.DecryptPrivateKey(testPassword);
    EXPECT_EQ(decryptedKey, privateKey);
}

TEST_F(WalletExtendedTest, TestAssetDescriptor)
{
    // Create NEO asset descriptor
    AssetDescriptor neo;
    neo.AssetId = UInt160::Parse("0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5");
    neo.AssetName = "NEO";
    neo.Symbol = "NEO";
    neo.Decimals = 0;
    
    EXPECT_EQ(neo.AssetName, "NEO");
    EXPECT_EQ(neo.Symbol, "NEO");
    EXPECT_EQ(neo.Decimals, 0);
    
    // Create GAS asset descriptor
    AssetDescriptor gas;
    gas.AssetId = UInt160::Parse("0xd2a4cff31913016155e38e474a2c06d08be276cf");
    gas.AssetName = "GAS";
    gas.Symbol = "GAS";
    gas.Decimals = 8;
    
    EXPECT_EQ(gas.AssetName, "GAS");
    EXPECT_EQ(gas.Decimals, 8);
    
    // Test amount formatting
    uint64_t amount = 123456789; // 1.23456789 GAS
    std::string formatted = gas.FormatAmount(amount);
    EXPECT_EQ(formatted, "1.23456789");
    
    // Parse amount
    uint64_t parsed = gas.ParseAmount("1.23456789");
    EXPECT_EQ(parsed, amount);
}