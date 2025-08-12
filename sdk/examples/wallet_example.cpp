/**
 * @file wallet_example.cpp
 * @brief Example demonstrating wallet operations using Neo C++ SDK
 */

#include <neo/sdk.h>
#include <iostream>
#include <iomanip>

using namespace neo::sdk;

void PrintAccount(const wallet::Account& account) {
    std::cout << "  Address: " << account.GetAddress() << std::endl;
    std::cout << "  Label: " << account.GetLabel() << std::endl;
    std::cout << "  Script Hash: " << account.GetScriptHash().ToString() << std::endl;
    std::cout << "  Public Key: " << account.GetPublicKey().ToString() << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        std::cout << "Neo C++ SDK Wallet Example" << std::endl;
        std::cout << "SDK Version: " << neo::sdk::GetVersion() << std::endl;
        std::cout << "================================" << std::endl << std::endl;
        
        // Initialize SDK
        if (!neo::sdk::Initialize()) {
            std::cerr << "Failed to initialize SDK" << std::endl;
            return 1;
        }
        
        // 1. Create a new wallet
        std::cout << "1. Creating new wallet..." << std::endl;
        auto wallet = wallet::Wallet::Create(
            "example_wallet.json",
            "MySecurePassword123!",
            "Example Wallet"
        );
        std::cout << "   Wallet created: " << wallet->GetName() << std::endl;
        std::cout << "   Version: " << wallet->GetVersion() << std::endl << std::endl;
        
        // 2. Create accounts
        std::cout << "2. Creating accounts..." << std::endl;
        auto account1 = wallet->CreateAccount("Main Account");
        std::cout << "   Account 1 created:" << std::endl;
        PrintAccount(account1);
        
        auto account2 = wallet->CreateAccount("Savings Account");
        std::cout << "   Account 2 created:" << std::endl;
        PrintAccount(account2);
        
        // 3. Import account from WIF
        std::cout << "3. Importing account from WIF..." << std::endl;
        // Example WIF (DO NOT USE IN PRODUCTION - This is just for demonstration)
        std::string wif = "L1QqQJnpBwbsPGAuutuzPTac8piqvbR1HRjrY5qHup48TBCBFe4g";
        auto importedAccount = wallet->ImportAccount(wif, "Imported Account");
        std::cout << "   Account imported:" << std::endl;
        PrintAccount(importedAccount);
        
        // 4. List all accounts
        std::cout << "4. Listing all accounts in wallet..." << std::endl;
        auto accounts = wallet->GetAccounts();
        std::cout << "   Total accounts: " << accounts.size() << std::endl;
        for (size_t i = 0; i < accounts.size(); ++i) {
            std::cout << "   Account " << (i + 1) << ":" << std::endl;
            PrintAccount(accounts[i]);
        }
        
        // 5. Set default account
        std::cout << "5. Setting default account..." << std::endl;
        wallet->SetDefaultAccount(account1.GetAddress());
        auto defaultAccount = wallet->GetDefaultAccount();
        std::cout << "   Default account: " << defaultAccount.GetAddress() << std::endl << std::endl;
        
        // 6. Sign a message
        std::cout << "6. Signing a message..." << std::endl;
        std::string message = "Hello, Neo Blockchain!";
        std::vector<uint8_t> messageBytes(message.begin(), message.end());
        auto signature = wallet->Sign(messageBytes, account1);
        std::cout << "   Message: " << message << std::endl;
        std::cout << "   Signature: ";
        for (auto byte : signature) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(byte);
        }
        std::cout << std::dec << std::endl << std::endl;
        
        // 7. Lock and unlock wallet
        std::cout << "7. Testing wallet lock/unlock..." << std::endl;
        wallet->Lock();
        std::cout << "   Wallet locked: " << (wallet->IsLocked() ? "Yes" : "No") << std::endl;
        
        bool unlocked = wallet->Unlock("MySecurePassword123!");
        std::cout << "   Unlock successful: " << (unlocked ? "Yes" : "No") << std::endl;
        std::cout << "   Wallet locked: " << (wallet->IsLocked() ? "Yes" : "No") << std::endl << std::endl;
        
        // 8. Save wallet
        std::cout << "8. Saving wallet..." << std::endl;
        if (wallet->Save()) {
            std::cout << "   Wallet saved to: " << wallet->GetPath() << std::endl;
        } else {
            std::cout << "   Failed to save wallet" << std::endl;
        }
        
        std::cout << std::endl;
        
        // 9. Open existing wallet
        std::cout << "9. Opening existing wallet..." << std::endl;
        auto wallet2 = wallet::Wallet::Open("example_wallet.json", "MySecurePassword123!");
        std::cout << "   Wallet opened: " << wallet2->GetName() << std::endl;
        std::cout << "   Accounts: " << wallet2->GetAccounts().size() << std::endl << std::endl;
        
        // 10. Delete an account
        std::cout << "10. Deleting an account..." << std::endl;
        bool deleted = wallet->DeleteAccount(account2.GetAddress());
        std::cout << "    Account deleted: " << (deleted ? "Yes" : "No") << std::endl;
        std::cout << "    Remaining accounts: " << wallet->GetAccounts().size() << std::endl;
        
        // Cleanup
        neo::sdk::Shutdown();
        
        std::cout << std::endl << "Example completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}