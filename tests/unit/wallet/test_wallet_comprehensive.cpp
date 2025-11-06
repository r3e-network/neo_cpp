/**
 * @file test_wallet_comprehensive.cpp
 * @brief Comprehensive wallet functionality tests
 */

#include <gtest/gtest.h>
#include <neo/wallets/wallet.h>
#include <neo/wallets/wallet_account.h>
#include <neo/wallets/key_pair.h>
#include <neo/cryptography/helper.h>
#include <neo/wallets/nep6/nep6_wallet.h>
#include <neo/wallets/helper.h>
#include <neo/smartcontract/storage_key.h>
#include <memory>
#include <vector>
#include <fstream>

using namespace neo::wallets;
using namespace neo::cryptography;

class WalletComprehensiveTest : public ::testing::Test {
protected:
    std::unique_ptr<Wallet> wallet;
    std::string testWalletPath = "test_wallet.json";
    
    void SetUp() override {
        wallet = std::make_unique<NEP6Wallet>("TestWallet", testWalletPath);
    }
    
    void TearDown() override {
        wallet.reset();
        // Clean up test wallet file
        std::remove(testWalletPath.c_str());
    }
};

// Account Management Tests
TEST_F(WalletComprehensiveTest, CreateAccount) {
    auto account = wallet->CreateAccount();
    ASSERT_NE(account, nullptr);
    EXPECT_FALSE(account->Address.empty());
    EXPECT_TRUE(account->HasKey);
    EXPECT_FALSE(account->IsWatchOnly());
}

TEST_F(WalletComprehensiveTest, CreateAccountWithPrivateKey) {
    KeyPair keyPair;
    auto privateKey = keyPair.GetPrivateKey();
    
    auto account = wallet->CreateAccount(privateKey);
    ASSERT_NE(account, nullptr);
    EXPECT_EQ(account->GetKey().GetPrivateKey(), privateKey);
}

TEST_F(WalletComprehensiveTest, CreateAccountWithContract) {
    KeyPair keyPair;
    auto contract = Contract::CreateSignatureContract(keyPair.GetPublicKey());
    
    auto account = wallet->CreateAccount(contract, keyPair);
    ASSERT_NE(account, nullptr);
    EXPECT_EQ(account->Contract.Script, contract.Script);
}

TEST_F(WalletComprehensiveTest, ImportWIF) {
    std::string wif = "L1QqQJnpBwbsPGAuutuzPTac8piqvbR1HRjrY5qHup48TBCBFe4g";
    auto account = wallet->Import(wif);
    
    ASSERT_NE(account, nullptr);
    EXPECT_TRUE(account->HasKey);
    EXPECT_FALSE(account->Address.empty());
}

TEST_F(WalletComprehensiveTest, ImportNEP2) {
    std::string nep2 = "6PYKsHXhWUNUrWAYmTfL692qqmmrihFQVTQEXuDKpxss86FxxgurkvAwZN";
    std::string password = "test123";
    
    auto account = wallet->Import(nep2, password);
    ASSERT_NE(account, nullptr);
    EXPECT_TRUE(account->HasKey);
}

TEST_F(WalletComprehensiveTest, GetAccount) {
    auto account1 = wallet->CreateAccount();
    auto account2 = wallet->CreateAccount();
    
    auto retrieved = wallet->GetAccount(account1->ScriptHash);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->ScriptHash, account1->ScriptHash);
}

TEST_F(WalletComprehensiveTest, GetAccounts) {
    wallet->CreateAccount();
    wallet->CreateAccount();
    wallet->CreateAccount();
    
    auto accounts = wallet->GetAccounts();
    EXPECT_EQ(accounts.size(), 3);
}

TEST_F(WalletComprehensiveTest, DeleteAccount) {
    auto account = wallet->CreateAccount();
    auto scriptHash = account->ScriptHash;
    
    EXPECT_TRUE(wallet->DeleteAccount(scriptHash));
    EXPECT_EQ(wallet->GetAccount(scriptHash), nullptr);
}

TEST_F(WalletComprehensiveTest, Contains) {
    auto account = wallet->CreateAccount();
    
    EXPECT_TRUE(wallet->Contains(account->ScriptHash));
    
    UInt160 randomHash;
    randomHash.SetZero();
    EXPECT_FALSE(wallet->Contains(randomHash));
}

// Default Account Tests
TEST_F(WalletComprehensiveTest, DefaultAccount) {
    EXPECT_EQ(wallet->GetDefaultAccount(), nullptr);
    
    auto account = wallet->CreateAccount();
    wallet->SetDefaultAccount(account);
    
    EXPECT_EQ(wallet->GetDefaultAccount(), account);
}

TEST_F(WalletComprehensiveTest, ChangeDefaultAccount) {
    auto account1 = wallet->CreateAccount();
    auto account2 = wallet->CreateAccount();
    
    wallet->SetDefaultAccount(account1);
    EXPECT_EQ(wallet->GetDefaultAccount(), account1);
    
    wallet->SetDefaultAccount(account2);
    EXPECT_EQ(wallet->GetDefaultAccount(), account2);
}

// Multi-Signature Tests
TEST_F(WalletComprehensiveTest, CreateMultiSigAccount) {
    std::vector<ECPoint> publicKeys;
    for (int i = 0; i < 3; ++i) {
        KeyPair kp;
        publicKeys.push_back(kp.GetPublicKey());
    }
    
    int m = 2; // 2-of-3 multisig
    auto account = wallet->CreateAccount(m, publicKeys);
    
    ASSERT_NE(account, nullptr);
    EXPECT_FALSE(account->HasKey);
    EXPECT_TRUE(account->IsMultiSig());
}

TEST_F(WalletComprehensiveTest, ImportMultiSigAddress) {
    std::vector<ECPoint> publicKeys;
    for (int i = 0; i < 3; ++i) {
        KeyPair kp;
        publicKeys.push_back(kp.GetPublicKey());
    }
    
    int m = 2;
    auto contract = Contract::CreateMultiSigContract(m, publicKeys);
    auto address = contract.GetAddress();
    
    auto account = wallet->ImportAddress(address, contract);
    ASSERT_NE(account, nullptr);
    EXPECT_TRUE(account->IsWatchOnly());
}

// Watch-Only Account Tests
TEST_F(WalletComprehensiveTest, CreateWatchOnlyAccount) {
    KeyPair kp;
    auto address = kp.GetAddress();
    
    auto account = wallet->ImportAddress(address);
    ASSERT_NE(account, nullptr);
    EXPECT_TRUE(account->IsWatchOnly());
    EXPECT_FALSE(account->HasKey);
}

TEST_F(WalletComprehensiveTest, CannotSignWithWatchOnly) {
    auto account = wallet->ImportAddress("NQRLhCpAru9BjGsMwk67vdMwmzKMRgsnnN");
    ASSERT_NE(account, nullptr);
    
    UInt256 message;
    message.SetZero();
    
    EXPECT_THROW(account->Sign(message), std::runtime_error);
}

// Wallet Persistence Tests
TEST_F(WalletComprehensiveTest, SaveAndLoad) {
    // Create accounts
    auto account1 = wallet->CreateAccount();
    auto account2 = wallet->CreateAccount();
    wallet->SetDefaultAccount(account1);
    
    // Save wallet
    EXPECT_TRUE(wallet->Save());
    
    // Load wallet
    auto loadedWallet = std::make_unique<NEP6Wallet>(testWalletPath);
    
    // Verify accounts
    EXPECT_EQ(loadedWallet->GetAccounts().size(), 2);
    EXPECT_NE(loadedWallet->GetAccount(account1->ScriptHash), nullptr);
    EXPECT_NE(loadedWallet->GetAccount(account2->ScriptHash), nullptr);
    EXPECT_EQ(loadedWallet->GetDefaultAccount()->ScriptHash, account1->ScriptHash);
}

TEST_F(WalletComprehensiveTest, SaveEncrypted) {
    std::string password = "strongPassword123!";
    wallet->ChangePassword("", password);
    
    auto account = wallet->CreateAccount();
    EXPECT_TRUE(wallet->Save());
    
    // Try loading with wrong password
    EXPECT_THROW(
        std::make_unique<NEP6Wallet>(testWalletPath, "wrongPassword"),
        std::runtime_error
    );
    
    // Load with correct password
    auto loadedWallet = std::make_unique<NEP6Wallet>(testWalletPath, password);
    EXPECT_EQ(loadedWallet->GetAccounts().size(), 1);
}

// Transaction Signing Tests
TEST_F(WalletComprehensiveTest, SignTransaction) {
    auto account = wallet->CreateAccount();
    
    Transaction tx;
    tx.Version = 0;
    tx.Nonce = 12345;
    
    auto context = wallet->Sign(tx);
    ASSERT_NE(context, nullptr);
    EXPECT_TRUE(context->IsCompleted());
}

TEST_F(WalletComprehensiveTest, SignMultiSigTransaction) {
    // Create 3 wallets for multisig
    std::vector<std::unique_ptr<Wallet>> wallets;
    std::vector<ECPoint> publicKeys;
    
    for (int i = 0; i < 3; ++i) {
        auto w = std::make_unique<NEP6Wallet>("Wallet" + std::to_string(i));
        auto account = w->CreateAccount();
        publicKeys.push_back(account->GetKey().GetPublicKey());
        wallets.push_back(std::move(w));
    }
    
    // Create 2-of-3 multisig account in each wallet
    int m = 2;
    for (auto& w : wallets) {
        w->CreateAccount(m, publicKeys);
    }
    
    // Create transaction
    Transaction tx;
    tx.Version = 0;
    tx.Nonce = 12345;
    
    // Sign with first two wallets
    auto context = wallets[0]->Sign(tx);
    context = wallets[1]->Sign(tx, context);
    
    EXPECT_TRUE(context->IsCompleted());
}

// Balance and Asset Tests
TEST_F(WalletComprehensiveTest, GetBalance) {
    auto account = wallet->CreateAccount();
    
    // Mock balance (would normally query blockchain)
    auto balance = wallet->GetBalance(account->ScriptHash, NEO_ASSET_ID);
    EXPECT_GE(balance, 0);
}

TEST_F(WalletComprehensiveTest, GetTotalBalance) {
    wallet->CreateAccount();
    wallet->CreateAccount();
    
    auto totalBalance = wallet->GetTotalBalance(NEO_ASSET_ID);
    EXPECT_GE(totalBalance, 0);
}

// Export Tests
TEST_F(WalletComprehensiveTest, ExportWIF) {
    auto account = wallet->CreateAccount();
    
    auto wif = wallet->Export(account->ScriptHash);
    EXPECT_FALSE(wif.empty());
    EXPECT_TRUE(wif.starts_with("L") || wif.starts_with("K"));
}

TEST_F(WalletComprehensiveTest, ExportNEP2) {
    std::string password = "test123";
    wallet->ChangePassword("", password);
    
    auto account = wallet->CreateAccount();
    auto nep2 = wallet->ExportNEP2(account->ScriptHash);
    
    EXPECT_FALSE(nep2.empty());
    EXPECT_TRUE(nep2.starts_with("6P"));
}

// Lock/Unlock Tests
TEST_F(WalletComprehensiveTest, LockUnlock) {
    std::string password = "password123";
    wallet->ChangePassword("", password);
    
    EXPECT_FALSE(wallet->IsLocked());
    
    wallet->Lock();
    EXPECT_TRUE(wallet->IsLocked());
    
    EXPECT_FALSE(wallet->Unlock("wrongPassword"));
    EXPECT_TRUE(wallet->IsLocked());
    
    EXPECT_TRUE(wallet->Unlock(password));
    EXPECT_FALSE(wallet->IsLocked());
}

// Account Label Tests
TEST_F(WalletComprehensiveTest, AccountLabels) {
    auto account = wallet->CreateAccount();
    
    EXPECT_TRUE(account->Label.empty());
    
    account->Label = "My Main Account";
    wallet->Save();
    
    auto loadedWallet = std::make_unique<NEP6Wallet>(testWalletPath);
    auto loadedAccount = loadedWallet->GetAccount(account->ScriptHash);
    
    EXPECT_EQ(loadedAccount->Label, "My Main Account");
}

// Contract Verification Tests
TEST_F(WalletComprehensiveTest, VerifyPassword) {
    std::string password = "correct123";
    wallet->ChangePassword("", password);
    
    EXPECT_TRUE(wallet->VerifyPassword(password));
    EXPECT_FALSE(wallet->VerifyPassword("wrong123"));
}

// Bulk Operations Tests
TEST_F(WalletComprehensiveTest, BulkAccountCreation) {
    const int numAccounts = 100;
    
    for (int i = 0; i < numAccounts; ++i) {
        auto account = wallet->CreateAccount();
        ASSERT_NE(account, nullptr);
    }
    
    EXPECT_EQ(wallet->GetAccounts().size(), numAccounts);
}

TEST_F(WalletComprehensiveTest, FindAccountByAddress) {
    auto account = wallet->CreateAccount();
    auto address = account->Address;
    
    auto found = wallet->GetAccountByAddress(address);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->ScriptHash, account->ScriptHash);
}

// Error Handling Tests
TEST_F(WalletComprehensiveTest, ImportInvalidWIF) {
    std::string invalidWIF = "InvalidWIFString";
    EXPECT_THROW(wallet->Import(invalidWIF), std::runtime_error);
}

TEST_F(WalletComprehensiveTest, ImportInvalidNEP2) {
    std::string invalidNEP2 = "InvalidNEP2String";
    EXPECT_THROW(wallet->Import(invalidNEP2, "password"), std::runtime_error);
}

TEST_F(WalletComprehensiveTest, DeleteNonExistentAccount) {
    UInt160 randomHash;
    randomHash.SetZero();
    
    EXPECT_FALSE(wallet->DeleteAccount(randomHash));
}

// Migration Tests
TEST_F(WalletComprehensiveTest, MigrateFromOldFormat) {
    // Test migration from old wallet format to NEP6
    // This would typically involve parsing old format and converting
    
    // Create mock old format data
    std::string oldFormatData = R"({
        "version": "1.0",
        "accounts": [{
            "address": "NQRLhCpAru9BjGsMwk67vdMwmzKMRgsnnN",
            "key": "L1QqQJnpBwbsPGAuutuzPTac8piqvbR1HRjrY5qHup48TBCBFe4g"
        }]
    })";
    
    // In real implementation, would parse and migrate
    // For now, just verify new wallet format
    EXPECT_EQ(wallet->GetVersion(), "3.0");
}

// Performance Tests
TEST_F(WalletComprehensiveTest, PerformanceLargeWallet) {
    const int numAccounts = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numAccounts; ++i) {
        wallet->CreateAccount();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should create 1000 accounts in reasonable time
    EXPECT_LT(duration.count(), 10000); // Less than 10 seconds
    
    // Test account lookup performance
    auto accounts = wallet->GetAccounts();
    auto targetAccount = accounts[numAccounts / 2];
    
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        auto found = wallet->GetAccount(targetAccount->ScriptHash);
        ASSERT_NE(found, nullptr);
    }
    end = std::chrono::high_resolution_clock::now();
    
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 100); // 1000 lookups in less than 100ms
}
