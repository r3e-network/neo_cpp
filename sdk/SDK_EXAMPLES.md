# Neo C++ SDK Examples

This guide provides practical examples for using the Neo C++ SDK to interact with the Neo blockchain.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Wallet Management](#wallet-management)
3. [Token Transfers](#token-transfers)
4. [Smart Contract Interaction](#smart-contract-interaction)
5. [Blockchain Queries](#blockchain-queries)
6. [Advanced Examples](#advanced-examples)

## Getting Started

### Include Headers

```cpp
#include <neo/sdk.h>
#include <neo/sdk/wallet/wallet.h>
#include <neo/sdk/transaction/transaction_builder.h>
#include <neo/sdk/blockchain/blockchain_client.h>
#include <neo/sdk/rpc/rpc_client.h>
#include <iostream>
```

### Initialize SDK

```cpp
int main() {
    // Initialize the SDK
    neo::sdk::Initialize();
    
    // Set network (MainNet, TestNet, or custom)
    neo::sdk::SetNetwork(neo::sdk::Network::TestNet);
    
    // Your code here
    
    return 0;
}
```

## Wallet Management

### Create a New Wallet

```cpp
#include <neo/sdk/wallet/wallet.h>

void createWallet() {
    try {
        // Create a new wallet
        auto wallet = neo::sdk::wallet::Wallet::Create(
            "my_wallet.json",     // Wallet file path
            "strongPassword123!"  // Wallet password
        );
        
        // Create a new account
        auto account = wallet->CreateAccount("Main Account");
        
        std::cout << "Wallet created successfully!" << std::endl;
        std::cout << "Address: " << account->GetAddress() << std::endl;
        std::cout << "Public Key: " << account->GetPublicKey() << std::endl;
        
        // Save the wallet
        wallet->Save();
        
    } catch (const std::exception& e) {
        std::cerr << "Error creating wallet: " << e.what() << std::endl;
    }
}
```

### Open an Existing Wallet

```cpp
void openWallet() {
    try {
        // Open existing wallet
        auto wallet = neo::sdk::wallet::Wallet::Open(
            "my_wallet.json",
            "strongPassword123!"
        );
        
        // List all accounts
        auto accounts = wallet->GetAccounts();
        for (const auto& account : accounts) {
            std::cout << "Account: " << account->GetLabel() << std::endl;
            std::cout << "Address: " << account->GetAddress() << std::endl;
            std::cout << "Is Default: " << account->IsDefault() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error opening wallet: " << e.what() << std::endl;
    }
}
```

### Import Private Key

```cpp
void importPrivateKey() {
    auto wallet = neo::sdk::wallet::Wallet::Open("wallet.json", "password");
    
    // Import from WIF (Wallet Import Format)
    std::string wif = "L1QqQJnpBwbsPGAuutuzPTac8piqvbR1HRjrY5qHup48TBCBFe4g";
    auto account = wallet->ImportAccount(wif, "Imported Account");
    
    std::cout << "Imported address: " << account->GetAddress() << std::endl;
    wallet->Save();
}
```

## Token Transfers

### Transfer NEO

```cpp
#include <neo/sdk/transaction/transaction_builder.h>
#include <neo/sdk/blockchain/blockchain_client.h>

void transferNEO() {
    try {
        // Initialize blockchain client
        auto client = std::make_unique<neo::sdk::blockchain::BlockchainClient>(
            "http://localhost:10332"
        );
        
        // Open wallet
        auto wallet = neo::sdk::wallet::Wallet::Open("wallet.json", "password");
        auto account = wallet->GetDefaultAccount();
        
        // Build transfer transaction
        std::string toAddress = "NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq";
        neo::sdk::BigInteger amount = 10; // 10 NEO
        
        auto tx = neo::sdk::transaction::TransactionBuilder()
            .SetSender(account->GetAddress())
            .AddTransfer(
                neo::sdk::NEO_TOKEN_HASH,  // NEO token hash
                account->GetAddress(),      // From
                toAddress,                  // To
                amount                       // Amount
            )
            .SetValidUntilBlock(client->getBlockHeight() + 100)
            .Build();
        
        // Sign transaction
        wallet->Sign(*tx, account->GetAddress());
        
        // Send transaction
        auto rpcClient = std::make_unique<neo::sdk::rpc::RPCClient>(
            "http://localhost:10332"
        );
        auto txid = rpcClient->SendTransaction(*tx);
        
        std::cout << "Transaction sent! TxID: " << txid << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Transfer failed: " << e.what() << std::endl;
    }
}
```

### Transfer GAS

```cpp
void transferGAS() {
    auto client = std::make_unique<neo::sdk::blockchain::BlockchainClient>();
    auto wallet = neo::sdk::wallet::Wallet::Open("wallet.json", "password");
    auto account = wallet->GetDefaultAccount();
    
    // GAS has 8 decimal places
    neo::sdk::BigInteger amount = neo::sdk::BigInteger("100000000"); // 1 GAS
    
    auto tx = neo::sdk::transaction::TransactionBuilder()
        .SetSender(account->GetAddress())
        .AddTransfer(
            neo::sdk::GAS_TOKEN_HASH,
            account->GetAddress(),
            "NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq",
            amount
        )
        .SetSystemFee(1000000)    // 0.01 GAS
        .SetNetworkFee(500000)    // 0.005 GAS
        .SetValidUntilBlock(client->getBlockHeight() + 100)
        .Build();
    
    wallet->Sign(*tx, account->GetAddress());
    
    auto rpcClient = std::make_unique<neo::sdk::rpc::RPCClient>();
    auto txid = rpcClient->SendTransaction(*tx);
    std::cout << "GAS transfer sent! TxID: " << txid << std::endl;
}
```

### Multi-Transfer Transaction

```cpp
void multiTransfer() {
    auto builder = neo::sdk::transaction::TransactionBuilder();
    
    // Add multiple transfers in one transaction
    builder.AddTransfer(NEO_HASH, from, "address1", 10)
           .AddTransfer(NEO_HASH, from, "address2", 20)
           .AddTransfer(GAS_HASH, from, "address3", BigInteger("50000000")); // 0.5 GAS
    
    auto tx = builder.SetSender(from)
                    .SetValidUntilBlock(currentHeight + 100)
                    .Build();
    
    // Sign and send...
}
```

## Smart Contract Interaction

### Deploy a Smart Contract

```cpp
#include <neo/sdk/contract/contract_deployer.h>
#include <fstream>

void deployContract() {
    try {
        // Read NEF file
        std::ifstream nefFile("contract.nef", std::ios::binary);
        std::vector<uint8_t> nefBytes(
            (std::istreambuf_iterator<char>(nefFile)),
            std::istreambuf_iterator<char>()
        );
        
        // Read manifest
        std::ifstream manifestFile("contract.manifest.json");
        std::string manifest(
            (std::istreambuf_iterator<char>(manifestFile)),
            std::istreambuf_iterator<char>()
        );
        
        // Deploy
        auto deployer = std::make_unique<neo::sdk::contract::ContractDeployer>();
        auto wallet = neo::sdk::wallet::Wallet::Open("wallet.json", "password");
        auto account = wallet->GetDefaultAccount();
        
        auto deployTx = deployer->Deploy(
            nefBytes,
            manifest,
            account->GetAddress()
        );
        
        wallet->Sign(*deployTx, account->GetAddress());
        
        auto rpcClient = std::make_unique<neo::sdk::rpc::RPCClient>();
        auto txid = rpcClient->SendTransaction(*deployTx);
        
        std::cout << "Contract deployed! TxID: " << txid << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Deployment failed: " << e.what() << std::endl;
    }
}
```

### Invoke a Smart Contract

```cpp
#include <neo/sdk/contract/contract_invoker.h>

void invokeContract() {
    try {
        auto invoker = std::make_unique<neo::sdk::contract::ContractInvoker>();
        
        // Contract hash
        std::string contractHash = "0xd2a4cff31913016155e38e474a2c06d08be276cf";
        
        // Prepare parameters
        std::vector<neo::sdk::contract::ContractParameter> params;
        params.push_back(neo::sdk::contract::ContractParameter::String("name"));
        
        // Test invoke (doesn't modify blockchain)
        auto result = invoker->TestInvoke(contractHash, "symbol", params);
        
        std::cout << "Test Result: " << std::endl;
        for (const auto& item : result.Stack) {
            std::cout << "  " << item.ToString() << std::endl;
        }
        std::cout << "GAS Consumed: " << result.GasConsumed << std::endl;
        
        // Real invoke (modifies blockchain)
        if (result.State == neo::sdk::VMState::HALT) {
            auto invokeTx = invoker->CreateInvokeTransaction(
                contractHash,
                "transfer",
                {
                    ContractParameter::Hash160(fromAddress),
                    ContractParameter::Hash160(toAddress),
                    ContractParameter::Integer(100),
                    ContractParameter::Any()
                }
            );
            
            // Sign and send transaction...
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Invocation failed: " << e.what() << std::endl;
    }
}
```

### Interact with NEP-17 Token

```cpp
#include <neo/sdk/contract/nep17_token.h>

void nep17Operations() {
    // Create NEP-17 token interface
    auto token = std::make_unique<neo::sdk::contract::NEP17Token>(
        "0xd2a4cff31913016155e38e474a2c06d08be276cf" // GAS token hash
    );
    
    // Get token information
    std::cout << "Symbol: " << token->Symbol() << std::endl;
    std::cout << "Decimals: " << (int)token->Decimals() << std::endl;
    std::cout << "Total Supply: " << token->TotalSupply().ToString() << std::endl;
    
    // Check balance
    std::string address = "NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq";
    auto balance = token->BalanceOf(address);
    std::cout << "Balance: " << balance.ToString() << std::endl;
    
    // Create transfer transaction
    auto transferTx = token->Transfer(
        "fromAddress",
        "toAddress",
        neo::sdk::BigInteger("1000000000"), // Amount with decimals
        "Optional data"
    );
    
    // Sign and send...
}
```

## Blockchain Queries

### Get Blockchain Information

```cpp
void getBlockchainInfo() {
    auto client = std::make_unique<neo::sdk::blockchain::BlockchainClient>();
    
    // Get current height
    uint32_t height = client->getBlockHeight();
    std::cout << "Current Height: " << height << std::endl;
    
    // Get latest block
    auto latestBlock = client->getBlock(height);
    std::cout << "Latest Block Hash: " << latestBlock->GetHash() << std::endl;
    std::cout << "Timestamp: " << latestBlock->GetTimestamp() << std::endl;
    std::cout << "Transaction Count: " << latestBlock->GetTransactions().size() << std::endl;
    
    // Get specific block by hash
    std::string blockHash = "0x1234567890abcdef...";
    auto block = client->getBlock(blockHash);
    
    // Get transaction
    std::string txid = "0xabcdef1234567890...";
    auto tx = client->getTransaction(txid);
    if (tx) {
        std::cout << "Transaction found!" << std::endl;
        std::cout << "System Fee: " << tx->GetSystemFee() << std::endl;
        std::cout << "Network Fee: " << tx->GetNetworkFee() << std::endl;
    }
}
```

### Get Account State

```cpp
void getAccountInfo() {
    auto client = std::make_unique<neo::sdk::blockchain::BlockchainClient>();
    
    std::string address = "NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq";
    auto accountState = client->getAccountState(address);
    
    std::cout << "Account: " << address << std::endl;
    
    // Get NEO balance
    auto neoBalance = accountState.GetBalance(neo::sdk::NEO_TOKEN_HASH);
    std::cout << "NEO Balance: " << neoBalance.ToString() << std::endl;
    
    // Get GAS balance
    auto gasBalance = accountState.GetBalance(neo::sdk::GAS_TOKEN_HASH);
    std::cout << "GAS Balance: " << gasBalance.ToString() << std::endl;
    
    // Get all token balances
    for (const auto& [tokenHash, balance] : accountState.GetBalances()) {
        std::cout << "Token " << tokenHash << ": " << balance.ToString() << std::endl;
    }
}
```

## Advanced Examples

### Multi-Signature Transaction

```cpp
void multiSigTransaction() {
    // Create multi-sig account (2 of 3)
    std::vector<std::string> publicKeys = {
        "02208aea0068c429a03316e37be0e3e8e21e6cda5442df4c5914a19b3a9b6de375",
        "03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c",
        "023a36c72844610b0b8a5e0e8e6b3e0e2c5a0c9e7f4e6b3a5e8f2a1c3d5e7f9b1d3e"
    };
    
    auto multiSigAccount = neo::sdk::wallet::Account::CreateMultiSig(
        2,  // Minimum signatures required
        publicKeys
    );
    
    // Create transaction
    auto tx = neo::sdk::transaction::TransactionBuilder()
        .SetSender(multiSigAccount->GetAddress())
        .AddTransfer(NEO_HASH, multiSigAccount->GetAddress(), toAddress, 10)
        .Build();
    
    // Sign with multiple wallets
    auto wallet1 = neo::sdk::wallet::Wallet::Open("wallet1.json", "pass1");
    wallet1->Sign(*tx, multiSigAccount->GetAddress());
    
    auto wallet2 = neo::sdk::wallet::Wallet::Open("wallet2.json", "pass2");
    wallet2->Sign(*tx, multiSigAccount->GetAddress());
    
    // Send transaction
    auto rpcClient = std::make_unique<neo::sdk::rpc::RPCClient>();
    auto txid = rpcClient->SendTransaction(*tx);
}
```

### Subscribe to Blockchain Events

```cpp
#include <neo/sdk/network/network_client.h>

void subscribeToEvents() {
    auto networkClient = std::make_unique<neo::sdk::network::NetworkClient>();
    
    // Connect to node
    networkClient->Connect("localhost", 10333);
    
    // Subscribe to new blocks
    networkClient->OnBlock([](const auto& block) {
        std::cout << "New Block: " << block->GetHash() << std::endl;
        std::cout << "Height: " << block->GetIndex() << std::endl;
    });
    
    // Subscribe to new transactions
    networkClient->OnTransaction([](const auto& tx) {
        std::cout << "New Transaction: " << tx->GetHash() << std::endl;
    });
    
    // Subscribe to contract events
    networkClient->OnContractEvent([](const auto& event) {
        std::cout << "Contract Event: " << event.EventName << std::endl;
        std::cout << "Contract: " << event.ScriptHash << std::endl;
    });
    
    // Keep listening
    std::this_thread::sleep_for(std::chrono::hours(1));
}
```

### Batch RPC Requests

```cpp
void batchRpcRequests() {
    auto rpcClient = std::make_unique<neo::sdk::rpc::RPCClient>(
        "http://localhost:10332"
    );
    
    // Create batch
    auto batch = rpcClient->CreateBatch();
    
    // Add multiple requests
    batch.Add("getblockcount");
    batch.Add("getbestblockhash");
    batch.Add("getconnectioncount");
    batch.Add("getversion");
    
    // Execute batch
    auto results = batch.Execute();
    
    // Process results
    std::cout << "Block Count: " << results[0]["result"] << std::endl;
    std::cout << "Best Block Hash: " << results[1]["result"] << std::endl;
    std::cout << "Connection Count: " << results[2]["result"] << std::endl;
    std::cout << "Version: " << results[3]["result"]["useragent"] << std::endl;
}
```

### Custom Transaction Attributes

```cpp
void customAttributes() {
    auto tx = neo::sdk::transaction::TransactionBuilder()
        .SetSender(senderAddress)
        .AddAttribute(neo::sdk::transaction::TransactionAttribute{
            .Type = neo::sdk::transaction::TransactionAttributeType::HighPriority,
            .Data = {}
        })
        .AddAttribute(neo::sdk::transaction::TransactionAttribute{
            .Type = neo::sdk::transaction::TransactionAttributeType::OracleResponse,
            .Data = responseData
        })
        .AddTransfer(NEO_HASH, from, to, 10)
        .Build();
}
```

### Create and Verify Signatures

```cpp
#include <neo/sdk/crypto/keypair.h>

void cryptographicOperations() {
    // Generate new key pair
    auto keyPair = neo::sdk::crypto::KeyPair::Generate();
    
    std::cout << "Private Key: " << keyPair.GetPrivateKeyHex() << std::endl;
    std::cout << "Public Key: " << keyPair.GetPublicKeyHex() << std::endl;
    std::cout << "Address: " << keyPair.GetAddress() << std::endl;
    
    // Sign message
    std::vector<uint8_t> message = {0x01, 0x02, 0x03, 0x04};
    auto signature = keyPair.Sign(message);
    
    // Verify signature
    bool isValid = keyPair.Verify(message, signature);
    std::cout << "Signature valid: " << (isValid ? "Yes" : "No") << std::endl;
    
    // Import from WIF
    auto imported = neo::sdk::crypto::KeyPair::FromWIF(
        "L1QqQJnpBwbsPGAuutuzPTac8piqvbR1HRjrY5qHup48TBCBFe4g"
    );
}
```

## Error Handling Best Practices

```cpp
void robustExample() {
    try {
        auto client = std::make_unique<neo::sdk::blockchain::BlockchainClient>();
        
        // Check connection
        if (!client->IsConnected()) {
            throw std::runtime_error("Cannot connect to blockchain");
        }
        
        auto wallet = neo::sdk::wallet::Wallet::Open("wallet.json", "password");
        
        // Check balance before transfer
        auto account = wallet->GetDefaultAccount();
        auto balance = client->getAccountState(account->GetAddress())
                             .GetBalance(neo::sdk::NEO_TOKEN_HASH);
        
        if (balance < 10) {
            throw std::runtime_error("Insufficient balance");
        }
        
        // Proceed with transfer...
        
    } catch (const neo::sdk::NetworkException& e) {
        std::cerr << "Network error: " << e.what() << std::endl;
        // Retry logic...
    } catch (const neo::sdk::InvalidArgumentException& e) {
        std::cerr << "Invalid input: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
```

## Compilation

To compile these examples:

```bash
# Using CMake
mkdir build && cd build
cmake .. -DNEO_BUILD_SDK=ON
make

# Direct compilation
g++ -std=c++20 example.cpp -lneo_sdk -lneo_cpp -lcrypto -o example

# With pkg-config
g++ -std=c++20 example.cpp `pkg-config --cflags --libs neo-sdk` -o example
```

## Additional Resources

- [API Reference](./api/API_REFERENCE.md)
- [Architecture Guide](./ARCHITECTURE.md)
- [Neo Protocol Documentation](https://docs.neo.org)
- [NEP Standards](https://github.com/neo-project/proposals)

---

*Last Updated: August 15, 2025*  
*SDK Version: 1.0.0*