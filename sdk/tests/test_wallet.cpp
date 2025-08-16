/**
 * @file test_wallet.cpp
 * @brief Unit tests for SDK wallet functionality
 * @author Neo C++ Team
 * @date 2025
 */

#include <gtest/gtest.h>
#include <neo/sdk/wallet/wallet.h>
#include <neo/sdk/wallet/account.h>
#include <neo/cryptography/crypto.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <filesystem>
#include <fstream>

using namespace neo::sdk::wallet;
using namespace neo::io;

class WalletTest : public ::testing::Test {
protected:
    std::string testWalletPath;
    std::string testPassword;
    
    void SetUp() override {
        testWalletPath = "test_wallet.json";
        testPassword = "TestPassword123!";
        
        // Clean up any existing test wallet
        if (std::filesystem::exists(testWalletPath)) {
            std::filesystem::remove(testWalletPath);
        }
    }
    
    void TearDown() override {
        // Clean up test files
        if (std::filesystem::exists(testWalletPath)) {
            std::filesystem::remove(testWalletPath);
        }
        if (std::filesystem::exists("test_wallet_save_as.json")) {
            std::filesystem::remove("test_wallet_save_as.json");
        }
    }
};

// Test wallet creation
TEST_F(WalletTest, CreateNewWallet) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    
    ASSERT_NE(wallet, nullptr);
    EXPECT_EQ(wallet->GetName(), "TestWallet");
    EXPECT_EQ(wallet->GetPath(), testWalletPath);
    EXPECT_FALSE(wallet->IsLocked());
    
    // File should be created
    EXPECT_TRUE(std::filesystem::exists(testWalletPath));
}

TEST_F(WalletTest, CreateWalletWithExistingFile) {
    // Create first wallet
    auto wallet1 = Wallet::Create("Wallet1", testWalletPath, testPassword);
    ASSERT_NE(wallet1, nullptr);
    
    // Try to create another wallet with same path should fail
    auto wallet2 = Wallet::Create("Wallet2", testWalletPath, testPassword);
    EXPECT_EQ(wallet2, nullptr);
}

// Test wallet opening
TEST_F(WalletTest, OpenExistingWallet) {
    // First create a wallet
    auto wallet1 = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet1, nullptr);
    
    // Add an account for verification
    auto account = wallet1->CreateAccount("Account1");
    ASSERT_NE(account, nullptr);
    auto address = account->GetAddress();
    
    // Close and reopen
    wallet1.reset();
    
    auto wallet2 = Wallet::Open(testWalletPath, testPassword);
    ASSERT_NE(wallet2, nullptr);
    EXPECT_EQ(wallet2->GetName(), "TestWallet");
    
    // Verify account was persisted
    auto accounts = wallet2->GetAccounts();
    EXPECT_EQ(accounts.size(), 1);
    if (!accounts.empty()) {
        EXPECT_EQ(accounts[0]->GetAddress(), address);
    }
}

TEST_F(WalletTest, OpenWalletWithWrongPassword) {
    // Create wallet
    auto wallet1 = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet1, nullptr);
    wallet1.reset();
    
    // Try to open with wrong password
    auto wallet2 = Wallet::Open(testWalletPath, "WrongPassword");
    EXPECT_EQ(wallet2, nullptr);
}

TEST_F(WalletTest, OpenNonExistentWallet) {
    auto wallet = Wallet::Open("non_existent.json", testPassword);
    EXPECT_EQ(wallet, nullptr);
}

// Test account management
TEST_F(WalletTest, CreateAccount) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    auto account = wallet->CreateAccount("MyAccount");
    ASSERT_NE(account, nullptr);
    EXPECT_EQ(account->GetLabel(), "MyAccount");
    EXPECT_FALSE(account->GetAddress().empty());
    EXPECT_FALSE(account->IsLocked());
    
    // Verify account is in wallet
    auto accounts = wallet->GetAccounts();
    EXPECT_EQ(accounts.size(), 1);
}

TEST_F(WalletTest, CreateMultipleAccounts) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    // Create multiple accounts
    for (int i = 0; i < 5; i++) {
        auto account = wallet->CreateAccount("Account" + std::to_string(i));
        ASSERT_NE(account, nullptr);
    }
    
    auto accounts = wallet->GetAccounts();
    EXPECT_EQ(accounts.size(), 5);
    
    // Verify all accounts have unique addresses
    std::set<std::string> addresses;
    for (const auto& account : accounts) {
        addresses.insert(account->GetAddress());
    }
    EXPECT_EQ(addresses.size(), 5);
}

TEST_F(WalletTest, ImportAccountFromWIF) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    // Use a test WIF (this is a well-known test key, do not use in production!)
    std::string wif = "L1QqQJnpBwbsPGAuutuzPTac8piqvbR1HRjrY5qHup48TBCBFe4g";
    
    auto account = wallet->ImportAccount(wif, "ImportedAccount");
    ASSERT_NE(account, nullptr);
    EXPECT_EQ(account->GetLabel(), "ImportedAccount");
    
    // Verify account was added to wallet
    auto accounts = wallet->GetAccounts();
    EXPECT_EQ(accounts.size(), 1);
}

TEST_F(WalletTest, ImportAccountFromPrivateKey) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    // Generate a private key
    std::vector<uint8_t> privateKey(32);
    for (size_t i = 0; i < 32; i++) {
        privateKey[i] = static_cast<uint8_t>(i + 1);
    }
    
    auto account = wallet->ImportAccount(privateKey, "ImportedFromKey");
    ASSERT_NE(account, nullptr);
    EXPECT_EQ(account->GetLabel(), "ImportedFromKey");
}

TEST_F(WalletTest, GetAccountByAddress) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    auto account1 = wallet->CreateAccount("Account1");
    auto account2 = wallet->CreateAccount("Account2");
    ASSERT_NE(account1, nullptr);
    ASSERT_NE(account2, nullptr);
    
    auto address1 = account1->GetAddress();
    auto address2 = account2->GetAddress();
    
    // Find accounts by address
    auto found1 = wallet->GetAccount(address1);
    auto found2 = wallet->GetAccount(address2);
    
    ASSERT_NE(found1, nullptr);
    ASSERT_NE(found2, nullptr);
    EXPECT_EQ(found1->GetLabel(), "Account1");
    EXPECT_EQ(found2->GetLabel(), "Account2");
    
    // Try non-existent address
    auto notFound = wallet->GetAccount("NInvalidAddress123");
    EXPECT_EQ(notFound, nullptr);
}

TEST_F(WalletTest, DeleteAccount) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    auto account1 = wallet->CreateAccount("Account1");
    auto account2 = wallet->CreateAccount("Account2");
    auto account3 = wallet->CreateAccount("Account3");
    
    EXPECT_EQ(wallet->GetAccounts().size(), 3);
    
    // Delete account2
    auto address2 = account2->GetAddress();
    EXPECT_TRUE(wallet->DeleteAccount(address2));
    
    EXPECT_EQ(wallet->GetAccounts().size(), 2);
    EXPECT_EQ(wallet->GetAccount(address2), nullptr);
    
    // Try to delete non-existent account
    EXPECT_FALSE(wallet->DeleteAccount("NInvalidAddress"));
}

// Test wallet saving
TEST_F(WalletTest, SaveWallet) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    // Add some accounts
    wallet->CreateAccount("Account1");
    wallet->CreateAccount("Account2");
    
    // Save should succeed
    EXPECT_TRUE(wallet->Save());
    
    // File should exist and be non-empty
    EXPECT_TRUE(std::filesystem::exists(testWalletPath));
    EXPECT_GT(std::filesystem::file_size(testWalletPath), 0);
}

TEST_F(WalletTest, SaveAsWallet) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    wallet->CreateAccount("Account1");
    
    std::string newPath = "test_wallet_save_as.json";
    EXPECT_TRUE(wallet->SaveAs(newPath));
    
    // Both files should exist
    EXPECT_TRUE(std::filesystem::exists(testWalletPath));
    EXPECT_TRUE(std::filesystem::exists(newPath));
    
    // Open the saved-as wallet
    auto wallet2 = Wallet::Open(newPath, testPassword);
    ASSERT_NE(wallet2, nullptr);
    EXPECT_EQ(wallet2->GetAccounts().size(), 1);
}

// Test wallet locking
TEST_F(WalletTest, LockUnlockWallet) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    EXPECT_FALSE(wallet->IsLocked());
    
    // Lock wallet
    wallet->Lock();
    EXPECT_TRUE(wallet->IsLocked());
    
    // Operations should fail when locked
    auto account = wallet->CreateAccount("ShouldFail");
    EXPECT_EQ(account, nullptr);
    
    // Unlock with correct password
    EXPECT_TRUE(wallet->Unlock(testPassword));
    EXPECT_FALSE(wallet->IsLocked());
    
    // Operations should work after unlock
    account = wallet->CreateAccount("ShouldWork");
    EXPECT_NE(account, nullptr);
    
    // Lock again and try wrong password
    wallet->Lock();
    EXPECT_FALSE(wallet->Unlock("WrongPassword"));
    EXPECT_TRUE(wallet->IsLocked());
}

// Test password change
TEST_F(WalletTest, ChangePassword) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    wallet->CreateAccount("Account1");
    
    std::string newPassword = "NewPassword456!";
    EXPECT_TRUE(wallet->ChangePassword(testPassword, newPassword));
    
    // Save and close
    EXPECT_TRUE(wallet->Save());
    wallet.reset();
    
    // Should not open with old password
    auto wallet2 = Wallet::Open(testWalletPath, testPassword);
    EXPECT_EQ(wallet2, nullptr);
    
    // Should open with new password
    auto wallet3 = Wallet::Open(testWalletPath, newPassword);
    ASSERT_NE(wallet3, nullptr);
    EXPECT_EQ(wallet3->GetAccounts().size(), 1);
}

TEST_F(WalletTest, ChangePasswordWithWrongOldPassword) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    EXPECT_FALSE(wallet->ChangePassword("WrongOldPassword", "NewPassword"));
}

// Test message signing
TEST_F(WalletTest, SignMessage) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    auto account = wallet->CreateAccount("SigningAccount");
    ASSERT_NE(account, nullptr);
    
    std::string message = "Hello, Neo!";
    auto signature = wallet->SignMessage(message, account->GetAddress());
    
    EXPECT_FALSE(signature.empty());
    // Signature should be base64 encoded
    EXPECT_GT(signature.length(), 0);
}

TEST_F(WalletTest, SignMessageWithNonExistentAccount) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    std::string message = "Hello, Neo!";
    auto signature = wallet->SignMessage(message, "NInvalidAddress");
    
    EXPECT_TRUE(signature.empty());
}

TEST_F(WalletTest, SignMessageWithLockedWallet) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    auto account = wallet->CreateAccount("Account1");
    ASSERT_NE(account, nullptr);
    
    wallet->Lock();
    
    std::string message = "Hello, Neo!";
    auto signature = wallet->SignMessage(message, account->GetAddress());
    
    EXPECT_TRUE(signature.empty());
}

// Test transaction signing (mock)
TEST_F(WalletTest, SignTransaction) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    auto account = wallet->CreateAccount("Account1");
    ASSERT_NE(account, nullptr);
    
    // Create a mock transaction
    neo::sdk::core::Transaction tx;
    tx.Version = 0;
    tx.Nonce = 12345;
    tx.SystemFee = 1000000;
    tx.NetworkFee = 500000;
    tx.ValidUntilBlock = 99999;
    
    // Add signer
    neo::sdk::core::Signer signer;
    signer.Account = neo::sdk::core::ScriptHashFromAddress(account->GetAddress()).value_or(UInt160::Zero());
    signer.Scopes = neo::sdk::core::WitnessScope::CalledByEntry;
    tx.Signers.push_back(signer);
    
    EXPECT_TRUE(wallet->SignTransaction(tx));
    
    // Transaction should have witness after signing
    EXPECT_GT(tx.Witnesses.size(), 0);
}

// Test default account
TEST_F(WalletTest, DefaultAccount) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    // No default account initially
    EXPECT_EQ(wallet->GetDefaultAccount(), nullptr);
    
    auto account1 = wallet->CreateAccount("Account1");
    auto account2 = wallet->CreateAccount("Account2");
    
    // Set default account
    wallet->SetDefaultAccount(account1->GetAddress());
    auto defaultAcc = wallet->GetDefaultAccount();
    ASSERT_NE(defaultAcc, nullptr);
    EXPECT_EQ(defaultAcc->GetAddress(), account1->GetAddress());
    
    // Change default account
    wallet->SetDefaultAccount(account2->GetAddress());
    defaultAcc = wallet->GetDefaultAccount();
    ASSERT_NE(defaultAcc, nullptr);
    EXPECT_EQ(defaultAcc->GetAddress(), account2->GetAddress());
}

// Test wallet persistence
TEST_F(WalletTest, WalletPersistence) {
    std::string address1, address2;
    
    // Create wallet with accounts
    {
        auto wallet = Wallet::Create("PersistTest", testWalletPath, testPassword);
        ASSERT_NE(wallet, nullptr);
        
        auto account1 = wallet->CreateAccount("Account1");
        auto account2 = wallet->CreateAccount("Account2");
        address1 = account1->GetAddress();
        address2 = account2->GetAddress();
        
        wallet->SetDefaultAccount(address1);
        EXPECT_TRUE(wallet->Save());
    }
    
    // Reopen and verify
    {
        auto wallet = Wallet::Open(testWalletPath, testPassword);
        ASSERT_NE(wallet, nullptr);
        
        EXPECT_EQ(wallet->GetName(), "PersistTest");
        EXPECT_EQ(wallet->GetAccounts().size(), 2);
        
        auto account1 = wallet->GetAccount(address1);
        auto account2 = wallet->GetAccount(address2);
        ASSERT_NE(account1, nullptr);
        ASSERT_NE(account2, nullptr);
        EXPECT_EQ(account1->GetLabel(), "Account1");
        EXPECT_EQ(account2->GetLabel(), "Account2");
        
        auto defaultAcc = wallet->GetDefaultAccount();
        ASSERT_NE(defaultAcc, nullptr);
        EXPECT_EQ(defaultAcc->GetAddress(), address1);
    }
}

// Test error conditions
TEST_F(WalletTest, InvalidOperations) {
    auto wallet = Wallet::Create("TestWallet", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    // Empty label
    auto account = wallet->CreateAccount("");
    EXPECT_EQ(account, nullptr);
    
    // Invalid WIF
    auto imported = wallet->ImportAccount("InvalidWIF", "Label");
    EXPECT_EQ(imported, nullptr);
    
    // Invalid private key size
    std::vector<uint8_t> invalidKey(31); // Should be 32 bytes
    auto imported2 = wallet->ImportAccount(invalidKey, "Label");
    EXPECT_EQ(imported2, nullptr);
    
    // Delete with empty address
    EXPECT_FALSE(wallet->DeleteAccount(""));
    
    // Sign with empty message
    auto sig = wallet->SignMessage("", "address");
    EXPECT_TRUE(sig.empty());
}

// Performance test
TEST_F(WalletTest, WalletPerformance) {
    auto wallet = Wallet::Create("PerfTest", testWalletPath, testPassword);
    ASSERT_NE(wallet, nullptr);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create 100 accounts
    for (int i = 0; i < 100; i++) {
        auto account = wallet->CreateAccount("Account" + std::to_string(i));
        ASSERT_NE(account, nullptr);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    
    // Should create 100 accounts in reasonable time
    EXPECT_LT(duration.count(), 30); // Less than 30 seconds
    
    // Save performance
    start = std::chrono::high_resolution_clock::now();
    EXPECT_TRUE(wallet->Save());
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    
    // Save should be quick even with many accounts
    EXPECT_LT(duration.count(), 5); // Less than 5 seconds
}