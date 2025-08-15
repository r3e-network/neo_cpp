/**
 * Complete Neo C++ SDK Example
 * Demonstrates all major SDK functionality including:
 * - RPC communication
 * - Wallet management
 * - Transaction creation and signing
 * - Smart contract interaction
 * - NEP-17 token transfers
 */

#include <neo/sdk/rpc/rpc_client.h>
#include <neo/sdk/wallet/wallet_manager.h>
#include <neo/sdk/transaction/transaction_manager.h>
#include <neo/sdk/contract/nep17_token.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace neo::sdk;
using namespace std;

// Configuration
const string RPC_ENDPOINT = "http://localhost:10332";  // TestNet RPC
const string WALLET_PATH = "my_wallet.json";
const string WALLET_PASSWORD = "MySecurePassword123!";

// Example 1: Connect to Neo node and get blockchain information
void example_rpc_connection() {
    cout << "\n=== Example 1: RPC Connection ===" << endl;
    
    try {
        // Create RPC client
        auto rpcClient = make_shared<rpc::RpcClient>(RPC_ENDPOINT);
        
        // Test connection
        if (!rpcClient->TestConnection()) {
            cerr << "Failed to connect to Neo node at " << RPC_ENDPOINT << endl;
            return;
        }
        
        // Get node information
        cout << "Node Version: " << rpcClient->GetVersion() << endl;
        cout << "Block Height: " << rpcClient->GetBlockCount() << endl;
        cout << "Connected Peers: " << rpcClient->GetConnectionCount() << endl;
        
        // Get latest block
        auto bestBlockHash = rpcClient->GetBestBlockHash();
        auto block = rpcClient->GetBlock(bestBlockHash, true);
        cout << "Latest Block Hash: " << bestBlockHash << endl;
        cout << "Block Time: " << block["time"].get<uint64_t>() << endl;
        cout << "Transactions in Block: " << block["tx"].size() << endl;
        
        // List available RPC methods
        auto methods = rpcClient->ListMethods();
        cout << "\nAvailable RPC Methods: " << methods.size() << endl;
        for (size_t i = 0; i < min(size_t(5), methods.size()); ++i) {
            cout << "  - " << methods[i] << endl;
        }
        cout << "  ... and " << (methods.size() - 5) << " more" << endl;
        
    } catch (const exception& e) {
        cerr << "RPC Error: " << e.what() << endl;
    }
}

// Example 2: Create and manage wallets
void example_wallet_management() {
    cout << "\n=== Example 2: Wallet Management ===" << endl;
    
    try {
        // Create a new wallet
        auto wallet = wallet::WalletManager::Create("MyWallet", WALLET_PASSWORD);
        cout << "Created new wallet: " << wallet->GetName() << endl;
        
        // Create accounts
        auto account1 = wallet->CreateAccount("Main Account");
        auto account2 = wallet->CreateAccount("Savings Account");
        cout << "Created account 1: " << account1->GetAddress() << endl;
        cout << "Created account 2: " << account2->GetAddress() << endl;
        
        // Set default account
        wallet->SetDefaultAccount(account1->GetAddress());
        cout << "Set default account: " << account1->GetAddress() << endl;
        
        // Import account from WIF
        string wif = "L1QqQJnpBwbsPGAuutuzPTac8piqvbR1HRjrY5qHup48TBCBFe4g"; // Example WIF
        auto importedAccount = wallet->ImportAccount(wif, "Imported Account");
        cout << "Imported account: " << importedAccount->GetAddress() << endl;
        
        // Generate mnemonic
        auto mnemonic = wallet::WalletManager::GenerateMnemonic(12);
        cout << "Generated mnemonic: " << mnemonic << endl;
        
        // Create account from mnemonic
        auto mnemonicAccount = wallet::WalletManager::FromMnemonic(mnemonic);
        cout << "Account from mnemonic: " << mnemonicAccount->GetAddress() << endl;
        
        // Save wallet to file
        wallet->Save(WALLET_PATH);
        cout << "Wallet saved to: " << WALLET_PATH << endl;
        
        // Lock and unlock wallet
        wallet->Lock();
        cout << "Wallet locked" << endl;
        wallet->Unlock(WALLET_PASSWORD);
        cout << "Wallet unlocked" << endl;
        
        // Export wallet as JSON
        string walletJson = wallet->ToJSON();
        cout << "Wallet exported to JSON (size: " << walletJson.size() << " bytes)" << endl;
        
        // Re-open wallet from file
        auto reopenedWallet = wallet::WalletManager::Open(WALLET_PATH, WALLET_PASSWORD);
        cout << "Reopened wallet with " << reopenedWallet->GetAccounts().size() << " accounts" << endl;
        
    } catch (const exception& e) {
        cerr << "Wallet Error: " << e.what() << endl;
    }
}

// Example 3: Create and send transactions
void example_transactions() {
    cout << "\n=== Example 3: Transaction Creation ===" << endl;
    
    try {
        // Setup
        auto rpcClient = make_shared<rpc::RpcClient>(RPC_ENDPOINT);
        auto wallet = wallet::WalletManager::Open(WALLET_PATH, WALLET_PASSWORD);
        auto account = wallet->GetDefaultAccount();
        
        // Create transaction manager
        transaction::TransactionManager txManager(rpcClient);
        
        // Example transfer transaction
        string toAddress = "NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq";
        string amount = "10";  // 10 NEO
        
        // Create NEO transfer transaction
        auto transferTx = txManager.CreateTransferTransaction(
            account->GetAddress(),
            toAddress,
            transaction::TokenHash::NEO,
            amount
        );
        
        cout << "Created transfer transaction:" << endl;
        cout << "  From: " << account->GetAddress() << endl;
        cout << "  To: " << toAddress << endl;
        cout << "  Amount: " << amount << " NEO" << endl;
        
        // Set optimal fees
        txManager.SetOptimalFees(*transferTx);
        cout << "  System Fee: " << transferTx->systemFee << endl;
        cout << "  Network Fee: " << transferTx->networkFee << endl;
        
        // Sign transaction
        wallet->SignTransaction(*transferTx, account->GetAddress());
        cout << "Transaction signed" << endl;
        
        // Get transaction hash (before sending)
        string txHash = transferTx->GetHash();
        cout << "Transaction Hash: " << txHash << endl;
        
        // Send transaction (commented out to prevent actual sending)
        // string txId = txManager.SendTransaction(*transferTx);
        // cout << "Transaction sent! TxID: " << txId << endl;
        
        // Create multi-transfer transaction
        vector<tuple<string, string, string, string>> transfers = {
            {transaction::TokenHash::NEO, account->GetAddress(), toAddress, "5"},
            {transaction::TokenHash::GAS, account->GetAddress(), toAddress, "100000000"}  // 1 GAS
        };
        
        auto multiTx = txManager.CreateMultiTransferTransaction(transfers);
        cout << "\nCreated multi-transfer transaction with " << transfers.size() << " transfers" << endl;
        
        // Create contract invocation transaction
        string contractHash = "0xd2a4cff31913016155e38e474a2c06d08be276cf"; // GAS contract
        auto contractTx = txManager.CreateContractTransaction(
            contractHash,
            "symbol",
            {},
            account->GetAddress()
        );
        cout << "\nCreated contract invocation transaction" << endl;
        cout << "  Contract: " << contractHash << endl;
        cout << "  Method: symbol" << endl;
        
    } catch (const exception& e) {
        cerr << "Transaction Error: " << e.what() << endl;
    }
}

// Example 4: Query account balances and transfers
void example_account_queries() {
    cout << "\n=== Example 4: Account Queries ===" << endl;
    
    try {
        auto rpcClient = make_shared<rpc::RpcClient>(RPC_ENDPOINT);
        auto wallet = wallet::WalletManager::Open(WALLET_PATH, WALLET_PASSWORD);
        auto account = wallet->GetDefaultAccount();
        
        // Validate address
        bool isValid = rpcClient->ValidateAddress(account->GetAddress());
        cout << "Address " << account->GetAddress() << " is valid: " << (isValid ? "Yes" : "No") << endl;
        
        // Get NEP-17 token balances
        auto balances = rpcClient->GetNep17Balances(account->GetAddress());
        cout << "\nNEP-17 Token Balances:" << endl;
        if (balances.contains("balance") && balances["balance"].is_array()) {
            for (const auto& token : balances["balance"]) {
                cout << "  Token: " << token["assethash"].get<string>() << endl;
                cout << "    Amount: " << token["amount"].get<string>() << endl;
                if (token.contains("lastupdatedblock")) {
                    cout << "    Last Updated: Block " << token["lastupdatedblock"].get<uint32_t>() << endl;
                }
            }
        }
        
        // Get unclaimed GAS
        uint64_t unclaimedGas = rpcClient->GetUnclaimedGas(account->GetAddress());
        cout << "\nUnclaimed GAS: " << unclaimedGas << endl;
        
        // Get recent transfers (last 30 days)
        uint64_t endTime = chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()
        ).count();
        uint64_t startTime = endTime - (30 * 24 * 60 * 60 * 1000); // 30 days ago
        
        auto transfers = rpcClient->GetNep17Transfers(account->GetAddress(), startTime, endTime);
        cout << "\nRecent NEP-17 Transfers:" << endl;
        if (transfers.contains("sent") && transfers["sent"].is_array()) {
            cout << "  Sent: " << transfers["sent"].size() << " transactions" << endl;
        }
        if (transfers.contains("received") && transfers["received"].is_array()) {
            cout << "  Received: " << transfers["received"].size() << " transactions" << endl;
        }
        
    } catch (const exception& e) {
        cerr << "Query Error: " << e.what() << endl;
    }
}

// Example 5: Smart contract interaction
void example_smart_contracts() {
    cout << "\n=== Example 5: Smart Contract Interaction ===" << endl;
    
    try {
        auto rpcClient = make_shared<rpc::RpcClient>(RPC_ENDPOINT);
        
        // Get native contracts
        auto nativeContracts = rpcClient->Call("getnativecontracts");
        cout << "Native Contracts:" << endl;
        if (nativeContracts.is_array()) {
            for (const auto& contract : nativeContracts) {
                cout << "  " << contract["name"].get<string>() 
                     << " (" << contract["hash"].get<string>() << ")" << endl;
            }
        }
        
        // Get contract state for GAS token
        string gasContract = transaction::TokenHash::GAS;
        auto contractState = rpcClient->GetContractState(gasContract);
        cout << "\nGAS Contract State:" << endl;
        cout << "  ID: " << contractState["id"].get<int>() << endl;
        cout << "  Update Counter: " << contractState["updatecounter"].get<int>() << endl;
        cout << "  NEF Checksum: " << contractState["nef"]["checksum"].get<uint64_t>() << endl;
        
        // Invoke contract method (test invocation)
        auto result = rpcClient->InvokeFunction(gasContract, "symbol", {});
        cout << "\nInvoke GAS.symbol():" << endl;
        cout << "  State: " << result["state"].get<string>() << endl;
        cout << "  GAS Consumed: " << result["gasconsumed"].get<string>() << endl;
        if (result.contains("stack") && result["stack"].is_array() && !result["stack"].empty()) {
            auto stackItem = result["stack"][0];
            if (stackItem["type"] == "ByteString") {
                // The value is base64 encoded
                cout << "  Result: " << stackItem["value"].get<string>() << endl;
            }
        }
        
        // Test invoke script
        string script = "0c14d2a4cff31913016155e38e474a2c06d08be276cf41c00c0673796d626f6c41c01f0c0d476173546f6b656e2e73796d626f6c419c6f1e2128";  // Example script
        auto scriptResult = rpcClient->InvokeScript(script);
        cout << "\nScript Invocation Result:" << endl;
        cout << "  State: " << scriptResult["state"].get<string>() << endl;
        
        // Create NEP-17 token interface
        contract::NEP17Token gasToken(rpcClient, gasContract);
        cout << "\nNEP-17 Token Information:" << endl;
        cout << "  Symbol: " << gasToken.Symbol() << endl;
        cout << "  Decimals: " << (int)gasToken.Decimals() << endl;
        cout << "  Total Supply: " << gasToken.TotalSupply().ToString() << endl;
        
    } catch (const exception& e) {
        cerr << "Contract Error: " << e.what() << endl;
    }
}

// Example 6: Monitor blockchain events
void example_monitoring() {
    cout << "\n=== Example 6: Blockchain Monitoring ===" << endl;
    
    try {
        auto rpcClient = make_shared<rpc::RpcClient>(RPC_ENDPOINT);
        
        cout << "Monitoring blockchain for 30 seconds..." << endl;
        
        uint32_t lastHeight = rpcClient->GetBlockCount();
        auto startTime = chrono::steady_clock::now();
        
        while (chrono::steady_clock::now() - startTime < chrono::seconds(30)) {
            uint32_t currentHeight = rpcClient->GetBlockCount();
            
            if (currentHeight > lastHeight) {
                cout << "\nNew block detected!" << endl;
                
                for (uint32_t h = lastHeight + 1; h <= currentHeight; ++h) {
                    auto block = rpcClient->GetBlock(h, true);
                    cout << "  Block #" << h << endl;
                    cout << "    Hash: " << block["hash"].get<string>() << endl;
                    cout << "    Time: " << block["time"].get<uint64_t>() << endl;
                    cout << "    Transactions: " << block["tx"].size() << endl;
                    
                    // Check for transactions
                    if (block["tx"].is_array() && !block["tx"].empty()) {
                        for (const auto& tx : block["tx"]) {
                            cout << "      TX: " << tx["hash"].get<string>() << endl;
                        }
                    }
                }
                
                lastHeight = currentHeight;
            }
            
            // Check mempool
            auto mempool = rpcClient->GetRawMempool();
            if (!mempool.empty()) {
                cout << "  Mempool: " << mempool.size() << " pending transactions" << endl;
            }
            
            // Sleep for a short time before checking again
            this_thread::sleep_for(chrono::seconds(1));
        }
        
        cout << "\nMonitoring complete." << endl;
        
    } catch (const exception& e) {
        cerr << "Monitoring Error: " << e.what() << endl;
    }
}

// Main function to run all examples
int main() {
    cout << "======================================" << endl;
    cout << "    Neo C++ SDK Complete Example     " << endl;
    cout << "======================================" << endl;
    
    // Run examples
    example_rpc_connection();
    example_wallet_management();
    example_transactions();
    example_account_queries();
    example_smart_contracts();
    // example_monitoring();  // Commented out as it takes 30 seconds
    
    cout << "\n======================================" << endl;
    cout << "         Examples Complete!           " << endl;
    cout << "======================================" << endl;
    
    return 0;
}