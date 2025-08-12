/**
 * @file transaction_example.cpp
 * @brief Example demonstrating transaction building and sending using Neo C++ SDK
 */

#include <neo/sdk.h>
#include <iostream>
#include <iomanip>

using namespace neo::sdk;

int main(int argc, char* argv[]) {
    try {
        std::cout << "Neo C++ SDK Transaction Example" << std::endl;
        std::cout << "================================" << std::endl << std::endl;
        
        // Initialize SDK
        if (!neo::sdk::Initialize()) {
            std::cerr << "Failed to initialize SDK" << std::endl;
            return 1;
        }
        
        // 1. Setup wallet
        std::cout << "1. Setting up wallet..." << std::endl;
        auto wallet = wallet::Wallet::Create("tx_wallet.json", "Password123!", "Transaction Wallet");
        auto account = wallet->CreateAccount("Main Account");
        std::cout << "   Account created: " << account.GetAddress() << std::endl << std::endl;
        
        // 2. Connect to RPC node
        std::cout << "2. Connecting to TestNet RPC..." << std::endl;
        rpc::RpcClient rpcClient("http://seed1.neo.org:20332");
        
        auto version = rpcClient.GetVersion();
        std::cout << "   Node version: " << version << std::endl;
        
        auto blockCount = rpcClient.GetBlockCount();
        std::cout << "   Current block height: " << blockCount << std::endl << std::endl;
        
        // 3. Build a simple transfer transaction
        std::cout << "3. Building NEO transfer transaction..." << std::endl;
        
        // NEO token script hash on TestNet
        core::UInt160 neoToken = core::UInt160::Parse("0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5");
        
        // Recipient address
        std::string recipientAddress = "NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq";
        core::UInt160 recipientHash = core::UInt160::FromAddress(recipientAddress);
        
        tx::TransactionBuilder builder;
        auto transaction = builder
            .SetSender(account.GetScriptHash())
            .SetSystemFee(100000)  // 0.001 GAS
            .SetNetworkFee(1000000) // 0.01 GAS
            .SetValidUntilBlock(blockCount + 100)  // Valid for next 100 blocks
            .InvokeContract(
                neoToken,
                "transfer",
                {
                    core::ContractParameter::FromHash160(account.GetScriptHash()),
                    core::ContractParameter::FromHash160(recipientHash),
                    core::ContractParameter::FromInteger(10),  // Transfer 10 NEO
                    core::ContractParameter::Null()  // No data
                }
            )
            .BuildAndSign(*wallet);
        
        std::cout << "   Transaction built successfully" << std::endl;
        std::cout << "   TX Hash: " << transaction->GetHash().ToString() << std::endl;
        std::cout << "   System Fee: " << transaction->GetSystemFee() << std::endl;
        std::cout << "   Network Fee: " << transaction->GetNetworkFee() << std::endl << std::endl;
        
        // 4. Test invoke before sending (dry run)
        std::cout << "4. Testing contract invocation..." << std::endl;
        auto testResult = contract::ContractInvoker::TestInvoke(
            neoToken,
            "balanceOf",
            {
                core::ContractParameter::FromHash160(account.GetScriptHash())
            }
        );
        
        std::cout << "   Gas consumed: " << testResult.gasConsumed << std::endl;
        std::cout << "   VM State: " << testResult.state << std::endl;
        if (!testResult.stack.empty()) {
            std::cout << "   Balance: " << testResult.stack[0].ToString() << " NEO" << std::endl;
        }
        std::cout << std::endl;
        
        // 5. Build a more complex transaction with multiple operations
        std::cout << "5. Building multi-operation transaction..." << std::endl;
        
        tx::TransactionBuilder complexBuilder;
        auto complexTx = complexBuilder
            .SetSender(account.GetScriptHash())
            .SetSystemFee(200000)
            .SetNetworkFee(2000000)
            .SetValidUntilBlock(blockCount + 50)
            // First transfer
            .InvokeContract(
                neoToken,
                "transfer",
                {
                    core::ContractParameter::FromHash160(account.GetScriptHash()),
                    core::ContractParameter::FromHash160(recipientHash),
                    core::ContractParameter::FromInteger(5),
                    core::ContractParameter::Null()
                }
            )
            // Second transfer to different address
            .InvokeContract(
                neoToken,
                "transfer",
                {
                    core::ContractParameter::FromHash160(account.GetScriptHash()),
                    core::ContractParameter::FromAddress("NZs2zXSPuuv9ZF6TDGSWT1RBmE8rfGj7UW"),
                    core::ContractParameter::FromInteger(3),
                    core::ContractParameter::Null()
                }
            )
            // Add custom attribute
            .AddAttribute({0x01, {0x01, 0x02, 0x03}})
            .Build();
        
        // Sign the transaction
        wallet->SignTransaction(complexTx);
        
        std::cout << "   Complex transaction built" << std::endl;
        std::cout << "   TX Hash: " << complexTx->GetHash().ToString() << std::endl;
        std::cout << "   Operations: 2 transfers" << std::endl << std::endl;
        
        // 6. Serialize transaction for broadcasting
        std::cout << "6. Serializing transaction..." << std::endl;
        auto txBytes = transaction->ToArray();
        std::cout << "   Transaction size: " << txBytes.size() << " bytes" << std::endl;
        
        auto txHex = transaction->ToHexString();
        std::cout << "   Transaction hex (first 100 chars): " << std::endl;
        std::cout << "   " << txHex.substr(0, 100) << "..." << std::endl << std::endl;
        
        // 7. Send transaction (commented out for safety)
        std::cout << "7. Sending transaction..." << std::endl;
        std::cout << "   [DEMO MODE - Not actually sending]" << std::endl;
        /*
        // Uncomment to actually send transaction
        auto txid = rpcClient.SendRawTransaction(txHex);
        std::cout << "   Transaction sent!" << std::endl;
        std::cout << "   TXID: " << txid << std::endl;
        */
        
        // 8. Query transaction status
        std::cout << std::endl << "8. Querying transaction (example)..." << std::endl;
        // Example transaction hash
        std::string exampleTxId = "0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef";
        
        try {
            auto txInfo = rpcClient.GetRawTransaction(exampleTxId, true);
            std::cout << "   Transaction found in blockchain" << std::endl;
            // Parse and display transaction info
        } catch (...) {
            std::cout << "   Transaction not found (expected for example hash)" << std::endl;
        }
        
        // Cleanup
        neo::sdk::Shutdown();
        
        std::cout << std::endl << "Transaction example completed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}